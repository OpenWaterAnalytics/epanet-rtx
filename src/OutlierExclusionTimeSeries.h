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
    
    RTX_BASE_PROPS(OutlierExclusionTimeSeries);
    OutlierExclusionTimeSeries();
    
    void setOutlierMultiplier(double multiplier);
    double outlierMultiplier();
    
    void setExclusionMode(exclusion_mode_t mode);
    exclusion_mode_t exclusionMode();
    
//    virtual Point pointBefore(time_t time);
//    virtual Point pointAfter(time_t time);
    
    // chainable
    OutlierExclusionTimeSeries::_sp multiplier(double m) {this->setOutlierMultiplier(m); return share_me(this);};
    OutlierExclusionTimeSeries::_sp exclusionMode(exclusion_mode_t mode) {this->setExclusionMode(mode); return share_me(this);};
    
  protected:
    virtual bool willResample();
    PointCollection filterPointsInRange(TimeRange range);
    bool canDropPoints();
//    bool canSetSource(TimeSeries::_sp ts);
//    void didSetSource(TimeSeries::_sp ts);
//    bool canChangeToUnits(Units units);
    
  private:
    double _outlierMultiplier;
    exclusion_mode_t _exclusionMode;
    Point pointWithCollectionAndPoint(PointCollection c, Point p);
  };
}

#endif /* defined(__epanet_rtx__OutlierExclusionTimeSeries__) */
