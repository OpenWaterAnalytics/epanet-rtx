//
//  LagTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/20/15.
//
//

#ifndef __epanet_rtx__LagTimeSeries__
#define __epanet_rtx__LagTimeSeries__

#include <stdio.h>

#include "TimeSeriesFilter.h"

namespace RTX {
  class LagTimeSeries : public TimeSeriesFilter {
  public:
    RTX_SHARED_POINTER(LagTimeSeries);
    LagTimeSeries();
    
    void setOffset(time_t offset);
    time_t offset();
    
    Point pointBefore(time_t time);
    Point pointAfter(time_t time);
    
  protected:
    virtual std::set<time_t> timeValuesInRange(TimeRange range);
    virtual PointCollection filterPointsAtTimes(std::set<time_t> times);
    
  private:
    time_t _lag;
    
  };
}

#endif /* defined(__epanet_rtx__LagTimeSeries__) */
