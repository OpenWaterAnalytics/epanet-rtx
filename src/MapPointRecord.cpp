//
//  MapPointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "MapPointRecord.h"
#include "boost/foreach.hpp"

using namespace RTX;
using namespace std;

MapPointRecord::MapPointRecord() {
  _nextKey = 0;
  _connectionString = "";
}

std::ostream& RTX::operator<< (std::ostream &out, MapPointRecord &pr) {
  return pr.toStream(out);
}

std::ostream& MapPointRecord::toStream(std::ostream &stream) {
  stream << "Map Point Record" << std::endl;
  return stream;
}


std::string MapPointRecord::registerAndGetIdentifier(std::string recordName, Units dataUnits) {
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

int MapPointRecord::identifierForName(std::string recordName) {
  if (_keys.find(recordName) != _keys.end()) {
    return _keys[recordName];
  }
  else return -1;
}

string MapPointRecord::nameForIdentifier(int identifier) {
  if (_names.find(identifier) != _names.end()) {
    return _names[identifier];
  }
  else return string("");
}

std::vector<std::string> MapPointRecord::identifiers() {
  typedef std::map< int, std::string >::value_type& nameMapValue_t;
  vector<string> names;
  BOOST_FOREACH(nameMapValue_t name, _names) {
    names.push_back(name.second);
  }
  return names;
}

/*
bool MapPointRecord::isPointAvailable(const string& identifier, time_t time) {
  if (_cachedPoint.time == time && RTX_STRINGS_ARE_EQUAL_CS(_cachedPointId, identifier) ) {
    return true;
  }
  
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return false;
  }
  
  pointMap_t &pointMap = (*it).second;
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
*/

Point MapPointRecord::point(const string& identifier, time_t time) {
  Point p;
  
  p = PointRecord::point(identifier,time);
  if (p.isValid) {
    return p;
  }
  
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return Point();
  }
  else {
    pointMap_t &pointMap = (*it).second;
    pointMap_t::iterator pointIt = pointMap.find(time);
    if (pointIt == pointMap.end()) {
      return Point();
    }
    else {
      PointRecord::addPoint(identifier, (*pointIt).second);
      return (*pointIt).second;
    }
  }
  
  if (!p.isValid) {
    //return pointBefore(identifier, time);
    return p;
  }
}


Point MapPointRecord::pointBefore(const string& identifier, time_t time) {
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


Point MapPointRecord::pointAfter(const string& identifier, time_t time) {
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


std::vector<Point> MapPointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point> pointVector;
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return pointVector;
  }
  pointMap_t pointMap = (*it).second;
  pointMap_t::iterator pointIt = pointMap.lower_bound(startTime);
  while (pointIt != pointMap.end() && (*pointIt).second.time <= endTime) {
    pointVector.push_back((*pointIt).second);
    ++pointIt;
  }
  return pointVector;
}

Point MapPointRecord::firstPoint(const string &id) {
  Point p;
  keyedPointMap_t::iterator it = _points.find(identifierForName(id));
  if (it == _points.end()) {
    return p;
  }
  
  pointMap_t pm = (*it).second;
  pointMap_t::iterator rIt = pm.begin();
  if (rIt != pm.end()) {
    p = (*rIt).second;
  }
  
  
  
  return p;
}

Point MapPointRecord::lastPoint(const string &id) {
  Point p;
  keyedPointMap_t::iterator it = _points.find(identifierForName(id));
  if (it == _points.end()) {
    return p;
  }
  
  pointMap_t pm = (*it).second;
  pointMap_t::reverse_iterator rIt = pm.rbegin();
  if (rIt != pm.rend()) {
    p = (*rIt).second;
  }
  
  
  
  return p;
}


void MapPointRecord::addPoint(const string& identifier, Point point) {
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return;
  }
  /* todo -- compile-time logging info
  if (it->second.find(point.time) != it->second.end()) {
    cerr <<  "overwriting point in " << identifier << " :: " << point << endl;
  }
   */
  it->second[point.time] = point;
  
}


void MapPointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  BOOST_FOREACH(Point thePoint, points) {
    MapPointRecord::addPoint(identifier, thePoint);
  }
}


void MapPointRecord::reset() {
  typedef keyedPointMap_t::value_type& keyedPointMapValue;
  BOOST_FOREACH(keyedPointMapValue pointMapValue, _points) {
    pointMapValue.second.clear();
  }
}

void MapPointRecord::reset(const string& identifier) {
  keyedPointMap_t::iterator it = _points.find(identifierForName(identifier));
  if (it == _points.end()) {
    return;
  }
  it->second.clear();
}



