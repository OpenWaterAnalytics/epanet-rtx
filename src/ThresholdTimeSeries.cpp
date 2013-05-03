#include <boost/foreach.hpp>

#include "ThresholdTimeSeries.h"

using namespace std;
using namespace RTX;

ThresholdTimeSeries::ThresholdTimeSeries() {
  _threshold = 0;
  _fixedValue = 1;
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

Point ThresholdTimeSeries::filteredSingle(RTX::Point p, RTX::Units sourceU) {
  // we don't care about units.
  double pointValue = (p.value > _threshold) ? _fixedValue : 0.;
  Point newPoint(p.time, pointValue, p.quality, p.confidence);
  return newPoint;
}

