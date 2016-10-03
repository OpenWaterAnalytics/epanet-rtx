#include "ModelPerformanceController.h"

using namespace std;
using namespace RTX;
using MP = RTX::ModelPerformance;

const map<ModelPerformance::MetricType, string> __metricTypeStrings({
  {MP::ModelPerformanceMetricFlow,          "flow"},
  {MP::ModelPerformanceMetricPressure,      "pressure"},
  {MP::ModelPerformanceMetricHead,          "head"},
  {MP::ModelPerformanceMetricLevel,         "level"},
  {MP::ModelPerformanceMetricVolume,        "volume"},
  {MP::ModelPerformanceMetricConcentration, "concentration"},
  {MP::ModelPerformanceMetricEnergy,        "energy"},
  {MP::ModelPerformanceMetricSetting,       "setting"},
  {MP::ModelPerformanceMetricStatus,        "status"},
  {MP::ModelPerformanceMetricDemand,        "demand"}
});

const map<MP::StatsType, string> __statsTypeStrings({
  {MP::ModelPerformanceStatsModel,            "model"},
  {MP::ModelPerformanceStatsMeasure,          "measure"},
  {MP::ModelPerformanceStatsError,            "error"},
  {MP::ModelPerformanceStatsMeanError,        "mean_error"},
  {MP::ModelPerformanceStatsQuantileError,    "quantile_error"},
  {MP::ModelPerformanceStatsIntegratedError,  "integrated_error"},
  {MP::ModelPerformanceStatsIntegratedAbsError,"integrated_abs_error"},
  {MP::ModelPerformanceStatsAbsError,         "abs_error"},
  {MP::ModelPerformanceStatsMeanAbsError,     "mean_abs_error"},
  {MP::ModelPerformanceStatsQuantileAbsError, "quantile_abs_error"},
  {MP::ModelPerformanceStatsRMSE,             "rmse"},
  {MP::ModelPerformanceStatsMaxCorrelation,   "correlation_max"},
  {MP::ModelPerformanceStatsCorrelationLag,   "correlation_lag"},
});

const map<Element::element_t, string> __elementTypeStrings({
  {Element::JUNCTION,   "junction"},
  {Element::TANK,       "tank"},
  {Element::RESERVOIR,  "reservoir"},
  {Element::PIPE,       "pipe"},
  {Element::PUMP,       "pump"},
  {Element::VALVE,      "valve"}
});


ModelPerformanceController::ModelPerformanceController(Model::_sp model) {
  _perf.reset(new ModelPerformance(model));
}




vector<TimeSeries::_sp> ModelPerformanceController::errorsForElement(Element::_sp element) {
  
  vector<TimeSeries::_sp> errors;
  
  Dma::_sp primaryDma, secondaryDma;
  // is this a pipe or junction type element?
  Junction::_sp j1ptr = dynamic_pointer_cast<Junction>(element);
  Junction::_sp j2ptr;
  Pipe::_sp pptr = dynamic_pointer_cast<Pipe>(element);
  string geoHashKV("");
  
  if (pptr) {
    // get junctions
    // positive direction is INTO the DMA. OUT of secondary DMA.
    j1ptr = dynamic_pointer_cast<Junction>(pptr->to());
    j2ptr = dynamic_pointer_cast<Junction>(pptr->from());
  }
  else if (j1ptr) {
    // for sure a junction element
    geoHashKV = ",geohash=" + GeoMetrics::geoHashWithJunction(j1ptr);
  }
  
  for (auto dma : _perf->model()->dmas()) {
    if (dma->doesHaveJunction(j1ptr)) {
      primaryDma = dma;
    }
    if (j2ptr && dma->doesHaveJunction(j2ptr)) {
      secondaryDma = dma;
    }
  }
  
  if (j2ptr && j1ptr && j1ptr == j2ptr) {
    j2ptr.reset(); // ignore secondary dma if they are the same.
  }
  
  string assetType = __elementTypeStrings.at(element->type());
  
  for (auto metricType : this->saveMetrics) {
    std::map<ModelPerformance::StatsType,TimeSeries::_sp> errorsMap = ModelPerformance::errorsForElementAndMetricType(element, metricType, _perf->samplingWindow(), _perf->quantile(), _perf->correlationWindow(), _perf->correlationLag());
    if (errorsMap.size() == 0) {
      continue;
    }
    for (auto statType : this->saveStats) {
      if (errorsMap.count(statType) == 0) {
        DebugLog << "could not find " << statType << " for errors map of element " << element->name() << EOL;
        continue;
      }
      auto ts = errorsMap.at(statType);
      auto generatorTypeStr = __statsTypeStrings.at(statType);
      stringstream ss;
      string metric = __metricTypeStrings.at(metricType);
      ss << metric;
      ss << ",asset_id=" << element->name() << ",asset_type=" << assetType << ",dma=" << primaryDma->name();
      if (secondaryDma) {
        ss << ",dma2=" << secondaryDma->name();
      }
      ss << ",generator=" << generatorTypeStr << geoHashKV;
      const string tsName = ss.str();
      ts->setName(tsName);
      errors.push_back(ts);
    }
  }
  
  return errors;
}

