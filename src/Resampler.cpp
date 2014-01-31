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
  _mode = linear;
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
    
    if (!source()) {
      return Point();
    }
    
    // now that that's settled, get some source points and interpolate.
    Point fromPoint, toPoint, newPoint, resampled;
    pair<time_t,time_t> sourceRange = expandedRange(source(), time,time);
    
    // get the source points
    std::vector<Point> sourcePoints = source()->points(sourceRange.first, sourceRange.second);
    
    // check the source points
    bool tooFewPoints = (_mode == step) ? (sourcePoints.size() < 1) : (sourcePoints.size() < 2);
    if (tooFewPoints) {
      return resampled;
    }
    
    // also check that there is some data in between the requested bounds
    if (_mode == step) {
      if ( time < sourcePoints.front().time ) {
        return resampled;
      }
    }
    else {
      if ( sourcePoints.back().time < time || time < sourcePoints.front().time ) {
        return resampled;
      }
    }
    
    pVec_cIt vecStart = sourcePoints.begin();
    pVec_cIt vecEnd = sourcePoints.end();
    pVec_cIt vecPos = sourcePoints.begin();
    resampled = filteredSingle(vecStart, vecEnd, vecPos, time, source()->units());
    insert(resampled);
    return resampled;
  }
}

Resampler::interpolateMode_t Resampler::mode() {
  return _mode;
}

void Resampler::setMode(interpolateMode_t mode) {
  _mode = mode;
}


#pragma mark - Protected Methods

int Resampler::margin() {
  return 1;
}

bool Resampler::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  // this time series can resample, so override with true always.
  return (units().isDimensionless() || units().isSameDimensionAs(withTimeSeries->units()));
}

std::vector<Point> Resampler::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  Units sourceUnits = sourceTs->units();
  std::vector<Point> resampled;
  Point fromPoint, toPoint, newPoint;

  // Try to expand the point range
  pair<time_t,time_t> sourceRange = expandedRange(sourceTs, fromTime, toTime);
  // get the source points
  std::vector<Point> sourcePoints = sourceTs->points(sourceRange.first, sourceRange.second);
  
  
  // check that it's ordered.
  if (sourcePoints.size() > 0) {
    time_t orderTime = sourcePoints.front().time;
    BOOST_FOREACH(const Point& p, sourcePoints) {
      if (p.time < orderTime) {
        cerr << "not ordered" << endl;
      }
      orderTime = p.time;
    }
  }
  
  
  
  // check the source points
  bool tooFewPoints = (_mode == step) ? (sourcePoints.size() < 1) : (sourcePoints.size() < 2);
  if (tooFewPoints) {
    return resampled;
  }

  // also check that there is some data in between the requested bounds
  if (_mode == step) {
    if ( toTime < sourcePoints.front().time ) {
      return resampled;
    }
    
    if ( fromTime < sourcePoints.front().time ) {
      // source data doesn't cover my whole range...
      cerr << "source data does not cover requested range" << endl;
    }
  }
  else {
    if ( sourcePoints.back().time < fromTime || toTime < sourcePoints.front().time ) {
      return resampled;
    }
    
    if ( fromTime < sourcePoints.front().time || sourcePoints.back().time < toTime) {
      // source data doesn't cover my whole range...
      cerr << "source data does not cover requested range" << endl;
    }
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
  
  // iterators for scrubbing through the source points
  pVec_cIt sourceBegin = sourcePoints.begin();
  pVec_cIt sourceEnd = sourcePoints.end();
  pVec_cIt filterLocation = sourcePoints.begin();
  
  // get the filtering location iterator into position.
  // this dude should track pretty closely to "now".
  while (filterLocation != sourceEnd && filterLocation->time < now) {
    ++filterLocation;
  }
  if (filterLocation == sourceEnd) {
    // at worst locate the last point
    --filterLocation;
  }
  
  while (now <= toTime) {
    
    Point p = filteredSingle(sourceBegin, sourceEnd, filterLocation, now, sourceUnits);
    if (p.isValid) {
      resampled.push_back(p);
    }
    else {
      cerr << "Filter failure: " << this->name() << " :: t = " << now << endl;
    }
    
    now = clock()->timeAfter(now);
    if (now == 0) {
      // edge case - could be reached if a resampler doesn't have a clock
      break;
    }
  }
  
  
  return resampled;
}


// expand the given time range so that it considers peripheral points in the source data.
// this range is controlled by a derived class' margin() method.
//
// source:    [x----x----x----x----x----x----x----x----x----x----x----x]
// request:                 {========}
// margin = 1:          [x----x----x----x]
// margin = 2:     [x----x----x----x----x----x]

std::pair<time_t,time_t> Resampler::expandedRange(TimeSeries::sharedPointer sourceTs, time_t start, time_t end) {
  
  // quick check for usage
  if (start == 0 || end == 0 || end < start) {
    cerr << "ERR: Resampler::expandedRange -- check usage" << endl;
    return make_pair(0,0);
  }
  
  
  time_t rangeStart = start, rangeEnd = end;
  
  int margin = this->margin();
  Clock::sharedPointer otherClock = sourceTs->clock();
  
  if (otherClock->isRegular()) {
    // much faster.
    rangeStart = (otherClock->isValid(rangeStart)) ? rangeStart : otherClock->timeBefore(rangeStart);
    rangeEnd = (otherClock->isValid(rangeEnd)) ? rangeEnd : otherClock->timeAfter(rangeEnd);
    
    rangeStart -= otherClock->period() * margin;
    rangeEnd += otherClock->period() * margin;
    
  }
  
  else {
    for (int iBackward = 0; iBackward < margin; ++iBackward) {
      Point behindPoint = sourceTs->pointBefore(rangeStart);
      if (behindPoint.isValid) {
        rangeStart = behindPoint.time;
      }
      else {
        break;
      }
    }
    
    for (int iForward = 0; iForward < margin; ++iForward) {
      Point inFrontPoint = sourceTs->pointAfter(rangeEnd);
      if (inFrontPoint.isValid) {
        rangeEnd = inFrontPoint.time;
      }
      else {
        break;
      }
    }
  }
  
  
  pair<time_t, time_t> newRange(rangeStart,rangeEnd);
  return newRange;
}


// align the position, back, and fwd iterators to where they will need to be.
// back and fwd are margin() points behind and forward of requested time.
// the number of points between back and fwd is 2*margin(), or 2*margin()+1
// if the requested time is on a point.

bool Resampler::alignVectorIterators(pVec_cIt& start, pVec_cIt& end, pVec_cIt& pos, time_t t, pVec_cIt& back, pVec_cIt& fwd) {
  
  // move the position vector to my desired time, or slightly after.
  while (pos != start && pos->time > t) {
    --pos;
  }
  while (pos != end && pos->time < t) {
    ++pos;
  }
  if (pos == end) {
    --pos;
  }
  
  // let's get centered.
  back = pos;
  fwd = pos;
  
  // get some vital info
  int marginDistance = this->margin();
  int iForwards = (t < pos->time) ? 1 : 0; // are we ahead of t? if so, we've got a head start.
  int iBackwards = (pos->time < t) ? 1 : 0; // are we behind t by one point? if so, we've got a head start
  
  // widen the bounds (within the allowable range) to account for my margin
  while (fwd != end && iForwards < marginDistance) {
    ++fwd;
    ++iForwards;
  }
  if (fwd == end) {
    --fwd;
    --iForwards;
  }
  while (back != start && iBackwards < marginDistance) {
    --back;
    ++iBackwards;
  }
  
  bool success = (iForwards == marginDistance && iBackwards == marginDistance);
  
  if (!success) {
    cerr << "cannot align" << endl;
  }
  
  return success;
  
  // ok, all done. everything is passed by ref.
  
}


// filteredSingle is responsible for generating a single Point that represents whatever transformation
// the class is meant to provide. give it a set of iterators that describe the source points,
// and the source units so it can do the unit conversion.

Point Resampler::filteredSingle(pVec_cIt& vecStart, pVec_cIt& vecEnd, pVec_cIt& vecPos, time_t t, Units fromUnits) {
  Point sourceInterp;
  
  pVec_cIt fwd_it = vecPos;
  pVec_cIt back_it = vecPos;
  bool success = alignVectorIterators(vecStart, vecEnd, vecPos, t, back_it, fwd_it);
  
  
  // with any luck, at this point we have the back and fwd iterators positioned just right.
  // one on either side of the time point we need.
  // however, we may have been unable to accomplish this task.
  bool notValid = (_mode == step) ? (t < back_it->time) : ( !success );
  if (notValid) {
    return Point(); // missing some source points
  }
  
  // it's possible that the vecPos is aligned right on the requested time, so check for that:
  if (vecPos->time == t) {
    return Point::convertPoint(*vecPos, fromUnits, this->units());
  }
  else {
    Point p1, p2;
    p1 = *back_it;
    p2 = *fwd_it;
    
    sourceInterp = (_mode == step) ? Point(t, p1.value, Point::good, p1.confidence) : Point::linearInterpolate(p1, p2, t);
    return Point::convertPoint(sourceInterp, fromUnits, this->units());
  }
  
}

