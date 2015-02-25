//
//  TimeRange.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/25/15.
//
//

#include "TimeRange.h"


using namespace RTX;


TimeRange::TimeRange() {
  start = 0;
  end = 0;
}
TimeRange::TimeRange(time_t i_start, time_t i_end) {
  start = i_start;
  end = i_end;
}
time_t TimeRange::duration() {
  return end - start;
}
bool TimeRange::contains(time_t time) {
  return (start <= time && time <= end);
}
bool TimeRange::containsRange(RTX::TimeRange range) {
  return ( this->contains(range.start) && this->contains(range.end) );
}

bool TimeRange::touches(TimeRange otherRange) {
  if (!this->isValid() || !otherRange.isValid()) {
    return false;
  }
  if (this->contains(otherRange.start) || this->contains(otherRange.end)) {
    return true;
  }
  else if ( otherRange.contains(this->start) || otherRange.contains(this->end) ) {
    return true;
  }
  else {
    return false;
  }
}
bool TimeRange::isValid() {
  if (this->contains(0)) {
    return false;
  }
  if (this->start <= 0 || this->end <= 0) {
    return false;
  }
  return true;
}

void TimeRange::correctWithRange(RTX::TimeRange otherRange) {
  
  if (this->start == 0) {
    this->start = otherRange.start;
  }
  if (this->end == 0) {
    this->end = otherRange.end;
  }
  
}

