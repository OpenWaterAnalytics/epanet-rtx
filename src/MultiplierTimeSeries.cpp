#include "MultiplierTimeSeries.h"

using namespace RTX;
using namespace std;



MultiplierTimeSeries::MultiplierTimeSeries() {
  _multiplierBasis = TimeSeries::_sp(new TimeSeries());
}

void MultiplierTimeSeries::setMultiplier(TimeSeries::_sp ts) {
  _multiplierBasis = ts;
  this->checkUnits();
}
TimeSeries::_sp MultiplierTimeSeries::multiplier() {
  return _multiplierBasis;
}

bool MultiplierTimeSeries::isCompatibleWith(TimeSeries::_sp otherSeries) {
  // basic check for compatible regular time series.
  Clock::_sp theirClock = otherSeries->clock(), myClock = this->clock();
  bool clocksCompatible = myClock->isCompatibleWith(theirClock);
//  bool unitsCompatible = units().isDimensionless() || (units()*otherSeries->units()).isSameDimensionAs(otherSeries->units());
  bool unitsCompatible = true;
  return (clocksCompatible && unitsCompatible);
}


Point MultiplierTimeSeries::filteredWithSourcePoint(Point sourcePoint) {
  if (!this->multiplier()) {
    return Point();
  }
  set<time_t> t;
  t.insert(sourcePoint.time);
  vector<Point> multiplierPoint = this->multiplier()->resampled(t).points;
  
  if (multiplierPoint.size() < 1) {
    return Point();
  }
  
  Point x = multiplierPoint.front();
  
  if (!x.isValid) {
    return Point();
  }
  
  Point multiplied = sourcePoint * x;
  Point outPoint = Point::convertPoint(multiplied, this->source()->units() * this->multiplier()->units(), this->units());
  
  return outPoint;
}

bool MultiplierTimeSeries::canSetSource(TimeSeries::_sp ts) {
  
  return true;
  
}

void MultiplierTimeSeries::didSetSource(TimeSeries::_sp ts) {
  if (!this->source() || !this->multiplier()) {
    return;
  }
  
  Units nativeDerivedUnits = this->source()->units() * this->multiplier()->units();
  if (!this->units().isSameDimensionAs(nativeDerivedUnits)) {
    this->setUnits(nativeDerivedUnits)
  }
  
}

bool MultiplierTimeSeries::canChangeToUnits(Units units) {
  if (!this->source() || !this->multiplier()) {
    return true; // if the inputs are not fully set, then accept any units.
  }
  else if (units.isSameDimensionAs(this->source()->units() * this->multiplier()->units())) {
    return true;
  }
  else {
    return false;
  }
}





void MultiplierTimeSeries::checkUnits() {
  if (_multiplierBasis && this->source()) {
    Units derivedUnits = _multiplierBasis->units() * this->source()->units();
    if (!derivedUnits.isSameDimensionAs(this->units())) {
      TimeSeries::setUnits(derivedUnits); // base class implementation does not check for source consistency.
    }
  }
}

std::vector<Point> MultiplierTimeSeries::filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime) {
  
  // call the multiplier basis method to cache points
  if (this->multiplier()) {
    vector<Point> multiplierPoints = this->multiplier()->points(fromTime, toTime);
  }
  else {
    vector<Point> blank;
    return blank;
  }
  
  
  return SinglePointFilterModularTimeSeries::filteredPoints(sourceTs, fromTime, toTime);
}

Point MultiplierTimeSeries::filteredSingle(Point p, Units sourceU) {
  
  Point newP = Point::convertPoint(p, sourceU, units() / this->multiplier()->units());
  Point multiplierPoint = (this->multiplier()) ? this->multiplier()->point(p.time) : Point(p.time,1.);
  if (multiplierPoint.isValid) {
    newP *= multiplierPoint.value;
  }
  else {
    newP = Point();
  }
  
  return newP;
}
