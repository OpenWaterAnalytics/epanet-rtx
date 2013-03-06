//
//  FirstDerivative.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include "FirstDerivative.h"

using namespace std;
using namespace RTX;

FirstDerivative::FirstDerivative() : ModularTimeSeries::ModularTimeSeries() {
  
}

FirstDerivative::~FirstDerivative() {
  
}

void FirstDerivative::setSource(TimeSeries::sharedPointer source) {
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  ModularTimeSeries::setSource(source);
  
  // get the rate of change units
  Units rate = source->units() / RTX_SECOND;
  
  this->setUnits(rate);  // re-set the units.
}

void FirstDerivative::setUnits(Units newUnits) {
  
  // only set the units if there is no source or the source's rate is dimensionally consistent with the passed-in units.
  if (!source() || (source()->units() / RTX_SECOND).isSameDimensionAs(newUnits) ) {
    // just use the base-est class method for this, since we don't really care
    // if the new units are the same as the source units.
    TimeSeries::setUnits(newUnits);
  }
  else if (!units().isDimensionless()) {
    std::cerr << "units are not dimensionally consistent" << std::endl;
  }
}

Point FirstDerivative::point(time_t time) {
  
  // return obj
  Point p;

  /* check the requested time for validity...
  if ( !(clock()->isValid(time)) ) {
    // if the time is not valid, rewind until a valid time is reached.
    time = clock()->timeBefore(time);
  }
  */
  p = TimeSeries::point(time);
  if (p.isValid) {
    return p;
  }
  else {
    std::pair<Point,Point> adjacent = source()->adjacentPoints(time);
    //Point secondPoint = source()->point(time);
    //if (!secondPoint.isValid) {
    Point secondPoint = adjacent.second;
    //}
    Point firstPoint = adjacent.first;
    if (!firstPoint.isValid) {
      firstPoint = Point();
    }
    Point p = deriv(firstPoint, secondPoint, time);
    insert(p);
  }
  return p;
}

std::vector<Point> FirstDerivative::points(time_t start, time_t end) {
  
  Units sourceU = source()->units();
  Units myU = units();
  
  // make sure the times are aligned with the clock.
  time_t newStart = (clock()->isValid(start)) ? start : clock()->timeAfter(start);
  time_t newEnd = (clock()->isValid(end)) ? end : clock()->timeBefore(end);
  
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
  std::vector<Point> fd;
  {
    std::vector<Point>::size_type s = (newEnd - newStart) / period();
    fd.reserve(s);
  }
  
  
  // scrub through the source Points and take derivative as we go.
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
    
    if (left.time == right.time) {
      break;
    }
    // we have two neighboring points. let's do it.
    Point p = deriv(left, right, now); // returns in my units
    fd.push_back(p);
    
    // and it is not now anymore
    now = clock()->timeAfter(now);
    
  }
  
  // finally, add the points to myself.
  this->insertPoints(fd);
  
  return fd;
  
}


Point FirstDerivative::deriv(RTX::Point p1, RTX::Point p2, time_t t) {
  if (!(p1.isValid && p2.isValid)) {
    return Point();
  }
  time_t dt = p2.time - p1.time;
  double dv = p2.value - p1.value;
  double dvdt = Units::convertValue(dv / double(dt), source()->units() / RTX_SECOND, this->units());
  return Point(t, dvdt);
}


std::ostream& FirstDerivative::toStream(std::ostream &stream) {
  TimeSeries::toStream(stream);
  stream << "First Derivative Of: " << *source() << "\n";
  return stream;
}
