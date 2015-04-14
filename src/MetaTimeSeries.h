//
//  MetaTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 4/14/15.
//
//

#ifndef __epanet_rtx__MetaTimeSeries__
#define __epanet_rtx__MetaTimeSeries__

#include <stdio.h>

#include <iostream>
#include "TimeSeriesFilter.h"


namespace RTX {
  //!   A Class to map the input time series into a series whos values are the time gaps between successive points
  
  class MetaTimeSeries : public TimeSeriesFilter {
    
  public:
    RTX_SHARED_POINTER(MetaTimeSeries);
    MetaTimeSeries();
    
    enum MetaMode : unsigned int {
      MetaModeGap        =  0,
      MetaModeConfidence =  1,
      MetaModeQuality    =  2
    };
    
    
    MetaMode metaMode();
    void setMetaMode(MetaMode mode);
    
  protected:
    PointCollection filterPointsInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    MetaMode _metaMode;
  };
}

#endif /* defined(__epanet_rtx__MetaTimeSeries__) */
