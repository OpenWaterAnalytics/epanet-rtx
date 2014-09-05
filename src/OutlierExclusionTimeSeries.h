//
//  OutlierExclusionTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/7/14.
//
//

#ifndef __epanet_rtx__OutlierExclusionTimeSeries__
#define __epanet_rtx__OutlierExclusionTimeSeries__

#include <iostream>
#include "BaseStatsTimeSeries.h"


#define RTX_OUTX_SUPER BaseStatsTimeSeries

namespace RTX {
  
  
  class OutlierExclusionTimeSeries : public BaseStatsTimeSeries {
    
  public:
    
    typedef enum {
      OutlierExclusionModeInterquartileRange,
      OutlierExclusionModeStdDeviation
    } exclusion_mode_t;
    
    RTX_SHARED_POINTER(OutlierExclusionTimeSeries);
    OutlierExclusionTimeSeries();
    
    void setOutlierMultiplier(double multiplier);
    double outlierMultiplier();
    
    void setExclusionMode(exclusion_mode_t mode);
    exclusion_mode_t exclusionMode();
    
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    std::vector<Point> filteredPointsWithMissing(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime, bool returnMissing);
    
    Point filteredSingle(time_t time);
    
  private:
    double _outlierMultiplier;
    exclusion_mode_t _exclusionMode;
        
  };
}

#endif /* defined(__epanet_rtx__OutlierExclusionTimeSeries__) */
