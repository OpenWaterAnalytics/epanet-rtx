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
  Clock::_sp window(new Clock(60));
  _window = window;
  _summaryOnly = true;
  _samplingMode = StatsSamplingModeLagging;
}


void BaseStatsTimeSeries::setWindow(Clock::_sp window) {
  _window = window;
  this->invalidate();
}
Clock::_sp BaseStatsTimeSeries::window() {
  return _window;
}


bool BaseStatsTimeSeries::summaryOnly() {
  return _summaryOnly;
}

void BaseStatsTimeSeries::setSummaryOnly(bool summaryOnly) {
  _summaryOnly = summaryOnly;
}


void BaseStatsTimeSeries::setSamplingMode(StatsSamplingMode_t mode) {
  _samplingMode = mode;
  this->invalidate();
}

BaseStatsTimeSeries::StatsSamplingMode_t BaseStatsTimeSeries::samplingMode() {
  return _samplingMode;
}


unique_ptr<BaseStatsTimeSeries::pointSummaryGroup> BaseStatsTimeSeries::filterSummaryCollection(std::set<time_t> times) {
  
  unique_ptr<pointSummaryGroup> group(new pointSummaryGroup);
  
  if (times.size() == 0) {
    return group;
  }
  
  TimeSeries::_sp sourceTs = this->source();
  time_t fromTime = *(times.begin());
  time_t toTime = *(times.rbegin());
  
  time_t windowLen = this->window()->period();
  
  time_t lagDistance  = 0,
  leadDistance = 0;
  
  switch (this->samplingMode()) {
    case StatsSamplingModeLeading:
    {
      leadDistance += windowLen;
      break;
    }
    case StatsSamplingModeLagging:
    {
      lagDistance += windowLen;
      break;
    }
    case StatsSamplingModeCentered:
    {
      time_t halfLen = windowLen / 2;
      lagDistance += halfLen;
      leadDistance += halfLen;
      break;
    }
    default: break;
  }
  
  
  pointSummaryGroup outSummaries;
  
  // force a pre-cache on the source time series
  TimeRange preFetchRange(fromTime - lagDistance, toTime + leadDistance);
  group->retainedPoints = sourceTs->pointCollection(preFetchRange);
  
  for(const time_t& t : times) {
    // get sub-ranges of the larger pre-fetched collection
    TimeRange subrange(t - lagDistance, t + leadDistance);
    
    // trim range with iterators.
    auto raw = group->retainedPoints.raw();
    auto it = raw.first;
    auto r1 = it, r2 = it;
    auto end = raw.second;
    
    while (it != end) {
      time_t t = it->time;
      if (subrange.contains(t)) {
        r1 = it;
        break;
      }
      ++it;
    }
    
    while (it != end) {
      time_t t = it->time;
      if (!subrange.contains(t)) {
        break;
      }
      ++it;
      r2 = it;
    }
    
    group->summaryMap[t] = PointCollection(r1,r2,this->units());
  }
  
  return group;
}



