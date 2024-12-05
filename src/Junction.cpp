//
//  Junction.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Junction.h"
#include <OffsetTimeSeries.h>
#include <GainTimeSeries.h>

using namespace std;
using namespace RTX;
using namespace TSF;

Junction::Junction(const std::string& name) : Node(name) {
  //_demand = 0;
  //_baseDemand = 0;
  setType(JUNCTION);
  
  // we have nothing yet
  _headState.reset(new TimeSeries("head,n=" + name, TSF_METER));
  _pressureState.reset(new TimeSeries("pressure,n=" + name, TSF_PASCAL));
  _qualityState.reset(new TimeSeries("quality,n=" + name, TSF_HOUR));
  _demandState.reset(new TimeSeries("demand,n=" + name, TSF_LITER_PER_SECOND));
  
  _baseDemand = 0;
  
  
  state_head = 0;
  state_pressure = 0; 
  state_demand = 0;
  state_quality = 0;
  state_inlet_quality = 0;
  state_volume = 0;
  state_flow = 0;
}
Junction::~Junction() {
  
}

double Junction::baseDemand() {
  return _baseDemand;
}

void Junction::setBaseDemand(double demand) {
  _baseDemand = demand;
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
  else if ( !(flow->units().isSameDimensionAs(TSF_GALLON_PER_MINUTE)) ) {
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
    DebugLog << "removing head measure: " << this->name() << EOL;
    return;
  }
  else if ( !(headMeas->units().isSameDimensionAs(TSF_METER)) ) {
    DebugLog << "removing head measure: " << this->name() << EOL;
    return;
  }
  
  _headMeasure = headMeas;
  
  OffsetTimeSeries::_sp relativeHead( new OffsetTimeSeries() );
  relativeHead->setUnits(this->head()->units());
  relativeHead->setOffset(this->elevation() * -1.);
  relativeHead->setSource(headMeas);
  
  auto pUnits = (this->head()->units() == TSF_METER) ? TSF_KILOPASCAL : TSF_PSI;
  
  // pressure depends on elevation --> head = mx(pressure) + elev
  GainTimeSeries::_sp gainTs( new GainTimeSeries() );
  gainTs
    ->gain(1.0)
    ->gainUnits(Units::unitOfType("psi-per-ft"))
    ->source(relativeHead)
    ->units(pUnits)
    ->name(this->name() + " pressure measure");
  
  _pressureMeasure = gainTs;
  
  DebugLog << "Setting head measure for: " << this->name() << " -> Units: " << this->head()->units().to_string() <<
    " -> Elev: " << this->elevation() << EOL;
  DebugLog << "Head series: " << headMeas->name() << " -> Units: " << headMeas->units().to_string() <<  EOL;
  DebugLog << "Pres series: " << gainTs->name() << " -> Units: " << gainTs->units().to_string() <<  EOL;
}
TimeSeries::_sp Junction::headMeasure() {
  return _headMeasure;
}

void Junction::setPressureMeasure(TimeSeries::_sp pressure) {
  if (pressure == NULL || !pressure) {
    _pressureMeasure = TimeSeries::_sp();
    _headMeasure = TimeSeries::_sp();
    return;
  }
  else if ( !(pressure->units().isSameDimensionAs(TSF_PASCAL)) ) {
    return;
  }
  
  _pressureMeasure = pressure;
  // pressure depends on elevation --> head = mx(pressure) + elev
  GainTimeSeries::_sp gainTs( new GainTimeSeries() );
  gainTs
    ->gain(1.0)
    ->gainUnits(Units::unitOfType("ft-per-psi"))
    ->source(pressure)
    ->units(this->head()->units());
  
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
