//
//  OffsetTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 10/18/12.
//
//

#ifndef __epanet_rtx__OffsetTimeSeries__
#define __epanet_rtx__OffsetTimeSeries__

#include <iostream>
#include "SinglePointFilterModularTimeSeries.h"

namespace RTX {
  class OffsetTimeSeries : public SinglePointFilterModularTimeSeries {
  
  public:
    RTX_SHARED_POINTER(OffsetTimeSeries);
    OffsetTimeSeries();
    void setOffset(double offset);
    double offset();
  protected:
    Point filteredSingle(Point p, Units sourceU);
  private:
    double _offset;
  };
}
#endif /* defined(__epanet_rtx__OffsetTimeSeries__) */
