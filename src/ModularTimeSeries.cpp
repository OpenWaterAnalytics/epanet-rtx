//
//  ModularTimeSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "ModularTimeSeries.h"

using namespace RTX;
using namespace std;

ModularTimeSeries::ModularTimeSeries() : TimeSeries::TimeSeries() {
  _doesHaveSource = false;
}

ModularTimeSeries::~ModularTimeSeries() {
  
}

ostream& ModularTimeSeries::toStream(ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "Connected to: " << *_source << "\n";
  return stream;
}

void ModularTimeSeries::setSource(TimeSeries::sharedPointer sourceTimeSeries) {
  if( isCompatibleWith(sourceTimeSeries) ) {
    _source = sourceTimeSeries;
    _doesHaveSource = true;
    //resetCache();
    // if this is an irregular time series, then set this clock to the same as that guy's clock.
    if (!clock()->isRegular()) {
      setClock(source()->clock());
    }
    // and if i don't have units, just borrow from the source.
    if (units().isDimensionless()) {
      setUnits(_source->units()); // as a copy, in case it changes.
    }
  }
  else {
    cerr << "Incompatible. Could not set source for:\n";
    cerr << *this;
    // TODO -- throw something?
  }
}

TimeSeries::sharedPointer ModularTimeSeries::source() {
  return _source;
}

bool ModularTimeSeries::doesHaveSource() {
  return _doesHaveSource;
}

void ModularTimeSeries::setUnits(Units newUnits) {
  if (!doesHaveSource() || (doesHaveSource() && newUnits.isSameDimensionAs(source()->units()))) {
    TimeSeries::setUnits(newUnits);
  }
  else {
    cerr << "could not set units for time series " << name() << endl;
  }
}

bool ModularTimeSeries::isPointAvailable(time_t time) {
  bool isCacheAvailable = false, isSourceAvailable = false;
  
  isCacheAvailable = TimeSeries::isPointAvailable(time);
  
  if (!isCacheAvailable) {
    isSourceAvailable = source()->isPointAvailable(time);
  }
  
  if (isCacheAvailable || isSourceAvailable) {
    return true;
  }
  else {
    return false;
  }
}

Point ModularTimeSeries::point(time_t time) {
  // check the base-class availability. if it's cached or stored here locally, then send it on.
  // otherwise, check the upstream availability. if it's there, store it locally and pass it on.
  
  // check the requested time for validity
  // if the time is not valid, rewind until a valid time is reached.
  time_t newTime = clock()->validTime(time);

  // if my clock can't find it, maybe my source's clock can?
  if (newTime == 0) {
    time = source()->clock()->validTime(time);
  } else {
    time = newTime;
  }
  
  if (TimeSeries::isPointAvailable(time)) {
    return TimeSeries::point(time);
  }
  else {
    
    Point sourcePoint = source()->point(time);

    if (sourcePoint.isValid()) {
      // create a new point object and convert from source units
      Point aPoint = Point::convertPoint(sourcePoint, source()->units(), units());
      insert(aPoint);
      return aPoint;
    }
    else {
      cerr << "ModularTimeSeries: check point availability first\n";
      // TODO -- throw something? reminder to check point availability first...
      Point point;
      return point;
    }
  }
}

Point ModularTimeSeries::pointBefore(time_t time) {
  time_t timeBefore = clock()->timeBefore(time);
  if (timeBefore == 0) {
    timeBefore = source()->clock()->timeBefore(time);
  }
  return point(timeBefore);
}

Point ModularTimeSeries::pointAfter(time_t time) {
  time_t timeAfter = clock()->timeAfter(time);
  if (timeAfter == 0) {
    timeAfter = source()->clock()->timeAfter(time);
  }
  return point(timeAfter);
}

vector< Point > ModularTimeSeries::points(time_t start, time_t end) {
  // call the source points method to get it to cache points...
  // but only for the time range that we need (i.e., have not generated yet.)
  // todo -- make this smarter. code below is only capable of prefetching the preceeding or succeeding subintervals.
  
  Point firstCachePoint = record()->firstPoint(name());
  Point lastCachePoint = record()->lastPoint(name());
  
  time_t firstTime = firstCachePoint.time();
  time_t lastTime = lastCachePoint.time();
  
  if (firstCachePoint.isValid() && lastCachePoint.isValid()) {
    if (start < firstTime && end >= firstTime && end <= lastTime) {
      // subinterval preceeding the cache
      _source->points(start, firstTime);
    }
    else if (end > lastTime && start <= lastTime && start >= firstTime) {
      // subinterval succeeding the cache
      _source->points(lastTime, end);
    }
    else if ( (end < firstTime || start > lastTime) || (start < firstTime && end > lastTime)) {
      _source->points(start, end);
    }
  } else {
    _source->points(start, end);
  }
  
  
  // then call the base class method.
  return TimeSeries::points(start, end);
}
