#ifndef ModelPerformanceController_h
#define ModelPerformanceController_h

#include <stdio.h>
#include <set>

#include "rtxMacros.h"
#include "Model.h"
#include "ModelPerformance.h"
#include "GeoMetrics.h"
#include "TimeSeries.h"

extern const std::map<RTX::ModelPerformance::MetricType, std::string> __metricTypeStrings;
extern const std::map<RTX::ModelPerformance::StatsType, std::string> __statsTypeStrings;
extern const std::map<RTX::Element::element_t, std::string> __elementTypeStrings;

namespace RTX {
  class ModelPerformanceController : public RTX_object  {
  public:
    RTX_BASE_PROPS(ModelPerformanceController);
    ModelPerformanceController(Model::_sp model);
    ModelPerformance::_sp options() { return _perf; };
    
    std::set<ModelPerformance::StatsType> saveStats;
    std::set<ModelPerformance::MetricType> saveMetrics;
    
    std::vector<TimeSeries::_sp> errorsForElement(Element::_sp element);
    
    
  private:
    ModelPerformance::_sp _perf;
    
  };
}


#endif /* ModelPerformanceController_h */
