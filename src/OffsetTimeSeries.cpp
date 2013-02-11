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
  
  Point sourcePoint = source()->point(time);
  Point convertedSourcePoint = Point::convertPoint(sourcePoint, source()->units(), units());
  double pointValue = convertedSourcePoint.value();
  pointValue += offset();
  
  Point newPoint(convertedSourcePoint.time(), pointValue);
  insert(newPoint);
  return newPoint;
  
  
}

void OffsetTimeSeries::setOffset(double offset) {
  _offset = offset;
}

double OffsetTimeSeries::offset() {
  return _offset;
}