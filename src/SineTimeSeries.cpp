#ifdef _WIN32
  #define _USE_MATH_DEFINES
#endif

#include <math.h>

#include "SineTimeSeries.h"


using namespace RTX;
using namespace std;

SineTimeSeries::SineTimeSeries(double magnitude, time_t period) {
  _period = period;
  _magnitude = magnitude;
}

time_t SineTimeSeries::period() {
  return _period;
}
double SineTimeSeries::magnitude() {
  return _magnitude;
}


Point SineTimeSeries::syntheticPoint(time_t time) {
  double value = _magnitude * sin((double)time * M_PI * 2 / (_period));
  return Point(time, value);
}