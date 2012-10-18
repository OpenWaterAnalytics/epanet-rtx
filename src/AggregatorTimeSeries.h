//
//  AggregatorTimeSeries.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_aggregator_timeseries_h
#define epanet_rtx_aggregator_timeseries_h


#include <vector>
#include "TimeSeries.h"
#include "rtxExceptions.h"

namespace RTX {
  
  /*!
   \class AggregatorTimeSeries
   \brief Aggregates (and optionally scales) arbitrary many input time series.
   
   Use addSource to add an input time series, with optional multiplier (-1 for subtraction).
   
   */
  
  class AggregatorTimeSeries : public TimeSeries {
  
  public:
    RTX_SHARED_POINTER(AggregatorTimeSeries);
    // add a time series to this aggregator. optional parameter "multiplier" allows you to scale
    // the aggregated time series (for instance, by -1 if it needs to be subtracted).
    void addSource(TimeSeries::sharedPointer timeSeries, double multiplier = 1.) throw(RtxException);
    void removeSource(TimeSeries::sharedPointer timeSeries);
    std::vector< std::pair<TimeSeries::sharedPointer,double> > sources();
    
    // reimplement the base class methods
    virtual Point point(time_t time);
    virtual std::vector< Point > points(time_t start, time_t end);

    
  private:
    // need to store several TimeSeries references...
    // _tsList[x].first == TimeSeries, _tsList[x].second == multipier
    std::vector< std::pair<TimeSeries::sharedPointer,double> > _tsList;
    
  };
  
}
#endif
