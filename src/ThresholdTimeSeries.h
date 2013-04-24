//
//  ThresholdTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__ThresholdTimeSeries__
#define __epanet_rtx__ThresholdTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

namespace RTX {
  //!   A Status Class to map the input time series into binary status based on a threshold.
  /*!
   This time series class maps a dimensional input into a dimensionless binary status (0/1),
   by comparing the input to a specified threshold condition. Can be used, for instance, to
   convert the derivative of a pump runtime into a pump on/off status.
   */
  
  class ThresholdTimeSeries : public ModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(ThresholdTimeSeries);
    ThresholdTimeSeries();
    virtual Point point(time_t time);
    void setThreshold(double threshold);
    double threshold();
    void setValue(double val);
    double value();
    
  protected:
    virtual std::vector<Point> filteredPoints(time_t fromTime, time_t toTime, const std::vector<Point>& sourcePoints);
        
  private:
    Point convertWithThreshold(Point p);
    double _threshold;
    double _fixedValue;
    
  };
}
#endif /* defined(__epanet_rtx__ThresholdTimeSeries__) */
