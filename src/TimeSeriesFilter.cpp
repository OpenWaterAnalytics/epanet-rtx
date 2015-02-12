//
//  TimeSeriesFilter.cpp
//  epanet-rtx
//

#include "TimeSeriesFilter.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

TimeSeriesFilter::TimeSeriesFilter() {
  _resampleMode = TimeSeriesResampleModeLinear;
}

Clock::_sp TimeSeriesFilter::clock() {
  return _clock;
}
void TimeSeriesFilter::setClock(Clock::_sp clock) {
  if (clock) {
    _clock = clock;
  }
  else {
    _clock.reset();
  }
  this->invalidate();
}

TimeSeries::TimeSeriesResampleMode TimeSeriesFilter::resampleMode() {
  return _resampleMode;
}

void TimeSeriesFilter::setResampleMode(TimeSeries::TimeSeriesResampleMode mode) {
  
  if (_resampleMode != mode) {
    this->invalidate();
  }
  
  _resampleMode = mode;
}

void TimeSeriesFilter::setRecord(PointRecord::_sp record) {
  if (record != this->record()) {
    this->invalidate();
    TimeSeries::setRecord(record);
    this->invalidate();
  }
}

// filters can invalidate themselves.
void TimeSeriesFilter::setUnits(RTX::Units newUnits) {
  if (! (newUnits == this->units())) {
    this->invalidate();
    TimeSeries::setUnits(newUnits);
  }
}

TimeSeries::_sp TimeSeriesFilter::source() {
  return _source;
}
void TimeSeriesFilter::setSource(TimeSeries::_sp ts) {
  if (ts && !this->canSetSource(ts)) {
    return;
  }
  _source = ts;
  TimeSeriesFilter::_sp filterSource = boost::dynamic_pointer_cast<TimeSeriesFilter>(ts);
  if (filterSource && !this->clock()) {
    this->setClock(filterSource->clock());
  }
  if (ts) {
    this->invalidate();
    this->didSetSource(ts);
  }
}


vector<Point> TimeSeriesFilter::points(time_t start, time_t end) {
  vector<Point> filtered;
  
  if (! this->source()) {
    return filtered;
  }
  
  set<time_t> pointTimes;
  pointTimes = this->timeValuesInRange(make_pair(start, end));
  
  PointCollection native;
  
  // important optimization. if this range has already been constructed and cached, then don't recreate it.
  bool alreadyCached = false;
  vector<Point> cached = TimeSeries::points(start, end); // base class call -> find any pre-cached points
  if (cached.size() == pointTimes.size()) {
    // looks good, let's make sure that all time values line up.
    alreadyCached = true;
    BOOST_FOREACH(const Point& p, cached) {
      if (pointTimes.count(p.time) == 0) {
        alreadyCached = false;
        break; // break foreach, with false "alreadyCached" flag
      }
    }
  }
  if (alreadyCached) {
    return cached; // we're done here. all points are present.
  }
  
  PointCollection outCollection = this->filterPointsInRange(make_pair(start, end));
  this->insertPoints(outCollection.points);
  return outCollection.points;
}


Point TimeSeriesFilter::pointBefore(time_t time) {
  Point p;
  p.time = time;
  time_t seekTime = time;
  
  if (!this->source()) {
    return p;
  }
  
  while (!p.isValid && p.time != 0) {
    if (this->clock()) {
      seekTime = this->clock()->timeBefore(seekTime);
    }
    else {
      seekTime = this->source()->pointBefore(seekTime).time;
    }
    p = this->point(seekTime);
  }
  
  return p;
}

Point TimeSeriesFilter::pointAfter(time_t time) {
  Point p;
  p.time = time;
  time_t seekTime = time;
  
  if (!this->source()) {
    return p;
  }
  
  while (!p.isValid && p.time != 0) {
    if (this->clock()) {
      seekTime = this->clock()->timeAfter(seekTime);
    }
    else {
      seekTime = this->source()->pointAfter(seekTime).time;
    }
    p = this->point(seekTime);
  }
  
  return p;
}


#pragma mark - Pseudo-Delegate methods

bool TimeSeriesFilter::willResample() {
  
  // if i don't have a clock, then the answer is obvious
  if (!this->clock()) {
    return false;
  }
  
  // no source, also not resampling. or rather, it's a moot point.
  if (!this->source()) {
    return false;
  }
  
  TimeSeriesFilter::_sp sourceFilter = boost::dynamic_pointer_cast<TimeSeriesFilter>(this->source());
  
  if (sourceFilter) {
    // my source is a filter, so it may have a clock
    Clock::_sp sourceClock = sourceFilter->clock();
    if (!sourceClock && this->clock()) {
      // no source clock, but i have a clock. i resample
      return true;
    }
    if (sourceClock && sourceClock != this->clock()) {
      // clocks are different, so i will resample.
      return true;
    }
  }
  else {
    // source is just a time series. i will only resample if i have a clock
    if (this->clock()) {
      return true;
    }
  }
  
  return false;
}


TimeSeries::PointCollection TimeSeriesFilter::filterPointsInRange(TimeRange range) {
  
  TimeRange queryRange = range;
  if (this->willResample()) {
    // expand range
    queryRange.first = this->source()->pointBefore(range.first + 1).time;
    queryRange.second = this->source()->pointAfter(range.second - 1).time;
  }
  
  queryRange.first = (queryRange.first == 0) ? range.first : queryRange.first;
  queryRange.second = (queryRange.second == 0) ? range.second : queryRange.second;
  
  PointCollection data = source()->points(queryRange);
  
  bool dataOk = false;
  dataOk = data.convertToUnits(this->units());
  if (dataOk && this->willResample()) {
    set<time_t> timeValues = this->timeValuesInRange(range);
    dataOk = data.resample(timeValues, _resampleMode);
  }
  
  if (dataOk) {
    return data;
  }
  
  return PointCollection(vector<Point>(),this->units());
}


set<time_t> TimeSeriesFilter::timeValuesInRange(TimeSeries::TimeRange range) {
  set<time_t> times;
  
  if (range.first == 0 || range.second == 0) {
    return times;
  }
  
  if (this->clock()) {
    times = this->clock()->timeValuesInRange(range.first, range.second);
  }
  else {
    vector<Point> points = source()->points(range.first, range.second);
    BOOST_FOREACH(const Point& p, points) {
      times.insert(p.time);
    }
  }
  
  return times;
}



bool TimeSeriesFilter::canSetSource(TimeSeries::_sp ts) {
  bool goodSource = (ts) ? true : false;
  bool unitsOk = ( ts->units().isSameDimensionAs(this->units()) || this->units().isDimensionless() );
  return goodSource && unitsOk;
}

void TimeSeriesFilter::didSetSource(TimeSeries::_sp ts) {
  // expected behavior if this has dimensionless units, take on the units of the source.
  // we are guaranteed that the units are compatible (if canChangeToUnits was properly implmented),
  // so no further checking is required.
  if (this->units().isDimensionless()) {
    this->setUnits(ts->units());
  }
  
  // if the source is a filter, and has a clock, and I don't have a clock, then use the source's clock.
  TimeSeriesFilter::_sp sourceFilter = boost::dynamic_pointer_cast<TimeSeriesFilter>(ts);
  if (sourceFilter && sourceFilter->clock() && !this->clock()) {
    this->setClock(sourceFilter->clock());
  }
  
}

bool TimeSeriesFilter::canChangeToUnits(Units units) {
  
  // basic pass-thru filter just alters units, not dimension.
  
  bool canChange = true;
  
  if (this->source()) {
    canChange = this->source()->units().isSameDimensionAs(units);
  }
  
  return canChange;
}





