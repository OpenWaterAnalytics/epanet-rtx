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

bool compareTimePointPair(const BufferPointRecord::TimePointPair_t& lhs, const BufferPointRecord::TimePointPair_t& rhs);

BufferPointRecord::BufferPointRecord() {
  
  _defaultCapacity = 1000;
}

std::ostream& RTX::operator<< (std::ostream &out, BufferPointRecord &pr) {
  return pr.toStream(out);
}

std::ostream& BufferPointRecord::toStream(std::ostream &stream) {
  stream << "Buffered Point Record - connection: " << this->connectionString() << std::endl;
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

bool compareTimePointPair(const BufferPointRecord::TimePointPair_t& lhs, const BufferPointRecord::TimePointPair_t& rhs) {
  return lhs.first < rhs.first;
}

// convenience method for making a new point from a buffer iterator
Point BufferPointRecord::makePoint(PointBuffer_t::const_iterator iterator) {
  return Point((*iterator).first, (*iterator).second.first, Point::good, (*iterator).second.second);
}

bool BufferPointRecord::isPointAvailable(const string& identifier, time_t time) {
  
  bool isAvailable = false;
  
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
  PointBuffer_t& buffer = (it->second.first);
  
  if (buffer.empty()) {
    return false;
  }
  
  // lock the buffer
  mutex->lock();
  
  TimePointPair_t tpPairFirst = buffer.front();
  TimePointPair_t tpPairLast = buffer.back();
  
  if (tpPairFirst.first <= time && time <= tpPairLast.first) {
    // search the buffer
    TimePointPair_t finder(time, PointPair_t(0,0));
    PointBuffer_t::const_iterator pbIt = std::lower_bound(buffer.begin(), buffer.end(), finder, compareTimePointPair);
    if (pbIt != buffer.end() && pbIt->first == time) {
      isAvailable = true;
      _cachedPoint = makePoint(pbIt);
    }
    else {
      //cerr << "whoops, not found" << endl;
      
      PointBuffer_t::const_iterator pbIt = buffer.begin();
      while (pbIt != buffer.end()) {
        time_t foundTime = pbIt->first;
        if (foundTime == time) {
          cout << "found it anyway!!" << endl;
          _cachedPoint = makePoint(pbIt);
        }
        ++pbIt;
      }
    }
  }
  
  
  // ok, all done
  mutex->unlock();
  
  return isAvailable;
  
}


Point BufferPointRecord::point(const string& identifier, time_t time) {
  // calling isPointAvailable pushes the point into _cachedPoint (if true)
  if (isPointAvailable(identifier, time)) {
    return _cachedPoint;
  }
  
  else {
    return pointBefore(identifier, time);
  }
}


Point BufferPointRecord::pointBefore(const string& identifier, time_t time) {
  
  Point foundPoint;
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::const_iterator it  = lower_bound(buffer.begin(), buffer.end(), finder, compareTimePointPair);
    if (it != buffer.end() && it != buffer.begin()) {
      if (--it != buffer.end()) {
        foundPoint = makePoint(it);
      }
    }
    
    // all done.
    mutex->unlock();
  }
  
  return foundPoint;
}


Point BufferPointRecord::pointAfter(const string& identifier, time_t time) {
  
  Point foundPoint;
  TimePointPair_t finder(time, PointPair_t(0,0));
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::const_iterator it = upper_bound(buffer.begin(), buffer.end(), finder, compareTimePointPair);
    if (it != buffer.end()) {
      foundPoint = makePoint(it);
    }
    
    // all done.
    mutex->unlock();
  }
  
  return foundPoint;
}


std::vector<Point> BufferPointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  
  std::vector<Point> pointVector;
  
  TimePointPair_t finder(startTime, PointPair_t(0,0));
  
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    PointBuffer_t::const_iterator it = lower_bound(buffer.begin(), buffer.end(), finder, compareTimePointPair);
    while ( (it != buffer.end()) && (it->first < endTime) ) {
      Point newPoint = makePoint(it);
      pointVector.push_back(newPoint);
      it++;
    }
    
    // all done.
    mutex->unlock();
  }
  
  return pointVector;
}


void BufferPointRecord::addPoint(const string& identifier, Point point) {
  
  if (BufferPointRecord::isPointAvailable(identifier, point.time())) {
    //cout << "skipping duplicate point" << endl;
    return;
  }
  
  
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
  
  time = point.time();
  value = point.value();
  confidence = point.confidence();
  
  PointPair_t newPoint(value, confidence);
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    // lock the buffer
    mutex->lock();
    
    if (buffer.size() == buffer.capacity()) {
      cout << "buffer full" << endl;
    }
    
    
    time_t firstTime = BufferPointRecord::firstPoint(identifier).time();
    time_t lastTime = BufferPointRecord::lastPoint(identifier).time();
    
    if (time > lastTime) {
      // end of the buffer
      buffer.push_back(TimePointPair_t(time,newPoint));
      skipped = false;
    }
    else if (time < firstTime) {
      // front of the buffer
      buffer.push_front(TimePointPair_t(time,newPoint));
      skipped = false;
    }
    
    if (skipped) {
      //cout << "point skipped: " << identifier;
      buffer.push_front(TimePointPair_t(time,newPoint));
      // make sure the points are in order.
      sort(buffer.begin(), buffer.end(), compareTimePointPair);
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
      cout << "enlarging capacity" << endl;
      buffer.set_capacity(points.size() + capacity);
    }
    
    // figure out the insert order...
    // if the set we're inserting has to be prepended to the buffer...
    
    Point firstCachePoint = BufferPointRecord::firstPoint(identifier);
    Point lastCachePoint = BufferPointRecord::lastPoint(identifier);
    
    time_t firstTime = firstCachePoint.time();
    time_t lastTime = lastCachePoint.time();
    
    time_t insertFirst = points.front().time();
    time_t insertLast = points.back().time();
    
    // make sure they're in order
    std::sort(points.begin(), points.end(), &Point::comparePointTime);
    
    if (insertLast > lastTime) {
      // insert onto end.
      Point finder(lastTime, 0);
      vector<Point>::const_iterator pIt = upper_bound(points.begin(), points.end(), finder, &Point::comparePointTime);
      while (pIt != points.end()) {
        // skip points that fall before the end of the buffer.
        
        if (pIt->time() > lastTime) {
          BufferPointRecord::addPoint(identifier, *pIt);
        }
        
        ++pIt;
      }
    }
    if (insertFirst < firstTime) {
      // insert onto front (reverse iteration)
      vector<Point>::const_reverse_iterator pIt = points.rbegin();
      while (pIt != points.rend()) {
        
        // skip overlapping points.
        // insert only the correct points.
        if (pIt->time() < firstTime) {
          BufferPointRecord::addPoint(identifier, *pIt);
        }
        // else { /* skip */ }
        
        ++pIt;
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
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    
    if (buffer.empty()) {
      return foundPoint;
    }
    
    TimePointPair_t tpPair = buffer.front();
    foundPoint = Point(tpPair.first, tpPair.second.first);
    
  }
  return foundPoint;
}

Point BufferPointRecord::lastPoint(const string& id) {
  Point foundPoint;
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(id);
  if (it != _keyedBufferMutex.end()) {
    // get the constituents
    boost::signals2::mutex *mutex = (it->second.second.get());
    PointBuffer_t& buffer = (it->second.first);
    
    if (buffer.empty()) {
      return foundPoint;
    }
    
    TimePointPair_t tpPair = buffer.back();
    foundPoint = Point(tpPair.first, tpPair.second.first);
    
  }
  return foundPoint;
}
