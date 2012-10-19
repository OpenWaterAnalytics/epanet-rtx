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
  Point aPoint = ModularTimeSeries::point(time);
  
  double pointValue = aPoint.value();
  pointValue += offset();
  
  return Point(aPoint.time(), pointValue);
}

void OffsetTimeSeries::setOffset(double offset) {
  _offset = offset;
}

double OffsetTimeSeries::offset() {
  return _offset;
}