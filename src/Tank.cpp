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
using namespace std;

Tank::Tank(const std::string& name) : Junction(name) {
  setType(TANK);
  // initialize time series states
  _willResetLevel = false;
  _level.reset( new TimeSeries() );
  _level->setName("N " + name + " Level");
  
  _minLevel = 0;
  _maxLevel = 0;
  
  
  _volumeMeasure.reset( new CurveFunction() );
  _volumeMeasure->setUnits(RTX_LITER);
  _volumeMeasure->setName("N " + name + " Calculated Volume");
  
  _flowMeasure.reset( new FirstDerivative() );
  _flowMeasure->setUnits(RTX_LITER_PER_SECOND);
  _flowMeasure->setSource(_volumeMeasure);
  _flowMeasure->setName("N " + name + " Calculated Flow");
  
  _volume.reset( new TimeSeries );
  _volume->setUnits(RTX_LITER);
  _volume->setName("N " + name + " Volume");
  
  _flow.reset( new TimeSeries );
  _flow->setUnits(RTX_LITER_PER_SECOND);
  _flow->setName("N " + name + " Flow");
  
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


void Tank::setGeometry(std::vector<std::pair<double, double> > levelVolumePoints, RTX::Units levelUnits, RTX::Units volumeUnits) {
  
  _geometry = levelVolumePoints;
  _geometryLevelUnits = levelUnits;
  _geometryVolumeUnits = volumeUnits;
  
  _volumeMeasure->setCurve(_geometry);
  _volumeMeasure->setInputUnits(levelUnits);
  _volumeMeasure->setUnits(volumeUnits);
  
}

vector< pair<double,double> > Tank::geometry() {
  return _geometry;
}
pair<Units,Units> Tank::geometryUnits() {
  return make_pair(_geometryLevelUnits, _geometryVolumeUnits);
}



void Tank::setElevation(double elevation) {
  Node::setElevation(elevation);
  
  // do we have a head or level measure??
  OffsetTimeSeries::_sp offsetTs;
  int direction = 1;
  
  if (levelMeasure() || headMeasure()) {
    direction = -1;
    offsetTs = boost::dynamic_pointer_cast<OffsetTimeSeries>(levelMeasure());
    if (!offsetTs) {
      direction = 1;
      offsetTs = boost::dynamic_pointer_cast<OffsetTimeSeries>(headMeasure());
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
    _volumeMeasure->setSource(blank);
  }
  else {
    _volumeMeasure->resetCache();
    _flowMeasure->resetCache();
    
    OffsetTimeSeries::_sp offsetHeadMeasure( new OffsetTimeSeries() );
    offsetHeadMeasure->setName(this->name() + ".measure.head");
    offsetHeadMeasure->setSource(levelMeasure);
    offsetHeadMeasure->setOffset( (this->elevation()) );
  
    Junction::setHeadMeasure(offsetHeadMeasure);
    // test for success:
    if (Junction::headMeasure() == offsetHeadMeasure) {
      _levelMeasure = levelMeasure;
      _volumeMeasure->setSource(levelMeasure);
    }
  }
}

TimeSeries::_sp Tank::levelMeasure() {
  return _levelMeasure;
}

void Tank::setHeadMeasure(TimeSeries::_sp head) {
  
  _volumeMeasure->resetCache();
  _flowMeasure->resetCache();
  
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
    
    _volumeMeasure->setClock(head->clock());
    _volumeMeasure->setSource(_levelMeasure);
    _flowMeasure->setClock(head->clock());
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



TimeSeries::_sp Tank::flowMeasure() {
  return _flowMeasure;
}
TimeSeries::_sp Tank::volumeMeasure() {
  return _volumeMeasure;
}





void Tank::setNeedsReset(bool reset) {
  _willResetLevel = reset;
}
bool Tank::needsReset() {
  return _willResetLevel;
}





