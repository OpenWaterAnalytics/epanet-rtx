//
//  QuartileTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/17/14.
//
//

#include "QuartileTimeSeries.h"
#include <boost/foreach.hpp>


using namespace RTX;
using namespace std;

QuartileTimeSeries::QuartileTimeSeries() {
  useQuartiles = (quartile_t)( quartile_25 | quartile_50 | quartile_75 ); // use all by default
  Clock::sharedPointer window(new Clock(60));
  _window = window;
}


void QuartileTimeSeries::setClock(Clock::sharedPointer clock) {
  // not allowed
  return;
}


void QuartileTimeSeries::setWindow(Clock::sharedPointer window) {
  _window = window;
  this->record()->invalidate(this->name());
}
Clock::sharedPointer QuartileTimeSeries::window() {
  return _window;
}


vector<QuartileTimeSeries::QuartilePoint> QuartileTimeSeries::filteredQuartilePoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime) {
  
  time_t windowLen = this->window()->period();
  
  // force a pre-cache on the source time series
  sourceTs->points(fromTime - windowLen, toTime);
  
  // TimeSeries::Summary already computes quartiles, so just use that.
  
  vector<Point> sourcePoints = sourceTs->points(fromTime,toTime);
  vector<QuartilePoint> qPoints;
  qPoints.reserve(sourcePoints.size());
  
  BOOST_FOREACH(const Point& p, sourcePoints) {
    TimeSeries::Summary s = sourceTs->summary( p.time - windowLen, p.time );
    QuartilePoint qPoint(p.time, s.stats.quartiles.q25, s.stats.quartiles.q50, s.stats.quartiles.q75);
    qPoints.push_back(qPoint);
  }
  
  
  return qPoints;
}

