//
//  SineTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__SineTimeSeries__
#define __epanet_rtx__SineTimeSeries__

#include "TimeSeriesSynthetic.h"

namespace RTX {
  
  class SineTimeSeries : public TimeSeriesSynthetic {
    
  public:
    RTX_BASE_PROPS(SineTimeSeries);
    SineTimeSeries(double magnitude = 1., time_t period = 86400);
    
    time_t period();
    double magnitude();
    
    void setPeriod(time_t p) {_period = p;};
    void setMagnitude(double m) {_magnitude = m;};
    
  protected:
    Point syntheticPoint(time_t time);
    
  private:
    time_t _period;
    double _magnitude;
  };
  
}


#endif /* defined(__epanet_rtx__SineTimeSeries__) */
