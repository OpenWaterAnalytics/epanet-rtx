#include <math.h>

#include "SineTimeSeries.h"


using namespace RTX;
using namespace std;

SineTimeSeries::SineTimeSeries() {
  Clock::sharedPointer clock(new Clock(600));
  setClock(clock);
}


Point SineTimeSeries::point(time_t time) {
  if ( !(clock()->isValid(time)) ) {
    // if the time is not valid, rewind until a valid time is reached.
    time = clock()->timeBefore(time);
  }
  
  double value = sin((double)time * M_PI * 2 / (24*60*60));
  return Point(time, value);
  
}