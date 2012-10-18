//
//  FirstDerivative.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "FirstDerivative.h"

using namespace RTX;

FirstDerivative::FirstDerivative() : ModularTimeSeries::ModularTimeSeries() {
  
}

FirstDerivative::~FirstDerivative() {
  
}

void FirstDerivative::setSource(TimeSeries::sharedPointer source) {
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  ModularTimeSeries::setSource(source);
  
  // get the rate of change units
  Units rate = source->units() / RTX_SECOND;
  
  this->setUnits(rate);  // re-set the units.
}

void FirstDerivative::setUnits(Units newUnits) {
  
  // only set the units if there is no source or the source's rate is dimensionally consistent with the passed-in units.
  if (!source() || (source()->units() / RTX_SECOND).isSameDimensionAs(newUnits) ) {
    // just use the base-est class method for this, since we don't really care
    // if the new units are the same as the source units.
    TimeSeries::setUnits(newUnits);
  }
  else {
    std::cerr << "units are not dimensionally consistent" << std::endl;
  }
}

Point FirstDerivative::point(time_t time) {
  
  // return obj
  Point point;

  // check the requested time for validity...
  if ( !(clock()->isValid(time)) ) {
    // if the time is not valid, rewind until a valid time is reached.
    time = clock()->timeBefore(time);
  }
  
  if (TimeSeries::isPointAvailable(time)) {
    return TimeSeries::point(time);
  }
  else {
    if (source()->isPointAvailable(time)) {
      Point secondPoint = source()->point(time);
      Point firstPoint = source()->point( source()->clock()->timeBefore(time) );
      if (!firstPoint.isValid()) {
        firstPoint = Point();
      }
      time_t dt = secondPoint.time() - firstPoint.time();
      double dv = secondPoint.value() - firstPoint.value();
      double dvdt = Units::convertValue(dv, source()->units() / RTX_SECOND, this->units()) / double(dt);
      point = Point(time, dvdt);
      insert(point);
    }
    else {
      std::cerr << "no point - check availability first\n";
      // TODO -- throw something? reminder to check point availability first...
    }
  }
  return point;
}

std::ostream& FirstDerivative::toStream(std::ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "First Derivative Of: " << *source() << "\n";
  return stream;
}
