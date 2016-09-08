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
#include "TimeSeriesFilter.h"

namespace RTX {
  class ValidRangeTimeSeries : public TimeSeriesFilter {
  public:
    RTX_BASE_PROPS(ValidRangeTimeSeries);
    ValidRangeTimeSeries();
    
    void setRange(double min, double max);
    std::pair<double, double> range();
    
    enum filterMode_t : int {saturate=0,drop=1};
    filterMode_t mode();
    void setMode(filterMode_t mode);
    
    // chainable
    ValidRangeTimeSeries::_sp range(double min, double max) {this->setRange(min,max); return share_me(this);};
    ValidRangeTimeSeries::_sp mode(filterMode_t m) {this->setMode(m); return share_me(this);};
    
  protected:
    virtual bool willResample(); // we are special !
    PointCollection filterPointsInRange(TimeRange range);
    bool canDropPoints();
    
  private:
    std::pair<double, double> _range;
    filterMode_t _mode;
  };
}

#endif /* defined(__epanet_rtx__ValidRangeTimeSeries__) */
