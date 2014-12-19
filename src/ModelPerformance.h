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
      ModelPerformanceStatsCorrelationCoefficient  = 2
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
    
    ModelPerformance(Model::sharedPointer model,
                     StatsType statsType = ModelPerformanceStatsRMSE,
                     AggregationType aggregationType = ModelPerformanceAggregationMean,
                     LocationType locationType = ModelPerformanceLocationTank);
    
    // specify the model and get the important information
    Model::sharedPointer model();
    TimeSeries::sharedPointer performance();          /*!< read-only timeseries that represents model performance with settings specified */
    std::vector<std::pair<Element::sharedPointer, TimeSeries::sharedPointer> > components(); /*!< constituent error time series. treat this as read-only */
    
    // specify the source data and the way the statistics are computed.
    StatsType statsType();
    void setStatsType(StatsType type);
    
    AggregationType aggregationType();
    void setAggregationType(AggregationType type);
    
    LocationType locationType();
    void setLocationType(LocationType type);
    
    
    // properties for statistic computation
    
    Clock::sharedPointer samplingWindow();            /*!< clock to be used as sampling window by each error time series */
    void setSamplingWindow(Clock::sharedPointer clock);
    
    Clock::sharedPointer errorClock();                /*!< clock for each differencing or correlation time series (error) */
    void setErrorClock(Clock::sharedPointer clock);
    
    Clock::sharedPointer aggregationClock();          /*!< clock for aggregating all the error time series */
    void setAggregationClock(Clock::sharedPointer clock);
    
    
    
  private:
    Model::sharedPointer _model;
    StatsType _statsType;
    AggregationType _aggregationType;
    LocationType _locationType;
    Clock::sharedPointer _samplingWindow, _errorClock, _aggregationClock;
    std::vector<std::pair<Element::sharedPointer, TimeSeries::sharedPointer> > _errorComponents;
    TimeSeries::sharedPointer _performance;
    
    void rebuildPerformanceCalculation();
    void buildPerformanceCalculatorWithElements(std::vector<Element::sharedPointer> elements);
    std::vector<Element::sharedPointer> elementsWithModelForLocationType(Model::sharedPointer model, LocationType locationType);
    std::pair<TimeSeries::sharedPointer, TimeSeries::sharedPointer> tsPairForElementWithLocationType(Element::sharedPointer e, ModelPerformance::LocationType location);
    TimeSeries::sharedPointer errorForPair(std::pair<TimeSeries::sharedPointer, TimeSeries::sharedPointer> tsPair);

  };
  
}








#endif /* defined(__epanet_rtx__ModelPerformance__) */
