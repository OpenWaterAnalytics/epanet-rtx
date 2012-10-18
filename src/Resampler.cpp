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
      
      double newValue = p0.value() + (( time - p0.time() )*( p1.value() - p0.value() )) / ( p1.time() - p0.time() );
      double newConfidence = (p0.confidence() + p1.confidence()) / 2; // TODO -- more elegant confidence estimation
      interpolatedPoint = Point::convertPoint( Point(time, newValue, Point::interpolated, newConfidence), source()->units(), units() );
    }
    
    insert(interpolatedPoint);
    return interpolatedPoint;
  }
}

#pragma mark - Protected Methods

bool Resampler::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  // this time series can resample, so override with true always.
  return true;
}