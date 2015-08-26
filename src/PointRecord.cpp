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


bool PointRecord::registerAndGetIdentifierForSeriesWithUnits(std::string recordName, Units units) {
  if (_singlePointCache.find(recordName) == _singlePointCache.end()) {
    _singlePointCache[recordName] = Point();
  }
  return true;
}

const map<string, Units> PointRecord::identifiersAndUnits() {
  return map<string, Units>();
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


std::vector<Point> PointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point> pointVector;
  
  if (startTime == endTime) {
    Point p = this->point(identifier, startTime);
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

PointRecord::time_pair_t PointRecord::range(const string& id) {
  return make_pair(this->firstPoint(id).time, this->lastPoint(id).time); // mostly for subclasses
}


void PointRecord::addPoint(const string& identifier, Point point) {
  // Cache this single point
  
  if (_singlePointCache.find(identifier) != _singlePointCache.end()) {
    _singlePointCache[identifier] = point;
  }
  
}


void PointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  
}


void PointRecord::reset() {
  
}

void PointRecord::reset(const string& identifier) {
  
}


