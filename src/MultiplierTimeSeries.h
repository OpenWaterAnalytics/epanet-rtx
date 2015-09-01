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
    
    enum MultiplierMode : unsigned int {
      MultiplierModeMultiply =  0,
      MultiplierModeDivide =  1
    };
    
    RTX_SHARED_POINTER(MultiplierTimeSeries);
    MultiplierTimeSeries();
    
    TimeSeries::_sp multiplier();
    void setMultiplier(TimeSeries::_sp ts);
    
    MultiplierMode multiplierMode();
    void setMultiplierMode(MultiplierMode mode);
    
    
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    TimeSeries::_sp _multiplierBasis;
    MultiplierMode _mode;
    Units nativeUnits();
  };
}



#endif /* defined(__epanet_rtx__MultiplierTimeSeries__) */
