//
//  ConstantSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  http://tinyurl.com/epanet-rtx

#include <iostream>

#include "ConstantSeries.h"

using namespace RTX;

ConstantSeries::ConstantSeries(double constantValue) : TimeSeries::TimeSeries() {
  _value = constantValue;
}

ConstantSeries::~ConstantSeries() {
  
}

Point::sharedPointer ConstantSeries::point(time_t time) {
  // check the requested time for validity, and rewind if necessary.
  time = clock()->validTime(time);

  Point::sharedPointer aPoint(new Point(time, _value, RTX::Point::good));
  return aPoint;
}

void ConstantSeries::setValue(double value) {
  _value = value;
  resetCache();
}