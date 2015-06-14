//
//  FailoverTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__FailoverTimeSeries__
#define __epanet_rtx__FailoverTimeSeries__

#include "TimeSeriesFilter.h"

#include <iostream>


namespace RTX {
  class FailoverTimeSeries : public TimeSeriesFilter
  {
  public:
    RTX_SHARED_POINTER(FailoverTimeSeries);
    
    time_t maximumStaleness();
    void setMaximumStaleness(time_t stale);
    
    TimeSeries::_sp failoverTimeseries();
    void setFailoverTimeseries(TimeSeries::_sp ts);
    
    void swapSourceWithFailover();
    
    Point pointBefore(time_t time);
    Point pointAfter(time_t time);

  protected:
    PointCollection filterPointsInRange(TimeRange range);
    std::set<time_t> timeValuesInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    
  private:
    TimeSeries::_sp _failover;
    time_t _stale;
  };
}








#endif /* defined(__epanet_rtx__FailoverTimeSeries__) */
