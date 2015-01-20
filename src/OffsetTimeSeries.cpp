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

Point OffsetTimeSeries::filteredWithSourcePoint(Point sourcePoint) {
  Point converted = Point::convertPoint(sourcePoint, source()->units(), this->units());
  converted += this->offset();
  return converted;
}
