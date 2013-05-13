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
  return (int)(this->windowSize() / 2);
}

#pragma mark - Public Overridden Methods

bool MovingAverage::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  // a MA can intrinsically resample
  return true;
}


#pragma mark - Protected

Point MovingAverage::filteredSingle(pVec_cIt &vecStart, pVec_cIt &vecEnd, pVec_cIt &vecPos, time_t t, RTX::Units fromUnits) {
  // quick sanity check
  if (vecPos == vecEnd) {
    return Point();
  }
  
//  cout << endl << "@@@ Moving Average t = " << t << endl;
  
  pVec_cIt fwd_it = vecPos;
  pVec_cIt back_it = vecPos;
  alignVectorIterators(vecStart, vecEnd, vecPos, t, back_it, fwd_it);
  
  
//  cout << "=======================" << endl;
  // great, we've got points on either side.
  // now we take an average.
//  int iPoints = 0;
  accumulator_set<double, stats<tag::mean> > meanAccumulator;
  while (back_it != fwd_it+1) {
    Point p = *back_it;
    meanAccumulator(p.value);
    ++back_it;
//    ++iPoints;
//    cout << p;
//    if (p.time == t) {
//      cout << " ***";
//    }
//    cout << endl;
  }
  
  double meanValue = mean(meanAccumulator);
  meanValue = Units::convertValue(meanValue, fromUnits, this->units());
  
//  cout << "+++++ Mean value (" << iPoints << " points) = " << meanValue << endl;
//  cout << "=======================" << endl;
  
  Point filtered(t, meanValue, Point::Qual_t::averaged);
  return filtered;
}



