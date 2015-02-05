//
//  ValidRangeTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__ValidRangeTimeSeries__
#define __epanet_rtx__ValidRangeTimeSeries__

#include <iostream>
#include "TimeSeriesFilterSinglePoint.h"

namespace RTX {
  class ValidRangeTimeSeries : public TimeSeriesFilterSinglePoint {
  public:
    RTX_SHARED_POINTER(ValidRangeTimeSeries);
    ValidRangeTimeSeries();
    
    void setRange(double min, double max);
    std::pair<double, double> range();
    
    typedef enum {saturate=0,drop=1} filterMode_t;
    filterMode_t mode();
    void setMode(filterMode_t mode);
    
    
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
    
  private:
    std::pair<double, double> _range;
    filterMode_t _mode;
  };
}

#endif /* defined(__epanet_rtx__ValidRangeTimeSeries__) */
