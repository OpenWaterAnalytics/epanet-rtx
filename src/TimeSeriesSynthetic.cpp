//
//  TimeSeriesSynthetic.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/19/15.
//
//

#include "TimeSeriesSynthetic.h"
#include <set>
#include <vector>
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

TimeSeriesSynthetic::TimeSeriesSynthetic() {
  Clock::_sp reg( new Clock(3600) );
  reg->setName("RTX CONSTANT 3600s");
  this->setClock(reg);
}

Clock::_sp TimeSeriesSynthetic::clock() {
  return _clock;
}

void TimeSeriesSynthetic::setClock(Clock::_sp clock) {
  this->invalidate();
  _clock = clock;
}

Point TimeSeriesSynthetic::point(time_t time) {
  if (this->clock() && this->clock()->isValid(time)) {
    return this->syntheticPoint(time);
  }
  return Point();
}

Point TimeSeriesSynthetic::pointBefore(time_t time) {
  if (!this->clock()) {
    return Point();
  }
  return this->point(this->clock()->timeBefore(time));
}

Point TimeSeriesSynthetic::pointAfter(time_t time) {
  if (!this->clock()) {
    return Point();
  }
  return this->point(this->clock()->timeAfter(time));
}

vector< Point > TimeSeriesSynthetic::points(TimeRange range) {
  vector<Point> outPoints;
  
  if (!this->clock()) {
    return outPoints;
  }
  
  set<time_t> times;
  
  times = this->clock()->timeValuesInRange(range.start, range.end);
  outPoints.reserve(times.size());
  
  BOOST_FOREACH(time_t now, times) {
    outPoints.push_back(this->syntheticPoint(now));
  }
  
  return outPoints;
}



