//
//  Tank.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Tank.h"

#include <OffsetTimeSeries.h>
#include <CurveFunction.h>
#include <FirstDerivative.h>

using namespace RTX;
using namespace TSF;
using namespace std;

Tank::Tank(const std::string& name) : Junction(name) {
  setType(TANK);
  // initialize time series states
  _willResetLevel = false;
  _level.reset( new TimeSeries() );
  _level->setName("level,n=" + name);
  
  _minLevel = 0;
  _maxLevel = 0;
  _initLevel = 0;
  _diameter = 0;
  
  
  _volumeCalc.reset( new CurveFunction() );
  _volumeCalc->setUnits(TSF_LITER);
  _volumeCalc->setName("calc_volume,n=" + name);
  _volumeCalc->setDoesSaturate(true);
  
  _flowCalc.reset( new FirstDerivative() );
  _flowCalc->setUnits(TSF_LITER_PER_SECOND);
  _flowCalc->setSource(_volumeCalc);
  _flowCalc->setName("calc_flow,n=" + name);
  
  _volume.reset( new TimeSeries );
  _volume->setUnits(TSF_LITER);
  _volume->setName("volume,n=" + name);
  
  _flow.reset( new TimeSeries );
  _flow->setUnits(TSF_LITER_PER_SECOND);
  _flow->setName("flow,n=" + name);
  
  _inletQualityState.reset( new TimeSeries );
  _inletQualityState->setUnits(TSF_HOUR);
  _inletQualityState->setName("inlet_concentration,n=" + name);
  
}

Tank::~Tank() {
  
}


void Tank::setMinMaxLevel(double minLevel, double maxLevel) {
  _minLevel = minLevel;
  _maxLevel = maxLevel;
}

double Tank::minLevel() {
  return _minLevel;
}

double Tank::maxLevel() {
  return _maxLevel;
}


void Tank::setEnProperties(double initLevel, double diameter) {
  _initLevel = initLevel;
  _diameter = diameter;
}

double Tank::initLevel() {
  return _initLevel;
}

double Tank::diameter() {
  return _diameter;
}

void Tank::setGeometry(Curve::_sp curve) {
  _geometry = curve;
  _volumeCalc->setCurve(_geometry);
  if (!curve) {
    return;
  }
  _volumeCalc->setUnits(_geometry->outputUnits);
}

Curve::_sp Tank::geometry() {
  return _geometry;
}


void Tank::setElevation(double elevation) {
  Node::setElevation(elevation);
  
  // do we have a head or level measure??
  OffsetTimeSeries::_sp offsetTs;
  int direction = 1;
  
  if (levelMeasure() || headMeasure()) {
    direction = -1;
    offsetTs = std::dynamic_pointer_cast<OffsetTimeSeries>(levelMeasure());
    if (!offsetTs) {
      direction = 1;
      offsetTs = std::dynamic_pointer_cast<OffsetTimeSeries>(headMeasure());
    }
    if (offsetTs) {
      offsetTs->setOffset(elevation * direction);
    }
  }
  
}

void Tank::setLevelMeasure(TimeSeries::_sp levelMeasure) {
  if (!levelMeasure) {
    Junction::setHeadMeasure(TimeSeries::_sp());
    _levelMeasure = TimeSeries::_sp();
    TimeSeries::_sp blank;
    _volumeCalc->setSource(blank);
  }
  else {
    _volumeCalc->resetCache();
    _flowCalc->resetCache();
    
    OffsetTimeSeries::_sp offsetHeadMeasure( new OffsetTimeSeries() );
    offsetHeadMeasure->setName(this->name() + ".measure.head");
    offsetHeadMeasure->setSource(levelMeasure);
    offsetHeadMeasure->setOffset( (this->elevation()) );
  
    Junction::setHeadMeasure(offsetHeadMeasure);
    // test for success:
    if (Junction::headMeasure() == offsetHeadMeasure) {
      _levelMeasure = levelMeasure;
      _volumeCalc->setSource(levelMeasure);
    }
  }
}

TimeSeries::_sp Tank::levelMeasure() {
  return _levelMeasure;
}

void Tank::setHeadMeasure(TimeSeries::_sp head) {
  
  _volumeCalc->resetCache();
  _flowCalc->resetCache();
  
  // base class method first
  Junction::setHeadMeasure(head);
  
  if (!this->headMeasure()) {
    return;
  }
  
  // now hook it up to the "measured" level->volume->flow chain.
  // assumption about elevation units is made here.
  // todo -- revise elevation units handling
  if (head) {
    
    OffsetTimeSeries::_sp offsetHeadMeasure( new OffsetTimeSeries() );
    offsetHeadMeasure->setName(this->name() + ".measure.level");
    offsetHeadMeasure->setSource(head);
    offsetHeadMeasure->setOffset( -(this->elevation()) );
    offsetHeadMeasure->setClock(head->clock());
    offsetHeadMeasure->setUnits(head->units());
    _levelMeasure = offsetHeadMeasure;
    
    _volumeCalc->setClock(head->clock());
    _volumeCalc->setSource(_levelMeasure);
    _flowCalc->setClock(head->clock());
  }
  else {
    // invalidate tank flow timeseries.
    _levelMeasure.reset();
  }

}


TimeSeries::_sp Tank::level() {
  return _level;
}
TimeSeries::_sp Tank::flow() {
  return _flow;
}
TimeSeries::_sp Tank::volume() {
  return _volume;
}

TimeSeries::_sp Tank::inletQuality() {
  return _inletQualityState;
}


TimeSeries::_sp Tank::flowCalc() {
  return _flowCalc;
}
TimeSeries::_sp Tank::volumeCalc() {
  return _volumeCalc;
}





void Tank::setNeedsReset(bool reset) {
  _willResetLevel = reset;
}
bool Tank::needsReset() {
  return _willResetLevel;
}





