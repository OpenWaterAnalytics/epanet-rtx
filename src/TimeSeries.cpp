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
#include "IrregularClock.h"
#include "PointRecord.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/tail_quantile.hpp>


using namespace RTX;
using namespace std;
using namespace boost::accumulators;


TimeSeries::PointCollection::PointCollection(vector<Point> points, Units units) : points(points), units(units) {
  // simple
}

double TimeSeries::PointCollection::percentile(double p) {
  int cacheSize = (int)this->points.size();
  using namespace boost::accumulators;
  
  accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::left> > > centile( tag::tail<boost::accumulators::left>::cache_size = cacheSize );
  BOOST_FOREACH(const Point& p, _collection) {
    centile(p.value);
  }
  
  double pct = quantile(centile, quantile_probability = p);
  return pct;
}
size_t TimeSeries::PointCollection::count() {
  return this->points.size();
}



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
  _points->registerAndGetIdentifier(name, this->units());
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
/*
bool TimeSeries::isPointAvailable(time_t time) {
  return ( _points->isPointAvailable(name(), time) );
}
*/
Point TimeSeries::point(time_t time) {
  Point p;
  //time = clock()->validTime(time);
  
  p = _points->point(name(), time);
  
  return p;
}


TimeSeries::PointCollection TimeSeries::pointCollection(time_t start, time_t end) {
  return PointCollection(this->points(start,end), this->units());
}


// get a range of points from this TimeSeries' point method
std::vector< Point > TimeSeries::points(time_t start, time_t end) {
  // container for points in this range
  std::vector< Point > points;
  
  // sanity
  if (start == 0 || end == 0 || (start < 0) || (end < 0)) {
    return points;
  }
  
  
  points = this->record()->pointsInRange(this->name(), start, end);
  return points;
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

Point TimeSeries::interpolatedPoint(time_t time) {
  
  Point p1,p2;
  
  p1 = this->pointAtOrBefore(time);
  p2 = this->pointAfter(time - 1);
  
  return Point::linearInterpolate(p1, p2, time);
}

vector<Point> TimeSeries::gaps(time_t start, time_t end) {
  
  vector<Point> points = this->points(start, end);
  
  if (points.size() == 0) {
    return points;
  }
  
  vector<Point> gaps;
  gaps.reserve(points.size());
  time_t prior = this->pointBefore(points.front().time).time;
  BOOST_FOREACH(const Point& p, points) {
    time_t gapLength = p.time - prior;
    Point newPoint(p.time, (double)gapLength);
    gaps.push_back(newPoint);
    prior = p.time;
  }
  
  return gaps;
}
/*
TimeSeries::Statistics TimeSeries::summary(time_t start, time_t end) {
  vector<Point> points = this->points(start, end);
  return getStats(points);
}

TimeSeries::Statistics TimeSeries::gapsSummary(time_t start, time_t end) {
  vector<Point> points = this->gaps(start, end);
  return getStats(points);
}

TimeSeries::Statistics TimeSeries::getStats(vector<Point> points) {
  Statistics s;
  
  if (points.size() == 0) {
    return s;
  }
  
  // stats
  int cacheSize = (int)points.size();
  using namespace boost::accumulators;
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::median, tag::variance(lazy)> > acc;
  accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::right> > > quant_right( tag::tail<boost::accumulators::right>::cache_size = cacheSize );
  accumulator_set<double, stats<tag::tail_quantile<boost::accumulators::left> > > quant_left( tag::tail<boost::accumulators::left>::cache_size = cacheSize );
  
  BOOST_FOREACH(const Point& p, points) {
    acc(p.value);
    quant_right(p.value);
    quant_left(p.value);
  }
  
  s.quartiles.q25 = quantile(quant_left, quantile_probability = 0.25);
  s.quartiles.q75 = quantile(quant_right, quantile_probability = 0.75);
  s.quartiles.q50    = extract::median(acc);
  s.mean      = extract::mean(acc);
  s.variance  = extract::variance(acc);
  s.count     = extract::count(acc);
  s.min       = extract::min(acc);
  s.max       = extract::max(acc);
  
  if (s.quartiles.q50 < s.min) { // weird edge case with accumulators. small populations sometimes return values of zero.
    s.quartiles.q50 = NAN;
    s.quartiles.q25 = NAN;
    s.quartiles.q75 = NAN;
  }
  
  return s;
}
*/



void TimeSeries::setRecord(PointRecord::sharedPointer record) {
  if(_points) {
    //_points->reset(name());
  }
  if (!record) {
    PointRecord::sharedPointer pr( new PointRecord() );
    record = pr;
    //cerr << "WARNING: removing record for Time Series \"" << this->name() << "\"" << endl;
  }
  
  _points = record;
  record->registerAndGetIdentifier(this->name(), this->units());
  
}

PointRecord::sharedPointer TimeSeries::record() {
  return _points;
}

void TimeSeries::resetCache() {
  _points->reset(name());
  //_points->registerAndGetIdentifier(this->name(), this->units());
}


void TimeSeries::setUnits(Units newUnits) {
  // changing units means the values here are no good anymore.
  if (! (newUnits == this->units())) {
    this->resetCache();
  }
  _units = newUnits;
}

Units TimeSeries::units() {
  return _units;
}

#pragma mark - Protected Methods

std::ostream& TimeSeries::toStream(std::ostream &stream) {
  stream << "Time Series: \"" << _name << "\"\n";
  if (_clock) {
    stream << "clock: " << *_clock;
  }
  stream << "Units: " << _units << std::endl;
  stream << "Cached Points:" << std::endl;
  stream << *_points;
  return stream;
}
/*
bool TimeSeries::isCompatibleWith(TimeSeries::sharedPointer otherSeries) {
  
  // basic check for compatible regular time series.
  Clock::sharedPointer theirClock = otherSeries->clock(), myClock = this->clock();
  bool clocksCompatible = myClock->isCompatibleWith(theirClock);
  bool unitsCompatible = units().isDimensionless() || units().isSameDimensionAs(otherSeries->units());
  return (clocksCompatible && unitsCompatible);
}

*/

