//
//  Pump.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Pump.h"

using namespace RTX;

Pump::Pump(const std::string& name, Node::_sp startNode, Node::_sp endNode) : Pipe(name, startNode, endNode) {
  setType(PUMP);
  _energyState.reset( new TimeSeries() );
  _energyState->setName("L " + name + " energy");
  _energyState->setUnits(RTX_KILOWATT_HOUR);
}
Pump::~Pump() {
  
}

void Pump::setRecord(PointRecord::_sp record) {
  _energyState->setRecord(record);
  // call base method
  Pipe::setRecord(record);
}

TimeSeries::_sp Pump::energy() {
  return _energyState;
}


TimeSeries::_sp Pump::curveParameter() {
  return _curve;
}

void Pump::setCurveParameter(TimeSeries::_sp curve) {
  _curve = curve;
}


TimeSeries::_sp Pump::energyMeasure() {
  return _energyMeasure;
}

void Pump::setEnergyMeasure(TimeSeries::_sp energy) {
  if (!energy || energy->units().isSameDimensionAs(RTX_KILOWATT_HOUR) ) {
    _energyMeasure = energy;
  }
}
