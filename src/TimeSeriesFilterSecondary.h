//
//  TimeSeriesFilterSecondary.hpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/15/16.
//
//

#ifndef TimeSeriesFilterSecondary_hpp
#define TimeSeriesFilterSecondary_hpp

#include <stdio.h>

#include "TimeSeriesFilter.h"

namespace RTX {
  class TimeSeriesFilterSecondary : public TimeSeriesFilter {
  public:
    RTX_BASE_PROPS(TimeSeriesFilterSecondary);
    TimeSeries::_sp secondary();
    void setSecondary(TimeSeries::_sp secondary);
    virtual bool canSetSecondary(TimeSeries::_sp secondary);
    virtual void didSetSecondary(TimeSeries::_sp secondary);
    TimeSeriesFilterSecondary::_sp secondary(TimeSeries::_sp sec) {this->setSecondary(sec); return share_me(this);};
    
    virtual bool hasUpstreamSeries(TimeSeries::_sp other);
    
  protected:
    TimeSeries::_sp _secondary;
    
  };
}

#endif /* TimeSeriesFilterSecondary_hpp */
