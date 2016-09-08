//
//  IntegratorTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/13/15.
//
//

#ifndef __epanet_rtx__IntegratorTimeSeries__
#define __epanet_rtx__IntegratorTimeSeries__

#include <vector>
#include <boost/foreach.hpp>

#include "TimeSeriesFilter.h"

namespace RTX {
  class IntegratorTimeSeries : public TimeSeriesFilter {
  public:
    RTX_BASE_PROPS(IntegratorTimeSeries);
    
    void setResetClock(Clock::_sp resetClock);
    Clock::_sp resetClock();
    
    IntegratorTimeSeries::_sp resetClock(Clock::_sp c) {this->setResetClock(c); return share_me(this);};
    
  protected:
    PointCollection filterPointsInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    Clock::_sp _reset;
    
  };
}



#endif /* defined(__epanet_rtx__IntegratorTimeSeries__) */
