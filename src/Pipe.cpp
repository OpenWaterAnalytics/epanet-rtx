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
  _doesHaveFlowMeasure = false;
  _doesHaveStatusParameter = false;
  _doesHaveSettingParameter = false;
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
  return _doesHaveStatusParameter;
}

TimeSeries::sharedPointer Pipe::statusParameter() {
  return _status;
}
void Pipe::setStatusParameter(TimeSeries::sharedPointer status) {
  _doesHaveStatusParameter = (status ? true : false);
  _status = status;
}

bool Pipe::doesHaveFlowMeasure() {
  return _doesHaveFlowMeasure;
}
TimeSeries::sharedPointer Pipe::flowMeasure() {
  return _flowMeasure;
}
void Pipe::setFlowMeasure(TimeSeries::sharedPointer flow) {
  if (flow && !flow->units().isSameDimensionAs(RTX_LITER_PER_SECOND)) {
    // not units of flow
    return;
  }
  _doesHaveFlowMeasure = (flow ? true : false);
  _flowMeasure = flow;
}

bool Pipe::doesHaveSettingParameter() {
  return _doesHaveSettingParameter;
}

TimeSeries::sharedPointer Pipe::settingParameter() {
  return _setting;
}

void Pipe::setSettingParameter(TimeSeries::sharedPointer setting) {
  _doesHaveSettingParameter = (setting ? true : false);
  _setting = setting;
}
