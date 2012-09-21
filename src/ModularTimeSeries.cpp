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

ModularTimeSeries::ModularTimeSeries() : TimeSeries::TimeSeries() {
  _doesHaveSource = false;
}

ModularTimeSeries::~ModularTimeSeries() {
  
}

std::ostream& ModularTimeSeries::toStream(std::ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "Connected to: " << *_source << "\n";
  return stream;
}

void ModularTimeSeries::setSource(TimeSeries::sharedPointer sourceTimeSeries) {
  if( isCompatibleWith(sourceTimeSeries) ) {
    _source = sourceTimeSeries;
    _doesHaveSource = true;
    resetCache();
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
    std::cerr << "Incompatible. Could not set source for:\n";
    std::cerr << *this;
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
    std::cerr << "could not set units for time series " << name() << std::endl;
  }
}

bool ModularTimeSeries::isPointAvailable(time_t time) {
  if (TimeSeries::isPointAvailable(time) || source()->isPointAvailable(time)) {
    return true;
  }
  else {
    return false;
  }
}

Point::sharedPointer ModularTimeSeries::point(time_t time) {
  // check the base-class availability. if it's cached or stored here locally, then send it on.
  // otherwise, check the upstream availability. if it's there, store it locally and pass it on.
  
  // check the requested time for validity...
  // if the time is not valid, rewind until a valid time is reached.
  time = clock()->validTime(time);

  
  if (TimeSeries::isPointAvailable(time)) {
    return TimeSeries::point(time);
  }
  else {
    if (source()->isPointAvailable(time)) {
      // create a new point object and convert from source units
      Point::sharedPointer sourcePoint = source()->point(time);
      Point::sharedPointer aPoint = Point::convertPoint(*sourcePoint, source()->units(), units());
      insert(aPoint);
      return aPoint;
    }
    else {
      std::cerr << "check point availability first\n";
      // TODO -- throw something? reminder to check point availability first...
      Point::sharedPointer point;
      return point;
    }
  }
}

std::vector< Point::sharedPointer > ModularTimeSeries::points(time_t start, time_t end) {
  // call the source points method to get it to cache points...
  _source->points(start, end);
  
  // then call the base class method.
  return TimeSeries::points(start, end);
}
