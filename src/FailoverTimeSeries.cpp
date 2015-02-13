//
//  FailoverTimeSeries.cpp
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//


#include "FailoverTimeSeries.h"

#include <boost/foreach.hpp>


using namespace RTX;
using namespace std;

#define MAX(x,y) (((x)>=(y)) ? (x) : (y))     /* maximum of x and y    */


time_t FailoverTimeSeries::maximumStaleness() {
  return _stale;
}

void FailoverTimeSeries::setMaximumStaleness(time_t stale) {
  if (!this->clock() || this->clock()->period() <= (int)stale) {
    _stale = stale;
  }
  else {
    cerr << "cannot set staleness: specified staleness is less than the clock's period." << endl;
  }
}

TimeSeries::_sp FailoverTimeSeries::failoverTimeseries() {
  return _failover;
}

void FailoverTimeSeries::setFailoverTimeseries(TimeSeries::_sp ts) {
  if (this->source() && ts && !this->source()->units().isSameDimensionAs(ts->units())) {
    // conflict
    return;
  }
  
  _failover = ts;
}



void FailoverTimeSeries::swapSourceWithFailover() {
  if (!this->source() || !this->failoverTimeseries()) {
    cerr << "nothing to swap" << endl;
    return;
  }
  TimeSeries::_sp tmp = this->source();
  this->setSource(this->failoverTimeseries());
  this->setFailoverTimeseries(tmp);
}




set<time_t> FailoverTimeSeries::timeValuesInRange(TimeRange range) {
  if (!this->failoverTimeseries() || this->clock()) {
    return TimeSeriesFilter::timeValuesInRange(range);
  }
  return set<time_t>();
}

TimeSeries::PointCollection FailoverTimeSeries::filterPointsInRange(TimeRange range) {
  if (!this->failoverTimeseries()) {
    return TimeSeriesFilter::filterPointsInRange(range);
  }
  
  Units myUnits = this->units();
  std::vector<Point> thePoints;
  TimeRange primarySourceRange = range;
  TimeRange secondarySourceRange = range;
  
  Point priorPoint = this->source()->pointBefore(range.start + 1);
  if (priorPoint.isValid) {
    primarySourceRange.start = priorPoint.time;
  }
  Point nextPoint = this->source()->pointAfter(range.end - 1).time;
  if (nextPoint.isValid) {
    primarySourceRange.end = nextPoint.time;
  }
  
  priorPoint = this->failoverTimeseries()->pointBefore(range.start + 1);
  nextPoint = this->failoverTimeseries()->pointAfter(range.end - 1);
  if (priorPoint.isValid) {
    secondarySourceRange.start = priorPoint.time;
  }
  if (nextPoint.isValid) {
    secondarySourceRange.end = nextPoint.time;
  }
  
  PointCollection primaryData = this->source()->points(primarySourceRange);
  primaryData.convertToUnits(myUnits);
  
  time_t prevTime;
  if (primaryData.count() > 0) {
    prevTime = primaryData.points.front().time;
  }
  else {
    prevTime = range.start - _stale - 1;
  }
  
  Point fakeEndPoint;
  fakeEndPoint.time = MAX(primarySourceRange.end, secondarySourceRange.end);
  primaryData.points.push_back(fakeEndPoint);
  
  BOOST_FOREACH(const Point& p, primaryData.points) {
    if (!p.isValid) {
      continue; // skip bad points
    }
    
    time_t now = p.time;
    if (now - prevTime > _stale) {
      // pay attention! back up by one second when requesting failover points, since we're NOW sitting on a valid primary point. Maybe.
      if (p.isValid) {
        --now;
      }
      PointCollection secondary = this->failoverTimeseries()->points(TimeRange(prevTime, now));
      secondary.convertToUnits(myUnits);
      BOOST_FOREACH(Point sp, secondary.points) {
        if (sp.isValid) {
          thePoints.push_back( sp );
        }
      }
    }
    
    // even if we added secondary points, there is still a good point here. maybe.
    if (p.isValid) {
      thePoints.push_back(p);
    }
    
    prevTime = p.time;
  }
  
  
  PointCollection outData(thePoints, myUnits);
  if (this->willResample()) {
    outData.resample(this->clock()->timeValuesInRange(range.start, range.end));
  }
  
  return outData;
  
}


bool FailoverTimeSeries::canSetSource(TimeSeries::_sp ts) {
  
  if (this->failoverTimeseries() && !this->failoverTimeseries()->units().isSameDimensionAs(ts->units())) {
    return false;
  }
  
  return true;
  
}




