//
//  BufferPointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include "BufferPointRecord.h"

#include "boost/foreach.hpp"

using namespace RTX;
using namespace std;

//bool compareTimePointPair(const BufferPointRecord::TimePointPair_t& lhs, const BufferPointRecord::TimePointPair_t& rhs);

BufferPointRecord::BufferPointRecord() {
  
  _defaultCapacity = 100;
}

std::ostream& RTX::operator<< (std::ostream &out, BufferPointRecord &pr) {
  return pr.toStream(out);
}

std::ostream& BufferPointRecord::toStream(std::ostream &stream) {
  stream << "Buffered Point Record" << std::endl;
  return stream;
}



std::string BufferPointRecord::registerAndGetIdentifier(std::string recordName) {
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




std::vector<std::string> BufferPointRecord::identifiers() {
  typedef std::map<std::string, BufferMutexPair_t >::value_type& nameMapValue_t;
  vector<string> names;
  BOOST_FOREACH(nameMapValue_t name, _keyedBufferMutex) {
    names.push_back(name.first);
  }
  return names;
}
/*
bool compareTimePointPair(const BufferPointRecord::TimePointPair_t& lhs, const BufferPointRecord::TimePointPair_t& rhs) {
  return lhs.first < rhs.first;
}
*/
/*
// convenience method for making a new point from a buffer iterator
Point BufferPointRecord::makePoint(PointBuffer_t::const_iterator iterator) {
  return Point((*iterator).first, (*iterator).second.first, Point::good, (*iterator).second.second);
}
*/

Point BufferPointRecord::point(const string& identifier, time_t time) {
  
  bool isAvailable = false;
  
  // quick check for repeated calls
  if (_cachedPoint.time == time && RTX_STRINGS_ARE_EQUAL_CS(_cachedPointId, identifier) ) {
    return _cachedPoint;
  }
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it == _keyedBufferMutex.end()) {
    // nobody here by that name
    return Point();
  }
  
  // get the constituents
  boost::signals2::mutex *mutex = (it->second.second.get());
  PointBuffer_t& buffer = (it->second.first);
  
  if (buffer.empty()) {
    return Point();
  }
  
  // lock the buffer
  mutex->lock();
  
  Point pFirst = buffer.front();
  Point pLast = buffer.back();
  
  if (pFirst.time <= time && time <= pLast.time) {
    // search the buffer
    //TimePointPair_t finder(time, PointPair_t(0,0));
    Point finder(time, 0);
    PointBuffer_t::const_iterator pbIt = std::lower_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
    if (pbIt != buffer.end() && pbIt->time == time) {
      isAvailable = true;
      _cachedPoint = *pbIt;
      _cachedPointId = identifier;
    }
    else {
      //cerr << "whoops, not found" << endl;
      // try brute-force (debug)
      PointBuffer_t::const_iterator pbIt = buffer.begin();
      while (pbIt != buffer.end()) {
        time_t foundTime = pbIt->time;
        if (foundTime == time) {
          cout << "found it anyway!!" << endl;
          Point aPoint = *pbIt;
          _cachedPoint = aPoint;
        }
        ++pbIt;
      }
    }
  }
  
  
  // ok, all done
  mutex->unlock();
  
  if (isAvailable) {
    return _cachedPoint;
  }
  else {
    return Point();//this->pointBefore(identifier, time);
  }
  
}



Point BufferPointRecord::pointBefore(const string& identifier, time_t time) {
  
  Point foundPoint;
  //TimePointPair_t finder(time, PointPair_t(0,0));
  Point finder(time, 0);
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::const_iterator it  = lower_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
    if (it != buffer.end() && it != buffer.begin()) {
      if (--it != buffer.end()) {
        foundPoint = *it;
        _cachedPoint = foundPoint;
        _cachedPointId = identifier;
      }
    }
    
    // all done.
    mutex->unlock();
  }
  
  return foundPoint;
}


Point BufferPointRecord::pointAfter(const string& identifier, time_t time) {
  
  Point foundPoint;
  //TimePointPair_t finder(time, PointPair_t(0,0));
  Point finder(time, 0);
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::const_iterator it = upper_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
    if (it != buffer.end()) {
      foundPoint = *it;
      _cachedPoint = foundPoint;
      _cachedPointId = identifier;
    }
    
    // all done.
    mutex->unlock();
  }
  
  return foundPoint;
}


std::vector<Point> BufferPointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  
  std::vector<Point> pointVector;
  
  //TimePointPair_t finder(startTime, PointPair_t(0,0));
  Point finder(startTime, 0);
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::const_iterator it = lower_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
    while ( (it != buffer.end()) && (it->time <= endTime) ) {
      Point newPoint = *it;
      pointVector.push_back(newPoint);
      it++;
    }
    
    // all done.
    mutex->unlock();
  }
  
  return pointVector;
}


void BufferPointRecord::addPoint(const string& identifier, Point point) {
  
  //if (BufferPointRecord::isPointAvailable(identifier, point.time)) {
    //cout << "skipping duplicate point" << endl;
   // return;
  //}
  
  /*
  vector<Point> single;
  single.push_back(point);
  BufferPointRecord::addPoints(identifier, single);
  
  */
  /*
   bool unordered = false;
   // test for continuity
   KeyedBufferMutexMap_t::iterator kbmmit = _keyedBufferMutex.find(identifier);
   if (kbmmit != _keyedBufferMutex.end()) {
   // get the constituents
   boost::signals2::mutex *mutex = (kbmmit->second.second.get());
   PointBuffer_t& buffer = (kbmmit->second.first);
   // lock the buffer
   mutex->lock();
   
   // check for monotonically increasing time values.
   PointBuffer_t::const_iterator pIt = buffer.begin();
   
   while (pIt != buffer.end()) {
   time_t t1 = pIt->first;
   ++pIt;
   if (pIt == buffer.end()) { break; }
   time_t t2 = pIt->first;
   
   if (t1 >= t2) {
   cerr << "Points unordered" << endl;
   unordered = true;
   break;
   }
   }
   
   if (unordered) {
   // make sure the points are in order.
   sort(buffer.begin(), buffer.end(), compareTimePointPair);
   }
   
   mutex->unlock();
   }
   
   */
  
  
  
  
  
  
  
  
  
  
  
  double value, confidence;
  time_t time;
  bool skipped = true;
  
  time = point.time;
  value = point.value;
  confidence = point.confidence;
  
  //PointPair_t newPoint(value, confidence);
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    if (buffer.size() == buffer.capacity()) {
      //cout << "buffer full" << endl;
    }
    
    
    time_t firstTime = BufferPointRecord::firstPoint(identifier).time;
    time_t lastTime = BufferPointRecord::lastPoint(identifier).time;
    if (time == lastTime || time == firstTime) {
      // skip it
      skipped = false;
    }
    if (time > lastTime) {
      // end of the buffer
      buffer.push_back(point);
      skipped = false;
    }
    else if (time < firstTime) {
      // front of the buffer
      buffer.push_front(point);
      skipped = false;
    }
    
    if (skipped) {
      //cout << "point skipped: " << identifier;
      //TimePointPair_t finder(time, PointPair_t(0,0));
      Point finder(time, 0);
      PointBuffer_t::const_iterator it = lower_bound(buffer.begin(), buffer.end(), finder, &Point::comparePointTime);
      if (it->time != time) {
        // if the time is not found here, then it's a new point.
        buffer.push_front(point);
        // make sure the points are in order.
        sort(buffer.begin(), buffer.end(), &Point::comparePointTime);
      }
      
    }
    
    
    
    
    
    
    // all done.
    mutex->unlock();
  }
  
  if (skipped) {
    //cout << "(" << isPointAvailable(identifier, time) << ")" << endl;
  }
  
  
  
}


void BufferPointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  if (points.size() == 0) {
    return;
  }
  // check the cache size, and upgrade if needed.
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    PointBuffer_t& buffer = (it->second.first);
    size_t capacity = buffer.capacity();
    if (capacity < points.size()) {
      // plenty of room
      buffer.set_capacity(points.size() + capacity);
    }
    
    // figure out the insert order...
    // if the set we're inserting has to be prepended to the buffer...
    
    time_t insertFirst = points.front().time;
    time_t insertLast = points.back().time;
    
    PointRecord::time_pair_t range = BufferPointRecord::range(identifier);
    
    // make sure they're in order
    std::sort(points.begin(), points.end(), &Point::comparePointTime);
    
   
    bool gap = true;
    
    if (insertFirst < range.second && range.second < insertLast) {
      // insert onto end.
      gap = false;
      Point finder(range.second, 0);
      vector<Point>::const_iterator pIt = upper_bound(points.begin(), points.end(), finder, &Point::comparePointTime);
      while (pIt != points.end()) {
        // skip points that fall before the end of the buffer.
        
        if (pIt->time > range.second) {
          BufferPointRecord::addPoint(identifier, *pIt);
        }
        ++pIt;
      }
    }
    if (insertFirst < range.first && range.first < insertLast) {
      // insert onto front (reverse iteration)
      gap = false;
      vector<Point>::const_reverse_iterator pIt = points.rbegin();
      while (pIt != points.rend()) {
        
        // skip overlapping points.
        if (pIt->time < range.first) {
          BufferPointRecord::addPoint(identifier, *pIt);
        }
        // else { /* skip */ }
        
        ++pIt;
      }
    }
    if (range.first <= insertFirst && insertLast <= range.second) {
      // complete overlap -- why did we even try to add these?
      gap = false;
    }
    
    if (gap) {
      // clear the buffer first.
      buffer.clear();
      
      // add new points.
      BOOST_FOREACH(Point p, points) {
        BufferPointRecord::addPoint(identifier, p);
      }
    }
    
  }
}


void BufferPointRecord::reset() {
  _keyedBufferMutex.clear();
}

void BufferPointRecord::reset(const string& identifier) {
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    PointBuffer_t& buffer = (it->second.first);
    buffer.clear();
  }
  
}



Point BufferPointRecord::firstPoint(const string& id) {
  Point foundPoint;
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(id);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    //boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    
    if (buffer.empty()) {
      return foundPoint;
    }
    
    //TimePointPair_t tpPair = buffer.front();
    foundPoint = buffer.front(); // Point(tpPair.first, tpPair.second.first);
    
  }
  return foundPoint;
}

Point BufferPointRecord::lastPoint(const string& id) {
  Point foundPoint;
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(id);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    //boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    
    if (buffer.empty()) {
      return foundPoint;
    }
    
    //TimePointPair_t tpPair = buffer.back();
    foundPoint = buffer.back(); // Point(tpPair.first, tpPair.second.first);
    
  }
  return foundPoint;
}

PointRecord::time_pair_t BufferPointRecord::range(const string& id) {
  return make_pair(BufferPointRecord::firstPoint(id).time, BufferPointRecord::lastPoint(id).time);
}
