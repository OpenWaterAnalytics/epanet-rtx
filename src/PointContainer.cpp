//
//  PointContainer.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "PointContainer.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

bool compareTimePointPair(const PointContainer::TimePointPair_t& lhs, const PointContainer::TimePointPair_t& rhs);

PointContainer::PointContainer() {
  _cacheSize = POINTCONTAINER_CACHESIZE;
  _buffer.set_capacity(_cacheSize);
  
}

PointContainer::~PointContainer() {
  
}


std::ostream& RTX::operator<< (std::ostream &out, PointContainer &pointContainer) {
  return pointContainer.toStream(out);
}

std::ostream& PointContainer::toStream(std::ostream &stream) {
  
  stream << "Local memory cache with " << _buffer.size() << " records" << std::endl;
  
  return stream;
}




bool PointContainer::isEmpty() {
  if (this->size() > 0) {
    return false;
  }
  else {
    return true;
  }
}

size_t PointContainer::size() {
  size_t count = PointContainer::numberOfPoints();
  return count;
}





/* overrideable methods */

bool compareTimePointPair(const PointContainer::TimePointPair_t& lhs, const PointContainer::TimePointPair_t& rhs) {
  return lhs.first < rhs.first;
}

bool PointContainer::isPointAvailable(time_t time) {
  bool isAvailable = false;
  
  if (time == _cachedPoint.time()) {
    return true;
  }
  
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  _bufferMutex.lock();
  PointBuffer_t::iterator it = std::lower_bound(_buffer.begin(), _buffer.end(), finder, compareTimePointPair);
  if (it != _buffer.end() && it->first == time) {
    isAvailable = true;
    _cachedPoint = makePoint(it);
  }
  _bufferMutex.unlock();
  return isAvailable;
}

Point PointContainer::findPoint(time_t time) {
  Point foundPoint;
  
  if (time == _cachedPoint.time()) {
    return _cachedPoint;
  }
  
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  _bufferMutex.lock();
  PointBuffer_t::iterator it = lower_bound(_buffer.begin(), _buffer.end(), finder, compareTimePointPair);
  if (it != _buffer.end() && it->first == time) {
    foundPoint = makePoint(it);
  }
  _bufferMutex.unlock();
  
  _cachedPoint = foundPoint;
  return foundPoint;
}

Point PointContainer::pointAfter(time_t time) {
  Point foundPoint;
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  _bufferMutex.lock();
  PointBuffer_t::iterator it = upper_bound(_buffer.begin(), _buffer.end(), finder, compareTimePointPair);
  if (it != _buffer.end()) {
    foundPoint = makePoint(it);
  }
  _bufferMutex.unlock();
  _cachedPoint = foundPoint;
  return foundPoint;
}

Point PointContainer::pointBefore(time_t time) {
  Point foundPoint;
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  _bufferMutex.lock();
  PointBuffer_t::iterator it  = lower_bound(_buffer.begin(), _buffer.end(), finder, compareTimePointPair);
  if (it != _buffer.end()) {
    it--;
    if (it != _buffer.end()) {
      foundPoint = makePoint(it);
    }
  }
  _bufferMutex.unlock();
  _cachedPoint = foundPoint;
  return foundPoint;
}

Point PointContainer::firstPoint() {
  Point foundPoint;
  
  _bufferMutex.lock();
  if (_buffer.size() != 0) {
    foundPoint = makePoint(_buffer.begin());
  }
  _bufferMutex.unlock();
  return foundPoint;
}

Point PointContainer::lastPoint() {
  Point foundPoint;
  
  _bufferMutex.lock();
  if (_buffer.size() != 0) {
    PointBuffer_t::iterator it = _buffer.end();
    it--;
    if (it != _buffer.end()) {
    foundPoint = makePoint(it);
    }
  }
  _bufferMutex.unlock();
  
  return foundPoint;
}

long int PointContainer::numberOfPoints() {
  return _buffer.size();
}


void PointContainer::insertPoint(Point point) {
  double value, confidence;
  time_t time;
  time = point.time();
  value = point.value();
  confidence = point.confidence();
  
  PointPair_t newPoint(value, confidence);
  PointBuffer_t::iterator retIt;
  
  _bufferMutex.lock();
  PointBuffer_t::iterator endPosition = _buffer.end();
  if (_buffer.size() > 0) {
    endPosition--;
  }
  _buffer.push_back(TimePointPair_t(time,newPoint));
  _bufferMutex.unlock();
}

void PointContainer::insertPoints(std::vector<Point> points) {
  _bufferMutex.lock();
  BOOST_FOREACH(Point thePoint, points) {
    _buffer.push_back(TimePointPair_t(thePoint.time(), PointPair_t(thePoint.value(),thePoint.confidence()) ));
  }
  _bufferMutex.unlock();
}

// convenience method for making a new point from a buffer iterator
Point PointContainer::makePoint(PointBuffer_t::iterator iterator) {
  return Point((*iterator).first, (*iterator).second.first, Point::good, (*iterator).second.second);
}

void PointContainer::reset() {
  _bufferMutex.lock();
  _buffer.clear();
  _bufferMutex.unlock();
}

void PointContainer::setCacheSize(int size) {
  _cacheSize = size;
  _bufferMutex.lock();
  _buffer.set_capacity(size);
  _bufferMutex.unlock();
}

int PointContainer::cacheSize() {
  return _cacheSize;
}
