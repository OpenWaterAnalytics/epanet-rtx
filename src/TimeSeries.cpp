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


using namespace RTX;
using namespace std;
using namespace boost::accumulators;

#pragma mark -

TimeSeries::PointCollection::PointCollection(vector<Point> points, Units units) : points(points), units(units) {
  // simple
}
TimeSeries::PointCollection::PointCollection() : points(vector<Point>()), units(1) {
  
}

set<time_t> TimeSeries::PointCollection::times() {
  set<time_t> t;
  BOOST_FOREACH(const Point& p, this->points) {
    t.insert(p.time);
  }
  return t;
}

bool TimeSeries::PointCollection::convertToUnits(RTX::Units u) {
  if (!u.isSameDimensionAs(this->units)) {
    return false;
  }
  vector<Point> converted;
  BOOST_FOREACH(const Point& p, this->points) {
    converted.push_back(Point::convertPoint(p, this->units, u));
  }
  this->points = converted;
  this->units = u;
  return true;
}

void TimeSeries::PointCollection::addQualityFlag(Point::PointQuality q) {
  BOOST_FOREACH(Point& p, this->points) {
    p.addQualFlag(q);
  }
}


double TimeSeries::PointCollection::percentile(double p) {
  if (p < 0. || p > 1.) {
    return 0.;
  }
  
  int cacheSize = (int)this->points.size();
  using namespace boost::accumulators;
  
  accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::left> > > centile( tag::tail<boost::accumulators::left>::cache_size = cacheSize );
  BOOST_FOREACH(const Point& point, this->points) {
    centile(point.value);
  }
  
  double pct = quantile(centile, quantile_probability = p);
  return pct;
}

size_t TimeSeries::PointCollection::count() {
  return this->points.size();
}


double TimeSeries::PointCollection::min() {
  
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  
  BOOST_FOREACH(const Point& p, points) {
    acc(p.value);
  }
  
  double min = extract::min(acc);
  return min;
}

double TimeSeries::PointCollection::max() {
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  BOOST_FOREACH(const Point& p, points) {
    acc(p.value);
  }
  
  double max = extract::max(acc);
  return max;
}

double TimeSeries::PointCollection::mean() {
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  
  BOOST_FOREACH(const Point& p, points) {
    acc(p.value);
  }
  
  double mean = extract::mean(acc);
  return mean;
}

double TimeSeries::PointCollection::variance() {
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  
  BOOST_FOREACH(const Point& p, points) {
    acc(p.value);
  }
  
  double variance = extract::variance(acc);
  return variance;
}




bool TimeSeries::PointCollection::resample(set<time_t> timeList, TimeSeriesResampleMode mode) {
  
  typedef std::vector<Point>::const_iterator pVec_cIt;
  
  // sanity
  if (timeList.empty()) {
    return false;
  }
  
  // are there enough points to return anything?
  bool tooFewPoints = (mode == TimeSeriesResampleModeStep) ? (this->count() < 1) : (this->count() < 2);
  if (tooFewPoints) {
    // the only way this is ok is if the collection has one point and it's the only time value there is.
    if (this->count() == 1 && this->points.front().time == *(timeList.begin())) {
      return true;
    }
    return false;
  }
  
  TimeRange listRange;
  listRange.start = *(timeList.cbegin());
  listRange.end = *(timeList.crbegin());
  
  TimeRange effectiveRange;
  effectiveRange.start = this->points.front().time;
  effectiveRange.end = this->points.back().time;
  
  effectiveRange.correctWithRange(listRange);
  
  vector<Point> resampled;
  vector<Point>::size_type s = timeList.size();
  resampled.reserve(s);
  
  
  // iterators for scrubbing through the source points
  pVec_cIt sourceBegin = this->points.begin();
  pVec_cIt sourceEnd = this->points.end();
  pVec_cIt right = sourceBegin;
  pVec_cIt left = sourceBegin;
  
  // prune the time list for valid time values (values within native point time range)
  set<time_t> validTimeList;
  BOOST_FOREACH(const time_t now, timeList) {
    if ( effectiveRange.start <= now && now <= effectiveRange.end ) {
      validTimeList.insert(now);
    }
  }
  
  ++right; // get one step ahead.
  
  // pre-set the right/left cursors
  time_t firstValidTime = *(validTimeList.begin());
  while (right != sourceEnd && right->time < firstValidTime) {
    ++right;
    ++left;
  }
  
  
  BOOST_FOREACH(const time_t now, validTimeList) {
    
    // we should be straddling.
    while ( ( right != sourceEnd && left != sourceEnd) &&
           (! (left->time <= now && right->time >= now) ) ) {
      // move cursors forward
      ++left;
      ++right;
    }
    
    // end of native points?
    if (right == sourceEnd) {
      break; // get out of foreach
    }
    Point p;
    if (mode == TimeSeriesResampleModeLinear) {
      p = Point::linearInterpolate(*left, *right, now);
    }
    else if (mode == TimeSeriesResampleModeStep) {
      if (now == right->time) {
        p = *right;
      }
      else {
        p = *left;
      }
      p.time = now;
    }
    resampled.push_back(p);
  }
  
  this->points = resampled;
  return true;
}


TimeSeries::PointCollection TimeSeries::PointCollection::trimmedToRange(TimeRange range) {
  
  vector<Point> filtered;
  
  BOOST_FOREACH(const Point& p, this->points) {
    if (range.contains(p.time)) {
      filtered.push_back(p);
    }
  }
  
  PointCollection pc(filtered, this->units);
  return pc;
}





#pragma mark -


TimeSeries::TimeSeries() : _units(1) {
  _name = "";
  _points.reset( new PointRecord() );
  setName("Time Series");
  _units = RTX_DIMENSIONLESS;
}



TimeSeries::~TimeSeries() {
  // empty Dtor
}

std::ostream& RTX::operator<< (std::ostream &out, TimeSeries &ts) {
  return ts.toStream(out);
}


#pragma mark - Public Methods


void TimeSeries::setName(const std::string& name) {
  _name = name;
  _points->registerAndGetIdentifier(name);
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
  
  points = this->record()->pointsInRange(this->name(), range.start, range.end);
  return points;
}

std::set<time_t> TimeSeries::timeValuesInRange(TimeRange range) {
  
  vector<Point> points = this->points(range);
  set<time_t> times;
  BOOST_FOREACH(const Point& p, points) {
    times.insert(p.time);
  }
  return times;
}



Point TimeSeries::pointBefore(time_t time) {
  Point myPoint;
  if (time == 0) {
    return myPoint;
  }
  
  myPoint = this->record()->pointBefore(this->name(), time);
  return myPoint;
}

Point TimeSeries::pointAfter(time_t time) {
  Point myPoint;
  if (time == 0) {
    return myPoint;
  }
  
  myPoint = this->record()->pointAfter(this->name(), time);
  return myPoint;
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
    //cerr << "WARNING: removing record for Time Series \"" << this->name() << "\"" << endl;
  }
  
  vector<string> existing = record->identifiers();
  BOOST_FOREACH( const string& name, existing) {
    if (RTX_STRINGS_ARE_EQUAL_CS(name,this->name())) {
      _points = record;
      record->registerAndGetIdentifier(this->name());
      return;
    }
  }
  
  return;
}

PointRecord::_sp TimeSeries::record() {
  return _points;
}

void TimeSeries::resetCache() {
  _points->reset(name());
  //_points->registerAndGetIdentifier(this->name(), this->units());
}

void TimeSeries::invalidate() {
  if(_points) {
    _points->invalidate(this->name());
    _points->registerAndGetIdentifier(this->name());
  }
}


void TimeSeries::setUnits(Units newUnits) {
  // changing units means the values here are no good anymore.
  if (this->canChangeToUnits(newUnits)) {
    if (! (newUnits == this->units())) {
      _units = newUnits;
    }
  }
}

Units TimeSeries::units() {
  return _units;
}


#pragma mark - Protected Methods

std::ostream& TimeSeries::toStream(std::ostream &stream) {
  stream << "Time Series: \"" << _name << "\"\n";
  stream << "Units: " << _units << std::endl;
  stream << "Cached Points:" << std::endl;
  stream << *_points;
  return stream;
}


