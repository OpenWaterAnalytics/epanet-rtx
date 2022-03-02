//
//  TimeRange.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/25/15.
//
//

#include "TimeRange.h"
#include "rtxMacros.h"

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
bool TimeRange::contains(const time_t& time) const {
  return (start <= time && time <= end);
}
bool TimeRange::containsRange(const TimeRange& range) const {
  return ( this->contains(range.start) && this->contains(range.end) );
}

bool TimeRange::touches(const TimeRange& otherRange) {
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
bool TimeRange::isValid() const {
  if (this->contains(0)) {
    return false;
  }
  if (this->start <= 0 || this->end <= 0) {
    return false;
  }
  return true;
}

void TimeRange::correctWithRange(const TimeRange& otherRange) {
  if (this->isValid()) {
    return;
  }
  if (this->start == 0) {
    this->start = otherRange.start;
  }
  if (this->end == 0) {
    this->end = otherRange.end;
  }
}

TimeRange TimeRange::unionOf(TimeRange r1, TimeRange r2) {
  
  TimeRange r(r1);
  
  if (r.start > r2.start) {
    r.start = r2.start;
  }
  if (r.end < r2.end) {
    r.end = r2.end;
  }
  
  return r;
}


TimeRange::intersect_type TimeRange::intersection(const TimeRange& otherRange) {
  
  if (!this->touches(otherRange)) {
    return intersect_none;
  }
  
  if (this->containsRange(otherRange) && otherRange.containsRange(*this)) {
    return intersect_equal;
  }
  
  if ( otherRange.start < this->start
      && otherRange.end < this->end
      && this->start < otherRange.end ) {
    return intersect_left;
  }
  
  if ( this->end < otherRange.end
      && this->start < otherRange.start
      && otherRange.start < this->end ) {
    return intersect_right;
  }
  
  if (this->containsRange(otherRange)) {
    return intersect_other_internal;
  }
  
  if (otherRange.containsRange(*this)) {
    return intersect_other_external;
  }
  
  return intersect_none;
  
}


TimeRange TimeRange::intersectionOf(TimeRange r1, TimeRange r2) {
  
  if (!r1.touches(r2)) {
    return TimeRange(0,0);
  }
  
  TimeRange net( RTX_MAX(r1.start,r2.start), RTX_MIN(r1.end, r2.end) );
  
  return net;
}




