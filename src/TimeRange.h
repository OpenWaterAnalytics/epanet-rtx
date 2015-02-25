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
    TimeRange();
    TimeRange(time_t start, time_t end);
    time_t duration();
    bool contains(time_t time);
    bool containsRange(TimeRange range);
    bool touches(TimeRange otherRange);
    bool isValid();
    void correctWithRange(TimeRange otherRange);
    time_t start, end;
  };
}

#endif /* defined(__epanet_rtx__TimeRange__) */
