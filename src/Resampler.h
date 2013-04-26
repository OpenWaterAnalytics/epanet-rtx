//
//  Resampler.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_Resampler_h
#define epanet_rtx_Resampler_h

#include <boost/circular_buffer.hpp>
#include "ModularTimeSeries.h"

namespace RTX {
  
  class Resampler : public ModularTimeSeries {
    
  public:
    typedef boost::circular_buffer<Point> pointBuffer_t;
    RTX_SHARED_POINTER(Resampler);
    Resampler();
    virtual ~Resampler();
    
    virtual Point point(time_t time);
    
  protected:
    virtual bool isCompatibleWith(TimeSeries::sharedPointer withTimeSeries);
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    virtual Point filteredSingle(const pointBuffer_t& window, time_t t, Units fromUnits);
  };
  
}

#endif
