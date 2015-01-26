#include "MathOpsTimeSeries.h"
#include <cmath>

using namespace RTX;
using namespace std;

MathOpsTimeSeries::MathOpsTimeSeries() {
  _mathOpsType = MathOpsTimeSeriesNoOp;
  _arg = 1.;
}



MathOpsTimeSeries::MathOpsTimeSeriesType MathOpsTimeSeries::mathOpsType() {
  return _mathOpsType;
}

void MathOpsTimeSeries::setMathOpsType(MathOpsTimeSeriesType type) {
  _mathOpsType = type;
  Units originalUnits = this->units();
  if (source()) {
    Units newUnits = mathOpsUnits(source()->units(), type);
    if (!newUnits.isSameDimensionAs(originalUnits)) {
      setUnits(newUnits);
    }
  }
  this->invalidate();
}

double MathOpsTimeSeries::argument() {
  return _arg;
}

void MathOpsTimeSeries::setArgument(double arg) {
  
  if (this->mathOpsType() == MathOpsTimeSeriesExpBase && arg < 0) {
    arg = 0;
  }
  _arg = arg;
  this->invalidate();
  
  Units originalUnits = this->units();
  if (this->source()) {
    Units newUnits = mathOpsUnits(source()->units(), this->mathOpsType());
    if (!newUnits.isSameDimensionAs(originalUnits)) {
      this->setUnits(newUnits);
    }
  }
  
}


Point MathOpsTimeSeries::filteredWithSourcePoint(Point sourcePoint) {
  
  Point p(sourcePoint.time);
  
  switch (_mathOpsType) {
    case MathOpsTimeSeriesAbs:
      p.value = abs(sourcePoint.value);
      break;
    case MathOpsTimeSeriesLog:
      p.value = log(sourcePoint.value);
      break;
    case MathOpsTimeSeriesLog10:
      p.value = log10(sourcePoint.value);
      break;
    case MathOpsTimeSeriesExp:
      p.value = exp(sourcePoint.value);
      break;
    case MathOpsTimeSeriesExpBase:
      p.value = pow(_arg,sourcePoint.value);
      break;
    case MathOpsTimeSeriesSqrt:
      p.value = sqrt(sourcePoint.value);
      break;
    case MathOpsTimeSeriesPow:
      p.value = pow(sourcePoint.value,_arg);
      break;
    case MathOpsTimeSeriesCeil:
      p.value = ceil(sourcePoint.value);
      break;
    case MathOpsTimeSeriesFloor:
      p.value = floor(sourcePoint.value);
      break;
    case MathOpsTimeSeriesRound:
      p.value = round(sourcePoint.value);
      break;
    default:
      p.value = sourcePoint.value;
      break;
  }
  
  Point newPoint = Point::convertPoint(p, mathOpsUnits(this->source()->units(), _mathOpsType), this->units());
  return newPoint;
}


bool MathOpsTimeSeries::canSetSource(TimeSeries::_sp ts) {
  
  return true;
  
}
void MathOpsTimeSeries::didSetSource(TimeSeries::_sp ts) {
  
  if (ts) {
    Units units = mathOpsUnits(ts->units(), mathOpsType());
    if (!units.isSameDimensionAs(this->units())) {
      this->setUnits(units);
    }
  }
  
}

bool MathOpsTimeSeries::canChangeToUnits(Units units) {
  if (!source()) {
    return true;
  }
  
  Units constraint = this->mathOpsUnits(this->source()->units(), this->mathOpsType());
  if (units.isSameDimensionAs(constraint)) {
    return true;
  }
  
  return false;
}




Units MathOpsTimeSeries::mathOpsUnits(Units sourceUnits, MathOpsTimeSeriesType type) {
  switch (type) {
    case MathOpsTimeSeriesNoOp:
      return sourceUnits;
      break;
    case MathOpsTimeSeriesAbs:
      return sourceUnits;
      break;
    case MathOpsTimeSeriesLog:
    case MathOpsTimeSeriesLog10:
    case MathOpsTimeSeriesExp:
    case MathOpsTimeSeriesExpBase:
      if (!sourceUnits.isDimensionless()) {
        cerr << "ERR: Math operation on dimensional series will break Units consistency" << endl;
      }
      return RTX_DIMENSIONLESS;
      break;
    case MathOpsTimeSeriesSqrt:
      return sourceUnits^0.5;
      break;
    case MathOpsTimeSeriesPow:
      return sourceUnits^_arg;
      break;
    case MathOpsTimeSeriesCeil:
      return sourceUnits;
      break;
    case MathOpsTimeSeriesFloor:
      return sourceUnits;
      break;
    case MathOpsTimeSeriesRound:
      return sourceUnits;
      break;
    default:
      break;
  }
}


