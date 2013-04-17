//
//  MovingAverage.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "MovingAverage.h"
#include <boost/foreach.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/circular_buffer.hpp>
#include <math.h>

#include <iostream>

using namespace RTX;
using namespace std;
using namespace boost::accumulators;


MovingAverage::MovingAverage() {
  _windowSize = 5;
}

MovingAverage::~MovingAverage() {
    
}

#pragma mark - Added Methods

void MovingAverage::setWindowSize(int numberOfPoints) {
  resetCache();
  _windowSize = numberOfPoints;
}

int MovingAverage::windowSize() {
  return _windowSize;
}

int MovingAverage::margin() {
  return this->windowSize();
}

#pragma mark - Public Overridden Methods

bool MovingAverage::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  // a MA can intrinsically resample
  return true;
}


#pragma mark - Protected

Point MovingAverage::filteredSingle(const pointBuffer_t &window, time_t t, RTX::Units fromUnits) {
  
  pointBuffer_t::const_reverse_iterator rIt = window.rbegin();
  accumulator_set<double, stats<tag::mean> > meanAccumulator;
  
  int i = 0;
  while (i < this->margin() && rIt != window.rend()) {
    Point p = *rIt;
    meanAccumulator(p.value);
    ++rIt;
    ++i;
  }
  
  // todo -- confidence
  
  double meanValue = mean(meanAccumulator);
  meanValue = Units::convertValue(meanValue, fromUnits, this->units());
  
  Point filtered(t, meanValue, Point::Qual_t::averaged);
  return filtered;
}


