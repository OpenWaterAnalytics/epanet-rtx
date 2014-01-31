#include "ConstantTimeSeries.h"

using namespace RTX;
using namespace std;

ConstantTimeSeries::ConstantTimeSeries() {
  _value = 0.;
  Clock::sharedPointer reg( new Clock(3600) );
  this->setClock(reg);
}

Point ConstantTimeSeries::point(time_t time)  {
  return Point(time, _value);
}

void ConstantTimeSeries::setValue(double value) {
  _value = value;
}

double ConstantTimeSeries::value() {
  return _value;
}