//
//  StatusTimeSeries.h
//  epanet-rtx
//
//  Created by Jim Uber on 4/9/2013 based on OffsetTimeSeries Class.
//
//

#ifndef __epanet_rtx__StatusTimeSeries__
#define __epanet_rtx__StatusTimeSeries__

#include <iostream>
#include "ModularTimeSeries.h"

namespace RTX {
  //!   A Status Class to map the input time series into binary status based on a threshold.
  /*!
   This time series class maps a dimensional input into a dimensionless binary status (0/1),
   by comparing the input to a specified threshold condition. Can be used, for instance, to
   convert the derivative of a pump runtime into a pump on/off status.
   */
  
  class StatusTimeSeries : public ModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(StatusTimeSeries);
    StatusTimeSeries();
    virtual Point point(time_t time);
    void setThreshold(double threshold);
    double threshold();
  protected:
    virtual std::vector<Point> filteredPoints(time_t fromTime, time_t toTime, const std::vector<Point>& sourcePoints);
    // overridden methods from parent class
    void setUnits(Units newUnits);
    
  private:
    Point convertWithThreshold(Point p);
    double _threshold;
    
  };
}
#endif /* defined(__epanet_rtx__StatusTimeSeries__) */
