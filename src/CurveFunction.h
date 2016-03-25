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
#include "TimeSeriesFilterSinglePoint.h"
#include <boost/foreach.hpp>

namespace RTX {
  
  //!   A Curve Function Class to perform arbitrary mapping of values.
  /*!
   This time series class allows you to specify points on a curve for value transformation, for instance
   transforming a tank level time series into a volume time series. Generally, can be used for dimensional conversions.
   */
  
  class CurveFunction : public TimeSeriesFilterSinglePoint {
    
  public:
    RTX_BASE_PROPS(CurveFunction);
    CurveFunction();
    
    // added functionality.
    void setInputUnits(Units inputUnits);
    Units inputUnits();
    
    void addCurveCoordinate(double inputValue, double outputValue);
    void setCurve( std::vector<std::pair<double,double> > curve);
    void clearCurve();
    std::vector<std::pair<double,double> > curve();
        
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);
    
  private:
    Point convertWithCurve(Point p, Units sourceU);
    std::vector< std::pair<double,double> > _curve;  // list of points for interpolation (x,y)
    Units _inputUnits;
  };
}



#endif
