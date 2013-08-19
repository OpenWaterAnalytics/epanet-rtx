//
//  FirstDerivative.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_FirstDerivative_h
#define epanet_rtx_FirstDerivative_h

#include "Resampler.h"

namespace RTX {
  
  class FirstDerivative : public Resampler {
  public:
    RTX_SHARED_POINTER(FirstDerivative);
    FirstDerivative();
    virtual ~FirstDerivative();
    
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual void setUnits(Units newUnits);
    virtual std::ostream& toStream(std::ostream &stream);
    
  protected:
    //virtual std::vector<Point> filteredPoints(time_t fromTime, time_t toTime, const std::vector<Point>& sourcePoints);
    virtual Point filteredSingle(pVec_cIt& vecStart, pVec_cIt& vecEnd, pVec_cIt& vecPos, time_t t, Units fromUnits);

  };
  
}// namespace

#endif
