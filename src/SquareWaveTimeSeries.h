//
//  SquareWaveTimeSeries.h
//  epanet-rtx
//

#ifndef __epanet_rtx__SquareWaveTimeSeries__
#define __epanet_rtx__SquareWaveTimeSeries__

#include "TimeSeriesSynthetic.h"

namespace RTX {
  
  class SquareWaveTimeSeries : public TimeSeriesSynthetic {
    
  public:
    RTX_BASE_PROPS(SquareWaveTimeSeries);
    SquareWaveTimeSeries();
    
    Clock::_sp period();
    time_t duration();
    
    void setPeriod(Clock::_sp period);
    void setDuration(time_t duration);
    
  protected:
    Point syntheticPoint(time_t time);
    
  private:
    time_t _duration;
    Clock::_sp _period;
  };
  
}


#endif /* defined(__epanet_rtx__SquareWaveTimeSeries__) */
