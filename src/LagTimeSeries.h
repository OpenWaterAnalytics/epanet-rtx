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
    RTX_BASE_PROPS(LagTimeSeries);
    LagTimeSeries();
    
    void setOffset(time_t offset);
    time_t offset();
    
    time_t timeAfter(time_t t);
    time_t timeBefore(time_t t);
    
  protected:
    bool willResample();
    PointCollection filterPointsInRange(TimeRange range);
    std::set<time_t> timeValuesInRange(TimeRange range);
    
  private:
    time_t _lag;
    
  };
}

#endif /* defined(__epanet_rtx__LagTimeSeries__) */
