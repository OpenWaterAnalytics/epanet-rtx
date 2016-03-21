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

time_t LagTimeSeries::timeBefore(time_t t) {
  
  time_t bef = 0;
  if (!this->source()) {
    return bef;
  }
  if (this->clock()) {
    return this->clock()->timeBefore(t);
  }
  
  bef = this->source()->timeBefore(t - _lag);
  if (bef != 0) {
    bef += _lag;
  }
  return bef;
}

time_t LagTimeSeries::timeAfter(time_t t) {
  
  time_t aft = 0;
  if (!this->source()) {
    return aft;
  }
  if (this->clock()) {
    return this->clock()->timeAfter(t);
  }
  
  aft = this->source()->timeAfter(t - _lag);
  if (aft != 0) {
    aft += _lag;
  }
  return aft;
}

bool LagTimeSeries::willResample() {
  
  if (!this->clock()) {
    return false;
  }
  
  bool resample = TimeSeriesFilter::willResample();
  
  if (!resample) {
    // make double sure.
    TimeSeriesFilter::_sp sourceFilter = std::dynamic_pointer_cast<TimeSeriesFilter>(this->source());
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
    return this->clock()->timeValuesInRange(range);
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
  
  TimeRange laggedRange = range;
  laggedRange.start -= _lag;
  laggedRange.end -= _lag;
  TimeRange queryRange = laggedRange;
  
  queryRange.start = this->source()->timeBefore(queryRange.start + 1);
  queryRange.end = this->source()->timeAfter(queryRange.end - 1);
  queryRange.correctWithRange(laggedRange);
  
  PointCollection data = this->source()->pointCollection(queryRange);
  
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


