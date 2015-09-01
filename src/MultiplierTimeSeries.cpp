#include "MultiplierTimeSeries.h"

using namespace RTX;
using namespace std;



MultiplierTimeSeries::MultiplierTimeSeries() {
  _multiplierBasis = TimeSeries::_sp(new TimeSeries());
  _mode = MultiplierModeMultiply;
}

void MultiplierTimeSeries::setMultiplier(TimeSeries::_sp ts) {
  _multiplierBasis = ts;
  this->invalidate();
  // post-fix units
  this->didSetSource(this->source()); // trigger re-evaluation of units.
}
TimeSeries::_sp MultiplierTimeSeries::multiplier() {
  return _multiplierBasis;
}

MultiplierTimeSeries::MultiplierMode MultiplierTimeSeries::multiplierMode() {
  return _mode;
}
void MultiplierTimeSeries::setMultiplierMode(MultiplierTimeSeries::MultiplierMode mode) {
  _mode = mode;
  this->invalidate();
  this->didSetSource(this->source());
}


Point MultiplierTimeSeries::filteredWithSourcePoint(Point sourcePoint) {
  if (!this->multiplier()) {
    return Point();
  }
  set<time_t> t;
  t.insert(sourcePoint.time);
  
  // get resampled point at this sourcepoint time value
  TimeRange effectiveRange;
  effectiveRange.start = this->multiplier()->timeBefore(sourcePoint.time + 1);
  effectiveRange.end = this->multiplier()->timeAfter(sourcePoint.time - 1);
  
  // if there are no points before or after the requested range, just get the points that are there.
  effectiveRange.correctWithRange(TimeRange(sourcePoint.time, sourcePoint.time));
  
  PointCollection nativePoints = this->multiplier()->pointCollection(effectiveRange);
  nativePoints.resample(t);
  vector<Point> multiplierPoint = nativePoints.points;
  
  if (multiplierPoint.size() < 1) {
    return Point();
  }
  
  Point x = multiplierPoint.front();
  
  if (!x.isValid) {
    return Point();
  }
  
  Point derivedPoint;
  
  switch (_mode) {
    case MultiplierModeMultiply:
      derivedPoint = sourcePoint * x.value;
      break;
    case MultiplierModeDivide:
      derivedPoint = sourcePoint / x.value;
      break;
    default:
      break;
  }
  
  Point outPoint = Point::convertPoint(derivedPoint, this->nativeUnits(), this->units());
  return outPoint;
}

bool MultiplierTimeSeries::canSetSource(TimeSeries::_sp ts) {
  return true;
}


Units MultiplierTimeSeries::nativeUnits() {
  Units nativeDerivedUnits = RTX_NO_UNITS;
  
  if (!this->source() || !this->multiplier()) {
    return RTX_NO_UNITS;
  }
  
  switch (_mode) {
    case MultiplierModeMultiply:
      nativeDerivedUnits = this->source()->units() * this->multiplier()->units();
      break;
    case MultiplierModeDivide:
      nativeDerivedUnits = this->source()->units() / this->multiplier()->units();
      break;
    default:
      break;
  }
  
  return nativeDerivedUnits;
}

void MultiplierTimeSeries::didSetSource(TimeSeries::_sp ts) {
  if (!this->source() || !this->multiplier()) {
    this->setUnits(RTX_DIMENSIONLESS);
    return;
  }
  
  Units nativeDerivedUnits = this->nativeUnits();
  
  if (!this->units().isSameDimensionAs(nativeDerivedUnits) || this->units() == RTX_NO_UNITS) {
    this->setUnits(nativeDerivedUnits);
  }
  
}

bool MultiplierTimeSeries::canChangeToUnits(Units units) {
  if (!this->source() || !this->multiplier()) {
    return true; // if the inputs are not fully set, then accept any units.
  }
  else if (units.isSameDimensionAs(this->nativeUnits())) {
    return true;
  }
  else {
    return false;
  }
}


