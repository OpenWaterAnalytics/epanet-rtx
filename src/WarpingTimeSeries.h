//
//  WarpingTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__WarpingTimeSeries__
#define __epanet_rtx__WarpingTimeSeries__

#include <iostream>

#include "ModularTimeSeries.h"

namespace RTX {
  class WarpingTimeSeries : public ModularTimeSeries {
  
  public:
    RTX_SHARED_POINTER(WarpingTimeSeries);
    TimeSeries::_sp warp();
    void setWarp(TimeSeries::_sp warp);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime);
    
  private:
    TimeSeries::_sp _warpingBasis;
  };
}

#endif /* defined(__epanet_rtx__WarpingTimeSeries__) */
