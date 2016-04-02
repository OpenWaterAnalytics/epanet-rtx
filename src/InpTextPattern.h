//
//  InpTextPattern.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 2/27/15.
//
//

#ifndef __epanet_rtx__InpTextPattern__
#define __epanet_rtx__InpTextPattern__

#include <stdio.h>
#include <string.h>

#include "TimeSeries.h"

namespace RTX {
  class InpTextPattern {
  public:
    typedef enum {
      InpControlTypeSetting, // real number
      InpControlTypeStatus   // OPEN / CLOSED
    } controlType;
    
    static std::string textPatternWithTimeSeries(TimeSeries::_sp ts, const std::string& patternName, time_t from, time_t to, int step, TimeSeries::TimeSeriesResampleMode interp);
    static std::string textControlWithTimeSeries(TimeSeries::_sp ts, const std::string& linkName, time_t from, time_t to, controlType type);
  };
}



#endif /* defined(__epanet_rtx__InpTextPattern__) */
