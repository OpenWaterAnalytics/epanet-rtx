//
//  Resampler.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Resampler.h"

using namespace RTX;
using namespace std;

Resampler::Resampler() : ModularTimeSeries::ModularTimeSeries() {
  
}

Resampler::~Resampler() {
  
}

#pragma mark - Public Methods

Point Resampler::point(time_t time) {
  // check the base-class availability. if it's cached or stored here locally, then send it on.
  // otherwise, check the upstream availability. if it can be produced, store it locally and pass it on.
  if (TimeSeries::isPointAvailable(time)) {
    return TimeSeries::point(time);
  }
  else {
    // check the requested time for validity...
    if ( !(clock()->isValid(time)) ) {
      // if the time is not valid, rewind until a valid time is reached.
      time = clock()->timeBefore(time);
    }
    // now that that's settled, get some source points and interpolate.
    Point p0, p1, interpolatedPoint;
    
    if (source()->isPointAvailable(time)) {
      // if the source has the point, then no interpolation is needed.
      interpolatedPoint = Point::convertPoint(source()->point(time), source()->units(), units());
    }
    else {
      std::pair< Point, Point > sourcePoints = source()->adjacentPoints(time);
      p0 = sourcePoints.first;
      p1 = sourcePoints.second;
      
      if (!p0.isValid() || !p1.isValid() || p0.quality()==Point::missing || p1.quality()==Point::missing) {
        // get out while the gettin's good
        interpolatedPoint = Point(time, 0, Point::missing);
        return interpolatedPoint;
      }
      
      Point interp = interpolated(p0, p1, time);
      interpolatedPoint = Point::convertPoint( interp, source()->units(), units() );
    }
    
    insert(interpolatedPoint);
    return interpolatedPoint;
  }
}

vector<Point> Resampler::points(time_t start, time_t end) {
  Units sourceU = source()->units();
  Units myU = units();
  
  // make sure the times are aligned with the clock.
  time_t newStart = (clock()->isValid(start)) ? start : clock()->timeAfter(start);
  time_t newEnd = (clock()->isValid(end)) ? end : clock()->timeBefore(end);
  
  // get the times for the source query
  time_t sourceStart, sourceEnd;
  {
    time_t s = source()->clock()->timeBefore(newStart);
    time_t e = source()->clock()->timeAfter(newEnd);
    sourceStart = (s>0)? s : newStart;
    sourceEnd = (e>0)? e : newEnd;
  }
  // get the source points
  std::vector<Point> sourcePoints = source()->points(sourceStart, sourceEnd);
  if (sourcePoints.size() < 2) {
    vector<Point> empty;
    return empty;
  }
  
  // create a place for the new points
  std::vector<Point> resampled;
  {
    std::vector<Point>::size_type s = (newEnd - newStart) / period();
    resampled.reserve(s);
  }
  
  
  // scrub through the source Points and interpolate as we go.
  time_t now = newStart;
  vector<Point>::const_iterator sourceIt = sourcePoints.begin();
  Point left, right;
  left = *sourceIt;
  ++sourceIt;
  right = *sourceIt;
  
  // fast forward now to meet our first available source point.
  while (left.time() > now) {
    now = clock()->timeAfter(now);
  }
  
  while (sourceIt != sourcePoints.end() && now <= newEnd) {
    
    if (right.time() < now) {
      // increment our source point iterator
      // if we've gone past our neighbor points.
      left = right;
      ++sourceIt;
      if (sourceIt == sourcePoints.end()) {
        break;
      }
      right = *sourceIt;
    }
    
    // we have two neighboring points. let's interpolate.
    Point iS = interpolated(left, right, now); // source units
    Point iP = Point::convertPoint(iS, sourceU, myU); // my units
    resampled.push_back(iP);
    
    // and it is not now anymore
    now = clock()->timeAfter(now);
  
  }
  
  // finally, add the points to myself.
  this->insertPoints(resampled);
  
  return resampled;
}


#pragma mark - Protected Methods

bool Resampler::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  // this time series can resample, so override with true always.
  return true;
}

Point Resampler::interpolated(Point p1, Point p2, time_t t) {
  double newValue = p1.value() + (( t - p1.time() )*( p2.value() - p1.value() )) / ( p2.time() - p1.time() );
  double newConfidence = (p1.confidence() + p2.confidence()) / 2; // TODO -- more elegant confidence estimation
  return Point(t, newValue, Point::interpolated, newConfidence);
}
