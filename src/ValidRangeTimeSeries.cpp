#include "ValidRangeTimeSeries.h"

using namespace std;
using namespace RTX;

ValidRangeTimeSeries::ValidRangeTimeSeries() {
  _mode = drop;
  _range = make_pair(0, 1);
}

pair<double,double> ValidRangeTimeSeries::range() {
  return _range;
}
void ValidRangeTimeSeries::setRange(double min, double max) {
  _range = make_pair(min, max);
}

ValidRangeTimeSeries::filterMode_t ValidRangeTimeSeries::mode() {
  return _mode;
}
void ValidRangeTimeSeries::setMode(filterMode_t mode) {
  _mode = mode;
}

Point ValidRangeTimeSeries::pointBefore(time_t time) {
  
  Point beforePoint = RTX_VRTS_SUPER::pointBefore(time);
    
  // If baseclass pointBefore returns an invalid point,
  // then keep searching backwards
  while (!beforePoint.isValid && beforePoint.time > 0) {
    beforePoint = RTX_VRTS_SUPER::pointBefore(beforePoint.time);
  }
  return beforePoint;
}

Point ValidRangeTimeSeries::pointAfter(time_t time) {
  
  Point afterPoint = RTX_VRTS_SUPER::pointAfter(time);
  
  // If baseclass pointBefore returns an invalid point,
  // then keep searching backwards
  while (!afterPoint.isValid && afterPoint.time > 0) {
    afterPoint = RTX_VRTS_SUPER::pointAfter(afterPoint.time);
  }
  return afterPoint;
}

Point ValidRangeTimeSeries::filteredSingle(RTX::Point p, RTX::Units sourceU) {
  Point convertedSourcePoint = Point::convertPoint(p, sourceU, units());
  Point newP;
  double pointValue = convertedSourcePoint.value;
  
  if (pointValue < _range.first || _range.second < pointValue) {
    // out of range.
    if (_mode == saturate) {
      // bring pointValue to the nearest max/min value
      pointValue = RTX_MAX(pointValue, _range.first);
      pointValue = RTX_MIN(pointValue, _range.second);
      newP = Point(convertedSourcePoint.time, pointValue, convertedSourcePoint.quality, convertedSourcePoint.confidence);
    }
    else {
      // drop point - return only the time
      newP.time = convertedSourcePoint.time;
    }
  }
  else {
    // just pass it through
    newP = convertedSourcePoint;
  }
  
  return newP;
}
