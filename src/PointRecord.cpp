//
//  PointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "PointRecord.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;


PointRecord::PointRecord() : _name("") {
}


std::string PointRecord::name() {
  return _name;
}

void PointRecord::setName(std::string name) {
  _name = name;
}


std::ostream& RTX::operator<< (std::ostream &out, PointRecord &pr) {
  return pr.toStream(out);
}

std::ostream& PointRecord::toStream(std::ostream &stream) {
  stream << "Point Record" << std::endl;
  return stream;
}


std::string PointRecord::registerAndGetIdentifier(std::string recordName, Units dataUnits) {
  
  if (_pointCache.find(recordName) == _pointCache.end()) {
    _pointCache[recordName] = Point();
  }
  
  return recordName;
}

std::vector<std::string> PointRecord::identifiers() {
  
  vector<string> names;
  
  return names;
}

/*
bool PointRecord::isPointAvailable(const string& identifier, time_t time) {
  
  return false;
}
*/

Point PointRecord::point(const string& identifier, time_t time) {
  // return the cached point if it is valid
  
  if (_cachedPointId == identifier && _cachedPoint.time == time) {
    return _cachedPoint;
  }
  
  if (_pointCache.find(identifier) != _pointCache.end()) {
    Point p = _pointCache[identifier];
    if (p.time == time) {
      return p;
    }
  }
  
  return Point();
  
}


Point PointRecord::pointBefore(const string& identifier, time_t time) {
  return Point();
}


Point PointRecord::pointAfter(const string& identifier, time_t time) {
  return Point();
}


std::vector<Point> PointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point> pointVector;
  return pointVector;
}

Point PointRecord::firstPoint(const string &id) {
  return Point();
}

Point PointRecord::lastPoint(const string &id) {
  return Point();
}

PointRecord::time_pair_t PointRecord::range(const string& id) {
  return make_pair(this->firstPoint(id).time, this->lastPoint(id).time); // mostly for subclasses
}


void PointRecord::addPoint(const string& identifier, Point point) {
  // Cache this single point
  _cachedPointId = identifier;
  _cachedPoint = point;
  
  if (_pointCache.find(identifier) != _pointCache.end()) {
    _pointCache[identifier] = point;
  }
  
}


void PointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  
}


void PointRecord::reset() {
  
}

void PointRecord::reset(const string& identifier) {
  
}


