//
//  RuntimeStatus.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef epanet_rtx_RuntimeStatus_h
#define epanet_rtx_RuntimeStatus_h

#include "Resampler.h"

namespace RTX {
  
  class RuntimeStatus : public Resampler {
  public:
    RTX_SHARED_POINTER(RuntimeStatus);
    RuntimeStatus();
    virtual ~RuntimeStatus();
    
    void setThreshold(double threshold);
    double threshold();
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual void setUnits(Units newUnits);
    virtual std::ostream& toStream(std::ostream &stream);
    
  protected:
    virtual Point filteredSingle(pVec_cIt& vecStart, pVec_cIt& vecEnd, pVec_cIt& vecPos, time_t t, Units fromUnits);
    virtual int margin();
    double _threshold;
    double _onValue;
    double _offValue;
    
  };
  
}// namespace

#endif
