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

Pipe::Pipe(const std::string& name, Node::sharedPointer startNode, Node::sharedPointer endNode) : Link(name, startNode, endNode) {
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

void Pipe::setRecord(PointRecord::sharedPointer record) {
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

TimeSeries::sharedPointer Pipe::flow() {
  return _flowState;
}

bool Pipe::doesHaveStatusParameter() {
  return _status ? true : false;
}

TimeSeries::sharedPointer Pipe::statusParameter() {
  return _status;
}
void Pipe::setStatusParameter(TimeSeries::sharedPointer status) {
  _status = status;
}

bool Pipe::doesHaveFlowMeasure() {
  return _flowMeasure ? true : false;
}
TimeSeries::sharedPointer Pipe::flowMeasure() {
  return _flowMeasure;
}
void Pipe::setFlowMeasure(TimeSeries::sharedPointer flow) {
  if (flow == NULL || !flow) {
    _flowMeasure = TimeSeries::sharedPointer();
  }
  else if ( !(flow->units().isSameDimensionAs(RTX_GALLON_PER_MINUTE)) ) {
    return;
  }
  _flowMeasure = flow;
  
}

bool Pipe::doesHaveSettingParameter() {
  return _setting ? true : false;
}

TimeSeries::sharedPointer Pipe::settingParameter() {
  return _setting;
}

void Pipe::setSettingParameter(TimeSeries::sharedPointer setting) {
  _setting = setting;
}
