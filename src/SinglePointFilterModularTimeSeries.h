//
//  SinglePointFilterModularTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__SinglePointFilterModularTimeSeries__
#define __epanet_rtx__SinglePointFilterModularTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

namespace RTX {
  class SinglePointFilterModularTimeSeries : public ModularTimeSeries {
  public:
    RTX_SHARED_POINTER(SinglePointFilterModularTimeSeries);
    virtual Point point(time_t time);
  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    virtual Point filteredSingle(Point p, Units sourceU); // override for extended functionality
  };
}


#endif /* defined(__epanet_rtx__SinglePointFilterModularTimeSeries__) */
