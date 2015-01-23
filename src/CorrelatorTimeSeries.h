//
//  CorrelatorTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__CorrelatorTimeSeries__
#define __epanet_rtx__CorrelatorTimeSeries__

#include "TimeSeriesFilter.h"

#include <iostream>


namespace RTX {
  class CorrelatorTimeSeries : public TimeSeriesFilter
  {
  public:
    RTX_SHARED_POINTER(CorrelatorTimeSeries);
    CorrelatorTimeSeries();
    
    TimeSeries::_sp correlatorTimeSeries();
    void setCorrelatorTimeSeries(TimeSeries::_sp ts);
    
    Clock::_sp correlationWindow();
    void setCorrelationWindow(Clock::_sp correlationWindow);
    
  protected:
    PointCollection filterPointsInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    TimeSeries::_sp _secondary;
    Clock::_sp _corWindow;
  };
}








#endif /* defined(__epanet_rtx__CorrelatorTimeSeries__) */
