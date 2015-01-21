#include "ValidRangeTimeSeries.h"

#include <boost/foreach.hpp>

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
  this->invalidate();
}

ValidRangeTimeSeries::filterMode_t ValidRangeTimeSeries::mode() {
  return _mode;
}
void ValidRangeTimeSeries::setMode(filterMode_t mode) {
  if (mode == drop) {
    Clock::_sp c;
    this->setClock(c);
  }
  else if (this->source()) {
    TimeSeriesFilter::_sp filterSource = boost::dynamic_pointer_cast<TimeSeriesFilter>(this->source());
    if (filterSource && filterSource->clock()) {
      this->setClock(filterSource->clock());
    }
  }
  _mode = mode;
  this->invalidate();
}


Point ValidRangeTimeSeries::filteredWithSourcePoint(RTX::Point sourcePoint) {
  
  Point convertedSourcePoint = Point::convertPoint(sourcePoint, this->source()->units(), this->units());
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


/*




Point ValidRangeTimeSeries::pointBefore(time_t time) {
  
  Point priorPoint;
  
  priorPoint = RTX_VRTS_SUPER::pointBefore(time);
  
  // If baseclass pointBefore returns an invalid point,
  // then keep searching backwards
  time_t priorTime = 1;
  while (!priorPoint.isValid && priorTime > 0) {
    time_t fetchTime = priorPoint.time;
    priorPoint = RTX_VRTS_SUPER::pointBefore(fetchTime);
    priorTime = priorPoint.time;
    if (priorTime == 0) {
      cout << "break" << endl;
    }
  }
//  std::cout << priorPoint << endl;
  return priorPoint;
}

Point ValidRangeTimeSeries::pointAfter(time_t time) {
  
  Point afterPoint = RTX_VRTS_SUPER::pointAfter(time);
  
  // If baseclass pointAfter returns an invalid point,
  // then keep searching 
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
*/