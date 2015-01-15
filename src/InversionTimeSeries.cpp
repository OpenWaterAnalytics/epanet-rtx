#include "InversionTimeSeries.h"

using namespace RTX;
using namespace std;

void InversionTimeSeries::setSource(TimeSeries::_sp ts) {
  ModularTimeSeries::setSource(ts);
  this->checkUnits();
}

void InversionTimeSeries::checkUnits() {
  if (this->source()) {
    Units derivedUnits = RTX_DIMENSIONLESS / this->source()->units();
    if (!derivedUnits.isSameDimensionAs(this->units())) {
      TimeSeries::setUnits(derivedUnits); // base class implementation does not check for source consistency.
    }
  }
}


Point InversionTimeSeries::filteredSingle(Point p, Units sourceU) {
  Point newP = p.inverse();
  return newP;
}