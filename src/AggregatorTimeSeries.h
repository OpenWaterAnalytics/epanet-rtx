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
#include <boost/foreach.hpp>

#include "TimeSeriesFilter.h"
#include "rtxExceptions.h"


namespace RTX {
  
  /*!
   \class AggregatorTimeSeries
   \brief Aggregates (and optionally scales) arbitrary many input time series.
   
   Use addSource to add an input time series, with optional multiplier (-1 for subtraction).
   
   */
  
  class AggregatorTimeSeries : public TimeSeriesFilter {
  
  public:
    RTX_SHARED_POINTER(AggregatorTimeSeries);
    
    typedef struct {
      TimeSeries::_sp timeseries;
      double multiplier;
    } AggregatorSource;
    
    enum AggregatorMode : unsigned int {
      AggregatorModeSum =  0,
      AggregatorModeMin =  1,
      AggregatorModeMax =  2,
      AggregatorModeMean = 3
    };
    
    virtual std::ostream& toStream(std::ostream &stream);
    
    TimeSeries::_sp source();
    void setSource(TimeSeries::_sp ts);
    
    // add a time series to this aggregator. optional parameter "multiplier" allows you to scale
    // the aggregated time series (for instance, by -1 if it needs to be subtracted).
    void addSource(TimeSeries::_sp timeSeries, double multiplier = 1.) throw(RtxException);
    void removeSource(TimeSeries::_sp timeSeries);
    std::vector< AggregatorSource > sources();
    void setMultiplierForSource(TimeSeries::_sp timeSeries, double multiplier);
    
    void setAggregatorMode(AggregatorMode mode);
    AggregatorMode aggregatorMode();
    
    // must reimplement these searching methods
    virtual Point pointBefore(time_t time);
    virtual Point pointAfter(time_t time);
    
  protected:
    PointCollection filterPointsInRange(TimeRange range);
    std::set<time_t> timeValuesInRange(TimeRange range);
    bool canSetSource(TimeSeries::_sp ts);
    void didSetSource(TimeSeries::_sp ts);
    bool canChangeToUnits(Units units);

  private:
    // need to store several TimeSeries references...
    // _tsList[x].first == TimeSeries, _tsList[x].second == multipier
    std::vector< AggregatorSource > _tsList;
    AggregatorMode _mode;
    
  };
  
}
#endif
