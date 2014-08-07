//
//  StatsTimeSeries.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 7/18/14.
//
//

#ifndef __epanet_rtx__StatsTimeSeries__
#define __epanet_rtx__StatsTimeSeries__

#include <iostream>
#include "BaseStatsTimeSeries.h"

namespace RTX {
  class StatsTimeSeries : public BaseStatsTimeSeries {
    
  public:
    typedef enum {
      StatsTimeSeriesMean,
      StatsTimeSeriesMedian,
      StatsTimeSeriesStdDev,
      StatsTimeSeriesQ25,
      StatsTimeSeriesQ75,
      StatsTimeSeriesInterQuartileRange,
      StatsTimeSeriesMax,
      StatsTimeSeriesMin,
      StatsTimeSeriesCount
    } StatsTimeSeriesType;
    
    RTX_SHARED_POINTER(StatsTimeSeries);
    StatsTimeSeries();
    
    StatsTimeSeriesType statsType();
    void setStatsType(StatsTimeSeriesType type);
    
  protected:
    virtual bool canAlterDimension() { return true; };
    virtual bool canAlterClock() { return true; };
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    StatsTimeSeriesType _statsType;
    double valueFromSummary(TimeSeries::Summary s);
    
  };
}

#endif /* defined(__epanet_rtx__StatsTimeSeries__) */
