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
#include "Units.h"

namespace RTX {
  
  /*!
   \class StatsTimeSeries
   \brief A modular timeseries class for delivering statistical information.
   
   The StatsTimeSeries class is designed to provide a moving window (in the form of a Clock), evaluated at regular intervals (also as a Clock), over which statistical measures are taken. The full list of statistical options is below.
   
   
   */
  
  /*!
   \fn StatsTimeSeries::StatsTimeSeriesType StatsTimeSeries::statsType()
   \brief Get the statistic type.
   \return The type of statistic that this object is configured to return.
   \sa StatsTimeSeries::setStatsType
   */
  /*!
   \fn void StatsTimeSeries::setStatsType(StatsTimeSeriesType type)
   \brief Set the statistic type.
   \param type The type of statistic to return.
   
   Types of statistics:
   
   - Mean
   - Standard Deviation
   - Median
   - 25th and 75th percentile
   - Interquartile Range
   - Max, Min, Count
   - Variance
   - Root Mean Squared
   
   \sa StatsTimeSeries::statsType
   */
  
  
  
  
  
  
  
  
  class StatsTimeSeries : public BaseStatsTimeSeries {
    
  public:
    typedef enum {
      StatsTimeSeriesMean = 0,
      StatsTimeSeriesStdDev = 1,
      StatsTimeSeriesMedian = 2,
      StatsTimeSeriesQ25 = 3,
      StatsTimeSeriesQ75 = 4,
      StatsTimeSeriesInterQuartileRange = 5,
      StatsTimeSeriesMax = 6,
      StatsTimeSeriesMin = 7,
      StatsTimeSeriesCount = 8,
      StatsTimeSeriesVar = 9,
      StatsTimeSeriesRMS = 10,
    } StatsTimeSeriesType;
    
    RTX_SHARED_POINTER(StatsTimeSeries);
    StatsTimeSeries();
    
    StatsTimeSeriesType statsType();
    void setStatsType(StatsTimeSeriesType type);
    
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual void setUnits(Units newUnits);

  protected:
    virtual bool canAlterDimension() { return true; };
    virtual bool canAlterClock() { return true; };
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);
    
  private:
    StatsTimeSeriesType _statsType;
    double valueFromSummary(TimeSeries::Statistics s);
    Units statsUnits(Units sourceUnits, StatsTimeSeriesType type);

  };
}

#endif /* defined(__epanet_rtx__StatsTimeSeries__) */
