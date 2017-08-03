//
//  TimeSeriesFilterSecondary.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/15/16.
//
//

#include "TimeSeriesFilterSecondary.h"

using namespace RTX;

void TimeSeriesFilterSecondary::setSecondary(TimeSeries::_sp secondary) {
  if (this->canSetSecondary(secondary)) {
    _secondary = secondary;
    this->didSetSecondary(secondary);
  }
}

TimeSeries::_sp TimeSeriesFilterSecondary::secondary() {
  return _secondary;
}

bool TimeSeriesFilterSecondary::canSetSecondary(TimeSeries::_sp secondary) {
  return true;
}

void TimeSeriesFilterSecondary::didSetSecondary(TimeSeries::_sp secondary) {
  this->invalidate();
}


bool TimeSeriesFilterSecondary::hasUpstreamSeries(TimeSeries::_sp other) {
  return TimeSeriesFilter::hasUpstreamSeries(other) || (this->secondary() && this->secondary()->hasUpstreamSeries(other));
}
