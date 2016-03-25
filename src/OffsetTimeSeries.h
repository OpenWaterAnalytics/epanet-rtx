//
//  OffsetTimeSeries.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__OffsetTimeSeries__
#define __epanet_rtx__OffsetTimeSeries__

#include <iostream>
#include "TimeSeriesFilterSinglePoint.h"

namespace RTX {
  class OffsetTimeSeries : public TimeSeriesFilterSinglePoint {
  
  public:
    RTX_BASE_PROPS(OffsetTimeSeries);
    OffsetTimeSeries();
    void setOffset(double offset);
    double offset();
  protected:
    Point filteredWithSourcePoint(Point sourcePoint);
  private:
    double _offset;
  };
}
#endif /* defined(__epanet_rtx__OffsetTimeSeries__) */
