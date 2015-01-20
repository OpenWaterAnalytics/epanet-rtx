//
//  InversionTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__InversionTimeSeries__
#define __epanet_rtx__InversionTimeSeries__

#include <iostream>
#include "TimeSeriesFilter.h"

namespace RTX {
  class InversionTimeSeries : public TimeSeriesFilter {
    
  public:
    RTX_SHARED_POINTER(InversionTimeSeries);
    
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
    
  };
}



#endif /* defined(__epanet_rtx__InversionTimeSeries__) */
