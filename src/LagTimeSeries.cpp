//
//  LagTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/20/15.
//
//

#include "LagTimeSeries.h"
#include <boost/foreach.hpp>


using namespace RTX;
using namespace std;

LagTimeSeries::LagTimeSeries() : _lag(0) {
  
}

void LagTimeSeries::setOffset(time_t offset) {
  _lag = offset;
  this->invalidate();
}

time_t LagTimeSeries::offset() {
  return _lag;
}

Point LagTimeSeries::pointBefore(time_t time) {
  Point p = TimeSeriesFilter::pointBefore(time - _lag);
  p.time += _lag;
  return p;
}

Point LagTimeSeries::pointAfter(time_t time) {
  Point p = TimeSeriesFilter::pointAfter(time - _lag);
  p.time += _lag;
  return p;
}



set<time_t> LagTimeSeries::timeValuesInRange(TimeRange range) {
  TimeRange lagRange = range;
  lagRange.first -= _lag;
  lagRange.second -= _lag;
  set<time_t> sourceTimes = TimeSeriesFilter::timeValuesInRange(lagRange);
  set<time_t> outTimes;
  BOOST_FOREACH(time_t t, sourceTimes) {
    outTimes.insert(t + _lag);
  }
  return outTimes;
}

TimeSeries::PointCollection LagTimeSeries::filterPointsAtTimes(std::set<time_t> times) {
  
  set<time_t> queryTimes;
  BOOST_FOREACH(time_t t, times) {
    queryTimes.insert(t - _lag);
  }
  
  PointCollection col = TimeSeriesFilter::filterPointsAtTimes(queryTimes);
  
  BOOST_FOREACH(Point& p, col.points) {
    p.time += _lag;
  }
  
  return col;
  
}


