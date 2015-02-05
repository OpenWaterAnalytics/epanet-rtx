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

using namespace RTX;
using namespace std;

ModelPerformance::ModelPerformance(Model::_sp model, StatsType statsType, AggregationType aggregationType, LocationType locationType) {
  
  if (!model) {
    return;
  }
  _model = model;
  _statsType = statsType;
  _aggregationType = aggregationType;
  _locationType = locationType;
  
  this->rebuildPerformanceCalculation();
  
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


ModelPerformance::LocationType ModelPerformance::locationType() {
  return _locationType;
}

void ModelPerformance::setLocationType(ModelPerformance::LocationType type) {
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


Clock::_sp ModelPerformance::aggregationClock() {
  return _aggregationClock;
}

void ModelPerformance::setAggregationClock(Clock::_sp clock) {
  _aggregationClock = clock;
  this->rebuildPerformanceCalculation();
}


void ModelPerformance::rebuildPerformanceCalculation() {
  
  vector<Element::_sp> elementsToConsider = this->elementsWithModelForLocationType(this->model(), this->locationType());
  this->buildPerformanceCalculatorWithElements(elementsToConsider);
  
}


vector<Element::_sp> ModelPerformance::elementsWithModelForLocationType(Model::_sp model, LocationType locationType) {
  
  // get the collection of elements to consider.
  vector<Element::_sp> elementsToConsider;
  
  switch (this->locationType()) {
    case ModelPerformanceLocationFlow:
    {
      BOOST_FOREACH( Pipe::_sp p, model->pipes()) {
        if (p->doesHaveFlowMeasure()) {
          elementsToConsider.push_back(p);
        }
      }
      BOOST_FOREACH( Pipe::_sp p, model->valves()) {
        if (p->doesHaveFlowMeasure()) {
          elementsToConsider.push_back(p);
        }
      }
      BOOST_FOREACH( Pipe::_sp p, model->pumps()) {
        if (p->doesHaveFlowMeasure()) {
          elementsToConsider.push_back(p);
        }
      }
    }
      break;
    case ModelPerformanceLocationPressure:
    {
      BOOST_FOREACH(Junction::_sp j, model->junctions()) {
        if (j->pressureMeasure()) {
          elementsToConsider.push_back(j);
        }
      }
    }
      break;
    case ModelPerformanceLocationHead:
    {
      BOOST_FOREACH(Junction::_sp j, model->junctions()) {
        if (j->doesHaveHeadMeasure()) {
          elementsToConsider.push_back(j);
        }
      }
    }
      break;
    case ModelPerformanceLocationTank:
    {
      BOOST_FOREACH(Tank::_sp t, model->tanks()) {
        if (t->doesHaveHeadMeasure()) {
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
    pair<TimeSeries::_sp,TimeSeries::_sp> tsPair = this->tsPairForElementWithLocationType(e, this->locationType());
    
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
  performance->setClock(this->aggregationClock());
  
  typedef pair<Element::_sp, TimeSeries::_sp> elementTimeseriesPair_t;
  BOOST_FOREACH(const elementTimeseriesPair_t &p, components) {
    performance->addSource(p.second);
  }
  
  if (this->aggregationType() == ModelPerformanceAggregationMean) {
    // divide by number of error sources
    GainTimeSeries::_sp meanPerf(new GainTimeSeries);
    meanPerf->setClock(this->aggregationClock());
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
    case ModelPerformanceStatsCorrelationCoefficient:
    {
      CorrelatorTimeSeries::_sp corr(new CorrelatorTimeSeries());
      corr->setClock(this->errorClock());
      corr->setCorrelationWindow(this->samplingWindow());
      corr->setSource(tsPair.first);
      corr->setCorrelatorTimeSeries(tsPair.second);
      err = corr;
    }
    default:
      break;
  }
  
  return err;
  
}



pair<TimeSeries::_sp, TimeSeries::_sp> ModelPerformance::tsPairForElementWithLocationType(Element::_sp e, ModelPerformance::LocationType location) {
  
  TimeSeries::_sp measured, modeled;
  switch (location) {
    case ModelPerformanceLocationFlow:
    {
      Pipe::_sp p = boost::dynamic_pointer_cast<Pipe>(e);
      measured = p->flowMeasure();
      modeled = p->flow();
    }
      break;
    case ModelPerformanceLocationHead:
    {
      Junction::_sp j = boost::dynamic_pointer_cast<Junction>(e);
      measured = j->headMeasure();
      modeled = j->head();
    }
      break;
    case ModelPerformanceLocationPressure:
    {
      Junction::_sp j = boost::dynamic_pointer_cast<Junction>(e);
      measured = j->pressureMeasure();
      modeled = j->pressure();
    }
      break;
    case ModelPerformanceLocationTank:
    {
      Tank::_sp t = boost::dynamic_pointer_cast<Tank>(e);
      measured = t->levelMeasure();
      modeled = t->level();
    }
      break;
    default:
      break;
  }
  
  return make_pair(measured, modeled);
  
}


