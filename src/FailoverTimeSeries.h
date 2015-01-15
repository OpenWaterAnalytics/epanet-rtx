//
//  FailoverTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__FailoverTimeSeries__
#define __epanet_rtx__FailoverTimeSeries__

#include "ModularTimeSeries.h"

#include <iostream>


namespace RTX {
  class FailoverTimeSeries : public ModularTimeSeries
  {
  public:
    RTX_SHARED_POINTER(FailoverTimeSeries);
    
    time_t maximumStaleness();
    void setMaximumStaleness(time_t stale);
    
    TimeSeries::_sp failoverTimeseries();
    void setFailoverTimeseries(TimeSeries::_sp ts);
    
    void swapSourceWithFailover();
    
    // superclass overrides
    virtual void setSource(TimeSeries::_sp source);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime);
    virtual bool isCompatibleWith(TimeSeries::_sp withTimeSeries);
    
  private:
    TimeSeries::_sp _failover;
    time_t _stale;
  };
}








#endif /* defined(__epanet_rtx__FailoverTimeSeries__) */
