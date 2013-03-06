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
    SineTimeSeries();
    virtual Point point(time_t time);
    
  };
  
}


#endif /* defined(__epanet_rtx__SineTimeSeries__) */
