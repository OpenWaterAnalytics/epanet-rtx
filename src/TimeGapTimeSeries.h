//
//  TimeGapTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__TimeGapTimeSeries__
#define __epanet_rtx__TimeGapTimeSeries__

#include <iostream>
#include "TimeSeriesFilter.h"


namespace RTX {
  //!   A Class to map the input time series into a series whos values are the time gaps between successive points
  
  class TimeGapTimeSeries : public TimeSeriesFilter {
    
  public:
    RTX_SHARED_POINTER(TimeGapTimeSeries);
    TimeGapTimeSeries();
    
  protected:
    virtual PointCollection filterPointsAtTimes(std::set<time_t> times);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
  };
}
#endif /* defined(__epanet_rtx__TimeGapTimeSeries__) */
