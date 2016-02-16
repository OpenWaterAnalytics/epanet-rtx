//
//  FailoverTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__FailoverTimeSeries__
#define __epanet_rtx__FailoverTimeSeries__

#include "TimeSeriesFilterSecondary.h"

#include <iostream>


namespace RTX {
  class FailoverTimeSeries : public TimeSeriesFilterSecondary
  {
  public:
    RTX_SHARED_POINTER(FailoverTimeSeries);
    
    time_t maximumStaleness();
    void setMaximumStaleness(time_t stale);
    
    void swapSourceWithFailover();
    
    time_t timeBefore(time_t time);
    time_t timeAfter(time_t time);

  protected:
    bool canSetSecondary(TimeSeries::_sp secondary);
    PointCollection filterPointsInRange(TimeRange range);
    std::set<time_t> timeValuesInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    
  private:
    time_t _stale;
  };
}








#endif /* defined(__epanet_rtx__FailoverTimeSeries__) */
