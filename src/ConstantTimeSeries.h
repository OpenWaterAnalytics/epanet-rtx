//
//  ConstantTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__ConstantTimeSeries__
#define __epanet_rtx__ConstantTimeSeries__

#include "TimeSeriesSynthetic.h"

namespace RTX {
  
  class ConstantTimeSeries : public TimeSeriesSynthetic {
  public:
    RTX_SHARED_POINTER(ConstantTimeSeries);
    ConstantTimeSeries();
    
    void setValue(double value);
    double value();
    
  protected:
    Point syntheticPoint(time_t time);
    
  private:
    double _value;
  };
  
}




#endif /* defined(__epanet_rtx__ConstantTimeSeries__) */
