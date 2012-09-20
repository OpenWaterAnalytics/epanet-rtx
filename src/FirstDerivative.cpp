//
//  FirstDerivative.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  http://tinyurl.com/epanet-rtx

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

Point::sharedPointer FirstDerivative::point(time_t time) {
  
  // return obj
  Point::sharedPointer point;

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
      Point::sharedPointer secondPoint = source()->point(time);
      Point::sharedPointer firstPoint = source()->point( source()->clock()->timeBefore(time) );
      if (!firstPoint) {
        firstPoint.reset( new Point() );
      }
      time_t dt = secondPoint->time() - firstPoint->time();
      double dv = secondPoint->value() - firstPoint->value();
      double dvdt = dv / double(dt);
      point.reset( new Point(time, dvdt));
      Point::sharedPointer myNewPoint = Point::convertPoint(*point, source()->units() / RTX_SECOND, this->units());
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
