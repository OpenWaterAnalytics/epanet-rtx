#ifdef _WIN32
  #define _USE_MATH_DEFINES
#endif

#include <math.h>

#include "SineTimeSeries.h"


using namespace RTX;
using namespace std;

SineTimeSeries::SineTimeSeries(double magnitude, time_t period) {
  Clock::sharedPointer clock(new Clock(600));
  setClock(clock);
  _period = period;
  _magnitude = magnitude;
}

time_t SineTimeSeries::period() {
  return _period;
}
double SineTimeSeries::magnitude() {
  return _magnitude;
}


Point SineTimeSeries::point(time_t time) {
  if ( !(clock()->isValid(time)) ) {
    // if the time is not valid, rewind until a valid time is reached.
    time = clock()->timeBefore(time);
  }
  
  double value = _magnitude * sin((double)time * M_PI * 2 / (_period));
  return Point(time, value);
  
}