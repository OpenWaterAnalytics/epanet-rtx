//
//  Pipe.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "Pipe.h"

using namespace RTX;

Pipe::Pipe(const std::string& name, Node::_sp startNode, Node::_sp endNode) : Link(name, startNode, endNode) {
  setType(PIPE);
  _flowState.reset( new TimeSeries() );
  _flowState->setUnits(RTX_LITER_PER_SECOND);
  _flowState->setName("L " + name + " flow");
  
  _setting.reset( new TimeSeries() );
  _setting->setUnits(RTX_DIMENSIONLESS);
  _setting->setName("L " + name + " setting");
  
  _status.reset( new TimeSeries() );
  _status->setUnits(RTX_DIMENSIONLESS);
  _status->setName("L " + name + " status");
  
  _fixedStatus = Pipe::OPEN;
  
  state_flow    = 0.;
  state_setting = 0.;
  state_status  = 0.;
}

Pipe::~Pipe() {
  
}

void Pipe::setRecord(PointRecord::_sp record) {
  _flowState->setRecord(record);
  _setting->setRecord(record);
  _status->setRecord(record);
}


#pragma mark Physical Properties

Pipe::status_t Pipe::fixedStatus() {
  return _fixedStatus;
}

void Pipe::setFixedStatus(status_t status) {
  _fixedStatus = status;
}

double Pipe::diameter() {
  return _diameter;
}

void Pipe::setDiameter(double diameter) {
  _diameter = diameter;
}

double Pipe::length() {
  return _length;
}

void Pipe::setLength(double length) {
  _length = length;
}

double Pipe::roughness() {
  return _roughness;
}
void Pipe::setRoughness(double roughness) {
  _roughness = roughness;
}

#pragma mark States

TimeSeries::_sp Pipe::flow() {
  return _flowState;
}
TimeSeries::_sp Pipe::status() {
  return _status;
}
TimeSeries::_sp Pipe::setting() {
  return _setting;
}


#pragma mark Boundaries

TimeSeries::_sp Pipe::statusBoundary() {
  return _statusBoundary;
}
void Pipe::setStatusBoundary(TimeSeries::_sp status) {
  _statusBoundary = status;
}

TimeSeries::_sp Pipe::settingBoundary() {
  return _settingBoundary;
}

void Pipe::setSettingBoundary(TimeSeries::_sp setting) {
  _settingBoundary = setting;
}


#pragma mark Measures

TimeSeries::_sp Pipe::flowMeasure() {
  return _flowMeasure;
}
void Pipe::setFlowMeasure(TimeSeries::_sp flow) {
  if (flow == NULL || !flow) {
    _flowMeasure = TimeSeries::_sp();
  }
  else if ( !(flow->units().isSameDimensionAs(RTX_GALLON_PER_MINUTE)) ) {
    return;
  }
  _flowMeasure = flow;
}

