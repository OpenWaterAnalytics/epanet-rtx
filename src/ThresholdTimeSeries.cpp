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
}

double ThresholdTimeSeries::threshold() {
  return _threshold;
}

void ThresholdTimeSeries::setValue(double val) {
  _fixedValue = val;
}

double ThresholdTimeSeries::value() {
  return _fixedValue;
}

void ThresholdTimeSeries::setMode(thresholdMode_t mode) {
  _mode = mode;
}

ThresholdTimeSeries::thresholdMode_t ThresholdTimeSeries::mode() {
  return _mode;
}

void ThresholdTimeSeries::setSource(TimeSeries::sharedPointer source) {
  Units originalUnits = this->units();
  this->setUnits(RTX_DIMENSIONLESS);  // non-dimensionalize so that we can accept this source.
  ModularTimeSeries::setSource(source);
  
  // Threshold units can be anything - reset them
  this->setUnits(originalUnits);
}

void ThresholdTimeSeries::setUnits(Units newUnits) {
  
  // Threshold units can be different from the source units; the value is assumed
  // to be in Threshold units, and the threshold is assumed to be in source units -
  // so just set them
  TimeSeries::setUnits(newUnits);
}


Point ThresholdTimeSeries::filteredSingle(RTX::Point p, RTX::Units sourceU) {
  // we don't care about units.
  double pointValue;
  if (_mode == thresholdModeAbsolute) {
    pointValue = (abs(p.value) > _threshold) ? _fixedValue : 0.;
  }
  else {
    pointValue = (p.value > _threshold) ? _fixedValue : 0.;
  }
  Point newPoint(p.time, pointValue, p.quality, p.confidence);
  return newPoint;
}

