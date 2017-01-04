//
//  TimeSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include <limits.h>
#include <boost/foreach.hpp>

#include "TimeSeries.h"
#include "PointRecord.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>
#include <boost/foreach.hpp>

#include "TimeSeriesFilter.h"

using namespace RTX;
using namespace std;
using namespace boost::accumulators;

#pragma mark - Point Collection methods

TimeSeries::PointCollection::PointCollection(vector<Point>::iterator start, vector<Point>::iterator end, Units units) : _start(start), _end(end), units(units) { 
  _isRef = true;
}

TimeSeries::PointCollection::PointCollection(const PointCollection &pc) {
  if (pc._isRef) {
    _isRef = true;
    _start = pc._start;
    _end = pc._end;
    units = pc.units;
  }
  else {
    _points = pc._points;
    units = pc.units;
  }
}

TimeSeries::PointCollection::PointCollection(vector<Point> points, Units units) : units(units) {
  _isRef = false;
  this->setPoints(points);
}
TimeSeries::PointCollection::PointCollection() : units(1) { 
  _isRef = false;
}

pair<vector<Point>::iterator,vector<Point>::iterator> TimeSeries::PointCollection::raw() {
  return make_pair(_start,_end);
}

void TimeSeries::PointCollection::apply(std::function<void(Point)> function) {
  for (auto i = _start; i != _end; ++i) {
    function(*i);
  }
}

TimeRange TimeSeries::PointCollection::range() {
  auto times = this->times();
  return TimeRange(*times.begin(), *times.rbegin()); // because set is ordered
}

vector<Point> TimeSeries::PointCollection::points() {
  if (_isRef) {
    vector<Point> pv;
    this->apply([&](Point p){
      pv.push_back(p);
    });
    return pv;
  }
  else {
    return *_points.get();
  }
}

void TimeSeries::PointCollection::setPoints(vector<Point> points) {
  _points.reset(new vector<Point>(points));
  _start = _points->begin();
  _end = _points->end();
  _isRef = false;
}

const set<time_t> TimeSeries::PointCollection::times() {
  set<time_t> t;
  this->apply([&](Point p){
    t.insert(p.time);
  });
  return t;
}

bool TimeSeries::PointCollection::convertToUnits(RTX::Units u) {
  if (!u.isSameDimensionAs(this->units)) {
    return false;
  }
  vector<Point> converted;
  this->apply([&](Point p){
    converted.push_back(Point::convertPoint(p, this->units, u));
  });
  this->setPoints(converted);
  this->units = u;
  return true;
}

void TimeSeries::PointCollection::addQualityFlag(Point::PointQuality q) {
  this->apply([&](Point p){
    p.addQualFlag(q);
  });
}


double TimeSeries::PointCollection::percentile(double p) {
  using namespace boost::accumulators;
  
  if (p < 0. || p > 1.) {
    return 0.;
  }
  
  int cacheSize = (int)this->count();
  if (cacheSize == 0) {
    return 0;
  }
  
  if (cacheSize == 1 && p == 0.5) {
    // single point median
    return _start->value;
  }
  
  if (p <= 0.5) {
    accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::left> > > centile( tag::tail<boost::accumulators::left>::cache_size = cacheSize );
    this->apply([&](Point p){
      centile(p.value);
    });
    double pct = quantile(centile, quantile_probability = p);
    return pct;
  }
  else {
    accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::right> > > centile( tag::tail<boost::accumulators::right>::cache_size = cacheSize );
    this->apply([&](Point p){
      centile(p.value);
    });
    double pct = quantile(centile, quantile_probability = p);
    return pct;
  }
  
}

size_t TimeSeries::PointCollection::count() {
  if (_isRef) {
    size_t c = 0;
    this->apply([&](Point p){
      ++c;
    });
    return c;
  }
  else {
    return _points->size();
  }
}


double TimeSeries::PointCollection::min() {
  if (this->count() == 0) {
    return NAN;
  }
  
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  
  this->apply([&](Point p){
    acc(p.value);
  });
  
  double min = extract::min(acc);
  return min;
}

double TimeSeries::PointCollection::max() {
  if (this->count() == 0) {
    return NAN;
  }
  
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  this->apply([&](Point p){
    acc(p.value);
  });
  
  double max = extract::max(acc);
  return max;
}

double TimeSeries::PointCollection::mean() {
  if (this->count() == 0) {
    return NAN;
  }
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  
  this->apply([&](Point p){
    acc(p.value);
  });
  
  double mean = extract::mean(acc);
  return mean;
}

double TimeSeries::PointCollection::variance() {
  if (this->count() == 0) {
    return NAN;
  }
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  
  this->apply([&](Point p){
    acc(p.value);
  });
  
  double variance = extract::variance(acc);
  return variance;
}




bool TimeSeries::PointCollection::resample(set<time_t> timeList, TimeSeriesResampleMode mode) {
  PointCollection c = this->resampledAtTimes(timeList,mode);
  this->setPoints(c.points());
  
  if (this->count() > 0) {
    return true;
  }
  return false;
}

TimeSeries::PointCollection TimeSeries::PointCollection::resampledAtTimes(std::set<time_t> timeList, TimeSeriesResampleMode mode) {
  
  typedef std::vector<Point>::iterator pVec_it;
  
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
  pVec_it sourceBegin = _start;
  pVec_it sourceEnd = _end;
  pVec_it right = sourceBegin;
  pVec_it left = sourceBegin;
  
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
    if (mode == TimeSeriesResampleModeLinear) {
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
    else if (mode == TimeSeriesResampleModeStep) {
      p = *left;
      p.time = now;
      resampled.push_back(p);
    }
  }
  
  return PointCollection(resampled,this->units);
}


TimeSeries::PointCollection TimeSeries::PointCollection::trimmedToRange(TimeRange range) {
  
  vector<Point>::iterator it = _start;
  vector<Point>::iterator r1 = it, r2 = it;
  vector<Point>::iterator end = _end;
  
  
  while (it != end) {
    time_t t = it->time;
    if (range.contains(t)) {
      r1 = it;
      break;
    }
    ++it;
  }
  
  while (it != end) {
    time_t t = it->time;
    if (!range.contains(t)) {
      break;
    }
    ++it;
    r2 = it;
  }
  
  
  vector<Point> filtered;
  if (r1 != r2) {
    filtered = vector<Point>(r1,r2);
  }
  
  PointCollection c(filtered, this->units);
  return c;
}


TimeSeries::PointCollection TimeSeries::PointCollection::asDelta() {
  vector<Point> deltaPoints;
  
  if (this->count() == 0) {
    return PointCollection(deltaPoints, this->units);
  }
  
  Point lastP = *_start;
  deltaPoints.push_back(lastP);
  
  this->apply([&](Point p){
    if (p.value != lastP.value) {
      lastP = p;
      deltaPoints.push_back(lastP);
    }
  });
  
  return PointCollection(deltaPoints, this->units);
}


#pragma mark - Time Series methods


TimeSeries::TimeSeries() {
  _name = "";
  _points.reset( new PointRecord() );
  setName("Time Series");
  _units = RTX_NO_UNITS;
  _valid = true;
}

TimeSeries::~TimeSeries() {
  // empty Dtor
}

std::ostream& RTX::operator<< (std::ostream &out, TimeSeries &ts) {
  return ts.toStream(out);
}

#pragma mark Public Methods

bool TimeSeries::valid(time_t t) {
  return _valid;
}
void TimeSeries::setValid(bool v) {
  _valid = v;
}

void TimeSeries::setName(const std::string& name) {
  _name = name;
  _points->registerAndGetIdentifierForSeriesWithUnits(name, this->units());
}

std::string TimeSeries::name() {
  return _name;
}

void TimeSeries::insert(Point thisPoint) {
  _points->addPoint(name(), thisPoint);
}

void TimeSeries::insertPoints(std::vector<Point> points) {
  _points->addPoints(name(), points);
}

Point TimeSeries::point(time_t time) {
  Point p;
  vector<Point> single = this->points(TimeRange(time,time));
  if (single.size() > 0) {
    Point goodPoint = single.front();
    if (goodPoint.time == time) {
      p = goodPoint;
    }
  }
  return p;
}

TimeSeries::PointCollection TimeSeries::pointCollection(TimeRange range) {
  return PointCollection(this->points(range), this->units());
}

// get a range of points from this TimeSeries' point method
std::vector< Point > TimeSeries::points(TimeRange range) {
  // container for points in this range
  std::vector< Point > points;
  
  if (!range.isValid()) {
    return points;
  }
  
  if (!this->record()->exists(this->name(), this->units())) {
    this->record()->registerAndGetIdentifierForSeriesWithUnits(this->name(), this->units());
  }
  
  points = this->record()->pointsInRange(this->name(), range);
  return points;
}

std::set<time_t> TimeSeries::timeValuesInRange(TimeRange range) {
  auto points = this->pointCollection(range);
  return points.times();
}

time_t TimeSeries::timeAfter(time_t t) {
  if (this->clock()) {
    return this->clock()->timeAfter(t);
  }
  else {
    return TimeSeries::pointAfter(t).time;
  }
}


time_t TimeSeries::timeBefore(time_t t) {
  if (this->clock()) {
    return this->clock()->timeBefore(t);
  }
  else {
    return TimeSeries::pointBefore(t).time;
  }
}


Point TimeSeries::pointBefore(time_t time) {
  if (time == 0) {
    return Point();
  }
  return this->record()->pointBefore(this->name(), time);
}

Point TimeSeries::pointAfter(time_t time) {
  if (time == 0) {
    return Point();
  }
  return this->record()->pointAfter(this->name(), time);
}

Point TimeSeries::pointAtOrBefore(time_t time) {
  Point p = this->point(time);
  if (!p.isValid) {
    p = this->pointBefore(time);
  }
  return p;
}


void TimeSeries::setRecord(PointRecord::_sp record) {
  if (!record) {
    PointRecord::_sp pr( new PointRecord() );
    record = pr;
  }
  if (record->registerAndGetIdentifierForSeriesWithUnits(this->name(),this->units())) {
    _points = record;
  }
  return;
}

PointRecord::_sp TimeSeries::record() {
  return _points;
}

void TimeSeries::resetCache() {
  _points->reset(name());
}

void TimeSeries::invalidate() {
  if(_points) {
    _points->invalidate(this->name());
    if (!_points->registerAndGetIdentifierForSeriesWithUnits(this->name(), this->units())) {
      PointRecord::_sp pr( new PointRecord() );
      _points = pr;
    }
  }
}


void TimeSeries::setUnits(Units newUnits) {
  // changing units means the values here are no good anymore.
  if (this->canChangeToUnits(newUnits)) {
    if (! (newUnits == this->units())) {
      // if original units were NONE, then we don't need to invalidate. we are just setting this up for the first time.
      bool shouldInvalidate = true;
      if (_units == RTX_NO_UNITS) {
        shouldInvalidate = false;
      }
      _units = newUnits;
      if (shouldInvalidate) {
        this->invalidate();
      }
      else {
        if (_points) {
          _points->registerAndGetIdentifierForSeriesWithUnits(this->name(), this->units());
        }
      }
    }
  }
}

Units TimeSeries::units() {
  return _units;
}


time_t TimeSeries::expectedPeriod() {
  return _expectedPeriod;
}
void TimeSeries::setExpectedPeriod(time_t seconds) {
  _expectedPeriod = seconds;
}

#pragma mark Protected Methods

std::ostream& TimeSeries::toStream(std::ostream &stream) {
  stream << "Time Series: \"" << _name << "\"\n";
  stream << "Units: " << _units << std::endl;
  stream << "Cached Points:" << std::endl;
  stream << *_points;
  return stream;
}

// chaning

TimeSeries::_sp TimeSeries::sp() {
  return shared_from_this();
}







