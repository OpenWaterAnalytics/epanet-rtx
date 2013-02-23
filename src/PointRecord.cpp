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

bool compareTimePointPair(const PointRecord::TimePointPair_t& lhs, const PointRecord::TimePointPair_t& rhs);

PointRecord::PointRecord() {
  _name = "";
  _connectionString = "";
  _defaultCapacity = 1000;
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
  // register the recordName internally and generate a buffer and mutex
  
  // check to see if it's there first
  if (_keyedBufferMutex.find(recordName) == _keyedBufferMutex.end()) {
    PointBuffer_t buffer;
    buffer.set_capacity(_defaultCapacity);
    // shared pointer since it's not copy-constructable.
    boost::shared_ptr< boost::signals2::mutex >mutexPtr(new boost::signals2::mutex );
    BufferMutexPair_t bmPair(buffer, mutexPtr);
    _keyedBufferMutex.insert(make_pair(recordName, bmPair));
  }
  
  return recordName;
}




std::vector<std::string> PointRecord::identifiers() {
  typedef std::map<std::string, BufferMutexPair_t >::value_type& nameMapValue_t;
  vector<string> names;
  BOOST_FOREACH(nameMapValue_t name, _keyedBufferMutex) {
    names.push_back(name.first);
  }
  return names;
}


void PointRecord::preFetchRange(const string& identifier, time_t start, time_t end) {
  
}

bool compareTimePointPair(const PointRecord::TimePointPair_t& lhs, const PointRecord::TimePointPair_t& rhs) {
  return lhs.first < rhs.first;
}

// convenience method for making a new point from a buffer iterator
Point PointRecord::makePoint(PointBuffer_t::iterator iterator) {
  return Point((*iterator).first, (*iterator).second.first, Point::good, (*iterator).second.second);
}

bool PointRecord::isPointAvailable(const string& identifier, time_t time) {
  // quick check for repeated calls
  if (_cachedPoint.time() == time && RTX_STRINGS_ARE_EQUAL(_cachedPointId, identifier) ) {
    return true;
  }
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it == _keyedBufferMutex.end()) {
    // nobody here by that name
    return false;
  }
  
  // get the constituents
  boost::signals2::mutex *mutex = (it->second.second.get());
  PointBuffer_t *buffer = &(it->second.first);
  
  // lock the buffer
  mutex->lock();
  
  // search the buffer
  bool isAvailable = false;
  TimePointPair_t finder(time, PointPair_t(0,0));
  PointBuffer_t::iterator pbIt = std::lower_bound((*buffer).begin(), (*buffer).end(), finder, compareTimePointPair);
  if (pbIt != (*buffer).end() && pbIt->first == time) {
    isAvailable = true;
    _cachedPoint = makePoint(pbIt);
  }
  
  // ok, all done
  mutex->unlock();
  
  return isAvailable;
  
}


Point PointRecord::point(const string& identifier, time_t time) {
  // calling isPointAvailable pushes the point into _cachedPoint (if true)
  if (isPointAvailable(identifier, time)) {
    return _cachedPoint;
  }
  
  else {
    return pointBefore(identifier, time);
  }
}


Point PointRecord::pointBefore(const string& identifier, time_t time) {
  
  Point foundPoint;
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t *buffer = &(it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::iterator it  = lower_bound((*buffer).begin(), (*buffer).end(), finder, compareTimePointPair);
    if (it != (*buffer).end() && it != (*buffer).begin()) {
      if (--it != (*buffer).end()) {
        foundPoint = makePoint(it);
      }
    }
    
    // all done.
    mutex->unlock();
  }
  
  return foundPoint;
}


Point PointRecord::pointAfter(const string& identifier, time_t time) {
  
  Point foundPoint;
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t *buffer = &(it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::iterator it = upper_bound((*buffer).begin(), (*buffer).end(), finder, compareTimePointPair);
    if (it != (*buffer).end()) {
      foundPoint = makePoint(it);
    }
    
    // all done.
    mutex->unlock();
  }
  
  return foundPoint;
}


std::vector<Point> PointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  
  std::vector<Point> pointVector;
  
  TimePointPair_t finder(startTime, PointPair_t(0,0));
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t *buffer = &(it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::iterator it = upper_bound((*buffer).begin(), (*buffer).end(), finder, compareTimePointPair);
    while ( (it != (*buffer).end()) && (it->first < endTime) ) {
      Point newPoint = makePoint(it);
      pointVector.push_back(newPoint);
      it++;
    }
    
    // all done.
    mutex->unlock();
  }
  
  return pointVector;
}


void PointRecord::addPoint(const string& identifier, Point point) {
  
  double value, confidence;
  time_t time;
  
  time = point.time();
  value = point.value();
  confidence = point.confidence();
  
  PointPair_t newPoint(value, confidence);
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t *buffer = &(it->second.first);
    // lock the buffer
    mutex->lock();
    
    if (time > PointRecord::lastPoint(identifier).time()) {
      // end of the buffer
      (*buffer).push_back(TimePointPair_t(time,newPoint));
    }
    else if (time < PointRecord::firstPoint(identifier).time()) {
      // front of the buffer
      (*buffer).push_front(TimePointPair_t(time,newPoint));
    }
    
    
    // all done.
    mutex->unlock();
  }
  
}


void PointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  if (points.size() == 0) {
    return;
  }
  // check the cache size, and upgrade if needed.
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    PointBuffer_t *buffer = &(it->second.first);
    size_t capacity = (*buffer).capacity();
    if (capacity < points.size()) {
      (*buffer).set_capacity(points.size());
    }
    
    // figure out the insert order...
    // if the set we're inserting has to be prepended to the buffer...
    
    Point firstCachePoint = PointRecord::firstPoint(identifier);
    Point lastCachePoint = PointRecord::lastPoint(identifier);
    
    time_t firstTime = firstCachePoint.time();
    time_t lastTime = lastCachePoint.time();
    
    time_t insertFirst = points.front().time();
    time_t insertLast = points.back().time();
    
    if (insertFirst > insertLast) {
      // wha!
      cerr << "check time ordering!" << endl;
      return;
    }
    
    if (insertLast > lastTime) {
      // insert onto end.
      vector<Point>::const_iterator pIt = points.begin();
      while (pIt != points.end()) {
        
        if (pIt->time() > lastTime) {
          PointRecord::addPoint(identifier, *pIt);
        }
        
        ++pIt;
      }
    }
    else if (insertFirst < firstTime) {
      // insert onto front (reverse iteration)
      vector<Point>::const_reverse_iterator pIt = points.rbegin();
      while (pIt != points.rend()) {
        
        // skip overlapping points.
        if (pIt->time() < firstTime) {
          PointRecord::addPoint(identifier, *pIt);
        }
        // else { /* skip */ }
        
        ++pIt;
      }
    }
    else {
      // some overlap?
      cout << "fully overlapping condition" << endl;
    }
  }
}


void PointRecord::reset() {
  _keyedBufferMutex.clear();
}

void PointRecord::reset(const string& identifier) {
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    PointBuffer_t *buffer = &(it->second.first);
    (*buffer).clear();
  }
  
}



Point PointRecord::firstPoint(const string& id) {
  Point foundPoint;
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(id);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t *buffer = &(it->second.first);
    
    if ((*buffer).empty()) {
      return foundPoint;
    }
    
    TimePointPair_t tpPair = (*buffer).front();
    foundPoint = Point(tpPair.first, tpPair.second.first);
    
  }
  return foundPoint;
}

Point PointRecord::lastPoint(const string& id) {
  Point foundPoint;
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(id);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t *buffer = &(it->second.first);
    
    if ((*buffer).empty()) {
      return foundPoint;
    }
    
    TimePointPair_t tpPair = (*buffer).back();
    foundPoint = Point(tpPair.first, tpPair.second.first);
    
  }
  return foundPoint;
}

std::string PointRecord::name() {
  return _name;
}

void PointRecord::setName(const std::string& name) {
  _name = name;
}

