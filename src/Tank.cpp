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
#include "CurveFunction.h"
#include "FirstDerivative.h"

using namespace RTX;

Tank::Tank(const std::string& name) : Junction(name) {
  setType(TANK);
  // initialize time series states
  
  OffsetTimeSeries::sharedPointer levelOffset( new OffsetTimeSeries() );
  levelOffset->setUnits(RTX_FOOT);
  levelOffset->setSource(head());
  levelOffset->setOffset(-elevation());
  _level = levelOffset;
  
  CurveFunction::sharedPointer volumeCurve( new CurveFunction() );
  volumeCurve->setUnits(RTX_LITER);
  volumeCurve->setSource(_level);
  _volume = volumeCurve;
  
  FirstDerivative::sharedPointer flowDerivative( new FirstDerivative() );
  flowDerivative->setUnits(RTX_LITER_PER_SECOND);
  flowDerivative->setSource(_volume);
  _flow = flowDerivative;
  
}

Tank::~Tank() {
  
}

void Tank::setElevation(double elevation) {
  Node::setElevation(elevation);
  // re-set the elevation offset for the level timeseries
  _level->setOffset(-elevation);
}

void Tank::setLevelMeasure(TimeSeries::sharedPointer levelMeasure) {
  
  OffsetTimeSeries::sharedPointer headMeasureTs(new OffsetTimeSeries());
  headMeasureTs->setUnits(levelMeasure->units());
  headMeasureTs->setName(levelMeasure->name() + " (head)");
  headMeasureTs->setClock(levelMeasure->clock());
  headMeasureTs->setOffset(elevation());
  headMeasureTs->setSource(levelMeasure);
  
  setHeadMeasure(headMeasureTs);
}


TimeSeries::sharedPointer Tank::level() {
  return _level;
}
TimeSeries::sharedPointer Tank::flowMeasure() {
  return _flowMeasure;
}
TimeSeries::sharedPointer Tank::volumeMeasure() {
  return _volumeMeasure;
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
