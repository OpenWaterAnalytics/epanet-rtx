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



TimeSeries::PointCollection MovingAverage::filterPointsAtTimes(std::set<time_t> times) {
  vector<Point> filteredPoints;
  
  time_t firstTime = *(times.cbegin());
  time_t lastTime = *(times.crbegin());
  
  int margin = this->windowSize() / 2;
  
  // expand source lookup bounds, counting as we go and ignoring invalid points.
  for (int leftSeek = 0; leftSeek <= margin; ) {
    Point p = this->source()->pointBefore(firstTime);
    if (p.isValid && p.time != 0) {
      firstTime = p.time;
      ++leftSeek;
    }
    if (p.time == 0) {
      // end of valid points
      break; // out of for(leftSeek)
    }
  }
  for (int rightSeek = 0; rightSeek <= margin; ) {
    Point p = this->source()->pointAfter(lastTime);
    if (p.isValid && p.time != 0) {
      lastTime = p.time;
      ++rightSeek;
    }
    if (p.time == 0) {
      // end of valid points
      break; // out of for(rightSeek)
    }
  }
  
  // get the source's points, but
  // only retain valid points.
  vector<Point> sourcePoints;
  {
    // use our new righ/left bounds to get the source data we need.
    vector<Point> sourceRaw = this->source()->points(firstTime, lastTime);
    BOOST_FOREACH(Point p, sourceRaw) {
      if (p.isValid) {
        sourcePoints.push_back(p);
      }
    }
  }
  
  // set up some iterators to start scrubbing through the source data.
  typedef std::vector<Point>::const_iterator pVec_cIt;
  pVec_cIt vecBegin = sourcePoints.cbegin();
  pVec_cIt vecEnd = sourcePoints.cend();
  
  
  // simple approach: take each proposed time value, and search through the vector for where
  // to place the left/right cursors, then take the average.
  
  BOOST_FOREACH(const time_t now, times) {
    
    // initialize the cursors
    pVec_cIt seekCursor = vecBegin;
    
    // maybe the data doesn't support this particular time we want
    if (seekCursor->time > now) {
      continue; // go to the next time
    }
    
    // seek to this time value in the source data
    while (seekCursor->time < now && seekCursor != vecEnd) {
      ++seekCursor;
    }
    // have we run off the end?
    if (seekCursor == vecEnd) {
      break; // no more work here.
    }
    
    // back-off to the left
    pVec_cIt leftCurs = seekCursor;
    for (int iLeft = 0; iLeft < margin; ) {
      --leftCurs;
      ++iLeft;
      // did we stumble upon the vector.begin?
      if (leftCurs == vecBegin) {
        break;
      }
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
    meanPoint.addQualFlag(Point::averaged);
//    cout << "accum: " << nAccumulated << endl;
    
    Point filtered = Point::convertPoint(meanPoint, this->source()->units(), this->units());
    filteredPoints.push_back(filtered);
  }
  
  return PointCollection(filteredPoints, this->units());
}


