//
//  Junction.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Junction.h"
#include "OffsetTimeSeries.h"
#include "GainTimeSeries.h"

using namespace RTX;

Junction::Junction(const std::string& name) : Node(name) {
  //_demand = 0;
  //_baseDemand = 0;
  setType(JUNCTION);
  
  // we have nothing yet
  _headState.reset(new TimeSeries());
  _pressureState.reset(new TimeSeries());
  _qualityState.reset(new TimeSeries());
  _demandState.reset(new TimeSeries());
  
  _headState->setUnits(RTX_METER);
  _pressureState->setUnits(RTX_PASCAL);
  _qualityState->setUnits(RTX_SECOND);
  _demandState->setUnits(RTX_LITER_PER_SECOND);
  
  _headState->setName("N_" + name + "_head");
  _pressureState->setName("N_" + name + "_pressure");
  _qualityState->setName("N_" + name + "_quality");
  _demandState->setName("N_" + name + "_demand");
  
  _baseDemand = 0;
  _initialQuality = 0;
}
Junction::~Junction() {
  
}

double Junction::baseDemand() {
  return _baseDemand;
}

void Junction::setBaseDemand(double demand) {
  _baseDemand = demand;
}

double Junction::initialQuality() {
  return _initialQuality;
}

void Junction::setInitialQuality(double quality) {
  _initialQuality = quality;
}

void Junction::setRecord(PointRecord::_sp record) {
  _headState->setRecord(record);
  _pressureState->setRecord(record);
  _qualityState->setRecord(record);
  _demandState->setRecord(record);
}

// states - these recieve simulation results from the Model containing class
TimeSeries::_sp Junction::head() {
  return _headState;
}
TimeSeries::_sp Junction::pressure() {
  return _pressureState;
}
TimeSeries::_sp Junction::quality() {
  return _qualityState;
}
TimeSeries::_sp Junction::demand() {
  return _demandState;
}

// measured demand (boundary flow condition)

void Junction::setBoundaryFlow(TimeSeries::_sp flow) {
  if (flow == NULL || !flow) {
    _boundaryFlow = TimeSeries::_sp();
  }
  else if ( !(flow->units().isSameDimensionAs(RTX_GALLON_PER_MINUTE)) ) {
    return;
  }
  _boundaryFlow = flow;
}
TimeSeries::_sp Junction::boundaryFlow() {
  return _boundaryFlow;
}

// head measurement
void Junction::setHeadMeasure(TimeSeries::_sp headMeas) {
  if (headMeas == NULL || !headMeas) {
    _headMeasure = TimeSeries::_sp();
    _pressureMeasure = TimeSeries::_sp();
  }
  else if ( !(headMeas->units().isSameDimensionAs(RTX_METER)) ) {
    return;
  }
  
  _headMeasure = headMeas;
  
  OffsetTimeSeries::_sp relativeHead( new OffsetTimeSeries() );
  relativeHead->setUnits(this->head()->units());
  relativeHead->setOffset(this->elevation() * -1.);
  relativeHead->setSource(headMeas);
  
  // pressure depends on elevation --> head = mx(pressure) + elev
  GainTimeSeries::_sp gainTs( new GainTimeSeries() );
  gainTs->setUnits(RTX_PASCAL);
  gainTs->setGainUnits( RTX_PASCAL / RTX_METER);
  gainTs->setGain(9804.13943198467193);
  gainTs->setSource(relativeHead);
  gainTs->setName(this->name() + " pressure measure");
  
  _pressureMeasure = gainTs;
}
TimeSeries::_sp Junction::headMeasure() {
  return _headMeasure;
}

void Junction::setPressureMeasure(TimeSeries::_sp pressure) {
  if (pressure == NULL || !pressure) {
    _pressureMeasure = TimeSeries::_sp();
    _headMeasure = TimeSeries::_sp();
  }
  else if ( !(pressure->units().isSameDimensionAs(RTX_PASCAL)) ) {
    return;
  }
  
  _pressureMeasure = pressure;
  // pressure depends on elevation --> head = mx(pressure) + elev
  GainTimeSeries::_sp gainTs( new GainTimeSeries() );
  gainTs->setUnits(this->head()->units());
  gainTs->setGainUnits( RTX_METER / RTX_PASCAL );
  gainTs->setGain(0.0001019977334);
  gainTs->setSource(pressure);
  
  OffsetTimeSeries::_sp headMeas( new OffsetTimeSeries() );
  headMeas->setUnits(this->head()->units());
  headMeas->setOffset(this->elevation());
  headMeas->setSource(gainTs);
  headMeas->setName(this->name() + " head measure");
  
  _headMeasure = headMeas;
}

TimeSeries::_sp Junction::pressureMeasure() {
  return _pressureMeasure;
}


// quality measurement
void Junction::setQualityMeasure(TimeSeries::_sp quality) {
  _qualityMeasure = quality;
}
TimeSeries::_sp Junction::qualityMeasure() {
  return _qualityMeasure;
}

// quality boundary condition
void Junction::setQualitySource(TimeSeries::_sp quality) {
  _qualityBoundary = quality;
}
TimeSeries::_sp Junction::qualitySource() {
  return _qualityBoundary;
}
