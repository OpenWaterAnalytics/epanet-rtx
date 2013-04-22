//
//  ConstantTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__ConstantTimeSeries__
#define __epanet_rtx__ConstantTimeSeries__

#include "TimeSeries.h"

namespace RTX {
  
  class ConstantTimeSeries : public TimeSeries {
  public:
    RTX_SHARED_POINTER(ConstantTimeSeries);
    ConstantTimeSeries();
    virtual Point point(time_t time);
  private:
    double _value;
  };
  
}




#endif /* defined(__epanet_rtx__ConstantTimeSeries__) */
