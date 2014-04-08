//
//  FailoverTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/12/14.
//
//

#include "FailoverTimeSeries.h"

#include <boost/foreach.hpp>


using namespace RTX;
using namespace std;


bool FailoverTimeSeries::isCompatibleWith(TimeSeries::sharedPointer withTimeSeries) {
  return (units().isDimensionless() || units().isSameDimensionAs(withTimeSeries->units()));
  // it's ok if the clocks aren't compatible.
}

time_t FailoverTimeSeries::maximumStaleness() {
  return _stale;
}

void FailoverTimeSeries::setMaximumStaleness(time_t stale) {
  if (this->clock()->period() <= (int)stale) {
    _stale = stale;
  }
  else {
    cerr << "cannot set staleness: specified staleness is less than the clock's period." << endl;
  }
}

TimeSeries::sharedPointer FailoverTimeSeries::failoverTimeseries() {
  return _failover;
}

void FailoverTimeSeries::setFailoverTimeseries(TimeSeries::sharedPointer ts) {
  _failover = ts;
}



void FailoverTimeSeries::swapSourceWithFailover() {
  if (!this->source() || !this->failoverTimeseries()) {
    cerr << "nothing to swap" << endl;
    return;
  }
  TimeSeries::sharedPointer tmp = this->source();
  this->setSource(this->failoverTimeseries());
  this->setFailoverTimeseries(tmp);
}




#pragma mark - superclass overrides

void FailoverTimeSeries::setSource(TimeSeries::sharedPointer source) {
  ModularTimeSeries::setSource(source);
  if (source) {
    if (source->clock()->isRegular() && this->maximumStaleness() < source->clock()->period()) {
      this->setMaximumStaleness(source->clock()->period());
    }
  }
}


vector<Point> FailoverTimeSeries::filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  if (!this->failoverTimeseries()) {
    return ModularTimeSeries::filteredPoints(sourceTs, fromTime, toTime);
  }
  
  Units sourceU = sourceTs->units();
  Units failoverUnits = this->failoverTimeseries()->units();
  Units myUnits = this->units();
  std::vector<Point> sourcePoints = sourceTs->points(fromTime, toTime);
  
  std::vector<Point> thePoints;
  thePoints.reserve(sourcePoints.size());
  
  time_t prevTime;
  
  
  // guard for missing source points.
  if (sourcePoints.size() > 0) {
    // if there are source points, no problem.
    prevTime = sourcePoints.front().time;
  }
  else {
    // if there are no source points returned, then set up the vars
    // so that I will have to fill the entire gap with the failover source.
    prevTime = fromTime - (this->maximumStaleness() + 1);
    sourcePoints.push_back(Point(toTime + 1, 0));
  }
  
  
  if (fromTime < prevTime) {
    // this causes the next block of code to fill the leading gap (if any)
    prevTime = fromTime;
  }
  if (sourcePoints.back().time < toTime) {
    // this causes the next block of code to fill trailing gap (if any)
    sourcePoints.push_back(Point(toTime+1,0));
  }
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    
    time_t now = p.time;
    if (now - prevTime > this->maximumStaleness()) {
      // get a set of points from the source
      vector<Point> missingPoints = this->failoverTimeseries()->points(prevTime, now);
      BOOST_FOREACH(const Point& mp, missingPoints) {
        thePoints.push_back( Point::convertPoint(mp, failoverUnits, myUnits) );
      }
    }
    else {
      thePoints.push_back( Point::convertPoint(p, sourceU, myUnits) );
    }
    prevTime = p.time;
  }
  
  
  return thePoints;
}







