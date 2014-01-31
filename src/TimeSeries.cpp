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


using namespace RTX;
using namespace std;
using namespace boost::accumulators;


TimeSeries::TimeSeries() : _units(1) {
  _name = "";  
  _points.reset( new PointRecord() );
  setName("Time Series");
  _clock.reset( new IrregularClock(_points, "Time Series") );
  _units = RTX_DIMENSIONLESS;
  _validTimeRange = std::make_pair(1, LONG_MAX);
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
  if (_clock && !_clock->isRegular()) {
    // reset the clock to point to the new record ID, but only if we start with an irregular clock.
    _clock.reset( new IrregularClock(_points, name) );
  }
  
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

// get a range of points from this TimeSeries' point method
std::vector< Point > TimeSeries::points(time_t start, time_t end) {
  // container for points in this range
  std::vector< Point > points;
  
  // sanity
  if ((start == end) || (start < 0) || (end < 0)) {
    return points;
  }
  
  std::vector<time_t> timeList;
  
  if (_clock) {
    timeList = _clock->timeValuesInRange(start, end);
  }
  
  BOOST_FOREACH(time_t time, timeList) {
    // check the time
    if (! (time >= start && time <= end) ) {
      // skip this time
      std::cerr << "time out of bounds. ignoring." << std::endl;
      continue;
    }
    Point aNewPoint;
    if ( !points.empty() && points.back().time == time) {
      //std::cerr << "duplicate time detected" << std::endl;
      continue;
    }
    aNewPoint = point(time);
    
    if (!aNewPoint.isValid) {
      //std::cerr << "bad point" << std::endl;
    }
    else {
      points.push_back(aNewPoint);
    }
  }
    
  return points;
}


// gets adjacent points such that ( first < time <= second )
std::pair< Point, Point > TimeSeries::adjacentPoints(time_t time) {
  Point previous, next;
  previous = pointBefore(time);
  next = point(time);
  if (!next.isValid) {
    next = pointAfter(time);
  }
  return std::make_pair(previous, next);
}

Point TimeSeries::pointBefore(time_t time) {
  Point myPoint;
  
  time_t timeBefore = clock()->timeBefore(time);
  if (timeBefore > 0) {
    myPoint = point(timeBefore);
  }
  
  return myPoint;
}

Point TimeSeries::pointAfter(time_t time) {
  Point myPoint;
  
  time_t timeAfter = clock()->timeAfter(time);
  if (timeAfter > 0) {
    myPoint = point(timeAfter);

  }
  
  
  /* / if not, we depend on the PointRecord to tell us what the next point is.
  else {
    myPoint = _points->pointAfter(time);  
    myPoint = point(myPoint->time);   // funny, but we have to call the point() method on the time that we discover here. this will call the most-derived point method.
  }
  */
  
  return myPoint;
}

Point TimeSeries::pointAtOrBefore(time_t time) {
  Point p = this->point(time);
  if (!p.isValid) {
    p = this->pointBefore(time);
  }
  return p;
}


TimeSeries::Summary TimeSeries::summary(time_t start, time_t end) {
  Summary s;
  s.points = this->points(start, end);
  
  // gaps
  s.gaps.reserve(s.points.size());
  time_t prior = this->pointBefore(s.points.front().time).time;
  BOOST_FOREACH(const Point& p, s.points) {
    time_t gapLength = p.time - prior;
    Point newPoint(p.time, (double)gapLength);
    s.gaps.push_back(newPoint);
    prior = p.time;
  }
  
  // stats
  accumulator_set<double, features<tag::max, tag::min, tag::count, tag::mean, tag::variance(lazy)> > acc;
  BOOST_FOREACH(const Point& p, s.points) {
    acc(p.value);
  }
  
  s.mean = mean(acc);
  s.variance = variance(acc);
  s.count = extract::count(acc);
  s.min = extract::min(acc);
  s.max = extract::max(acc);
  
  return s;
}


time_t TimeSeries::period() {
  if (_clock) {
    return _clock->period();
  }
  else {
    return 0;
  }
}

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
  
  // if my clock is irregular, then re-set it with the current pointRecord as the master synchronizer.
  if (!_clock || !_clock->isRegular()) {
    _clock.reset( new IrregularClock(_points, name()) );
  }
}

PointRecord::sharedPointer TimeSeries::record() {
  return _points;
}

void TimeSeries::resetCache() {
  _points->reset(name());
  _points->registerAndGetIdentifier(this->name(), this->units());
}

void TimeSeries::setClock(Clock::sharedPointer clock) {
  //_hasClock = (clock ? true : false);
  _clock = clock;
}

Clock::sharedPointer TimeSeries::clock() {
  return _clock;
}

void TimeSeries::setUnits(Units newUnits) {
  // changing units means the values here are no good anymore.
  //this->resetCache();
  _units = newUnits;
}

Units TimeSeries::units() {
  return _units;
}

void TimeSeries::setFirstTime(time_t time) {
  _validTimeRange.first = time;
}

void TimeSeries::setLastTime(time_t time) {
  _validTimeRange.second = time;
}

time_t TimeSeries::firstTime() {
  return _validTimeRange.first;
}

time_t TimeSeries::lastTime() {
  return _validTimeRange.second;
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

bool TimeSeries::isCompatibleWith(TimeSeries::sharedPointer otherSeries) {
  
  // basic check for compatible regular time series.
  Clock::sharedPointer theirClock = otherSeries->clock(), myClock = this->clock();
  bool clocksCompatible = myClock->isCompatibleWith(theirClock);
  bool unitsCompatible = units().isDimensionless() || units().isSameDimensionAs(otherSeries->units());
  return (clocksCompatible && unitsCompatible);
}



