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

  _level.reset( new OffsetTimeSeries() );
  _level->setSource(this->head());
  _level->setName(name + " level (offset)");
  
  // likely to be used.
  _levelMeasure.reset( new OffsetTimeSeries() );
  _levelMeasure->setUnits(RTX_METER);
  _levelMeasure->setName(name + " level measure");
  
  _volumeMeasure.reset( new CurveFunction() );
  _volumeMeasure->setUnits(RTX_LITER);
  _volumeMeasure->setSource(_levelMeasure);
  _volumeMeasure->setName(name + " volume measure");
  
  _flowMeasure.reset( new FirstDerivative() );
  _flowMeasure->setUnits(RTX_LITER_PER_SECOND);
  _flowMeasure->setSource(_volumeMeasure);
  _flowMeasure->setName(name + " flow measure");
}

Tank::~Tank() {
  
}


void Tank::setElevation(double elevation) {
  Node::setElevation(elevation);
  // re-set the elevation offset for the level timeseries
  _level->setOffset(-elevation);
  _levelMeasure->setOffset(-elevation);
}

void Tank::setLevelMeasure(TimeSeries::sharedPointer levelMeasure) {
  //std::cout << "oops, depricated" << std::endl;
  
  OffsetTimeSeries::sharedPointer offsetHeadMeasure( new OffsetTimeSeries() );
  offsetHeadMeasure->setName(this->name() + " offset");
  offsetHeadMeasure->setSource(levelMeasure);
  offsetHeadMeasure->setOffset( (this->elevation()) );
  
  this->setHeadMeasure(offsetHeadMeasure);
}

TimeSeries::sharedPointer Tank::levelMeasure() {
  return _levelMeasure;
}

void Tank::setHeadMeasure(TimeSeries::sharedPointer head) {
  // base class method first
  Junction::setHeadMeasure(head);
  
  // now hook it up to the "measured" level->volume->flow chain.
  // assumption about elevation units is made here.
  // todo -- revise elevation units handling
  _levelMeasure->setUnits(head->units());
  _levelMeasure->setSource(head);
  _levelMeasure->setClock(head->clock());
  _volumeMeasure->setClock(head->clock());
  _flowMeasure->setClock(head->clock());

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
