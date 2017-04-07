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


#pragma mark - Added Methods

void MovingAverage::setWindowSize(int numberOfPoints) {
  this->invalidate();
  _windowSize = numberOfPoints;
}

int MovingAverage::windowSize() {
  return _windowSize;
}


#pragma mark - Delegate Overridden Methods



PointCollection MovingAverage::filterPointsInRange(TimeRange range) {
  vector<Point> filteredPoints;
  
  TimeRange rangeToResample = range;
  if (this->willResample()) {
    // expand range
    rangeToResample.start = this->source()->timeBefore(range.start + 1);
    rangeToResample.end = this->source()->timeAfter(range.end - 1);
  }
  
  TimeRange queryRange = rangeToResample;
  queryRange.correctWithRange(range);
  
  int margin = this->windowSize() / 2;
  
  // expand source lookup bounds, counting as we go and ignoring invalid points.
  for (int leftSeek = 0; leftSeek <= margin; ) {
    time_t left = this->source()->timeBefore(queryRange.start);
    if (left != 0) {
      queryRange.start = left;
      ++leftSeek;
    }
    else {
      // end of valid points
      break; // out of for(leftSeek)
    }
  }
  for (int rightSeek = 0; rightSeek <= margin; ) {
    time_t right = this->source()->timeAfter(queryRange.end);
    if (right != 0) {
      queryRange.end = right;
      ++rightSeek;
    }
    else {
      // end of valid points
      break; // out of for(rightSeek)
    }
  }
  
  // get the source's points, but
  // only retain valid points.
  vector<Point> sourcePoints;
  {
    // use our new righ/left bounds to get the source data we need.
    PointCollection sourceRaw = this->source()->pointCollection(queryRange);
    sourceRaw.apply([&](Point& p){
      if (p.isValid) {
        sourcePoints.push_back(p);
      }
    });
  }
  
  // set up some iterators to start scrubbing through the source data.
  typedef std::vector<Point>::const_iterator pVec_cIt;
  pVec_cIt vecBegin = sourcePoints.cbegin();
  pVec_cIt vecEnd = sourcePoints.cend();
  
  
  // simple approach: take each point, and search through the vector for where
  // to place the left/right cursors, then take the average.
  
  for(const Point& sPoint : sourcePoints) {
    time_t now = sPoint.time;
    if (!rangeToResample.contains(now)) {
      continue; // out of bounds.
    }
    
    // initialize the cursors
    pVec_cIt seekCursor = sourcePoints.cbegin();
    
    // maybe the data doesn't support this particular time we want
    if (seekCursor->time > now) {
      continue; // go to the next time
    }
    
    // seek to this time value in the source data
    while (seekCursor != vecEnd && seekCursor->time < now ) {
      ++seekCursor;
    }
    // have we run off the end?
    if (seekCursor == vecEnd) {
      break; // no more work here.
    }
    
    // back-off to the left
    pVec_cIt leftCurs = seekCursor;
    for (int iLeft = 0; iLeft < margin; ) {
      // did we stumble upon the vector.begin?
      if (leftCurs == vecBegin) {
        break;
      }
      --leftCurs;
      ++iLeft;
    }
    
    // back-off to the right
    pVec_cIt rightCurs = seekCursor;
    for (int iRight = 0; iRight < margin; ) {
      ++rightCurs;
      // end?
      if (rightCurs == vecEnd) {
        --rightCurs; // shaky standard?
        break;
      }
      ++iRight;
    }
    
    
    // at this point, the left and right cursors should be located properly.
    // take an average between them.
    accumulator_set<double, stats<tag::mean> > meanAccumulator;
    accumulator_set<double, stats<tag::mean> > confidenceAccum;
    int nAccumulated = 0;
    Point meanPoint(now);
    
    seekCursor = leftCurs;
    while (seekCursor != rightCurs+1) {
      
      Point p = *seekCursor;
      meanAccumulator(p.value);
      confidenceAccum(p.confidence);
      meanPoint.addQualFlag(p.quality);
      ++nAccumulated;
      ++seekCursor;
    }
    
    // done with computing the average. Save the new value.
    meanPoint.value = mean(meanAccumulator);
    meanPoint.confidence = mean(confidenceAccum);
    filteredPoints.push_back(meanPoint);
  }
  
  
  PointCollection outData = PointCollection(filteredPoints, source()->units());
  outData.addQualityFlag(Point::rtx_averaged);
  
  bool dataOk = false;
  dataOk = outData.convertToUnits(this->units());
  if (dataOk && this->willResample()) {
    set<time_t> timeValues = this->timeValuesInRange(range);
    dataOk = outData.resample(timeValues);
  }
  
  if (dataOk) {
    return outData;
  }
  
  return PointCollection(vector<Point>(),this->units());
}
