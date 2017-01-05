#include "PointCollection.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>


using namespace RTX;
using namespace std;
using namespace boost::accumulators;
using pvIt = PointCollection::pvIt;



inline void __apply(RTX::PointCollection::pvRange r, function<void(const Point&)> fn) {
  auto i = r.first;
  while (i != r.second) {
    fn(*i);
    ++i;
  }
}



PointCollection::PointCollection(vector<Point> points, Units units) : units(units) {
  this->setPoints(points);
}
PointCollection::PointCollection() : units(1) { 
  
}

pair<pvIt,pvIt> PointCollection::raw() const {
  return make_pair(_points->begin(),_points->end());
}

void PointCollection::apply(std::function<void(const Point&)> function) const {
  auto raw = this->raw();
  __apply(raw, function);
}

TimeRange PointCollection::range() const {
  auto times = this->times();
  return TimeRange(*times.begin(), *times.rbegin()); // because set is ordered
}

vector<Point> PointCollection::points() const {
  return vector<Point>(*_points.get());
}

void PointCollection::setPoints(vector<Point> points) {
  _points = make_shared< vector<Point> >(points);
}

const set<time_t> PointCollection::times() const {
  set<time_t> t;
  this->apply([&](const Point& p){
    t.insert(p.time);
  });
  return t;
}

bool PointCollection::convertToUnits(RTX::Units u) {
  if (!u.isSameDimensionAs(this->units)) {
    return false;
  }
  vector<Point> converted;
  this->apply([&](const Point& p){
    converted.push_back(Point::convertPoint(p, this->units, u));
  });
  this->setPoints(converted);
  this->units = u;
  return true;
}

void PointCollection::addQualityFlag(Point::PointQuality q) {
  auto pv = this->points();
  for(Point &p : pv) {
    p.addQualFlag(q);
  }
  this->setPoints(pv);
}



bool PointCollection::resample(set<time_t> timeList, ResampleMode mode) {
  PointCollection c = this->resampledAtTimes(timeList,mode);
  auto p = c.points();
  this->setPoints(p);
  
  if (this->count() > 0) {
    return true;
  }
  return false;
}

PointCollection PointCollection::resampledAtTimes(const std::set<time_t>& timeList, ResampleMode mode) const {
  
  
  // sanity
  if (timeList.empty()) {
    return PointCollection();
  }
  if (this->count() < 1) {
    return PointCollection();
  }
  
  
  vector<Point> resampled;
  vector<Point>::size_type s = timeList.size();
  resampled.reserve(s);
  
  
  // iterators for scrubbing through the source points
  pvIt sourceBegin = _points->begin();
  pvIt sourceEnd = _points->end();
  pvIt right = sourceBegin;
  pvIt left = sourceBegin;
  
  ++right; // get one step ahead.
  
  for (const time_t now : timeList) {
    
    // maybe we can't resample at now
    if (now < left->time) {
      continue;
    }
    
    // get positioned
    while (right != sourceEnd && right->time <= now) {
      ++left;
      ++right;
    }
    
    Point p;
    if (mode == ResampleModeLinear) {
      if (right != sourceEnd) {
        p = Point::linearInterpolate(*left, *right, now);
        resampled.push_back(p);
      }
      else {
        if (left->time == now) {
          p = *left;
          resampled.push_back(p);
        }
        break;
      }
    }
    else if (mode == ResampleModeStep) {
      p = *left;
      p.time = now;
      resampled.push_back(p);
    }
  }
  
  return PointCollection(resampled,this->units);
}


PointCollection::pvRange PointCollection::subRange(TimeRange r) const {
  pvIt it = _points->begin();
  pvIt r1 = it, r2 = it;
  pvIt end = _points->end();
  
  while (it != end) {
    time_t t = it->time;
    if (r.contains(t)) {
      r1 = it;
      break;
    }
    ++it;
  }
  
  while (it != end) {
    time_t t = it->time;
    if (!r.contains(t)) {
      break;
    }
    ++it;
    r2 = it;
  }
  
  return make_pair(r1,r2);
}

PointCollection PointCollection::trimmedToRange(TimeRange range) const {
  auto iters = this->subRange(range);
  vector<Point> ranged(iters.first,iters.second);
  PointCollection c(ranged,this->units);
  return c;
}


PointCollection PointCollection::asDelta() const {
  vector<Point> deltaPoints;
  
  if (this->count() == 0) {
    return PointCollection(deltaPoints, this->units);
  }
  
  Point lastP = _points->front();
  deltaPoints.push_back(lastP);
  
  this->apply([&](const Point& p){
    if (p.value != lastP.value) {
      lastP = p;
      deltaPoints.push_back(lastP);
    }
  });
  
  return PointCollection(deltaPoints, this->units);
}










double PointCollection::percentile(double p, pvRange r) {
  using namespace boost::accumulators;
  
  if (p < 0. || p > 1.) {
    return 0.;
  }
  
  size_t cacheSize = PointCollection::count(r);
  if (cacheSize == 0) {
    return 0;
  }
  
  if (cacheSize == 1 && p == 0.5) {
    // single point median
    return r.first->value;
  }
  
  if (p <= 0.5) {
    accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::left> > > centile( tag::tail<boost::accumulators::left>::cache_size = cacheSize );
    __apply(r,[&](const Point& p){
      centile(p.value);
    });
    
    double pct = quantile(centile, quantile_probability = p);
    return pct;
  }
  else {
    accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::right> > > centile( tag::tail<boost::accumulators::right>::cache_size = cacheSize );
    __apply(r,[&](const Point& p){
      centile(p.value);
    });
    double pct = quantile(centile, quantile_probability = p);
    return pct;
  }
  
}

size_t PointCollection::count(pvRange r) {
  size_t count = 0;
  __apply(r,[&](const Point& p){
    ++count;
  });
  return count;
}


double PointCollection::min(pvRange r) {
  if (PointCollection::count(r) == 0) {
    return NAN;
  }
  
  accumulator_set<double, features<tag::min> > acc;
  
  __apply(r,[&](const Point& p){
    acc(p.value);
  });
  
  double min = extract::min(acc);
  return min;
}

double PointCollection::max(pvRange r) {
  if (PointCollection::count(r) == 0) {
    return NAN;
  }
  
  accumulator_set<double, features<tag::max> > acc;
  __apply(r,[&](const Point& p){
    acc(p.value);
  });
  
  double max = extract::max(acc);
  return max;
}

double PointCollection::mean(pvRange r) {
  if (PointCollection::count(r) == 0) {
    return NAN;
  }
  accumulator_set<double, features<tag::mean> > acc;
  
  __apply(r,[&](const Point& p){
    acc(p.value);
  });
  
  double mean = extract::mean(acc);
  return mean;
}

double PointCollection::variance(pvRange r) {
  if (PointCollection::count(r) == 0) {
    return NAN;
  }
  accumulator_set<double, features<tag::variance(lazy)> > acc;
  
  __apply(r,[&](const Point& p){
    acc(p.value);
  });
  
  double variance = extract::variance(acc);
  return variance;
}

double PointCollection::interquartilerange(pvRange r) {
  if (PointCollection::count(r) == 0) {
    return NAN;
  }
  return PointCollection::percentile(.75,r) - PointCollection::percentile(.25,r);
}

TimeRange PointCollection::timeRange(pvRange r) {
  return TimeRange(r.first->time, r.second->time);
}




