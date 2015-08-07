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
  _fixedStatus = Pipe::OPEN;
}
Pipe::~Pipe() {
  
}

Pipe::status_t Pipe::fixedStatus() {
  return _fixedStatus;
}

void Pipe::setFixedStatus(status_t status) {
  _fixedStatus = status;
}

void Pipe::setRecord(PointRecord::_sp record) {
  _flowState->setRecord(record);
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

TimeSeries::_sp Pipe::flow() {
  return _flowState;
}


TimeSeries::_sp Pipe::statusParameter() {
  return _status;
}
void Pipe::setStatusParameter(TimeSeries::_sp status) {
  _status = status;
}

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


TimeSeries::_sp Pipe::settingParameter() {
  return _setting;
}

void Pipe::setSettingParameter(TimeSeries::_sp setting) {
  _setting = setting;
}
