//
//  TimeRange.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/25/15.
//
//

#ifndef __epanet_rtx__TimeRange__
#define __epanet_rtx__TimeRange__

#include <stdio.h>
#include <time.h>

namespace RTX {
  class TimeRange {
  public:
    enum intersect_type {
      intersect_left,           /// other range is to the left
      intersect_right,          /// other range is to the right
      intersect_equal,          /// ranges are equal
      intersect_other_internal, /// other range is fully internal / contained within me
      intersect_other_external, /// other range contains and extends mine
      intersect_none            /// no intersection
    };
    
    TimeRange();
    TimeRange(time_t start, time_t end);
    time_t duration();
    bool contains(const time_t& time) const;
    bool containsRange(const TimeRange& range) const;
    bool touches(const TimeRange& otherRange);
    intersect_type intersection(const TimeRange& otherRange);
    bool isValid() const;
    void correctWithRange(const TimeRange& otherRange);
    
    static TimeRange unionOf(TimeRange r1, TimeRange r2);
    static TimeRange intersectionOf(TimeRange r1, TimeRange r2);
    
    time_t start, end;
  };
}

#endif /* defined(__epanet_rtx__TimeRange__) */
