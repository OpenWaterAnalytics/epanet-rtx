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
}

Units GainTimeSeries::gainUnits() {
  return _gainUnits;
}
void GainTimeSeries::setGainUnits(RTX::Units u) {
  _gainUnits = u;
  
  // need to alter dimension of my own units in response?
  if (this->source() && !(this->units().isSameDimensionAs(u * this->source()->units()))) {
    this->setUnits(u * this->source()->units());
  }
  
}


bool GainTimeSeries::isCompatibleWith(TimeSeries::sharedPointer ts) {
  return true;
}

void GainTimeSeries::setUnits(RTX::Units u) {
  TimeSeries::setUnits(u); // bypass ModularTimeSeries checking
}



Point GainTimeSeries::filteredSingle(RTX::Point p, RTX::Units sourceU) {
  p *= gain();
  Point newPoint = Point::convertPoint(p, (sourceU * gainUnits()), this->units());
  return newPoint;
}




