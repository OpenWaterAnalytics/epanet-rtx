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
      
      interpolatedPoint = interpolated(p0, p1, time, source()->units());
    }
    
    insert(interpolatedPoint);
    return interpolatedPoint;
  }
}

vector<Point> Resampler::points(time_t start, time_t end) {
  Units sourceU = source()->units();
  
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
    if (rpVec.size() == 0) {
      // fully internal
      // like this:
      // ppppp---[--- req ---]---ppppp
      
      rpVec.push_back(record()->pointBefore(name(), newStart));
      rpVec.push_back(record()->pointAfter(name(), newEnd));
      
    }
    
    
    vector<Point> stitchedPoints;
    vector<Point>::const_iterator it = rpVec.begin();
    while (it != rpVec.end()) {
      Point recordPoint = *it;
      // cout << "P: " << recordPoint << endl;
      
      if (recordPoint.time == now) {
        stitchedPoints.push_back(recordPoint);
        now += period;
      }
      else if (recordPoint.time < now) {
        ++it;
        continue;
      }
      else {
        // aha, a gap.
        // determine the size of the gap
        time_t gapStart, gapEnd;
        
        gapStart = now;
        gapEnd = recordPoint.time;
        
        
        Point gapSourceStart = source()->pointBefore(gapStart);
        Point gapSourceEnd = source()->pointAfter(gapEnd);
        
        vector<Point> gapSourcePoints = source()->points(gapSourceStart.time, gapSourceEnd.time);
        vector<Point> gapPoints = interpolatedGivenSourcePoints(gapStart, gapEnd, gapSourcePoints);
        if (gapPoints.size() > 0) {
          this->insertPoints(gapPoints);
          now = gapPoints.back().time;
          BOOST_FOREACH(Point p, gapPoints) {
            stitchedPoints.push_back(p);
          }
        }
        else {
          // skipping the gap.
          now = recordPoint.time;
          continue; // skip the ++it
        }
        
      }
      
      ++it;
    }
    
    return stitchedPoints;
  }
  
  // otherwise, construct new points.
  // get the times for the source query
  Point sourceStart, sourceEnd;
  {
    Point s = source()->pointBefore(newStart);
    Point e = source()->pointAfter(newEnd);
    sourceStart = (s.time>0)? s : Point(newStart,0); //newStart;
    sourceEnd = (e.time>0)? e : Point(newEnd,0); //newEnd;
  }
   
  // get the source points
  std::vector<Point> sourcePoints = source()->points(sourceStart.time, sourceEnd.time);
  if (sourcePoints.size() < 2) {
    vector<Point> empty;
    return empty;
  }
  
  // create a place for the new points
  std::vector<Point> resampled;
  
  resampled = interpolatedGivenSourcePoints(newStart, newEnd, sourcePoints);
  
  // finally, add the points to myself.
  this->insertPoints(resampled);
  
  return resampled;
}


#pragma mark - Protected Methods

bool Resampler::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  // this time series can resample, so override with true always.
  return (units().isDimensionless() || units().isSameDimensionAs(withTimeSeries->units()));
}



vector<Point> Resampler::interpolatedGivenSourcePoints(time_t fromTime, time_t toTime, std::vector<Point> sourcePoints) {
  std::vector<Point> resampled;
  Units sourceU = source()->units();
  
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
    std::vector<Point>::size_type s = (toTime - fromTime) / period() + 1;
    resampled.reserve(s);
  }
  
  
  // scrub through the source Points and interpolate as we go.
  time_t now = fromTime;
  vector<Point>::const_iterator sourceIt = sourcePoints.begin();
    
  Point sourceLeft, sourceRight;
  sourceLeft = *sourceIt;
  ++sourceIt;
  sourceRight = *sourceIt;
  
  // fast forward now to meet our first available source point.
  while (sourceLeft.time > now && now <= toTime) {
    // warning!
    now = clock()->timeAfter(now);
  }
  
  // start at the beginning, don't go past the end
  while (now <= toTime && sourceIt != sourcePoints.end()) {
    
    // are we in the right position?
    if (sourceLeft.time <= now && now <= sourceRight.time ) {
      // ok, interpolate.
      Point interp = interpolated(sourceLeft, sourceRight, now, sourceU);
      if (interp.isValid) {
        resampled.push_back(interp);
      }
      now = clock()->timeAfter(now);
      continue;
    }
    
    // if we weren't in the right position, let's try to get there.
    else if ( now < sourceLeft.time ) {
      // this shouldn't happen
      cerr << "what did you do??" << endl;
    }
    
    else if ( sourceRight.time < now ) {
      sourceLeft = sourceRight;
      ++sourceIt;
      if (sourceIt == sourcePoints.end()) {
        cerr << "ending resample before the requested bound" << endl;
        break;
      }
      sourceRight = *sourceIt;
    }
    
  }
  
  
  return resampled;
  
}


Point Resampler::interpolated(Point p1, Point p2, time_t t, Units fromUnits) {
  
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
