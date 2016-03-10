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
#include "TimeSeriesFilterSecondary.h"

namespace RTX {
  class MultiplierTimeSeries : public TimeSeriesFilterSecondary {
    
  public:
    
    enum MultiplierMode : unsigned int {
      MultiplierModeMultiply =  0,
      MultiplierModeDivide =  1
    };
    
    RTX_SHARED_POINTER(MultiplierTimeSeries);
    MultiplierTimeSeries();
    
    MultiplierMode multiplierMode();
    void setMultiplierMode(MultiplierMode mode);
    
    time_t timeBefore(time_t t);
    time_t timeAfter(time_t t);
    
  protected:
    void didSetSecondary(TimeSeries::_sp secondary);
    PointCollection filterPointsInRange(TimeRange range);
    std::set<time_t> timeValuesInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    TimeSeries::_sp _multiplierBasis;
    MultiplierMode _mode;
    Units nativeUnits();
  };
}



#endif /* defined(__epanet_rtx__MultiplierTimeSeries__) */
