//
//  PersistentMap.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/12/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>

#include "PointContainer.h"

using namespace RTX;

PersistentMap::PersistentMap(const std::string &name, PointRecord::sharedPointer pointRecord) {
  _name = name;
  setPointRecord(pointRecord);
}

PersistentMap::~PersistentMap() {
  
}

// new public methods for derived class
void PersistentMap::setPointRecord(PointRecord::sharedPointer pointRecord) {
  _id = pointRecord->registerAndGetIdentifier(_name);
  _pointRecord = pointRecord;
}

PointRecord::sharedPointer PersistentMap::getPointRecord() {
  return _pointRecord;
}

// access functions used by the iterator -- derived classes must override these
Point::sharedPointer PersistentMap::findPoint(time_t time) {
  
}

Point::sharedPointer PersistentMap::pointAfter(time_t time) {
  return _pointRecord->pointAfter(_id, time);
}

Point::sharedPointer PersistentMap::pointBefore(time_t time) {
  return _pointRecord->pointBefore(_id, time);
}

Point::sharedPointer PersistentMap::firstPoint() {
  
}

Point::sharedPointer PersistentMap::lastPoint() {
  
}

long int PersistentMap::numberOfPoints() {
  
}
