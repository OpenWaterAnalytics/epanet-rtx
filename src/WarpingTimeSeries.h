//
//  WarpingTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 4/8/14.
//
//

#ifndef __epanet_rtx__WarpingTimeSeries__
#define __epanet_rtx__WarpingTimeSeries__

#include <iostream>

#include "ModularTimeSeries.h"

namespace RTX {
  class WarpingTimeSeries : public ModularTimeSeries {
  
  public:
    RTX_SHARED_POINTER(WarpingTimeSeries);
    TimeSeries::sharedPointer warp();
    void setWarp(TimeSeries::sharedPointer warp);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    TimeSeries::sharedPointer _warpingBasis;
  };
}

#endif /* defined(__epanet_rtx__WarpingTimeSeries__) */
