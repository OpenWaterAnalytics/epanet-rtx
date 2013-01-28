//
//  PointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "PointRecord.h"
#include "boost/foreach.hpp"

using namespace RTX;
using namespace std;

PointRecord::PointRecord() {
  _nextKey = 0;
  _connectionString = "";
}

std::ostream& RTX::operator<< (std::ostream &out, PointRecord &pr) {
  return pr.toStream(out);
}

std::ostream& PointRecord::toStream(std::ostream &stream) {
  stream << "Point Record - connection: " << this->connectionString() << std::endl;
  return stream;
}

void PointRecord::setConnectionString(const std::string& connection) {
  _connectionString = connection;
}
const std::string& PointRecord::connectionString() {
  return _connectionString;
}


std::string PointRecord::registerAndGetIdentifier(std::string recordName) {
  // register the recordName internally and generate a unique key identifier
  
  // check to see if it's there first
  if (_keys.find(recordName) == _keys.end()) {
    _keys[recordName] = _nextKey;
    _names[_nextKey] = recordName;
    pointMap_t emptyPointMap;
    _points.insert(std::make_pair(_nextKey, emptyPointMap));
    _nextKey++;
  }
  
  return recordName;
}

int PointRecord::identifierForName(std::string recordName) {
  if (_keys.find(recordName) != _keys.end()) {
    return _keys[recordName];
  }
  else return -1;
}

string PointRecord::nameForIdentifier(int identifier) {
  if (_names.find(identifier) != _names.end()) {
    return _names[identifier];
  }
  else return string("");
}

std::vector<std::string> PointRecord::identifiers() {
  typedef std::map< int, std::string >::value_type& nameMapValue_t;
  vector<string> names;
  BOOST_FOREACH(nameMapValue_t name, _names) {
    names.push_back(name.second);
  }
  return names;
}


void PointRecord::hintAtRange(const string& identifier, time_t start, time_t end) {
  
}



bool PointRecord::isPointAvailable(const string& identifier, time_t time) {
  if (_cachedPoint.time() == time && RTX_STRINGS_ARE_EQUAL(_cachedPointId, identifier) ) {
    return true;
  }
  
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return false;
  }
  
  pointMap_t pointMap = (*it).second;
  pointMap_t::iterator pointIt = pointMap.find(time);
  if (pointIt == pointMap.end()) {
    return false;
  }
  else {
    _cachedPointId = identifier;
    _cachedPoint = (*pointIt).second;
    return true;
  }
  
}


Point PointRecord::point(const string& identifier, time_t time) {
  
  if (_cachedPoint.time() == time && RTX_STRINGS_ARE_EQUAL(_cachedPointId, identifier) ) {
    return _cachedPoint;
  }
  
  if (isPointAvailable(identifier, time)) {
    return _points[identifierForName(identifier)][time];
  }
  else {
    return pointBefore(identifier, time);
  }
}


Point PointRecord::pointBefore(const string& identifier, time_t time) {
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return Point();
  }
  pointMap_t pointMap = (*it).second;
  pointMap_t::iterator pointIt = pointMap.lower_bound(time);
  if (pointIt == pointMap.end()) {
    return Point();
  }
  else {
    pointIt--;
    if (pointIt == pointMap.end()) {
      return Point();
    }
    else {
      return (*pointIt).second;
    }
  }
}


Point PointRecord::pointAfter(const string& identifier, time_t time) {
  int id = identifierForName(identifier);
  keyedPointMap_t::iterator it = _points.find(id);
  if (it == _points.end()) {
    return Point();
  }
  pointMap_t pointMap = (*it).second;
  pointMap_t::iterator pointIt = pointMap.upper_bound(time);
  if (pointIt == pointMap.end()) {
    return Point();
  }
  else {
    return (*pointIt).second;
  }
}


std::vector<Point> PointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point> pointVector;
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return pointVector;
  }
  pointMap_t pointMap = (*it).second;
  pointMap_t::iterator pointIt = pointMap.lower_bound(startTime);
  while (pointIt != pointMap.end() && (*pointIt).second.time() <= endTime) {
    pointVector.push_back((*pointIt).second);
  }
  return pointVector;
}


void PointRecord::addPoint(const string& identifier, Point point) {
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return;
  }
  
  if (it->second.find(point.time()) != it->second.end()) {
    cerr <<  "overwriting point in " << identifier << " :: " << point << endl;
  }
  it->second[point.time()] = point;
  
}


void PointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  BOOST_FOREACH(Point thePoint, points) {
    addPoint(identifier, thePoint);
  }
}


void PointRecord::reset() {
  typedef keyedPointMap_t::value_type& keyedPointMapValue;
  BOOST_FOREACH(keyedPointMapValue pointMapValue, _points) {
    pointMapValue.second.clear();
  }
}



