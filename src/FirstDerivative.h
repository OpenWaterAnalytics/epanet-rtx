//
//  FirstDerivative.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_FirstDerivative_h
#define epanet_rtx_FirstDerivative_h

#include "TimeSeriesFilter.h"

namespace RTX {
  
  class FirstDerivative : public TimeSeriesFilter {
  public:
    RTX_SHARED_POINTER(FirstDerivative);
    FirstDerivative();
    virtual ~FirstDerivative();
    virtual std::ostream& toStream(std::ostream &stream);
    
  protected:
    virtual PointCollection filterPointsAtTimes(std::set<time_t> times);
    virtual bool canSetSource(TimeSeries::_sp ts);
    virtual void didSetSource(TimeSeries::_sp ts);
    virtual bool canChangeToUnits(Units units);

  };
  
}// namespace

#endif
