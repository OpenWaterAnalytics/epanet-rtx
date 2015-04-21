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
      ModelPerformanceStatsModel                   = 0,
      ModelPerformanceStatsMeasure                 = 1,
      ModelPerformanceStatsError                   = 2,
      ModelPerformanceStatsMeanError               = 3,
      ModelPerformanceStatsQuantileError           = 4,
      ModelPerformanceStatsIntegratedError         = 5,
      ModelPerformanceStatsAbsError                = 6,
      ModelPerformanceStatsMeanAbsError            = 7,
      ModelPerformanceStatsQuantileAbsError        = 8,
      ModelPerformanceStatsIntegratedAbsError      = 9,
      ModelPerformanceStatsRMSE                    = 10,
      ModelPerformanceStatsMaxCorrelation          = 11,
      ModelPerformanceStatsCorrelationLag          = 12
    } StatsType;
    
    typedef enum {
      ModelPerformanceAggregationSum  = 0,
      ModelPerformanceAggregationMean = 1,
      ModelPerformanceAggregationMax  = 2,
      ModelPerformanceAggregationMin  = 3
    } AggregationType;
    
    typedef enum {
      ModelPerformanceMetricFlow          = 0,
      ModelPerformanceMetricPressure      = 1,
      ModelPerformanceMetricHead          = 2,
      ModelPerformanceMetricLevel         = 3,
      ModelPerformanceMetricVolume        = 4,
      ModelPerformanceMetricConcentration = 5
    } MetricType;
    
    
    // static class method for constructing error series from an element
    static std::map<StatsType,TimeSeries::_sp> errorsForElementAndMetricType(Element::_sp element, MetricType type, Clock::_sp samplingWindow, double quantile);
    
    RTX_SHARED_POINTER(ModelPerformance);
    
    ModelPerformance(Model::_sp model,
                     StatsType statsType = ModelPerformanceStatsRMSE,
                     AggregationType aggregationType = ModelPerformanceAggregationMean,
                     MetricType mType = ModelPerformanceMetricLevel);
    
    // specify the model and get the important information
    Model::_sp model();
    TimeSeries::_sp performance();          /*!< read-only timeseries that represents model performance with settings specified */
    std::vector<std::pair<Element::_sp, TimeSeries::_sp> > components(); /*!< constituent error time series. treat this as read-only */
    
    // specify the source data and the way the statistics are computed.
    StatsType statsType();
    void setStatsType(StatsType type);
    
    AggregationType aggregationType();
    void setAggregationType(AggregationType type);
    
    MetricType locationType();
    void setLocationType(MetricType type);
    
    
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
    MetricType _locationType;
    Clock::_sp _samplingWindow, _errorClock;
    std::vector<std::pair<Element::_sp, TimeSeries::_sp> > _errorComponents;
    TimeSeries::_sp _performance;
    time_t _correlationLag;
    double _quantile;
    
    void rebuildPerformanceCalculation();  /*!< rebuild calculation time series workflow */
    std::vector<Element::_sp> elementsWithModelForLocationType(Model::_sp model, MetricType locationType);
    static std::pair<TimeSeries::_sp, TimeSeries::_sp> tsPairForElementWithMetricType(Element::_sp e, ModelPerformance::MetricType location);
    TimeSeries::_sp errorForPair(std::pair<TimeSeries::_sp, TimeSeries::_sp> tsPair);

  };
  
}








#endif /* defined(__epanet_rtx__ModelPerformance__) */
