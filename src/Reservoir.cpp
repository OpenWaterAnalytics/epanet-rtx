//
//  Reservoir.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Reservoir.h"

using namespace RTX;

Reservoir::Reservoir(const std::string& name) : Junction(name) {
  setType(RESERVOIR);
}
Reservoir::~Reservoir() {
  
}

void Reservoir::setFixedLevel(double level) {
  _fixedLevel = level;
}

double Reservoir::fixedLevel() {
  return _fixedLevel;
}

// for the following methods, default to the headMeasure Junction:: methods,
// since if a reservoir has a measurement, it simply must be a boundary condition.

void Reservoir::setBoundaryHead(TimeSeries::sharedPointer head) {
  setHeadMeasure(head);
}

bool Reservoir::doesHaveBoundaryHead() {
  return doesHaveHeadMeasure();
}

TimeSeries::sharedPointer Reservoir::boundaryHead() {
  return headMeasure();
}