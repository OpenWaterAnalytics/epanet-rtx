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
#include "SinglePointFilterModularTimeSeries.h"

namespace RTX {
  class MultiplierTimeSeries : public SinglePointFilterModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(MultiplierTimeSeries);
    MultiplierTimeSeries();
    
    TimeSeries::_sp multiplier();
    void setMultiplier(TimeSeries::_sp ts);
    
    void setSource(TimeSeries::_sp ts);
    bool isCompatibleWith(TimeSeries::_sp ts);
    
    virtual void setUnits(Units u);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::_sp sourceTs, time_t fromTime, time_t toTime);
    void checkUnits();
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    TimeSeries::_sp _multiplierBasis;
  };
}



#endif /* defined(__epanet_rtx__MultiplierTimeSeries__) */
