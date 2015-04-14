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
  
  
  //! The correlator will resample the secondary "correlatorTimeSeries" at the time values of its source, if needed.
  
  
  class CorrelatorTimeSeries : public TimeSeriesFilter
  {
  public:
    RTX_SHARED_POINTER(CorrelatorTimeSeries);
    CorrelatorTimeSeries();
    
    TimeSeries::_sp correlatorTimeSeries();
    void setCorrelatorTimeSeries(TimeSeries::_sp ts);
    
    Clock::_sp correlationWindow();
    void setCorrelationWindow(Clock::_sp correlationWindow);
    
    ///! this will yield a timeseries who's points are the maximum correlation, and who's confidence is the lag at which
    //   that correlation occurs. Lag indexes are calculated at the primary clock frequency.
    static TimeSeries::_sp correlationArray(TimeSeries::_sp primary, TimeSeries::_sp secondary, Clock::_sp window, int nLags);
    
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
