//
//  QuartileTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/17/14.
//
//

#ifndef __epanet_rtx__QuartileTimeSeries__
#define __epanet_rtx__QuartileTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

namespace RTX {
    
  class QuartileTimeSeries : public ModularTimeSeries {
    
  public:
    
    class QuartilePoint {
    public:
      QuartilePoint(time_t t, double q_25, double q_50, double q_75) : time(t),q25(q_25),q50(q_50),q75(q_75) {}
      time_t time;
      double q25, q50, q75;
    };
    
    typedef enum {
      quartile_25 = 1 << 0,
      quartile_50 = 1 << 1,
      quartile_75 = 1 << 2
    } quartile_t;
    
    RTX_SHARED_POINTER(QuartileTimeSeries);
    QuartileTimeSeries();
    
    void setWindow(Clock::sharedPointer window);
    Clock::sharedPointer window();
    
    // overrides
    void setClock(Clock::sharedPointer clock);
    
  protected:
    virtual std::vector<QuartilePoint> filteredQuartilePoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    quartile_t useQuartiles;
    
  private:
    Clock::sharedPointer _window;
  };
}



#endif /* defined(__epanet_rtx__QuartileTimeSeries__) */
