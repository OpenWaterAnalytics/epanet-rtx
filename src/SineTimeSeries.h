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
    SineTimeSeries(time_t period = 86400);
    virtual Point point(time_t time);
  private:
    time_t _period;
  };
  
}


#endif /* defined(__epanet_rtx__SineTimeSeries__) */
