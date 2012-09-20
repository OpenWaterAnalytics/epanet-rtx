//
//  Reservoir.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

#include <iostream>

#include "Reservoir.h"

using namespace RTX;

Reservoir::Reservoir(const std::string& name) : Junction(name) {
  setType(RESERVOIR);
}
Reservoir::~Reservoir() {
  
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