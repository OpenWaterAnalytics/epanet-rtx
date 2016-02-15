//
//  LogicTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 6/12/15.
//
//

#ifndef __epanet_rtx__LogicTimeSeries__
#define __epanet_rtx__LogicTimeSeries__

#include "TimeSeriesFilterSecondary.h"

#include <iostream>

namespace RTX {
  
  
  //! The logic filter implements common logical operations.
  
  
  class LogicTimeSeries : public TimeSeriesFilterSecondary
  {
  public:
    
    typedef enum {
      LogicTimeSeriesAND  = 0,
      LogicTimeSeriesOR   = 1,
      LogicTimeSeriesNOT  = 2,
      LogicTimeSeriesXOR  = 3,
      LogicTimeSeriesFlipFlop = 4
    } LogicType;
    
    RTX_SHARED_POINTER(LogicTimeSeries);
    LogicTimeSeries();
    
  protected:
    bool canSetSecondary(TimeSeries::_sp secondary);
    PointCollection filterPointsInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    TimeSeries::_sp _secondary;
    
  };
}

#endif /* defined(__epanet_rtx__LogicTimeSeries__) */
