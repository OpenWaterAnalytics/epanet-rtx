//
//  Pump.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include <iostream>

#include "Pump.h"

using namespace RTX;

Pump::Pump(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode) : Pipe(name, startNode, endNode) {
  setType(PUMP);
  _doesHaveCurveParameter = false;
  _doesHaveEnergyParameter = false;
  _energyState.reset( new TimeSeries() );
  _energyState->setName("L " + name + " energy");
}
Pump::~Pump() {
  
}

void Pump::setRecord(PointRecord::sharedPointer record) {
  _energyState->newCacheWithPointRecord(record);
  // call base method
  Pipe::setRecord(record);
}

TimeSeries::sharedPointer Pump::energy() {
  return _energyState;
}

bool Pump::doesHaveCurveParameter() {
  return _doesHaveCurveParameter;
}

TimeSeries::sharedPointer Pump::curveParameter() {
  return _curve;
}

void Pump::setCurveParameter(TimeSeries::sharedPointer curve) {
  _doesHaveCurveParameter = (curve ? true : false);
  _curve = curve;
}

bool Pump::doesHaveEnergyMeasure() {
  return _doesHaveEnergyParameter;
}

TimeSeries::sharedPointer Pump::energyMeasure() {
  return _energyMeasure;
}

void Pump::setEnergyMeasure(TimeSeries::sharedPointer energy) {
  _doesHaveEnergyParameter = (energy ? true : false);
  _energyMeasure = energy;
}
