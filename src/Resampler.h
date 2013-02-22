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
    virtual std::vector<Point> points(time_t start, time_t end);
    
  protected:
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    
  private:
    Point interpolated(Point p1, Point p2, time_t t);
  };
  
}

#endif
