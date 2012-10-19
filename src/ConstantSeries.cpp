//
//  ConstantSeries.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "ConstantSeries.h"

using namespace RTX;

ConstantSeries::ConstantSeries(double constantValue) : TimeSeries::TimeSeries() {
  _value = constantValue;
  Clock::sharedPointer clock1second(new Clock(1));
  setClock(clock1second);
}

ConstantSeries::~ConstantSeries() {
  
}

Point ConstantSeries::point(time_t time) {
  // check the requested time for validity, and rewind if necessary.
  //time = clock()->validTime(time);
  return Point(time, _value, RTX::Point::constant);
}

void ConstantSeries::setValue(double value) {
  _value = value;
  resetCache();
}