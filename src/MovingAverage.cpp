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
  //resetCache();
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
  bool success = alignVectorIterators(vecStart, vecEnd, vecPos, t, back_it, fwd_it);
  
  // with any luck, at this point we have the back and fwd iterators positioned just right.
  // +/- the margin from the time point we need.
  // however, we may have been unable to accomplish this task.
  if (!success) {
    return Point(); // invalid
  }

//  cout << "=======================" << endl;
  // great, we've got points on either side.
  // now we take an average.
//  int iPoints = 0;
  accumulator_set<double, stats<tag::mean> > meanAccumulator;
  accumulator_set<double, stats<tag::mean> > confidenceAccum;
  int nAccumulated = 0;
  while (back_it != fwd_it+1 && nAccumulated++ < this->windowSize()) {
    Point p = *back_it;
    meanAccumulator(p.value);
    confidenceAccum(p.confidence);
    ++back_it;
//    ++iPoints;
//    cout << p;
//    if (p.time == t) {
//      cout << " ***";
//    }
//    cout << endl;
  }
  
  double meanValue = mean(meanAccumulator);
  double confidenceMean = mean(confidenceAccum);
  //meanValue = Units::convertValue(meanValue, fromUnits, this->units());
  Point meanPoint(t, meanValue, Point::good, confidenceMean);
  Point filtered = Point::convertPoint(meanPoint, fromUnits, this->units());
  return filtered;
}



