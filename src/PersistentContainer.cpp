//
//  PersistentContainer.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "PersistentContainer.h"

using namespace RTX;

PersistentContainer::PersistentContainer(const std::string &name, PointRecord::sharedPointer pointRecord) {
  _name = name;
  setPointRecord(pointRecord);
}

PersistentContainer::~PersistentContainer() {
  
}

#pragma mark - Public Methods

void PersistentContainer::setPointRecord(PointRecord::sharedPointer pointRecord) {
  _pointRecord = pointRecord;
  _id = _pointRecord->registerAndGetIdentifier(_name);
}

PointRecord::sharedPointer PersistentContainer::pointRecord() {
  return _pointRecord;
}

void PersistentContainer::hintAtRange(time_t start, time_t end) {
  // assuming there's a cache already, find missing sections and fill them in.
  if (PointContainer::firstPoint().isValid() && PointContainer::lastPoint().isValid()) {
    time_t cacheStart = PointContainer::firstPoint().time();
    time_t cacheEnd = PointContainer::lastPoint().time();
    
    // fill in leading points
    if (start < cacheStart) {
      PointContainer::insertPoints(_pointRecord->pointsInRange(_id, start, RTX_MIN(cacheStart, end)));
    }
    // fill in trailing points
    if (end > cacheEnd) {
      PointContainer::insertPoints(_pointRecord->pointsInRange(_id, RTX_MAX(cacheEnd, start), end));
    }
  }
  else {
    PointContainer::insertPoints(_pointRecord->pointsInRange(_id, start, end));
  }
}

void PersistentContainer::hintAtBulkInsertion(time_t start, time_t end) {
  
}

bool PersistentContainer::isPointAvailable(time_t time) {
  if (PointContainer::isPointAvailable(time)) {
    return true;
  }
  else {
    bool isAvailable = _pointRecord->isPointAvailable(_id, time);
    if (isAvailable) {
      Point newPoint = _pointRecord->point(_id, time);
      PointContainer::insertPoint(newPoint);
    }
    return isAvailable;
  }
}

Point PersistentContainer::findPoint(time_t time) {
  if (isPointAvailable(time)) {
    return PointContainer::findPoint(time);
  }
  else {
    return _pointRecord->point(_id, time);
  }
}

Point PersistentContainer::pointAfter(time_t time) {
  if (PointContainer::pointAfter(time).isValid()) {
    return PointContainer::pointAfter(time);
  }
  else {
    Point aPoint = _pointRecord->pointAfter(_id, time);
    if (aPoint.isValid()) {
      PointContainer::insertPoint(aPoint);
    }
    return aPoint;
  }
}

Point PersistentContainer::pointBefore(time_t time) {
  if (PointContainer::pointBefore(time).isValid()) {
    return PointContainer::pointBefore(time);
  }
  else {
    Point aPoint = _pointRecord->pointBefore(_id, time);
    if (aPoint.isValid()) {
      PointContainer::insertPoint(aPoint);
    }
    return aPoint;
  }
}

void PersistentContainer::insertPoint(Point point) {
  PointContainer::insertPoint(point);
  _pointRecord->addPoint(_id, point);
}

void PersistentContainer::insertPoints(std::vector<Point> points) {
  PointContainer::insertPoints(points);
  _pointRecord->addPoints(_id, points);
}

#pragma mark - Protected Methods

std::ostream& PersistentContainer::toStream(std::ostream &stream) {
  stream << "Persistent Container: " << _name << " (id: " << _id << ")\n";
  return stream;
}
