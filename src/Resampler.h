//
//  Resampler.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Resampler_h
#define epanet_rtx_Resampler_h

#include "ModularTimeSeries.h"

namespace RTX {
  
  class Resampler : public ModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(Resampler);
    Resampler();
    virtual ~Resampler();
    
    virtual Point point(time_t time);
    
  protected:
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    
  private:
    
  };
  
}

#endif
