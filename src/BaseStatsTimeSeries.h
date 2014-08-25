//
//  BaseStatsTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/17/14.
//
//

#ifndef __epanet_rtx__BaseStatsTimeSeries__
#define __epanet_rtx__BaseStatsTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

namespace RTX {
    
  class BaseStatsTimeSeries : public ModularTimeSeries {
    
  public:
    
    typedef enum {
      StatsSamplingModeLeading = 0,
      StatsSamplingModeLagging = 1,
      StatsSamplingModeCentered = 2
    } StatsSamplingMode_t;
    
    
    typedef std::pair< Point, Summary > pointSummaryPair_t;
    
    RTX_SHARED_POINTER(BaseStatsTimeSeries);
    BaseStatsTimeSeries();
    
    void setWindow(Clock::sharedPointer window);
    Clock::sharedPointer window();
    
    void setSamplingMode(StatsSamplingMode_t mode);
    StatsSamplingMode_t samplingMode();
    
    bool summaryOnly();
    void setSummaryOnly(bool summaryOnly);
    
    // overrides
//    void setClock(Clock::sharedPointer clock);
    
  protected:
    virtual std::vector< pointSummaryPair_t > filteredSummaryPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    Clock::sharedPointer _window;
    bool _summaryOnly;
    StatsSamplingMode_t _samplingMode;
  };
}



#endif /* defined(__epanet_rtx__QuartileTimeSeries__) */
