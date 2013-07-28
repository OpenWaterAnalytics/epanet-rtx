//
//  BufferPointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//

#include "BufferPointRecord.h"
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

using namespace RTX;
using namespace std;

BufferPointRecord::BufferPointRecord() {
  _defaultCapacity = RTX_BUFFER_DEFAULT_CACHESIZE;
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
    if (it != buffer.end() && it != buffer.begin()) {
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
  
  // no-op. do something more interesting in your derived class.
  // why no-op? because how can you ensure that a point you want to insert here is contiguous?
  // there's now way without knowing about clocks and all that business.
//  cout << "BufferPointRecord::addPoint() -- no point added for TimeSeries " << identifier << endl;
  
}


void BufferPointRecord::addPoints(const string& identifier, std::vector<Point> points) {
  if (points.size() == 0) {
    return;
  }
  // check the cache size, and upgrade if needed.
  KeyedBufferMutexMap_t::iterator it = _keyedBufferMutex.find(identifier);
  if (it != _keyedBufferMutex.end()) {
    PointBuffer_t& buffer = (it->second.first);
    boost::signals2::mutex *mutex = (it->second.second.get());
    
    size_t capacity = buffer.capacity();
    if (capacity < points.size()) {
      // plenty of room
      buffer.set_capacity(points.size() + capacity);
    }
    
    // figure out the insert order...
    // if the set we're inserting has to be prepended to the buffer...
    
    time_t firstInsertionTime = points.front().time;
    time_t lastInsertionTime = points.back().time;
    
    PointRecord::time_pair_t existingRange = BufferPointRecord::range(identifier);
    
    // make sure they're in order
    std::sort(points.begin(), points.end(), &Point::comparePointTime);
    
    // scoped lock so we don't have to worry about cleanup.
    // locking the buffer because we're changing it.
    {
      boost::interprocess::scoped_lock<boost::signals2::mutex> lock(*mutex);
      // more gap detection? righ on!
      bool gap = true;
      
      if (firstInsertionTime <= existingRange.second && existingRange.second < lastInsertionTime) {
        // some of these new points need to be inserted ono the end.
        // fast-fwd along the new points vector, ignoring any points that will not be appended.
        gap = false;
        Point finder(existingRange.second, 0);
        vector<Point>::const_iterator pIt = upper_bound(points.begin(), points.end(), finder, &Point::comparePointTime);
        // now insert these trailing points.
        while (pIt != points.end()) {
          if (pIt->time > existingRange.second) {
            //BufferPointRecord::addPoint(identifier, *pIt);
            // append to circular buffer.
            buffer.push_back(*pIt);
          }
          else {
            cerr << "BufferPointRecord: inserted points not ordered correctly" << endl;
          }
          ++pIt;
        }
      } // appending
      
      if (firstInsertionTime < existingRange.first && existingRange.first <= lastInsertionTime) {
        // some of the new points need to be pre-pended to the buffer.
        // insert onto front (reverse iteration)
        gap = false;
        vector<Point>::const_reverse_iterator pIt = points.rbegin();

        while (pIt != points.rend()) {
          // make this smarter? using upper_bound maybe? todo - figure out upper_bound with a reverse_iterator
          // skip overlapping points.
          
          if (pIt->time < existingRange.first) {
            buffer.push_front(*pIt);
          }
          // else { /* skip */ }
          
          ++pIt;
        }
      } // prepending
      
      if (existingRange.first <= firstInsertionTime && lastInsertionTime <= existingRange.second) {
        // complete overlap -- why did we even try to add these?
        gap = false;
      } // complete overlap
      
      // gap means that the points we're trying to insert do not overlap the existing points.
      // therefore, there's no guarantee of contiguity. so our only option is to clear out the existing cache
      // and insert the new points all by themselves.
      if (gap) {
        // clear the buffer first.
        buffer.clear();
        
        // add new points.
        BOOST_FOREACH(Point p, points) {
          buffer.push_back(p);
        }
      } // gap
    }// scoped mutex lock
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
