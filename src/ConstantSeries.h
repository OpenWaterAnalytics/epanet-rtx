//
//  ConstantSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_ConstantSeries_h
#define epanet_rtx_ConstantSeries_h

#include "TimeSeries.h"

namespace RTX {
  
  class ConstantSeries : public TimeSeries {
  public:
    RTX_SHARED_POINTER(ConstantSeries);
    ConstantSeries(double constantValue = 0);
    virtual ~ConstantSeries();
    
    virtual Point point(time_t time);
    void setValue(double value);
    
  private:
    double _value;
  };
  
}


#endif
