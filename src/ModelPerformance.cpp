//
//  ModelPerformance.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/18/14.
//
//

#include <boost/foreach.hpp>

#include "ModelPerformance.h"
#include "AggregatorTimeSeries.h"
#include "CorrelatorTimeSeries.h"
#include "StatsTimeSeries.h"
#include "GainTimeSeries.h"
#include "MathOpsTimeSeries.h"
#include "IntegratorTimeSeries.h"
#include "MetaTimeSeries.h"

using namespace RTX;
using namespace std;

ModelPerformance::ModelPerformance(Model::_sp model, StatsType statsType, AggregationType aggregationType, MetricType locationType) {
  
  if (!model) {
    return;
  }
  _model = model;
  _statsType = statsType;
  _aggregationType = aggregationType;
  _locationType = locationType;
  this->rebuildPerformanceCalculation();
}

map<ModelPerformance::StatsType, TimeSeries::_sp> ModelPerformance::errorsForElementAndMetricType(Element::_sp element, MetricType type, Clock::_sp samplingWindow, double quantile, int correlationDiscretization, int correlationMaxLag) {
  
  map<ModelPerformance::StatsType, TimeSeries::_sp> errorSeries;
  
  // get the ts pair.
  TimeSeriesFilter::_sp measured(new TimeSeriesFilter), modeled(new TimeSeriesFilter);
  
  pair<TimeSeries::_sp,TimeSeries::_sp> pair = tsPairForElementWithMetricType(element, type);
  
  
  if (!pair.first || !pair.second) {
    return errorSeries;
  }
  
  measured->setSource(pair.first);
  modeled->setSource(pair.second);
  
  AggregatorTimeSeries::_sp error(new AggregatorTimeSeries);
  error->addSource(modeled);
  error->addSource(measured, -1);
  
  StatsTimeSeries::_sp meanErr(new StatsTimeSeries);
  meanErr->setStatsType(StatsTimeSeries::StatsTimeSeriesMean);
  meanErr->setWindow(samplingWindow);
  meanErr->setSource(error);
  
  StatsTimeSeries::_sp rmse(new StatsTimeSeries);
  rmse->setStatsType(StatsTimeSeries::StatsTimeSeriesRMS);
  rmse->setWindow(samplingWindow);
  rmse->setSource(error);

  CorrelatorTimeSeries::_sp maxCor(new CorrelatorTimeSeries);
  maxCor->setUnits(RTX_DIMENSIONLESS);
  maxCor->setLagSeconds(correlationMaxLag);
  maxCor->setSource(measured);
  maxCor->setCorrelationWindow(samplingWindow);
  Clock::_sp corWindow(new Clock(correlationDiscretization));
  maxCor->setClock(corWindow);
  maxCor->setSecondary(modeled);
  MetaTimeSeries::_sp maxCorLag(new MetaTimeSeries);
  maxCorLag->setSource(maxCor);
  maxCorLag->setMetaMode(MetaTimeSeries::MetaModeConfidence); // sneaky! 💩
  maxCorLag->setUnits(RTX_SECOND);
  
  MathOpsTimeSeries::_sp absError(new MathOpsTimeSeries());
  absError->setMathOpsType(MathOpsTimeSeries::MathOpsTimeSeriesAbs);
  absError->setSource(error);
  
  StatsTimeSeries::_sp meanAbsErr(new StatsTimeSeries);
  meanAbsErr->setStatsType(StatsTimeSeries::StatsTimeSeriesMean);
  meanAbsErr->setWindow(samplingWindow);
  meanAbsErr->setSource(absError);
  
  StatsTimeSeries::_sp quantileAbsErr(new StatsTimeSeries);
  quantileAbsErr->setStatsType(StatsTimeSeries::StatsTimeSeriesPercentile);
  quantileAbsErr->setArbitraryPercentile(quantile);
  quantileAbsErr->setWindow(samplingWindow);
  quantileAbsErr->setSource(absError);
  
  IntegratorTimeSeries::_sp integratedAbs(new IntegratorTimeSeries);
  integratedAbs->setResetClock(samplingWindow);
  integratedAbs->setSource(absError);
  
  StatsTimeSeries::_sp quantileTs(new StatsTimeSeries);
  quantileTs->setWindow(samplingWindow);
  quantileTs->setStatsType(StatsTimeSeries::StatsTimeSeriesPercentile);
  quantileTs->setArbitraryPercentile(quantile);
  quantileTs->setSource(absError);
  
  IntegratorTimeSeries::_sp integrated(new IntegratorTimeSeries);
  integrated->setResetClock(samplingWindow);
  integrated->setSource(error);
  
  // special case for integrated errors: if we are integrating flow, then set volume units
  if (type == ModelPerformanceMetricFlow) {
    Units u = RTX_GALLON;
    integrated->setUnits(u);
    integratedAbs->setUnits(u);
  }
  
  
  errorSeries[ModelPerformanceStatsModel] = modeled;
  errorSeries[ModelPerformanceStatsMeasure] = measured;
  errorSeries[ModelPerformanceStatsError] = error;
  errorSeries[ModelPerformanceStatsMeanError] = meanErr;
  errorSeries[ModelPerformanceStatsQuantileError] = quantileTs;
  errorSeries[ModelPerformanceStatsIntegratedError] = integrated;
  errorSeries[ModelPerformanceStatsAbsError] = absError;
  errorSeries[ModelPerformanceStatsMeanAbsError] = meanAbsErr;
  errorSeries[ModelPerformanceStatsQuantileAbsError] = quantileAbsErr;
  errorSeries[ModelPerformanceStatsIntegratedAbsError] = integratedAbs;
  errorSeries[ModelPerformanceStatsRMSE] = rmse;
  errorSeries[ModelPerformanceStatsMaxCorrelation] = maxCor;
  errorSeries[ModelPerformanceStatsCorrelationLag] = maxCorLag;
  
  return errorSeries;
}


// specify the model and get the important information
Model::_sp ModelPerformance::model() {
  return _model;
}

TimeSeries::_sp ModelPerformance::performance() {
  return _performance;
}

std::vector<std::pair<Element::_sp, TimeSeries::_sp> > ModelPerformance::components() {
  return _errorComponents;
}


// specify the source data and the way the statistics are computed.
ModelPerformance::StatsType ModelPerformance::statsType() {
  return _statsType;
}

void ModelPerformance::setStatsType(ModelPerformance::StatsType type) {
  _statsType = type;
  this->rebuildPerformanceCalculation();
}

ModelPerformance::AggregationType ModelPerformance::aggregationType() {
  return _aggregationType;
}

void ModelPerformance::setAggregationType(ModelPerformance::AggregationType type) {
  _aggregationType = type;
  this->rebuildPerformanceCalculation();
}


ModelPerformance::MetricType ModelPerformance::locationType() {
  return _locationType;
}

void ModelPerformance::setLocationType(ModelPerformance::MetricType type) {
  _locationType = type;
  this->rebuildPerformanceCalculation();
}



Clock::_sp ModelPerformance::samplingWindow() {
  return _samplingWindow;
}

void ModelPerformance::setSamplingWindow(Clock::_sp clock) {
  _samplingWindow = clock;
  this->rebuildPerformanceCalculation();
}


Clock::_sp ModelPerformance::errorClock() {
  return _errorClock;
}

void ModelPerformance::setErrorClock(Clock::_sp clock) {
  _errorClock = clock;
  this->rebuildPerformanceCalculation();
}


time_t ModelPerformance::correlationLag() {
  return _correlationLag;
}

void ModelPerformance::setCorrelationLag(time_t timeLag) {
  _correlationLag = timeLag;
  this->rebuildPerformanceCalculation();
}

double ModelPerformance::quantile() {
  return _quantile;
}

void ModelPerformance::setQuantile(double q) {
  if (q > 0 && q < 1) {
    _quantile = q;
    this->rebuildPerformanceCalculation();
  }
  else {
    cerr << "ERR: Quantile must be between 0 and 1" << endl;
  }
}

void ModelPerformance::rebuildPerformanceCalculation() {
  
  vector<Element::_sp> elementsToConsider = this->elementsWithModelForLocationType(this->model(), this->locationType());
  this->buildPerformanceCalculatorWithElements(elementsToConsider);
  
}


vector<Element::_sp> ModelPerformance::elementsWithModelForLocationType(Model::_sp model, MetricType locationType) {
  
  // get the collection of elements to consider.
  vector<Element::_sp> elementsToConsider;
  
  switch (this->locationType()) {
    case ModelPerformanceMetricFlow:
    {
      BOOST_FOREACH( Pipe::_sp p, model->pipes()) {
        if (p->flowMeasure()) {
          elementsToConsider.push_back(p);
        }
      }
      BOOST_FOREACH( Pipe::_sp p, model->valves()) {
        if (p->flowMeasure()) {
          elementsToConsider.push_back(p);
        }
      }
      BOOST_FOREACH( Pipe::_sp p, model->pumps()) {
        if (p->flowMeasure()) {
          elementsToConsider.push_back(p);
        }
      }
    }
      break;
    case ModelPerformanceMetricPressure:
    {
      BOOST_FOREACH(Junction::_sp j, model->junctions()) {
        if (j->pressureMeasure()) {
          elementsToConsider.push_back(j);
        }
      }
    }
      break;
    case ModelPerformanceMetricHead:
    {
      BOOST_FOREACH(Junction::_sp j, model->junctions()) {
        if (j->headMeasure()) {
          elementsToConsider.push_back(j);
        }
      }
    }
      break;
    case ModelPerformanceMetricLevel:
    {
      BOOST_FOREACH(Tank::_sp t, model->tanks()) {
        if (t->levelMeasure()) {
          elementsToConsider.push_back(t);
        }
      }
    }
      break;
    default:
      break;
  }
  
  
  return elementsToConsider;
  
}


void ModelPerformance::buildPerformanceCalculatorWithElements(std::vector<Element::_sp> elements) {
  
  
  vector<pair<Element::_sp, TimeSeries::_sp> > components;
  
  // go through the elements
  BOOST_FOREACH(Element::_sp e, elements) {
    // decide what time series pair to use.
    pair<TimeSeries::_sp,TimeSeries::_sp> tsPair = tsPairForElementWithMetricType(e, this->locationType());
    
    // assemble the difference or correlator
    TimeSeries::_sp error = this->errorForPair(tsPair);
    string errName("");
    errName += e->name();
    errName += "_";
    errName += "error";
    error->setName(errName);
    components.push_back(make_pair(e, error));
  }
  
  
  AggregatorTimeSeries::_sp performance(new AggregatorTimeSeries);
  performance->setClock(this->errorClock());
  
  typedef pair<Element::_sp, TimeSeries::_sp> elementTimeseriesPair_t;
  BOOST_FOREACH(const elementTimeseriesPair_t &p, components) {
    performance->addSource(p.second);
  }
  
  if (this->aggregationType() == ModelPerformanceAggregationMean) {
    // divide by number of error sources
    GainTimeSeries::_sp meanPerf(new GainTimeSeries);
    meanPerf->setClock(this->errorClock());
    meanPerf->setGainUnits(RTX_DIMENSIONLESS);
    meanPerf->setGain( 1. / (double)(performance->sources().size()) );
    meanPerf->setSource(performance);
    _performance = performance;
  }
  else if (this->aggregationType() == ModelPerformanceAggregationSum) {
    _performance = performance;
  }
  
  
  _errorComponents = components;
}


TimeSeries::_sp ModelPerformance::errorForPair(std::pair<TimeSeries::_sp, TimeSeries::_sp> tsPair)
{
  
  TimeSeries::_sp err;
  
  switch (this->statsType()) {
    case ModelPerformanceStatsRMSE:
    {
      AggregatorTimeSeries::_sp diff(new AggregatorTimeSeries);
      diff->addSource(tsPair.first);
      diff->addSource(tsPair.second, -1);
      diff->setClock(this->errorClock());
      
      StatsTimeSeries::_sp rmse(new StatsTimeSeries);
      rmse->setSource(diff);
      rmse->setWindow(this->samplingWindow());
      rmse->setStatsType(StatsTimeSeries::StatsTimeSeriesRMS);
      rmse->setClock(this->errorClock());
      
      err = rmse;
    }
      break;
    case ModelPerformanceStatsMaxCorrelation:
    {
      int nLags = (24*60*60) / tsPair.first->clock()->period();
      //TimeSeries::_sp maxCor = CorrelatorTimeSeries::correlationArray(tsPair.first, tsPair.second, this->samplingWindow(), nLags);
      
      CorrelatorTimeSeries::_sp corr(new CorrelatorTimeSeries());
      corr->setUnits(RTX_DIMENSIONLESS);
      corr->setClock(this->errorClock());
      corr->setCorrelationWindow(this->samplingWindow());
      corr->setSource(tsPair.first);
      corr->setSecondary(tsPair.second);
      err = corr;
    }
      break;
    case ModelPerformanceStatsCorrelationLag:
    {
      // not complete!!!
    }
      break;
      
    case ModelPerformanceStatsQuantileError:
    {
      AggregatorTimeSeries::_sp diff(new AggregatorTimeSeries);
      diff->addSource(tsPair.second, -1);  // will adopt the uniform model units
      diff->addSource(tsPair.first);
      diff->setClock(this->errorClock());
      
      MathOpsTimeSeries::_sp abs(new MathOpsTimeSeries());
      abs->setSource(diff);
      abs->setMathOpsType(MathOpsTimeSeries::MathOpsTimeSeriesAbs);
      
      StatsTimeSeries::_sp quant(new StatsTimeSeries);
      quant->setSource(abs);
      quant->setWindow(this->samplingWindow());
      quant->setStatsType(StatsTimeSeries::StatsTimeSeriesPercentile);
      quant->setClock(this->errorClock());
      quant->setArbitraryPercentile(_quantile);
      
      err = quant;
    }
    default:
      break;
  }
  
  return err;
  
}



pair<TimeSeries::_sp, TimeSeries::_sp> ModelPerformance::tsPairForElementWithMetricType(Element::_sp e, ModelPerformance::MetricType location) {
  
  TimeSeries::_sp measured, modeled;
  switch (location) {
    case ModelPerformanceMetricFlow:
    {
      // could be link or tank.
      if (e->type() == Element::TANK) {
        Tank::_sp t = boost::dynamic_pointer_cast<Tank>(e);
        if (!t) {
          break;
        }
        measured = t->flowMeasure();
        modeled = t->flow();
      }
      else {
        Pipe::_sp p = boost::dynamic_pointer_cast<Pipe>(e);
        if (!p) {
          break;
        }
        measured = p->flowMeasure();
        modeled = p->flow();
      }
    }
      break;
    case ModelPerformanceMetricHead:
    {
      Junction::_sp j = boost::dynamic_pointer_cast<Junction>(e);
      if (!j) {
        break;
      }
      measured = j->headMeasure();
      modeled = j->head();
    }
      break;
    case ModelPerformanceMetricPressure:
    {
      Junction::_sp j = boost::dynamic_pointer_cast<Junction>(e);
      if (!j) {
        break;
      }
      measured = j->pressureMeasure();
      modeled = j->pressure();
    }
      break;
    case ModelPerformanceMetricLevel:
    {
      Tank::_sp t = boost::dynamic_pointer_cast<Tank>(e);
      if (!t) {
        break;
      }
      measured = t->levelMeasure();
      modeled = t->level();
    }
      break;
    case ModelPerformanceMetricVolume:
    {
      Tank::_sp t = boost::dynamic_pointer_cast<Tank>(e);
      if (!t) {
        break;
      }
      measured = t->volumeMeasure();
      modeled = t->volume();
    }
      break;
    case ModelPerformanceMetricConcentration:
    {
      Junction::_sp j = boost::dynamic_pointer_cast<Junction>(e);
      if (j) {
        measured = j->qualityMeasure();
        modeled = j->quality();
        break;
      }
      Pipe::_sp p = boost::dynamic_pointer_cast<Pipe>(e);
      if (p) {
        /// TO-DO
      }
    }
      break;
    case ModelPerformanceMetricEnergy:
    {
      Pump::_sp p = boost::dynamic_pointer_cast<Pump>(e);
      if (p) {
        measured = p->energyMeasure();
        modeled = p->energy();
      }
    }
      break;
    case ModelPerformanceMetricSetting:
    {
      Pipe::_sp p = boost::dynamic_pointer_cast<Pipe>(e);
      if (p) {
        measured = p->settingBoundary();
        modeled = p->settingBoundary();
      }
    }
      break;
    case ModelPerformanceMetricStatus:
    {
      Pipe::_sp p = boost::dynamic_pointer_cast<Pipe>(e);
      if (p) {
        measured = p->statusBoundary();
        modeled = p->statusBoundary();
      }
    }
      break;
    case ModelPerformanceMetricDemand:
    {
      Junction::_sp j = boost::dynamic_pointer_cast<Junction>(e);
      if (j) {
        measured = j->boundaryFlow();
        modeled = j->demand();
      }
    }
      break;
    default:
      break;
  }
  
  return make_pair(measured, modeled);
  
}


