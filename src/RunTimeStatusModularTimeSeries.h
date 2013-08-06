//
//  RunTimeStatusModularTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__RunTimeStatusModularTimeSeries__
#define __epanet_rtx__RunTimeStatusModularTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

#define RTX_RTSTS_SUPER ModularTimeSeries

namespace RTX {
  class RunTimeStatusModularTimeSeries : public RTX_RTSTS_SUPER {
  public:
    RTX_SHARED_POINTER(RunTimeStatusModularTimeSeries);
    void setThreshold(double threshold);

    // Overridden methods
    virtual Point point(time_t time);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    
  private:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    double _threshold;
    Point _cachedPoint;
    double _cachedRuntime;

  };
}

#endif /* defined(__epanet_rtx__RunTimeStatusModularTimeSeries__) */
