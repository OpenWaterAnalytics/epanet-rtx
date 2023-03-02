//
//  TimeSeriesFilter.cpp
//  epanet-rtx
//

#include "TimeSeriesFilter.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

const size_t _tsfilter_max_search = 6; // FIXME 💩
const time_t _stride_basis = 60*60; // 1 hour, doubled each search iteration

TimeSeriesFilter::TimeSeriesFilter() {
  _resampleMode = ResampleModeLinear;
}

Clock::_sp TimeSeriesFilter::clock() {
  return _clock;
}
void TimeSeriesFilter::setClock(Clock::_sp clock) {
  if (clock == _clock) {
    return;
  }
  else {
    _clock = clock;
    this->invalidate();
  }
}

ResampleMode TimeSeriesFilter::resampleMode() {
  return _resampleMode;
}

void TimeSeriesFilter::setResampleMode(ResampleMode mode) {
  
  if (_resampleMode != mode) {
    this->invalidate();
  }
  
  _resampleMode = mode;
}


// filters can invalidate themselves.
void TimeSeriesFilter::setUnits(RTX::Units newUnits) {
  if (! (newUnits == this->units())) {
    //this->invalidate();
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
  if (_source) {
    _source->filterDidRemoveSource(share_me(this));
  }
  _source = ts;
  if (ts) {
    this->invalidate();
    this->didSetSource(ts);
    ts->filterDidAddSource(share_me(this));
  }
}

bool TimeSeriesFilter::hasUpstreamSeries(TimeSeries::_sp ts) {
  if (!_source) {
    return false;
  }
  return _source == ts || _source->hasUpstreamSeries(ts);
}

vector<Point> TimeSeriesFilter::points(TimeRange range) {
  
  PointCollection cached;
  set<time_t> pointTimes;
  auto canDrop = this->canDropPoints();
  PointCollection outCollection;
  bool didFetch = false;
  
  if (!this->source()) {
    return vector<Point>();
  }
  
  cached = PointCollection(TimeSeries::points(range), this->units()); // base class call -> find any pre-cached points
  
  if (canDrop) {
    // optmized fetching: 
    // if this filter can drop points, then asking for the time values implies a lookup.
    // so if we find that the existing (base TimeSeries::points) cache is not valid, then 
    // we will be forced to do another fetch, and  
    // a full point lookup will have been wasted. so this piece of logic
    // is a partial duplication of that method that allows us to do it all here instead
    // of making a round trip. If the cache is complete, then we have wasted some time
    // although doing a fetch is the only way to know for sure...
    // but if the cache is not complete, then we will have saved time by fetching here.
    outCollection = this->filterPointsInRange(range);
    pointTimes = outCollection.times();
    didFetch = true;
  }
  else {
    pointTimes = this->timeValuesInRange(range); // what time value should we expect?
  }
  
  // important optimization. if this range has already been constructed and cached, then don't recreate it.
  if (cached.times() == pointTimes) {
    // all time values are there, so the cache is valid and complete.
    return cached.points();
  }
  else if (!didFetch) {
    // expensive lookup needed.
    // otherwise we've already hit the stack.
    outCollection = this->filterPointsInRange(range);
  }
  
  this->insertPoints(outCollection.points());
  outCollection = outCollection.trimmedToRange(range); // safeguard if filter doesn't respected the range
  return outCollection.points();
}


Point TimeSeriesFilter::pointBefore(time_t time) {
  Point p;
  p.time = time;
  time_t seekTime = time;
  
  if (!this->source()) {
    return Point();
  }
  
  if (this->canDropPoints()) {
    // search iteratively
    
    
    PointCollection c;
    
    TimeRange q(time - _stride_basis, time - 1);
    size_t n_strides = 0;
    
    while ( ++n_strides <= _tsfilter_max_search ) {
      if (this->source()->timeBefore(q.end + 1) == 0) {
        break; // actually no points. futile to search.
      }
      c = this->pointCollection(q);
      if (c.count() > 0) {
        break; // found something
      }
      q.start -= _stride_basis * (int)n_strides;
    }
    
    // if we found something:
    if (c.count() > 0) {
      p = c.points().back();
    }
    else {
      struct tm * timeinfo = localtime (&time);
      cerr << "pointBefore Iterative search exceeded max strides:" << this->name() << " time=" << asctime(timeinfo) << endl;
      cerr << "Root Series -> ";
      for (auto s : this->rootTimeSeries()) {
        cerr << " :: " << s->name();
      }
      cerr << EOL << flush;
    }
  }
  else {
    // points in, points out
    seekTime = this->timeBefore(seekTime);
    p = this->point(seekTime);
  }
  
  return p;
}

Point TimeSeriesFilter::pointAfter(time_t time) {
  Point p;
  p.time = time;
  time_t seekTime = time;
  
  if (!this->source()) {
    return Point();
  }
  
  
  if (this->canDropPoints()) {
    // search iteratively - this is basic functionality. Override ::pointAfter for special cases or optimized uses.
    
    PointCollection c;
    
    TimeRange q(time + 1, time + _stride_basis);
    size_t n_strides = 0;
    
    while ( ++n_strides <= _tsfilter_max_search ) {
      if (this->source()->timeAfter(q.start - 1) == 0) {
        break; // actually no points. futile to search.
      }
      c = this->pointCollection(q);
      if (c.count() > 0) {
        break; // found something
      }
      q.end += _stride_basis * (int)n_strides;
    }
    
    // if we found something:
    if (c.count() > 0) {
      p = c.points().front();
    }
    else {
      struct tm * timeinfo = localtime (&time);
      cerr << "pointAfter Iterative search exceeded max strides:" << this->name() << " time=" << asctime(timeinfo) << endl;
      cerr << "Root Series -> ";
      for (auto s : this->rootTimeSeries()) {
        cerr << " :: " << s->name();
      }
      cerr << EOL << flush;
    }
  }
  else {
    // points in, points out
    seekTime = this->timeAfter(seekTime);
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
  
  TimeSeriesFilter::_sp sourceFilter = std::dynamic_pointer_cast<TimeSeriesFilter>(this->source());
  
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


TimeRange TimeSeriesFilter::expandedRange(RTX::TimeRange r) {
  TimeRange q = r;
  bool canDrop = this->canDropPoints();
  Clock::_sp myClock = this->clock();
  // expand range so that we can resample at the start and/or end of the range requested
  // tricky trick here. un-set my clock to find the actual range accounting for dropped points.
  // then re-set the clock after. But do this using private ivar so that we don't trigger invalidation.
  if (canDrop) {
    _clock = nullptr;
  }
  q.start = this->timeBefore(r.start);
  q.end = this->timeAfter(r.end);
  q.correctWithRange(r);
  r = q;
  q.start = this->source()->timeBefore(r.start);
  q.end = this->source()->timeAfter(r.end);
  q.correctWithRange(r);
  if (canDrop) {
    _clock = myClock;
  }
  return q;
}


PointCollection TimeSeriesFilter::filterPointsInRange(TimeRange range) {
  
  TimeRange queryRange = range;
  if (this->willResample()) {
    // expand range
    queryRange.start = this->source()->timeBefore(range.start + 1);
    switch (_resampleMode) {
      case ResampleModeStep:
        queryRange.end = this->source()->timeBefore(range.end + 1);
        break;
      case ResampleModeLinear:
        queryRange.end = this->source()->timeAfter(range.end - 1);
      default:
        break;
    }
  }
  
  
  // in case the source has data, but that data is truncated within the range requested
  // i.e.,   ******[******-------]-------
  if (!queryRange.isValid()) {
    if (queryRange.start == 0) {
      queryRange.start = this->source()->timeAfter(range.start); // go to next available point.
    }
    if (queryRange.end == 0) {
      queryRange.end = this->source()->timeBefore(range.end); // go to previous availble point.
    }
  }
  
  queryRange.correctWithRange(range);
  
  PointCollection data = source()->pointCollection(queryRange);
  
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


set<time_t> TimeSeriesFilter::timeValuesInRange(TimeRange range) {
  set<time_t> times;
  
  if (!range.isValid() || !this->source()) {
    return times;
  }
  
  if (this->clock()) {
    times = this->clock()->timeValuesInRange(range);
  }
  else {
    if (this->canDropPoints()) {
      PointCollection c = this->filterPointsInRange(range);
      times = c.times();
    }
    else {
      times = this->source()->timeValuesInRange(range);
    }
  }
  
  return times;
}


time_t TimeSeriesFilter::timeAfter(time_t t) {
  if (!this->source()) {
    return 0;
  }
  if (this->clock()) {
    return this->clock()->timeAfter(t);
  }
  else if (this->canDropPoints()) {
    Point pAfter = this->pointAfter(t);
    if (pAfter.isValid) {
      return pAfter.time;
    }
    else {
      return 0;
    }

  }
  else {
    return this->source()->timeAfter(t);
  }
}

time_t TimeSeriesFilter::timeBefore(time_t t) {
  if (!this->source()) {
    return 0;
  }
  else if (this->clock()) {
    return this->clock()->timeBefore(t);
  }
  else if (this->canDropPoints()) {
    Point pBefore = this->pointBefore(t);
    if (pBefore.isValid) {
      return pBefore.time;
    }
    else {
      return 0;
    }
  }
  else {
    return this->source()->timeBefore(t);
  }
}


bool TimeSeriesFilter::canSetSource(TimeSeries::_sp ts) {
  bool goodSource = (ts) ? true : false;
  bool unitsOk = ( ts->units().isSameDimensionAs(this->units()) || this->units().isDimensionless() );
  return goodSource && unitsOk;
}

void TimeSeriesFilter::didSetSource(TimeSeries::_sp ts) {
  // expected behavior if this has no dimension, take on the units of the source.
  // we are guaranteed that the units are compatible (if canChangeToUnits was properly implmented),
  // so no further checking is required.
  if (this->units().isDimensionless()) {
    this->setUnits(ts->units());
  }
  
  // if the source is a filter, and has a clock, and I don't have a clock, then use the source's clock.
  TimeSeriesFilter::_sp sourceFilter = std::dynamic_pointer_cast<TimeSeriesFilter>(ts);
  if (sourceFilter && sourceFilter->clock() && !this->clock() && !this->canDropPoints()) {
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


std::vector<TimeSeries::_sp> TimeSeriesFilter::rootTimeSeries() {
  std::vector<TimeSeries::_sp> roots;
  TimeSeries::_sp source = this->source();
  if (source) {
    for (auto r : source->rootTimeSeries()) {
      roots.push_back(r);
    }
  }
  return roots;
}







