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
  _energyState->setName("L " + name + " Energy");
  _energyState->setUnits(RTX_KILOWATT);
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


TimeSeries::_sp Pump::energyMeasure() {
  return _energyMeasure;
}

void Pump::setEnergyMeasure(TimeSeries::_sp energy) {
  if (!energy || energy->units().isSameDimensionAs(RTX_KILOWATT) ) {
    _energyMeasure = energy;
  }
}


Curve::_sp Pump::headCurve() {
  return _headCurve;
}
void Pump::setHeadCurve(Curve::_sp curve) {
  _headCurve = curve;
}

Curve::_sp Pump::efficiencyCurve() {
  return _efficiencyCurve;
}
void Pump::setEfficiencyCurve(Curve::_sp curve) {
  _efficiencyCurve = curve;
}

