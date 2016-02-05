#include "GainTimeSeries.h"

using namespace RTX;
using namespace std;

GainTimeSeries::GainTimeSeries() {
  _gain = 1.;
  _gainUnits = RTX_DIMENSIONLESS;
}

double GainTimeSeries::gain() {
  return _gain;
}
void GainTimeSeries::setGain(double gain) {
  _gain = gain;
  this->invalidate();
}

Units GainTimeSeries::gainUnits() {
  return _gainUnits;
}
void GainTimeSeries::setGainUnits(RTX::Units u) {
  _gainUnits = u;
  this->invalidate();
  
  // need to alter dimension of my own units in response?
  if (this->source() && !(this->units().isSameDimensionAs(u * this->source()->units()))) {
    this->setUnits(u * this->source()->units());
  }
}


Point GainTimeSeries::filteredWithSourcePoint(RTX::Point sourcePoint) {
  Point p = sourcePoint * gain();
  Point newPoint = Point::convertPoint(p, (this->source()->units() * this->gainUnits()), this->units());
  return newPoint;
}


bool GainTimeSeries::canSetSource(TimeSeries::_sp ts) {
  return true;
}

void GainTimeSeries::didSetSource(TimeSeries::_sp ts) {
  Units derived = ts->units() * this->gainUnits();
  if (!this->units().isSameDimensionAs(derived) || this->units() == RTX_NO_UNITS) {
    this->setUnits(ts->units() * this->gainUnits());
  }
}

bool GainTimeSeries::canChangeToUnits(Units units) {
  
  if (!this->source()) {
    return true;
  }
  
  if (units.isSameDimensionAs(this->source()->units() * this->gainUnits())) {
    return true;
  }
  
  if ((this->units().isInvalid() || units.isInvalid()) && units.isInvalid()) {
    return true;
  }
  
  return false;
}

