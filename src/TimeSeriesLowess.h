#ifndef TimeSeriesLowess_h
#define TimeSeriesLowess_h

#include <stdio.h>
#include "Clock.h"
#include "BaseStatsTimeSeries.h"

namespace RTX {
  class TimeSeriesLowess : public BaseStatsTimeSeries {
  public:
    RTX_BASE_PROPS(TimeSeriesLowess);
    TimeSeriesLowess();
    
    void setFraction(double f);
    double fraction();
    
  protected:
    PointCollection filterPointsInRange(TimeRange range);
    
  private:
    double _fraction;
    double valueFromSampleAtTime(PointCollection::pvRange r, time_t t);

  };
}




#endif /* TimeSeriesLowess_h */
