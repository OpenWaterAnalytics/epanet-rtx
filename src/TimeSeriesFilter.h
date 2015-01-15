//
//  TimeSeriesFilter.h
//  epanet-rtx
//
#ifndef __epanet_rtx__TimeSeriesFilter__
#define __epanet_rtx__TimeSeriesFilter__

#include <stdio.h>

#include "TimeSeries.h"

namespace RTX {
  
  /*!
   \class TimeSeriesFilter
   \brief A modular time series operation. Filter base class provides resampling, unit conversion, and caching.
   
   The base TimeSeriesFilter class doesn't do very much. Derive for added flavor.
   */
  
  /*!
   \fn virtual TimeSeries::TimeRange TimeSeriesFilter::willGetRangeFromSource(TimeSeries::_sp source, TimeSeries::TimeRange range)
   \brief Allow derived class to expand the source search bounds prior to a filtering operation
   \param source The source that will be queried.
   \param range The range that will be requested from the source
   \return The range that should be queried from the source
   
   The intention of this method is to allow derived classes the opportunity to expand the search bounds and ensure that they have enough data to perform the desired filtering operation. For example, a lagging moving average will need to expand the left side of the range by either a number of points, or by a fixed time window depending on particular implementation details.
   
   */
  /*!
   \fn virtual PointCollection TimeSeriesFilter::filterPointsInRange(PointCollection inputCollection, TimeRange outRange)
   \brief Perform a specific filtering operation on a collection of points
   \param inputCollection The points on which to perform the operation. These have been 
   \param end The end of the requested time range.
   \return The requested Points (as a vector)
   
   The base class provides some brute-force logic to retrieve points, by calling Point() repeatedly. For more efficient access, you may wish to override this method.
   
   \sa Point
   */
  
  
  
  class TimeSeriesFilter : public TimeSeries {
  public:
    RTX_SHARED_POINTER(TimeSeriesFilter);
    
    TimeSeries::_sp source();
    void setSource(TimeSeries::_sp ts);
    
    Clock::_sp clock();
    void setClock(Clock::_sp clock);
    
    // methods you must override to provide info to the base class
    virtual TimeRange willGetRangeFromSource(TimeSeries::_sp source, TimeRange range) = 0;
    virtual PointCollection filterPointsInRange(PointCollection inputCollection, TimeRange outRange) = 0;
    
    
  private:
    TimeSeries::_sp _source;
    Clock::_sp _clock;
    
  };
}

#endif /* defined(__epanet_rtx__TimeSeriesFilter__) */
