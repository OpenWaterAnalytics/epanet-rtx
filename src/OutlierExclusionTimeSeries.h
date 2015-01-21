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
    virtual PointCollection filterPointsAtTimes(std::set<time_t> times);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
  private:
    double _outlierMultiplier;
    exclusion_mode_t _exclusionMode;
  };
}

#endif /* defined(__epanet_rtx__OutlierExclusionTimeSeries__) */
