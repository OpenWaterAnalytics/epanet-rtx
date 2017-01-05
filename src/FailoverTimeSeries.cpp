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

bool FailoverTimeSeries::canSetSecondary(TimeSeries::_sp secondary) {
  if (this->source() && secondary && !this->source()->units().isSameDimensionAs(secondary->units())) {
    // conflict
    return false;
  }
  else {
    return true;
  }
}

void FailoverTimeSeries::swapSourceWithFailover() {
  if (!this->source() || !this->secondary()) {
    cerr << "nothing to swap" << endl;
    return;
  }
  TimeSeries::_sp tmp = this->source();
  this->setSource(this->secondary());
  this->setSecondary(tmp);
}




set<time_t> FailoverTimeSeries::timeValuesInRange(TimeRange range) {
  if (!this->secondary() || this->clock()) {
    return TimeSeriesFilter::timeValuesInRange(range);
  }
  else if (!this->clock()) {
    set<time_t> times;
    PointCollection pc = this->filterPointsInRange(range);
    return pc.times();
  }
  
  return set<time_t>();
}

PointCollection FailoverTimeSeries::filterPointsInRange(TimeRange range) {
  if (!this->secondary()) {
    return TimeSeriesFilter::filterPointsInRange(range);
  }
  
  TimeRange qPrimary = range;
  time_t priPrev = this->source()->timeBefore(range.start + 1);
  time_t priNext = this->source()->timeAfter(range.end - 1);
  qPrimary = TimeRange(priPrev,priNext);
  
  TimeRange qSecondary = range;
  time_t secPrev = this->secondary()->timeBefore(range.start + 1);
  time_t secNext = this->secondary()->timeAfter(range.end - 1);
  qSecondary = TimeRange(secPrev,secNext);
  
  // make these valid & queryable
  qPrimary.correctWithRange(range);
  qSecondary.correctWithRange(range);
  
  // this is to make sure we have enough secondary data to fill leading & trailing gaps
  if (qPrimary.start < qSecondary.start) {
    qSecondary.start = qPrimary.start;
  }
  if (qPrimary.end > qSecondary.end) {
    qSecondary.end = qPrimary.end;
  }
  
  // get source and secondary data
  PointCollection primaryData = this->source()->pointCollection(qPrimary);
  PointCollection secondaryData = this->secondary()->pointCollection(qSecondary);
  
  primaryData.convertToUnits(this->units());
  secondaryData.convertToUnits(this->units());
  
  // if there's no source data at all, just give the secondary.
  if (primaryData.count() == 0) {
    if (this->willResample()) {
      secondaryData.resample(this->timeValuesInRange(range));
    }
    return secondaryData;
  }
  
  vector<Point> merged;
  
  vector<Point> paddedPrimary;
  paddedPrimary.reserve(primaryData.count() + 2);
  auto primaryRaw = primaryData.raw();
  time_t staleStart = range.start - (_stale + 1);
  
  if (primaryRaw.first->time > staleStart) {
    // pad with fake point. This is important because the while-loop below will ignore the first point
    // and only use the time information to inspect the gap.
    Point fakeP;
    fakeP.time = staleStart;
    paddedPrimary.push_back(fakeP);
  }
  
  // fold in the data
  paddedPrimary.insert(paddedPrimary.end(), primaryRaw.first, primaryRaw.second);
  
  if (primaryRaw.first->time < range.end) {
    // primary data ends before the end of the needed range. put a fake point at the end
    // as a marker for the while-iteration below, so that we can safely go off the
    // end of the valid data.
    Point fakePoint;
    fakePoint.time = range.end + _stale;
    paddedPrimary.push_back(fakePoint);
  }
  
  vector<Point>::const_iterator it = paddedPrimary.cbegin();
  vector<Point>::const_iterator end = paddedPrimary.cend();
  
  TimeRange gap;
  gap.start = it->time;
  ++it;
  gap.end = it->time;
  
  while (gap.duration() > 0) {
    
    if (gap.duration() > _stale) {
      vector<Point> sec = secondaryData.trimmedToRange(TimeRange(gap.start+1,gap.end-1)).points();
      merged.insert(merged.end(), sec.begin(), sec.end());
    }
    if (it->isValid) {
      merged.push_back(*it);
    }
    
    gap.start = gap.end;
    ++it;
    if (it != end) {
      gap.end = it->time;
    }
    
  }
  
  PointCollection outData(merged, this->units());
  
  if (this->willResample()) {
    outData.resample(this->timeValuesInRange(range));
  }
  else {
    outData = outData.trimmedToRange(range);
  }
  
  return outData;
}

time_t FailoverTimeSeries::timeBefore(time_t time) {
  if (!this->source()) {
    return 0;
  }
  if (this->clock()) {
    return this->clock()->timeBefore(time);
  }
  
  time_t t = this->source()->timeBefore(time);
  
  if (t == 0 && this->secondary()) {
    return this->secondary()->timeBefore(time);
  }
  
  // check staleness
  if ((time - t) > this->maximumStaleness() && this->secondary()) {
    t = this->secondary()->timeBefore(time);
  }
  
  return t;
}

time_t FailoverTimeSeries::timeAfter(time_t time) {
  if (!this->source()) {
    return 0;
  }
  if (this->clock()) {
    return this->clock()->timeAfter(time);
  }
  // going forward is a different scenario.
  // rewind so we can really test for staleness
  time_t bef = this->timeBefore(time);
  time_t maxTime = bef + this->maximumStaleness();
  
  time_t aft = this->source()->timeAfter(time);
  
  if (aft > 0 && aft <= maxTime) {
    return aft;
  } else if (this->secondary()) {
    return this->secondary()->timeAfter(maxTime - 1);
  }
  return 0;
}

bool FailoverTimeSeries::canSetSource(TimeSeries::_sp ts) {
  
  if (this->secondary() && !this->secondary()->units().isSameDimensionAs(ts->units())) {
    return false;
  }
  return true;
}




