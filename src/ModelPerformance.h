//
//  ModelPerformance.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/18/14.
//
//

#ifndef __epanet_rtx__ModelPerformance__
#define __epanet_rtx__ModelPerformance__

#include <stdio.h>

#include "Model.h"
#include "TimeSeries.h"

namespace RTX {
  
  class ModelPerformance {
    
  public:
    
    typedef enum {
      ModelPerformanceStatsRMSE                    = 0,
      ModelPerformanceStatsMeanAbsoluteError       = 1,
      ModelPerformanceStatsCorrelationCoefficient  = 2,
      ModelPerformanceStatsAbsoluteErrorQuantile   = 3
    } StatsType;
    
    typedef enum {
      ModelPerformanceAggregationSum  = 0,
      ModelPerformanceAggregationMean = 1/*,
      ModelPerformanceAggregationMax  = 2*/
    } AggregationType;
    
    typedef enum {
      ModelPerformanceLocationFlow     = 0,
      ModelPerformanceLocationPressure = 1,
      ModelPerformanceLocationHead     = 2,
      ModelPerformanceLocationTank     = 3
    } LocationType;
    
    
    RTX_SHARED_POINTER(ModelPerformance);
    
    ModelPerformance(Model::_sp model,
                     StatsType statsType = ModelPerformanceStatsRMSE,
                     AggregationType aggregationType = ModelPerformanceAggregationMean,
                     LocationType locationType = ModelPerformanceLocationTank);
    
    // specify the model and get the important information
    Model::_sp model();
    TimeSeries::_sp performance();          /*!< read-only timeseries that represents model performance with settings specified */
    std::vector<std::pair<Element::_sp, TimeSeries::_sp> > components(); /*!< constituent error time series. treat this as read-only */
    
    // specify the source data and the way the statistics are computed.
    StatsType statsType();
    void setStatsType(StatsType type);
    
    AggregationType aggregationType();
    void setAggregationType(AggregationType type);
    
    LocationType locationType();
    void setLocationType(LocationType type);
    
    
    // properties for statistic computation
    
    Clock::_sp samplingWindow();            /*!< clock to be used as sampling window by each error time series */
    void setSamplingWindow(Clock::_sp clock);
    
    Clock::_sp errorClock();                /*!< clock for each differencing or correlation time series (error), and the aggregation thereof (or mean) */
    void setErrorClock(Clock::_sp clock);
    
    time_t correlationLag();                          /*!< model time lag (+ or -) relative to measure time */
    void setCorrelationLag(time_t timeLag);
    
    double quantile();                                /*!< Quantile to use for Abs Error Quantile stat */
    void setQuantile(double q);
    
    void buildPerformanceCalculatorWithElements(std::vector<Element::_sp> elements); /*!< rebuild calculation time series workflow with arbitrary elements */
    
  private:
    Model::_sp _model;
    StatsType _statsType;
    AggregationType _aggregationType;
    LocationType _locationType;
    Clock::_sp _samplingWindow, _errorClock;
    std::vector<std::pair<Element::_sp, TimeSeries::_sp> > _errorComponents;
    TimeSeries::_sp _performance;
    time_t _correlationLag;
    double _quantile;
    
    void rebuildPerformanceCalculation();  /*!< rebuild calculation time series workflow */
    std::vector<Element::_sp> elementsWithModelForLocationType(Model::_sp model, LocationType locationType);
    std::pair<TimeSeries::_sp, TimeSeries::_sp> tsPairForElementWithLocationType(Element::_sp e, ModelPerformance::LocationType location);
    TimeSeries::_sp errorForPair(std::pair<TimeSeries::_sp, TimeSeries::_sp> tsPair);

  };
  
}








#endif /* defined(__epanet_rtx__ModelPerformance__) */
