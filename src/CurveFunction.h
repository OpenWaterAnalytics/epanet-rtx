//
//  CurveFunctionTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_CurveFunctionTimeSeries_h
#define epanet_rtx_CurveFunctionTimeSeries_h

#include <vector>
#include "ModularTimeSeries.h"

namespace RTX {
  
  //!   A Curve Function Class to perform arbitrary mapping of values.
  /*!
   This time series class allows you to specify points on a curve for value transformation, for instance
   transforming a tank level time series into a volume time series. Generally, can be used for dimensional conversions.
   */
  
  class CurveFunction : public ModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(CurveFunction);
    CurveFunction();
    // overridden methods from parent class
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual Point point(time_t time);
    virtual void setUnits(Units newUnits);
    
    // added functionality.
    void setInputUnits(Units inputUnits);
    void addCurveCoordinate(double inputValue, double outputValue);
    
  private:
    std::vector< std::pair<double,double> > _curve;  // list of points for interpolation (x,y)
    Units _inputUnits;
  };
}



#endif
