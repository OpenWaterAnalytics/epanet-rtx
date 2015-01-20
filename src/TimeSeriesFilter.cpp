//
//  TimeSeriesFilter.cpp
//  epanet-rtx
//

#include "TimeSeriesFilter.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

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

TimeSeries::_sp TimeSeriesFilter::source() {
  return _source;
}
void TimeSeriesFilter::setSource(TimeSeries::_sp ts) {
  if (!this->canSetSource(ts)) {
    return;
  }
  this->invalidate();
  _source = ts;
  TimeSeriesFilter::_sp filterSource = boost::dynamic_pointer_cast<TimeSeriesFilter>(ts);
  if (filterSource && !this->clock()) {
    this->setClock(filterSource->clock());
  }
  this->didSetSource(ts);
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
  
  PointCollection outCollection = this->filterPointsAtTimes(pointTimes);
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


TimeSeries::PointCollection TimeSeriesFilter::filterPointsAtTimes(std::set<time_t> times) {
  vector<Point> outPoints;
  
  // just resample
  PointCollection pointsCollection = this->source()->resampled(times);
  
  bool convertOk = pointsCollection.convertToUnits(this->units());
  if (convertOk) {
    outPoints = pointsCollection.points;
  }
  
  PointCollection outCollection(outPoints,this->units());
  return outCollection;
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





