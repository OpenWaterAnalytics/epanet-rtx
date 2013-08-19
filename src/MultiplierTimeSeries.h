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
    
    TimeSeries::sharedPointer multiplier();
    void setMultiplier(TimeSeries::sharedPointer ts);
    
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    TimeSeries::sharedPointer _multiplierBasis;
  };
}



#endif /* defined(__epanet_rtx__MultiplierTimeSeries__) */
