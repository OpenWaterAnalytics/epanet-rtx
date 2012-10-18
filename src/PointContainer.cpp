//
//  PointContainer.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#include <iostream>

#include "PointContainer.h"

using namespace RTX;

PointContainer::PointContainer() {
  _cacheSize = POINTCONTAINER_CACHESIZE;
  
}

PointContainer::~PointContainer() {
  
}


std::ostream& RTX::operator<< (std::ostream &out, PointContainer &pointContainer) {
  return pointContainer.toStream(out);
}

std::ostream& PointContainer::toStream(std::ostream &stream) {
  
  stream << "Local memory cache with " << _pairMap.size() << " records" << std::endl;
  
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

bool PointContainer::isPointAvailable(time_t time) {
  PairMap_t::iterator it = _pairMap.find(time);
  if (it == _pairMap.end()) {
    return false;
  }
  else {
    return true;
  }
}

Point PointContainer::findPoint(time_t time) {
  PairMap_t::iterator it = _pairMap.find(time);
  if (it == _pairMap.end()) {
    // TODO -- throw something?
    return Point();
  }
  else {
    return makePoint(it);
  }
}

Point PointContainer::pointAfter(time_t time) {
  PairMap_t::iterator it;
  it = _pairMap.upper_bound(time);
  if (it == _pairMap.end()) {
    return Point();
  }
  else {
    return makePoint(it); 
  }
}

Point PointContainer::pointBefore(time_t time) {
  PairMap_t::iterator it;
  it = _pairMap.lower_bound(time);
  if (it == _pairMap.end()) {
    return Point();
  }
  it--;
  if (it == _pairMap.end()) {
    return Point();
  }
  else {
    return makePoint(it); 
  }
}
Point PointContainer::firstPoint() {
  if (this->size() == 0) {
    return Point();
  }
  
  PairMap_t::iterator it;
  it = _pairMap.begin();
  return makePoint(it);
}

Point PointContainer::lastPoint() {
  if (this->size() == 0) {
    return Point();
  }
  PairMap_t::reverse_iterator it;
  it = _pairMap.rbegin();
  time_t time = (*it).first;
  return findPoint(time);
}

long int PointContainer::numberOfPoints() {
  return _pairMap.size();
}


void PointContainer::insertPoint(Point point) {
  double value, confidence;
  time_t time;
  time = point.time();
  value = point.value();
  confidence = point.confidence();
  _pairMap[time] = std::make_pair(value, confidence); // automatically replaces the value if the time slot is already filled
  
  // check cache size, drop points off if needed
  if (size() > cacheSize()) {
    // where did we insert?
    time_t start, end;
    start = _pairMap.begin()->first;
    end = _pairMap.rbegin()->first;
    /* -- cache clearing -- TODO - put this back in!!!
    // if we inserted to the right of middle (more common)...
    if ( time > (start + end)/2 ) {
      // erase by iterator
      //std::cout << "removing from left side" << std::endl;
      _pairMap.erase( _pairMap.begin() );
    }
    else {
      // erase by key
      //std::cout << "removing from right side" << std::endl;
      _pairMap.erase( _pairMap.rbegin()->first );
    }
    */
  }
  
}

void PointContainer::insertPoints(std::vector<Point> points) {
  while (!points.empty()) {
    PointContainer::insertPoint(points.back());
    points.pop_back();
  }
}


Point PointContainer::makePoint(PairMap_t::iterator iterator) {
  // dense line, no? confusing, but it just provides a simpler method for extracting the right information to create a new point from an iterator.
  Point aPoint((*iterator).first, (*iterator).second.first, Point::good, (*iterator).second.second);
  return aPoint;
}

void PointContainer::reset() {
  _pairMap.clear();
}

void PointContainer::setCacheSize(int size) {
  _cacheSize = size;
}

int PointContainer::cacheSize() {
  return _cacheSize;
}
