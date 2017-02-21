//
//  PointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "PointRecord.h"

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


bool PointRecord::registerAndGetIdentifierForSeriesWithUnits(std::string recordName, Units units) {
  
  _idsCache.set(recordName, units);
  
  if (_singlePointCache.find(recordName) == _singlePointCache.end()) {
    _singlePointCache[recordName] = Point();
  }
  return true;
}

IdentifierUnitsList PointRecord::identifiersAndUnits() {
  return _idsCache;
}

bool PointRecord::exists(const std::string &name, const RTX::Units &units) {
  IdentifierUnitsList avail = this->identifiersAndUnits();
  return avail.hasIdentifierAndUnits(name, units);
}


Point PointRecord::point(const string& identifier, time_t time) {
  // return the cached point if it is valid
  
  if (_singlePointCache.find(identifier) != _singlePointCache.end()) {
    Point p = _singlePointCache[identifier];
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


std::vector<Point> PointRecord::pointsInRange(const string& identifier, TimeRange range) {
  std::vector<Point> pointVector;
  
  if (range.duration() == 0) {
    Point p = this->point(identifier, range.start);
    if (p.isValid) {
      pointVector.push_back(p);
    }
  }
  
  return pointVector;
}

Point PointRecord::firstPoint(const string &id) {
  return Point();
}

Point PointRecord::lastPoint(const string &id) {
  return Point();
}

TimeRange PointRecord::range(const string& id) {
  return TimeRange(this->firstPoint(id).time, this->lastPoint(id).time); // mostly for subclasses
}


void PointRecord::addPoint(const string& identifier, Point point) {
  // Cache this single point
  _singlePointCache[identifier] = point;
}


void PointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  
}


void PointRecord::reset() {
  
}

void PointRecord::reset(const string& identifier) {
  
}


