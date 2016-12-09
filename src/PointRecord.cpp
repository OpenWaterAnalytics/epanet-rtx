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

PointRecord::IdentifierUnitsList::IdentifierUnitsList() {
  _d.reset(new map<string,Units>);
}
bool PointRecord::IdentifierUnitsList::hasIdentifierAndUnits(const std::string &identifier, const RTX::Units &units) {
  auto pr = this->doesHaveIdUnits(identifier, units);
  return pr.first && pr.second;
}
pair<bool,bool> PointRecord::IdentifierUnitsList::doesHaveIdUnits(const string& identifier, const Units& units) {
  bool nameExists = false, unitsMatch = false;
  map<string,Units>::const_iterator found = _d->find(identifier);
  if (found != _d->end()) {
    nameExists = true;
    const Units existingUnits = found->second;
    if (existingUnits == units) {
      unitsMatch = true;
    }
  }
  return make_pair(nameExists,unitsMatch);
}


map<string,Units>* PointRecord::IdentifierUnitsList::get() {
  return _d.get();
}
void PointRecord::IdentifierUnitsList::set(const std::string &identifier, const RTX::Units &units) {
  (*_d.get())[identifier] = units;
}
void PointRecord::IdentifierUnitsList::clear() {
  _d.reset(new map<string,Units>);
}
size_t PointRecord::IdentifierUnitsList::count() {
  return _d->size();
}
bool PointRecord::IdentifierUnitsList::empty() {
  return _d->empty();
}


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

PointRecord::IdentifierUnitsList PointRecord::identifiersAndUnits() {
  return IdentifierUnitsList();
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


