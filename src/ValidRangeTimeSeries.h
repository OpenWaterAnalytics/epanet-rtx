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

#define RTX_VRTS_SUPER SinglePointFilterModularTimeSeries

namespace RTX {
  class ValidRangeTimeSeries : public RTX_VRTS_SUPER {
  public:
    RTX_SHARED_POINTER(ValidRangeTimeSeries);
    ValidRangeTimeSeries();
    void setRange(double min, double max);
    std::pair<double, double> range();
    
    virtual void setClock(Clock::_sp clock);
    
    typedef enum {saturate=0,drop=1} filterMode_t;
    filterMode_t mode();
    void setMode(filterMode_t mode);
    
    // Overridden methods
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    
  private:
    Point filteredSingle(Point p, Units sourceU);
    std::pair<double, double> _range;
    filterMode_t _mode;
  };
}

#endif /* defined(__epanet_rtx__ValidRangeTimeSeries__) */
