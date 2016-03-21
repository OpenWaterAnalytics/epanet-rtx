#include <boost/foreach.hpp>

#include "ThresholdTimeSeries.h"
#include <cmath>

using namespace std;
using namespace RTX;

ThresholdTimeSeries::ThresholdTimeSeries() {
  _threshold = 0;
  _fixedValue = 1;
  _mode = thresholdModeNormal;
}

void ThresholdTimeSeries::setThreshold(double threshold) {
  _threshold = threshold;
  this->invalidate();
}
double ThresholdTimeSeries::threshold() {
  return _threshold;
}

void ThresholdTimeSeries::setValue(double val) {
  _fixedValue = val;
  this->invalidate();
}
double ThresholdTimeSeries::value() {
  return _fixedValue;
}

void ThresholdTimeSeries::setMode(thresholdMode_t mode) {
  _mode = mode;
  this->invalidate();
}
ThresholdTimeSeries::thresholdMode_t ThresholdTimeSeries::mode() {
  return _mode;
}



Point ThresholdTimeSeries::filteredWithSourcePoint(RTX::Point sourcePoint) {
  double pointValue;
  if (_mode == thresholdModeAbsolute) {
    pointValue = (abs(sourcePoint.value) > _threshold) ? _fixedValue : 0.;
  }
  else {
    pointValue = (sourcePoint.value > _threshold) ? _fixedValue : 0.;
  }
  Point newPoint(sourcePoint.time, pointValue, sourcePoint.quality, sourcePoint.confidence);
  return newPoint;
}

void ThresholdTimeSeries::didSetSource(TimeSeries::_sp ts) {
  // don't mess with units.
  // if the source is a filter, and has a clock, and I don't have a clock, then use the source's clock.
  TimeSeriesFilter::_sp sourceFilter = std::dynamic_pointer_cast<TimeSeriesFilter>(ts);
  if (sourceFilter && sourceFilter->clock() && !this->clock()) {
    this->setClock(sourceFilter->clock());
  }
}

bool ThresholdTimeSeries::canChangeToUnits(RTX::Units units) {
  return true; // any units are ok.
}
