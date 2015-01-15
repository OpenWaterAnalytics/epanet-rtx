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
    
    TimeSeries::_sp correlatorTimeSeries();
    void setCorrelatorTimeSeries(TimeSeries::_sp ts);
    
    // superclass overrides
    virtual void setSource(TimeSeries::_sp source);
    virtual bool canAlterDimension() { return true; };
    
    Clock::_sp correlationWindow();
    void setCorrelationWindow(Clock::_sp correlationWindow);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime);
    virtual bool isCompatibleWith(TimeSeries::_sp withTimeSeries);
    
  private:
    TimeSeries::_sp _secondary;
    Clock::_sp _corWindow;
  };
}








#endif /* defined(__epanet_rtx__CorrelatorTimeSeries__) */
