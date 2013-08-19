//
//  OffsetTimeSeries.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 10/18/12.
//
//

#include <boost/foreach.hpp>

#include "OffsetTimeSeries.h"

using namespace std;
using namespace RTX;

OffsetTimeSeries::OffsetTimeSeries() {
  setOffset(0.);
}

void OffsetTimeSeries::setOffset(double offset) {
  _offset = offset;
}

double OffsetTimeSeries::offset() {
  return _offset;
}

Point OffsetTimeSeries::filteredSingle(Point p, Units sourceU) {
  Point convertedSourcePoint = Point::convertPoint(p, sourceU, units());
  double pointValue = convertedSourcePoint.value;
  pointValue += offset();
  Point newPoint(convertedSourcePoint.time, pointValue, convertedSourcePoint.quality, convertedSourcePoint.confidence);
  return newPoint;
}
