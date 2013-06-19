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

bool Pipe::isAlwaysClosed() {
  return (this->fixedStatus() == Pipe::CLOSED) && (this->type() != Element::PUMP);
}

Pipe::direction_t Pipe::assumedFlowDirectionAtNode(Node::sharedPointer node) {
  direction_t dir;
  if (from() == node) {
    dir = outDirection;
  }
  else if (to() == node) {
    dir = inDirection;
  }
  else {
    // should not happen
    dir = unknownDirection;
    std::cerr << "direction could not be found for pipe: " << name() << std::endl;
  }
  return dir;
}

TimeSeries::sharedPointer Pipe::flowMeasure() {
  return _flowMeasure;
}
void Pipe::setFlowMeasure(TimeSeries::sharedPointer flow) {
  _doesHaveFlowMeasure = (flow ? true : false);
  _flowMeasure = flow;
}
