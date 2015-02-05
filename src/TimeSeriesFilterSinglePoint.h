//
//  TimeSeriesFilterSinglePoint.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/16/15.
//
//

#ifndef __epanet_rtx__TimeSeriesFilterSinglePoint__
#define __epanet_rtx__TimeSeriesFilterSinglePoint__

#include <stdio.h>
#include "TimeSeriesFilter.h"

namespace RTX {
  class TimeSeriesFilterSinglePoint : public TimeSeriesFilter {
  protected:
    PointCollection filterPointsInRange(TimeRange range); // non-virtual
    virtual Point filteredWithSourcePoint(Point sourcePoint) = 0; // pure virtual. override must convert units.
  };
}

#endif /* defined(__epanet_rtx__TimeSeriesFilterSinglePoint__) */
