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
    
    Clock::_sp correlationWindow();
    void setCorrelationWindow(Clock::_sp correlationWindow);
    
  protected:
    virtual PointCollection filterPointsAtTimes(std::set<time_t> times);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
  private:
    TimeSeries::_sp _secondary;
    Clock::_sp _corWindow;
  };
}








#endif /* defined(__epanet_rtx__CorrelatorTimeSeries__) */
