//
//  FailoverTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/12/14.
//
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
    
    TimeSeries::sharedPointer failoverTimeseries();
    void setFailoverTimeseries(TimeSeries::sharedPointer ts);
    
    void swapSourceWithFailover();
    
    // superclass overrides
    virtual void setSource(TimeSeries::sharedPointer source);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    
  private:
    TimeSeries::sharedPointer _failover;
    time_t _stale;
  };
}








#endif /* defined(__epanet_rtx__FailoverTimeSeries__) */
