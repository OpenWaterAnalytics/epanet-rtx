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
#include "Resampler.h"
#include "rtxExceptions.h"
#include <boost/foreach.hpp>

namespace RTX {
  
  /*!
   \class AggregatorTimeSeries
   \brief Aggregates (and optionally scales) arbitrary many input time series.
   
   Use addSource to add an input time series, with optional multiplier (-1 for subtraction).
   
   */
  
  class AggregatorTimeSeries : public Resampler {
  
  public:
    RTX_SHARED_POINTER(AggregatorTimeSeries);
    TimeSeries::sharedPointer source();
    virtual void setSource(TimeSeries::sharedPointer source);
    virtual bool doesHaveSource();
    virtual std::ostream& toStream(std::ostream &stream);
    
    // add a time series to this aggregator. optional parameter "multiplier" allows you to scale
    // the aggregated time series (for instance, by -1 if it needs to be subtracted).
    void addSource(TimeSeries::sharedPointer timeSeries, double multiplier = 1.) throw(RtxException);
    void removeSource(TimeSeries::sharedPointer timeSeries);
    std::vector< std::pair<TimeSeries::sharedPointer,double> > sources();
    void setMultiplierForSource(TimeSeries::sharedPointer timeSeries, double multiplier);
    
    // reimplement the base class methods
    virtual Point point(time_t time);
    // points is handled by ModularTimeSeries, which calls filteredPoints (see below)
    //virtual std::vector< Point > points(time_t start, time_t end);

  protected:
    virtual std::vector<Point> filteredPoints(TimeSeries::sharedPointer sourceTs, time_t fromTime, time_t toTime);

  private:
    // need to store several TimeSeries references...
    // _tsList[x].first == TimeSeries, _tsList[x].second == multipier
    std::vector< std::pair<TimeSeries::sharedPointer,double> > _tsList;
    
  };
  
}
#endif
