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
    
    Point sp = source()->point(time);
    if (sp.isValid && sp.time == time) {
      // if the source has the point, then no interpolation is needed.
      interpolatedPoint = Point::convertPoint(sp, source()->units(), units());
    }
    else {
      std::pair< Point, Point > sourcePoints = source()->adjacentPoints(time);
      p0 = sourcePoints.first;
      p1 = sourcePoints.second;
      
      if (!p0.isValid || !p1.isValid || p0.quality==Point::missing || p1.quality==Point::missing) {
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
  
  PointRecord::time_pair_t prRange = record()->range(name());
  if (prRange.first <= newStart && newEnd <= prRange.second) {
    // the record's range covers it, but
    // the record may not be continuous -- so check it.
    time_t now = newStart;
    time_t period = this->period();
    vector<Point> rpVec = record()->pointsInRange(name(), newStart, newEnd);
    vector<Point> stitchedPoints;
    vector<Point>::const_iterator it = rpVec.begin();
    while (it != rpVec.end()) {
      
      Point recordPoint = *it;
      if (recordPoint.time == now) {
        stitchedPoints.push_back(recordPoint);
        now += period;
      }
      else {
        // aha, a gap.
        // determine the size of the gap
        time_t gapStart, gapEnd;
        
        gapStart = now;
        gapEnd = recordPoint.time;
        
        if (gapEnd - gapStart < period) {
          cerr << "gap is crazy" << endl;
          ++it;
          continue;
        }
        
        // todo -- do something more fancy with this. for now it just calls the source method to pre-fetch the points into the source's cache.
        // this speeds up the brute-force point pecking just below.
        vector<Point> gapPoints = source()->points(gapStart, gapEnd);
        
        while (now < gapEnd) {
          // stitch in some new points
          //cout << "stitching" << endl;
          Point stitchPoint = Resampler::point(now);
          if (stitchPoint.isValid) {
            stitchedPoints.push_back(stitchPoint);
          }
          else {
            cerr << "invalid interstitial point" << endl;
          }
          now += period;
        }
        
      }
      
      ++it;
    }
    
    return stitchedPoints;
  }
  
  // otherwise, construct new points.
  // get the times for the source query
  time_t sourceStart, sourceEnd;
  {
    time_t s = source()->pointBefore(newStart).time;
    time_t e = source()->pointAfter(newEnd).time;
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
  while (left.time > now) {
    now = clock()->timeAfter(now);
  }
  
  while (sourceIt != sourcePoints.end() && now <= newEnd) {
    
    while (right.time < now) {
      // increment our source point iterator
      // if we've gone past our neighbor points.
      left = right;
      ++sourceIt;
      if (sourceIt == sourcePoints.end()) {
        break;
      }
      right = *sourceIt;
    }
    if (right.time == left.time) {
      break;
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
  return (units().isDimensionless() || units().isSameDimensionAs(withTimeSeries->units()));
}

Point Resampler::interpolated(Point p1, Point p2, time_t t) {
  
  if (p1.time == t) {
    return p1;
  }
  if (p2.time == t) {
    return p2;
  }
  
  
  time_t dt = p2.time - p1.time;
  double dv = p2.value - p1.value;
  time_t dt2 = t - p1.time;
  double dv2 = dv * dt2 / dt;
  double newValue = p1.value + dv2;
  double newConfidence = (p1.confidence + p2.confidence) / 2; // TODO -- more elegant confidence estimation
  return Point(t, newValue, Point::interpolated, newConfidence);
}
