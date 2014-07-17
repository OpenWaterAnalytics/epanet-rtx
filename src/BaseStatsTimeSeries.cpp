//
//  BaseStatsTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/17/14.
//
//

#include "BaseStatsTimeSeries.h"
#include <boost/foreach.hpp>


using namespace RTX;
using namespace std;

BaseStatsTimeSeries::BaseStatsTimeSeries() {
  Clock::sharedPointer window(new Clock(60));
  _window = window;
  _summaryOnly = true;
}


void BaseStatsTimeSeries::setClock(Clock::sharedPointer clock) {
  // not allowed
  return;
}


void BaseStatsTimeSeries::setWindow(Clock::sharedPointer window) {
  _window = window;
  this->record()->invalidate(this->name());
}
Clock::sharedPointer BaseStatsTimeSeries::window() {
  return _window;
}


bool BaseStatsTimeSeries::summaryOnly() {
  return _summaryOnly;
}

void BaseStatsTimeSeries::setSummaryOnly(bool summaryOnly) {
  _summaryOnly = summaryOnly;
}

vector< BaseStatsTimeSeries::pointSummaryPair_t > BaseStatsTimeSeries::filteredSummaryPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  time_t windowLen = this->window()->period();
  
  // force a pre-cache on the source time series
  sourceTs->points(fromTime - windowLen, toTime);
  
  // TimeSeries::Summary already computes quartiles, so just use that.
  
  vector<Point> sourcePoints = sourceTs->points(fromTime,toTime);
  vector< pointSummaryPair_t > sPoints;
  sPoints.reserve(sourcePoints.size());
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    TimeSeries::Summary s = sourceTs->summary( p.time - windowLen, p.time );
    if (this->summaryOnly()) {
      s.points = vector<Point>();
      s.gaps = vector<Point>();
    }
    sPoints.push_back(make_pair(p, s));
  }
  
  
  return sPoints;
}

