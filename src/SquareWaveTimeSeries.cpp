#ifdef _WIN32
  #define _USE_MATH_DEFINES
#endif

#include <math.h>

#include "SquareWaveTimeSeries.h"


using namespace RTX;
using namespace std;

SquareWaveTimeSeries::SquareWaveTimeSeries() {
  _duration = 0;
}

void SquareWaveTimeSeries::setPeriod(Clock::_sp period) {
  _period = period;
  this->invalidate();
}

Clock::_sp SquareWaveTimeSeries::period() {
  return _period;
}

void SquareWaveTimeSeries::setDuration(time_t duration) {
  _duration = duration;
  this->invalidate();
}

time_t SquareWaveTimeSeries::duration() {
  return _duration;
}



Point SquareWaveTimeSeries::syntheticPoint(time_t time) {
  if (!this->clock() || !_period) {
    return Point();
  }
  
  if ( time <= (_period->timeBefore(time + 1) + _duration) ) {
    return Point(time, 1.0);
  }
  else {
    return Point(time, 0.0);
  }
  
}
