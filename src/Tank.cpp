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
  _level.reset( new OffsetTimeSeries() );
  _level->setSource(this->head());
  _level->setName(name + " level (offset)");
  
  _minLevel = 0;
  _maxLevel = 0;
  
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
  // re-set the elevation offset for the level timeseries
  _level->setOffset(-elevation);
  _levelMeasure->setOffset(-elevation);
}

void Tank::setLevelMeasure(TimeSeries::_sp levelMeasure) {
  if (!levelMeasure) {
    this->setHeadMeasure(TimeSeries::_sp());
  }
  else {
    OffsetTimeSeries::_sp offsetHeadMeasure( new OffsetTimeSeries() );
    offsetHeadMeasure->setName(this->name() + " offset");
    offsetHeadMeasure->setSource(levelMeasure);
    offsetHeadMeasure->setOffset( (this->elevation()) );
  
    this->setHeadMeasure(offsetHeadMeasure);
  }
}

TimeSeries::_sp Tank::levelMeasure() {
  if (_levelMeasure->source()) {
    return _levelMeasure;
  }
  TimeSeries::_sp blank;
  return blank;
}

void Tank::setHeadMeasure(TimeSeries::_sp head) {
  // base class method first
  Junction::setHeadMeasure(head);
  
  _levelMeasure->resetCache();
  _volumeMeasure->resetCache();
  _flowMeasure->resetCache();
  
  // now hook it up to the "measured" level->volume->flow chain.
  // assumption about elevation units is made here.
  // todo -- revise elevation units handling
  if (head) {
    _levelMeasure->setClock(head->clock());
    _levelMeasure->setUnits(head->units());
    _levelMeasure->setSource(head);
    
    _volumeMeasure->setClock(head->clock());
    _flowMeasure->setClock(head->clock());
  }
  else {
    // invalidate tank flow timeseries.
    TimeSeries::_sp blank;
    _levelMeasure->setSource(blank);
  }

}


TimeSeries::_sp Tank::level() {
  return _level;
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





