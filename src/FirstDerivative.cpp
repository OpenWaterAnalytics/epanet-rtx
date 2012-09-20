//
//  FirstDerivative.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include "FirstDerivative.h"

using namespace RTX;

FirstDerivative::FirstDerivative() : ModularTimeSeries::ModularTimeSeries() {
  
}

FirstDerivative::~FirstDerivative() {
  
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
