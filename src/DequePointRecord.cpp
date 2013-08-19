//
//  PointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "DequePointRecord.h"
#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>

using namespace RTX;
using namespace std;


DequePointRecord::DequePointRecord() {
}

std::ostream& RTX::operator<< (std::ostream &out, DequePointRecord &pr) {
  return pr.toStream(out);
}

std::ostream& DequePointRecord::toStream(std::ostream &stream) {
  stream << "Deque Point Record" << std::endl;
  return stream;
}


std::string DequePointRecord::registerAndGetIdentifier(std::string recordName, Units dataUnits) {
  
  // check to see if it's there first
  if (_points.find(recordName) == _points.end()) {
    pointQ_t newQueue;
    _points.insert(make_pair(recordName, newQueue));
  }
  
  return recordName;
}

std::vector<std::string> DequePointRecord::identifiers() {
  
  vector<string> names;
  BOOST_FOREACH(string name, _points | boost::adaptors::map_keys) {
    names.push_back(name);
  }
  return names;
}


Point DequePointRecord::point(const string& identifier, time_t time) {
  if (_cachedPoint.time == time && RTX_STRINGS_ARE_EQUAL_CS(_cachedPointId, identifier) ) {
    return _cachedPoint;
  }
  
  const pointQ_t& q = pointQueueWithKeyName(identifier);
  
  // find the point in time
  Point finder(time,0);
  pointQ_t::const_iterator qIt = std::lower_bound(q.begin(), q.end(), finder, &Point::comparePointTime);
  
  // no point found
  if (qIt == q.end()) {
    return Point();
  }
  
  // found point, cache it.
  _cachedPoint = (*qIt);
  _cachedPointId = identifier;
  
  // times don't match, so NO
  if (qIt->time != time ) {
    return Point();
    //return pointBefore(identifier, time);
  }
  else {
    return _cachedPoint;
  }
  
}

/*
Point DequePointRecord::point(const string& identifier, time_t time) {
  
  if (isPointAvailable(identifier, time)) {
    return _cachedPoint;
  }
  
  else {
    return pointBefore(identifier, time);
  }
}
*/

Point DequePointRecord::pointBefore(const string& identifier, time_t time) {
  
  pointQ_t& q = pointQueueWithKeyName(identifier);
  
  Point finder(time,0);
  pointQ_t::iterator qIt = std::lower_bound(q.begin(), q.end(), finder, &Point::comparePointTime);
  
  while (qIt != q.begin() && qIt != q.end() && (*qIt).time >= time) {
    --qIt;
  }
  
  if (qIt->time < time) {
    Point p = (*qIt);
    
    if (p.time < 0) {
      cerr << "whoops" << endl;
    }
    
    return p;
  }
  else {
    return Point();
  }
  
}


Point DequePointRecord::pointAfter(const string& identifier, time_t time) {
  const pointQ_t& q = pointQueueWithKeyName(identifier);
  
  Point finder(time,0);
  pointQ_t::const_iterator qIt = std::upper_bound(q.begin(), q.end(), finder, &Point::comparePointTime);
  
  if (qIt->time > time) {
    return (*qIt);
  }
  else {
    return Point();
  }
}


std::vector<Point> DequePointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point> pointVector;
  
  const pointQ_t& q = pointQueueWithKeyName(identifier);
  
  Point finder(startTime,0);
  pointQ_t::const_iterator qIt = std::lower_bound(q.begin(), q.end(), finder, &Point::comparePointTime);
  
  while (qIt != q.end() && qIt->time < endTime) {
    pointVector.push_back(*qIt);
    ++qIt;
  }

  
  return pointVector;
}

Point DequePointRecord::firstPoint(const string &id) {
  Point p;
  const pointQ_t& q = pointQueueWithKeyName(id);
  
  if (q.empty()) {
    return p;
  }
  
  p = q.front();
  
  return p;
}

Point DequePointRecord::lastPoint(const string &id) {
  Point p;
  const pointQ_t& q = pointQueueWithKeyName(id);
  
  if (q.empty()) {
    return p;
  }
  
  p = q.back();
  
  return p;
}


void DequePointRecord::addPoint(const string& identifier, Point point) {
  pointQ_t &q = pointQueueWithKeyName(identifier);
  
  q.push_back(point);
  std::sort(q.begin(), q.end(), &Point::comparePointTime);
  
  
}


void DequePointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  
  if (points.size() == 0) {
    return;
  }
  // adding new range. clear old cache.
  DequePointRecord::reset(identifier);
  
  BOOST_FOREACH(Point thePoint, points) {
    DequePointRecord::addPoint(identifier, thePoint);
  }
}


void DequePointRecord::reset() {
  cout << "warning: resetting entire point record" << endl;
  typedef keyedPointVector_t::value_type& keyedPointVecValue;
  BOOST_FOREACH(keyedPointVecValue pointMapValue, _points) {
    pointMapValue.second.clear();
  }
}

void DequePointRecord::reset(const string& identifier) {
  pointQ_t &q = pointQueueWithKeyName(identifier);
  q.clear();
}



DequePointRecord::pointQ_t& DequePointRecord::pointQueueWithKeyName(const std::string &name) {
  keyedPointVector_t::iterator it = _points.find(name);
  pointQ_t &pq = it->second;
  return pq;
}
