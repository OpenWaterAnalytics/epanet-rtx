//
//  MultiplierTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__MultiplierTimeSeries__
#define __epanet_rtx__MultiplierTimeSeries__

#include <iostream>
#include "TimeSeriesFilterSinglePoint.h"

namespace RTX {
  class MultiplierTimeSeries : public TimeSeriesFilterSinglePoint {
    
  public:
    RTX_SHARED_POINTER(MultiplierTimeSeries);
    MultiplierTimeSeries();
    
    TimeSeries::_sp multiplier();
    void setMultiplier(TimeSeries::_sp ts);
        
    
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    TimeSeries::_sp _multiplierBasis;
  };
}



#endif /* defined(__epanet_rtx__MultiplierTimeSeries__) */
