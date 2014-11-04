//
//  TimeGapTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__TimeGapTimeSeries__
#define __epanet_rtx__TimeGapTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

#define RTX_TIME_GAP_SUPER ModularTimeSeries

namespace RTX {
  //!   A Class to map the input time series into a series whos values are the time gaps between successive points
  
  class TimeGapTimeSeries : public RTX_TIME_GAP_SUPER {
    
  public:
    RTX_SHARED_POINTER(TimeGapTimeSeries);
    TimeGapTimeSeries();
    
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual void setUnits(Units newUnits);
    
    virtual bool canAlterDimension() { return true; };
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  };
}
#endif /* defined(__epanet_rtx__TimeGapTimeSeries__) */
