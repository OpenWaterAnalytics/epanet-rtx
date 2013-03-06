//
//  FirstDerivative.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_FirstDerivative_h
#define epanet_rtx_FirstDerivative_h

#include "ModularTimeSeries.h"

namespace RTX {
  
  class FirstDerivative : public ModularTimeSeries {
  public:
    RTX_SHARED_POINTER(FirstDerivative);
    FirstDerivative();
    virtual ~FirstDerivative();
    
    virtual Point point(time_t time);
    virtual std::vector<Point> points(time_t start, time_t end);
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual void setUnits(Units newUnits);
    virtual std::ostream& toStream(std::ostream &stream);
  private:
    Point deriv(Point p1, Point p2, time_t t);
  };
  
}// namespace

#endif
