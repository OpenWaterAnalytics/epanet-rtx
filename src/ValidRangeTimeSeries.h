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
#include "SinglePointFilterModularTimeSeries.h"

namespace RTX {
  class ValidRangeTimeSeries : public SinglePointFilterModularTimeSeries {
  public:
    RTX_SHARED_POINTER(ValidRangeTimeSeries);
    ValidRangeTimeSeries();
    void setRange(double min, double max);
    std::pair<double, double> range();
    
    typedef enum {saturate,drop} filterMode_t;
    filterMode_t mode();
    void setMode(filterMode_t mode);
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    std::pair<double, double> _range;
    filterMode_t _mode;
  };
}

#endif /* defined(__epanet_rtx__ValidRangeTimeSeries__) */
