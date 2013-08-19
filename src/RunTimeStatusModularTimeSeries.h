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
    void setResetCeiling(double ceiling);
    void setResetFloor(double floor);
    void setResetTolerance(double tolerance);

    // Overridden methods
    virtual Point point(time_t time);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    
  private:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    double _threshold; // Tolerance in seconds
    Point _cachedPoint; // Last status point
    Point _cachedSourcePoint; // Last source point, consistent with _cachedPoint
    double _resetCeiling = 0.0; // seconds - resets when hit (0 = non-reset)
    double _resetFloor = 0.0; // seconds - resets to when hit (0 = non-reset)
    double _resetTolerance = 0.0; // seconds - detects reset

  };
}

#endif /* defined(__epanet_rtx__RunTimeStatusModularTimeSeries__) */
