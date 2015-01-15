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
  _doesHaveCurveParameter = false;
  _doesHaveEnergyParameter = false;
  _energyState.reset( new TimeSeries() );
  _energyState->setName("L " + name + " energy");
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

bool Pump::doesHaveCurveParameter() {
  return _doesHaveCurveParameter;
}

TimeSeries::_sp Pump::curveParameter() {
  return _curve;
}

void Pump::setCurveParameter(TimeSeries::_sp curve) {
  _doesHaveCurveParameter = (curve ? true : false);
  _curve = curve;
}

bool Pump::doesHaveEnergyMeasure() {
  return _doesHaveEnergyParameter;
}

TimeSeries::_sp Pump::energyMeasure() {
  return _energyMeasure;
}

void Pump::setEnergyMeasure(TimeSeries::_sp energy) {
  _doesHaveEnergyParameter = (energy ? true : false);
  _energyMeasure = energy;
}
