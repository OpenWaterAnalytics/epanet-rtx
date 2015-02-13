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
  if (!p.isValid) {
    return Point();
  }
  p.time += _lag;
  return p;
}

Point LagTimeSeries::pointAfter(time_t time) {
  Point p = TimeSeriesFilter::pointAfter(time - _lag);
  if (!p.isValid) {
    return Point();
  }
  p.time += _lag;
  return p;
}

bool LagTimeSeries::willResample() {
  bool resample = TimeSeriesFilter::willResample();
  
  if (!resample) {
    // make double sure.
    TimeSeriesFilter::_sp sourceFilter = boost::dynamic_pointer_cast<TimeSeriesFilter>(this->source());
    if (sourceFilter && sourceFilter->clock()
        && _lag != 0
        && (_lag % this->clock()->period() != 0 || this->clock()->period() % _lag != 0)) {
      resample = true;
    }
  }
  
  return resample;
}


set<time_t> LagTimeSeries::timeValuesInRange(TimeRange range) {
  if (this->clock()) {
    return this->clock()->timeValuesInRange(range.start, range.end);
  }
  TimeRange lagRange = range;
  lagRange.start -= _lag;
  lagRange.end -= _lag;
  set<time_t> sourceTimes = TimeSeriesFilter::timeValuesInRange(lagRange);
  set<time_t> outTimes;
  BOOST_FOREACH(time_t t, sourceTimes) {
    outTimes.insert(t + _lag);
  }
  return outTimes;
}

TimeSeries::PointCollection LagTimeSeries::filterPointsInRange(TimeRange range) {
  
  TimeRange queryRange = range;
  queryRange.start -= _lag;
  queryRange.end -= _lag;
  
  queryRange.start = this->source()->pointBefore(queryRange.start + 1).time;
  queryRange.end = this->source()->pointAfter(queryRange.end - 1).time;
  
  PointCollection data = this->source()->points(queryRange);
  
  // move the points in time
  BOOST_FOREACH(Point& p, data.points) {
    p.time += _lag;
  }
  
  
  
  bool dataOk = false;
  dataOk = data.convertToUnits(this->units());
  if (dataOk && this->willResample()) {
    set<time_t> timeValues = this->timeValuesInRange(range);
    dataOk = data.resample(timeValues);
  }
  
  if (dataOk) {
    return data;
  }
  
  return PointCollection(vector<Point>(),this->units());
  
}


