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
    RTX_BASE_PROPS(FirstDerivative);
    FirstDerivative();
    virtual ~FirstDerivative();
    virtual std::ostream& toStream(std::ostream &stream);
    
  protected:
    PointCollection filterPointsInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);

  };
  
}// namespace

#endif
