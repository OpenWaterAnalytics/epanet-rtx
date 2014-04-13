//
//  SineTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__SineTimeSeries__
#define __epanet_rtx__SineTimeSeries__

#include "TimeSeries.h"

namespace RTX {
  
  class SineTimeSeries : public TimeSeries {
    
  public:
    RTX_SHARED_POINTER(SineTimeSeries);
    SineTimeSeries(double magnitude = 1., time_t period = 86400);
    virtual Point point(time_t time);
    
    time_t period();
    double magnitude();
    
    void setPeriod(time_t p) {_period = p;};
    void setMagnitude(double m) {_magnitude = m;};
    
  private:
    time_t _period;
    double _magnitude;
  };
  
}


#endif /* defined(__epanet_rtx__SineTimeSeries__) */
