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
#include <boost/foreach.hpp>

#include "TimeSeriesFilter.h"
#include "CurveFunction.h"
#include "Curve.h"

namespace RTX {
  
  //!   A Curve Function Class to perform arbitrary mapping of values.
  /*!
   This time series class allows you to specify points on a curve for value transformation, for instance
   transforming a tank level time series into a volume time series. Generally, can be used for dimensional conversions.
   */
  
  class CurveFunction : public TimeSeriesFilter {
    
  public:
    RTX_BASE_PROPS(CurveFunction);
    CurveFunction();
    
    Curve::_sp curve();
    void setCurve(Curve::_sp curve);
    void clearCurve();
        
  protected:
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    bool willResample();
    PointCollection filterPointsInRange(TimeRange range);
    bool canDropPoints();
    
  private:
    Point convertWithCurve(Point p, Units sourceU);
    Curve::_sp _curve;
    
  };
}



#endif
