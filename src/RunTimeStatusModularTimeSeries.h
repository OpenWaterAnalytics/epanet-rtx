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
    typedef enum {before,after} searchDirection_t;
    RTX_SHARED_POINTER(RunTimeStatusModularTimeSeries);
    RunTimeStatusModularTimeSeries();
    void setThreshold(double threshold);
    void setResetCeiling(double ceiling);
    void setResetFloor(double floor);
    void setResetTolerance(double tolerance);

    // Overridden methods
    virtual Point point(time_t time);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    virtual void setClock(Clock::sharedPointer clock);

  private:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    Point pointNext(time_t time, searchDirection_t dir);
    std::pair< Point, Point > beginningStatus(time_t time, searchDirection_t dir, Units sourceU);

    Point _cachedPoint; // Last status point
    Point _cachedSourcePoint; // Last source point, consistent with _cachedPoint
    double _threshold; // Tolerance in seconds
    double _resetCeiling; // seconds - resets when hit (0 = non-reset)
    double _resetFloor; // seconds - resets to when hit (0 = non-reset)
    double _resetTolerance; // seconds - detects reset
    time_t _statusPointShelfLife; // seconds
    int _windowSize; // number of points

  };
}

#endif /* defined(__epanet_rtx__RunTimeStatusModularTimeSeries__) */
