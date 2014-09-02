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
    void setRatioThreshold(double threshold);
    void setTimeThreshold(double threshold);
    void setResetCeiling(double ceiling);
    void setResetFloor(double floor);
    void setResetTolerance(double tolerance);
    void setTimeErrorTolerance(double tolerance);

    // Overridden methods
    virtual Point point(time_t time);
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    virtual void setClock(Clock::sharedPointer clock);

  private:
    Point pointNext(time_t time);
    Point beginningSource(Point lastStatusPoint);
    Point beginningStatus(time_t time);

    double _timeThreshold; // status change tolerance in seconds
    double _ratioThreshold; // status change tolerance expressed as ratio of run to wall times.
    double _resetCeiling; // seconds - resets when hit (0 = non-reset)
    double _resetFloor; // seconds - resets to when hit (0 = non-reset)
    double _resetTolerance; // seconds - detects reset
    double _timeErrorTolerance; // seconds - allowable time error

  };
}

#endif /* defined(__epanet_rtx__RunTimeStatusModularTimeSeries__) */
