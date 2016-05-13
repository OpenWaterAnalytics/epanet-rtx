#ifndef Curve_hpp
#define Curve_hpp

#include <stdio.h>
#include <map>

#include "rtxMacros.h"
#include "Units.h"
#include "Point.h"
#include "TimeSeries.h"

namespace RTX {
  class Curve : public RTX_object {
  public:
    RTX_BASE_PROPS(Curve);
    std::string name;
    Units inputUnits;
    Units outputUnits;
    std::map<double,double> curveData;
    
    TimeSeries::PointCollection convert(const TimeSeries::PointCollection& p);
    
  };
}


#endif /* Curve_hpp */
