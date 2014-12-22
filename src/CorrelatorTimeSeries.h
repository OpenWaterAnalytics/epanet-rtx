//
//  CorrelatorTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__CorrelatorTimeSeries__
#define __epanet_rtx__CorrelatorTimeSeries__

#include "ModularTimeSeries.h"

#include <iostream>


namespace RTX {
  class CorrelatorTimeSeries : public ModularTimeSeries
  {
  public:
    RTX_SHARED_POINTER(CorrelatorTimeSeries);
    CorrelatorTimeSeries();
    
    TimeSeries::sharedPointer correlatorTimeSeries();
    void setCorrelatorTimeSeries(TimeSeries::sharedPointer ts);
    
    // superclass overrides
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual bool canAlterDimension() { return true; };
    
    Clock::sharedPointer correlationWindow();
    void setCorrelationWindow(Clock::sharedPointer correlationWindow);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    
  private:
    TimeSeries::sharedPointer _secondary;
    Clock::sharedPointer _corWindow;
  };
}








#endif /* defined(__epanet_rtx__CorrelatorTimeSeries__) */
