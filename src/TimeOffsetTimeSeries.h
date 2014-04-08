//
//  TimeOffsetTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 4/2/14.
//
//

#ifndef __epanet_rtx__TimeOffsetTimeSeries__
#define __epanet_rtx__TimeOffsetTimeSeries__

#include <iostream>

#include "SinglePointFilterModularTimeSeries.h"

#define TIME_OFFSET_SUPER SinglePointFilterModularTimeSeries

namespace RTX {
  class TimeOffsetTimeSeries : public SinglePointFilterModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(TimeOffsetTimeSeries);
    TimeOffsetTimeSeries();
    void setOffset(time_t offset);
    time_t offset();
    
    Point point(time_t time);
    
  protected:
    std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    Point filteredSingle(Point p, Units sourceU); // override for extended functionality

  private:
    time_t _offset;
  };
}


#endif /* defined(__epanet_rtx__TimeOffsetTimeSeries__) */
