//
//  PointContainer.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See Readme.txt and license.txt for more information
//  https://sourceforge.net/projects/epanet-rtx/

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
  
  // print all points in the map...
  /*for (PointMap_t::iterator it = _points.begin(); it != _points.end(); ++it) {
    stream << *((*it).second) << "\n";
  }*/
  
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
  /*
  PointMap_t::iterator it;
  it = _points.find(time);
  if (it == _points.end()) {
    // the point does not exist
    return false;
  }
  else {
    // the point does exist
    return true;
  }
   */
  PairMap_t::iterator it = _pairMap.find(time);
  if (it == _pairMap.end()) {
    return false;
  }
  else {
    return true;
  }
}

Point::sharedPointer PointContainer::findPoint(time_t time) {
  /*
  PointMap_t::iterator it;
  it = _points.find(time);
  if (it == _points.end()) {
    // TODO -- throw something? check if the point exists first!
    return Point::sharedPointer(new Point());
  }
  else {
    return (*it).second;
  }
   */
  PairMap_t::iterator it = _pairMap.find(time);
  if (it == _pairMap.end()) {
    // TODO -- throw something?
    return Point::sharedPointer();
  }
  else {
    return makePoint(it);
  }
}

Point::sharedPointer PointContainer::pointAfter(time_t time) {
  /*PointMap_t::iterator it;
  it = _points.upper_bound(time);
  if (it == _points.end()) {
    return Point::sharedPointer(new Point());
  }
  else {
    return (*it).second;
  }*/
  PairMap_t::iterator it;
  it = _pairMap.upper_bound(time);
  if (it == _pairMap.end()) {
    return Point::sharedPointer();
  }
  else {
    return makePoint(it); 
  }
}

Point::sharedPointer PointContainer::pointBefore(time_t time) {
  /*PointMap_t::iterator it;
  it = _points.lower_bound(time);
  it--;
  if (it == _points.end()) {
    return Point::sharedPointer(new Point());
  }
  else {
    return (*it).second;
  }*/
  PairMap_t::iterator it;
  it = _pairMap.lower_bound(time);
  if (it == _pairMap.end()) {
    return Point::sharedPointer();
  }
  it--;
  if (it == _pairMap.end()) {
    return Point::sharedPointer();
  }
  else {
    return makePoint(it); 
  }
}
Point::sharedPointer PointContainer::firstPoint() {
  if (this->size() == 0) {
    return Point::sharedPointer();
  }
  
  PairMap_t::iterator it;
  it = _pairMap.begin();
  return makePoint(it);
}

Point::sharedPointer PointContainer::lastPoint() {
  /*if (this->size() == 0) {
    return Point::sharedPointer(new Point() );
  }
  
  PointMap_t::reverse_iterator it;
  it = _points.rbegin();
  return (*it).second;*/
  
  if (this->size() == 0) {
    return Point::sharedPointer();
  }
  PairMap_t::reverse_iterator it;
  it = _pairMap.rbegin();
  time_t time = (*it).first;
  return findPoint(time);
}

long int PointContainer::numberOfPoints() {
  return _pairMap.size();
}


void PointContainer::insertPoint(Point::sharedPointer point) {
  double value, confidence;
  time_t time;
  time = point->time();
  value = point->value();
  confidence = point->confidence();
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

void PointContainer::insertPoints(std::vector<Point::sharedPointer> points) {
  while (!points.empty()) {
    PointContainer::insertPoint(points.back());
    points.pop_back();
  }
}


Point::sharedPointer PointContainer::makePoint(PairMap_t::iterator iterator) {
  // dense line, no? confusing, but it just provides a simpler method for extracting the right information to create a new point from an iterator.
  Point::sharedPointer aPoint(new Point((*iterator).first, (*iterator).second.first, Point::good, (*iterator).second.second) );
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
