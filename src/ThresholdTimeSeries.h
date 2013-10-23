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
#include "SinglePointFilterModularTimeSeries.h"

namespace RTX {
  //!   A Status Class to map the input time series into binary status based on a threshold.
  /*!
   This time series class maps a dimensional input into a dimensionless binary status (0/1),
   by comparing the input to a specified threshold condition. Can be used, for instance, to
   convert the derivative of a pump runtime into a pump on/off status.
   */
  
  class ThresholdTimeSeries : public SinglePointFilterModularTimeSeries {
    
  public:
    RTX_SHARED_POINTER(ThresholdTimeSeries);
    ThresholdTimeSeries();
    
    void setThreshold(double threshold);
    double threshold();
    void setValue(double val);
    double value();
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual void setUnits(Units newUnits);
    typedef enum {thresholdModeNormal, thresholdModeAbsolute} thresholdMode_t;
    thresholdMode_t mode();
    void setMode(thresholdMode_t mode);

    
  private:
    Point filteredSingle(Point p, Units sourceU);
    double _threshold;
    double _fixedValue;
    thresholdMode_t _mode;
    
  };
}
#endif /* defined(__epanet_rtx__ThresholdTimeSeries__) */
