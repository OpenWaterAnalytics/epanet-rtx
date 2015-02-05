#include "InversionTimeSeries.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;


Point InversionTimeSeries::filteredWithSourcePoint(RTX::Point sourcePoint) {
  Units invertedUnits = RTX_DIMENSIONLESS / source()->units();
  return Point::convertPoint(sourcePoint.inverse(), invertedUnits, this->units());
}

bool InversionTimeSeries::canSetSource(TimeSeries::_sp ts) {
  if (ts) {
    if (ts->units().isSameDimensionAs(RTX_DIMENSIONLESS / this->units()) || this->units().isDimensionless()) {
      return true;
    }
  }
  return false;
}

void InversionTimeSeries::didSetSource(TimeSeries::_sp ts) {
  // don't re-set units if it's dimensionally consistent with inversion.
  if (!this->units().isSameDimensionAs(RTX_DIMENSIONLESS / source()->units())) {
    this->setUnits(RTX_DIMENSIONLESS / this->source()->units());
  }
  
}

bool InversionTimeSeries::canChangeToUnits(Units units) {
  
  if (this->units().isDimensionless() || !this->source()) {
    return true;
  }
  
  if (this->source()) {
    Units derivedUnits = RTX_DIMENSIONLESS / this->source()->units();
    if (!derivedUnits.isSameDimensionAs(this->units())) {
      return false;
    }
  }
  
  return true;
}
