//
//  Tank.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Tank.h"

#include "OffsetTimeSeries.h"

using namespace RTX;

Tank::Tank(const std::string& name) : Junction(name) {
  setType(TANK);
  // initialize level time series
  _level.reset( new OffsetTimeSeries() );
  _level->setUnits(RTX_FOOT);
  _level->setSource(head());
  _level->setOffset(-elevation());
}

Tank::~Tank() {
  
}

void Tank::setElevation(double elevation) {
  Node::setElevation(elevation);
  // re-set the elevation offset for the level timeseries
  _level->setOffset(-elevation);
}

void Tank::setLevelMeasure(TimeSeries::sharedPointer level) {
  
  OffsetTimeSeries::sharedPointer headMeasureTs(new OffsetTimeSeries());
  headMeasureTs->setUnits(level->units());
  headMeasureTs->setName(level->name() + " (head)");
  headMeasureTs->setClock(level->clock());
  headMeasureTs->setOffset(elevation());
  headMeasureTs->setSource(level);
  
  setHeadMeasure(headMeasureTs);
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
