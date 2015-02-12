#include "ConstantTimeSeries.h"

using namespace RTX;
using namespace std;

ConstantTimeSeries::ConstantTimeSeries() {
  _value = 0.;
}

Point ConstantTimeSeries::syntheticPoint(time_t time)  {
  Point p(time,_value);
  p.addQualFlag(Point::rtx_constant);
  return p;
}


void ConstantTimeSeries::setValue(double value) {
  _value = value;
}

double ConstantTimeSeries::value() {
  return _value;
}
