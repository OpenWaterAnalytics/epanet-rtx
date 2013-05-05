//
//  Resampler.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Resampler.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

Resampler::Resampler() {
  
}

Resampler::~Resampler() {
  
}

#pragma mark - Public Methods

Point Resampler::point(time_t time) {
  // check the base-class availability. if it's cached or stored here locally, then send it on.
  // otherwise, check the upstream availability. if it can be produced, store it locally and pass it on.
  Point p = TimeSeries::point(time);
  if (p.isValid) {
    return p;
  }
  else {
    // check the requested time for validity...
    if ( !(clock()->isValid(time)) ) {
      // if the time is not valid, get out.
      return Point();
      //time = clock()->timeBefore(time);
    }
    // now that that's settled, get some source points and interpolate.
    Point p0, p1, interpolatedPoint;
    
    std::pair< Point, Point > sourcePoints = source()->adjacentPoints(time);
    p0 = sourcePoints.first;
    p1 = sourcePoints.second;
    
    for (int i = 1; i < margin(); ++i) {
      Point p0prime, p1prime;
      p0prime = source()->pointBefore(p0.time);
      //p1prime = source()->pointAfter(p1.time);
      p0 = p0prime.isValid ? p0prime : p0;
      //p1 = p1prime.isValid ? p1prime : p1;
    }
    
    
    if (!p0.isValid || !p1.isValid || p0.quality==Point::missing || p1.quality==Point::missing) {
      // get out while the gettin's good
      interpolatedPoint = Point(time, 0, Point::missing);
      return interpolatedPoint;
    }
    
    pointBuffer_t buf;
    buf.set_capacity(2);
    buf.push_back(p0);
    buf.push_back(p1);
    
    interpolatedPoint = filteredSingle(buf, time, source()->units());
    
    
    insert(interpolatedPoint);
    return interpolatedPoint;
  }
}

#pragma mark - Protected Methods

bool Resampler::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  // this time series can resample, so override with true always.
  return (units().isDimensionless() || units().isSameDimensionAs(withTimeSeries->units()));
}



std::vector<Point> Resampler::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  std::vector<Point> resampled;
  Point fromPoint, toPoint, newPoint;
  time_t fromSourceTime, toSourceTime;

  // Try to expand the point range on either side by half the margin()
  fromPoint = sourceTs->pointBefore(fromTime);
  toPoint = sourceTs->pointAfter(toTime);
  for (int i = 1; i < this->margin()/2; ++i) {
    newPoint = sourceTs->pointBefore(fromPoint.time);
    if (newPoint.isValid) {fromPoint = newPoint;}
    newPoint = sourceTs->pointAfter(toPoint.time);
    if (newPoint.isValid) {toPoint = newPoint;}
  }
  
  fromSourceTime = fromPoint.isValid ? fromPoint.time : fromTime;
  toSourceTime = toPoint.isValid ? toPoint.time : toTime;
  
  // get the source points
  std::vector<Point> sourcePoints = sourceTs->points(fromSourceTime, toSourceTime);
  if (sourcePoints.size() < 2) {
    return resampled;
  }
  
  // check the source points
  if (sourcePoints.size() < 2) {
    return resampled;
  }
    
  // also check that there is some data in between the requested bounds
  if ( sourcePoints.back().time < fromTime || toTime < sourcePoints.front().time ) {
    return resampled;
  }
  
  if ( fromTime < sourcePoints.front().time || sourcePoints.back().time < toTime) {
    // source data doesn't cover my whole range...
    cerr << "source data does not cover requested range" << endl;
  }
  
  // reserve for the new points
  {
    if (this->clock()->isRegular()) {
      std::vector<Point>::size_type s = (toTime - fromTime) / period() + 1;
      resampled.reserve(s);
    }
    else {
      resampled.reserve(sourcePoints.size());
    }
    
  }
  
  
  // scrub through the source Points and interpolate as we go.
  time_t now;
  if (!this->clock()->isValid(fromTime)) {
    now = this->clock()->timeAfter(fromTime);
  }
  else {
    now = fromTime;
  }
  
  // Initialize the points buffer
  pointBuffer_t buffer;
  buffer.set_capacity(this->margin()+1);
  vector<Point>::const_iterator bufferIt = sourcePoints.begin();
  buffer.push_back(*bufferIt);
  // todo - this will get out of alignment if we didn't succeed in expanding the sourcePoints range
  for (int i = 1; i < buffer.capacity(); ++i) {
    newPoint = *(++bufferIt);
    buffer.push_back(newPoint);
  }
  
  // Initialize the interpolation interval
  Point sourceLeft, sourceRight;
  vector<Point>::const_iterator sourceIt = sourcePoints.begin();
  sourceLeft = *(sourceIt);
  sourceRight = *(++sourceIt);
  while (now > sourceRight.time && sourceIt != sourcePoints.end()) {
    sourceLeft = sourceRight;
    sourceRight = *(++sourceIt);
  }
  
  // start at the beginning, don't go past the end
  while (now <= toTime && bufferIt != sourcePoints.end()) {
    
    // are we in the right position?
    if (sourceLeft.time <= now && now <= sourceRight.time ) {
      // ok, interpolate.
      Point interp = filteredSingle(buffer, now, sourceTs->units());
      if (interp.isValid) {
        resampled.push_back(interp);
      }
      now = clock()->timeAfter(now);
      continue;
    }
    
    // if we weren't in the right position, let's try to get there.
    else if ( now < sourceLeft.time ) {
      // this shouldn't happen. but if it does, now could be zero
      if (now == 0) {
        // that means the window is exhausted.
        break;
      }
      // ok, this really shouldn't happen.
      cerr << "what did you do??" << endl;
      break;
    }
    
    else if ( sourceRight.time < now ) {
      sourceLeft = sourceRight;
      sourceRight = *(++sourceIt);
      newPoint = *(++bufferIt);
      buffer.push_back(newPoint);
      if (bufferIt == sourcePoints.end()) {
        cerr << "ending resample before the requested bound" << endl;
      }
    }
    
  }
  
  
  return resampled;
  
}


// filteredSingle is responsible for generating a single Point that represents whatever transformation
// the class is meant to provide. give it a const address of the pointBuffer_t window, which probably
// contains the source points, and the source units so it can do the unit conversion.

Point Resampler::filteredSingle(const pointBuffer_t& window, time_t t, Units fromUnits) {
  if (window.size() < 2) {
    return Point();
  }
  pointBuffer_t::const_iterator it = window.begin();
  Point p1, p2;
  p1 = *it;
  p2 = *(++it);

  Point sourceInterp = Point::linearInterpolate(p1, p2, t);
  return Point::convertPoint(sourceInterp, fromUnits, this->units());
}
