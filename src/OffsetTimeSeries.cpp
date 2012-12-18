//
//  OffsetTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 10/18/12.
//
//

#include "OffsetTimeSeries.h"

using namespace RTX;

OffsetTimeSeries::OffsetTimeSeries() {
  setOffset(0.);
}

Point OffsetTimeSeries::point(time_t time){
  time = clock()->validTime(time);
  
  
  if (TimeSeries::isPointAvailable(time)) {
    return TimeSeries::point(time);
  }
  else {
    if (source()->isPointAvailable(time)) {
      // create a new point object and convert from source units
      Point sourcePoint = source()->point(time);
      Point convertedSourcePoint = Point::convertPoint(sourcePoint, source()->units(), units());
      double pointValue = convertedSourcePoint.value();
      pointValue += offset();
      
      Point newPoint(convertedSourcePoint.time(), pointValue);
      insert(newPoint);
      return newPoint;
    }
    else {
      std::cerr << "check point availability first\n";
      // TODO -- throw something? reminder to check point availability first...
      Point point;
      return point;
    }
  }
  
  
}

void OffsetTimeSeries::setOffset(double offset) {
  _offset = offset;
}

double OffsetTimeSeries::offset() {
  return _offset;
}