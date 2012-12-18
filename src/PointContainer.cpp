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

#define POINTCONTAINER_CACHESIZE 2000;

bool comparePoints(const Point& lhs, const Point& rhs);

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

void PointContainer::hintAtRange(time_t start, time_t end) {
  
}

bool comparePoints(const Point& lhs, const Point& rhs) {
  return lhs.time() < rhs.time();
}

bool PointContainer::isPointAvailable(time_t time) {
  bool isAvailable = false;
  if (time == 0) {
    return false;
  }
  if (time == _cachedPoint.time()) {
    return true;
  }
  
  Point finder(time, 0);
  
  _bufferMutex.lock();
  
  // debugging
  std::cout << "Looking for " << time << " -- points in buffer: " << std::endl;
  BOOST_FOREACH(Point aPoint, _buffer) {
    std::cout << aPoint << endl;
  }
  
  
  PointBuffer_t::iterator it = std::lower_bound(_buffer.begin(), _buffer.end(), finder, comparePoints);
  if (it != _buffer.end() && it->time() == time) {
    isAvailable = true;
    std::cout << "*** FOUND" << std::endl;
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
  
  Point finder(time, 0);
  
  _bufferMutex.lock();
  PointBuffer_t::iterator it = lower_bound(_buffer.begin(), _buffer.end(), finder, comparePoints);
  if (it != _buffer.end() && it->time() == time) {
    foundPoint = makePoint(it);
  }
  _bufferMutex.unlock();
  
  _cachedPoint = foundPoint;
  return foundPoint;
}

Point PointContainer::pointAfter(time_t time) {
  Point foundPoint;
  Point finder(time, 0);
  
  _bufferMutex.lock();
  PointBuffer_t::iterator it = upper_bound(_buffer.begin(), _buffer.end(), finder, comparePoints);
  if (it != _buffer.end()) {
    foundPoint = makePoint(it);
  }
  _bufferMutex.unlock();
  _cachedPoint = foundPoint;
  return foundPoint;
}

Point PointContainer::pointBefore(time_t time) {
  Point foundPoint;
  Point finder(time, 0);
  
  _bufferMutex.lock();
  PointBuffer_t::iterator it  = lower_bound(_buffer.begin(), _buffer.end(), finder, comparePoints);
  if (it != _buffer.end() && it != _buffer.begin()) {
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
  if (point.isValid() && point.time() != 0) {
    _bufferMutex.lock();
    //PointBuffer_t::iterator endPosition = _buffer.end();
    //if (_buffer.size() > 0) {
    //  endPosition--;
    //}
    _buffer.push_back(point);
    _bufferMutex.unlock();
  }
  else {
    cerr << "cannot insert invalid point" << endl;
  }
  
}

void PointContainer::insertPoints(std::vector<Point> points) {
  
  size_t nPoints = points.size();
  if (nPoints > _buffer.capacity()) {
    this->setCacheSize(nPoints);
  }
  _bufferMutex.lock();
  BOOST_FOREACH(Point thePoint, points) {
    _buffer.push_back(thePoint);
  }
  _bufferMutex.unlock();
}

// convenience method for making a new point from a buffer iterator
Point PointContainer::makePoint(PointBuffer_t::iterator iterator) {
  return (*iterator);
}

void PointContainer::reset() {
  _bufferMutex.lock();
  _buffer.clear();
  _bufferMutex.unlock();
}

void PointContainer::setCacheSize(size_t size) {
  _cacheSize = (int)size;
  _bufferMutex.lock();
  _buffer.set_capacity(size);
  _bufferMutex.unlock();
}

