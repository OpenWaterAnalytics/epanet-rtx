#include "ConstantTimeSeries.h"

using namespace RTX;
using namespace std;

ConstantTimeSeries::ConstantTimeSeries() {
  _value = 0.;
  Clock::_sp reg( new Clock(3600) );
  reg->setName("RTX CONSTANT 3600s");
  this->setClock(reg);
}

Point ConstantTimeSeries::syntheticPoint(time_t time)  {
  return Point(time, _value, Point::constant);
}


void ConstantTimeSeries::setValue(double value) {
  _value = value;
}

double ConstantTimeSeries::value() {
  return _value;
}
