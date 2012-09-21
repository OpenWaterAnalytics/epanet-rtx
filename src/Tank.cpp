//
//  Tank.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Tank.h"

using namespace RTX;

Tank::Tank(const std::string& name) : Junction(name) {
  setType(TANK);
  // initialize level time series
  _elevationOffset.reset( new ConstantSeries(elevation()) );
  _elevationOffset->setUnits(RTX_METER);
  
  _level.reset( new AggregatorTimeSeries() );
  _level->setUnits(RTX_METER);
  _level->addSource(head());
  _level->addSource(_elevationOffset);
}

Tank::~Tank() {
  
}

void Tank::setElevation(double elevation) {
  Node::setElevation(elevation);
  // re-set the elevation offset for the level timeseries
  _elevationOffset->setValue(elevation);
}

TimeSeries::sharedPointer Tank::level() {
  return _level;
}

bool Tank::doesResetLevel() {
  return _doesResetLevel;
}
void Tank::setLevelResetClock(Clock::sharedPointer clock) {
  _doesResetLevel = (clock ? true : false);
  _resetLevel = clock;
}
Clock::sharedPointer Tank::levelResetClock() {
  return _resetLevel;
}
