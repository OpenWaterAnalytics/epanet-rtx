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
#include "ModularTimeSeries.h"

namespace RTX {
  class OffsetTimeSeries : public ModularTimeSeries {
  
  public:
    RTX_SHARED_POINTER(OffsetTimeSeries);
    OffsetTimeSeries();
    virtual Point point(time_t time);
    virtual std::vector<Point> points(time_t start, time_t end);
    void setOffset(double offset);
    double offset();
  private:
    Point convertWithOffset(Point p, Units sourceU);
    double _offset;
  
  };
}
#endif /* defined(__epanet_rtx__OffsetTimeSeries__) */
