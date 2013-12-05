//
//  DbPointRecord.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/31/13.
//
//

#include "DbPointRecord.h"

using namespace RTX;
using namespace std;


DbPointRecord::request_t::request_t(string id, time_t start, time_t end) : id(id), range(make_pair(start, end)) {
  
}

bool DbPointRecord::request_t::contains(std::string id, time_t t) {
  if (this->range.first <= t && t <= this->range.second && RTX_STRINGS_ARE_EQUAL(id, this->id)) {
    return true;
  }
  return false;
}



DbPointRecord::DbPointRecord() : request("",0,0) {
  _searchDistance = 60*60*24*7; // 1-week
  errorMessage = "Not Connected";
}


//void DbPointRecord::setConnectionString(const std::string& connection) {
//  _connectionString = connection;
//}
//const std::string& DbPointRecord::connectionString() {
//  return _connectionString;
//}

void DbPointRecord::setSearchDistance(time_t time) {
  _searchDistance = time;
}
time_t DbPointRecord::searchDistance() {
  return _searchDistance;
}

// trying to unify some of the cache-checking code so it's not spread out over the subclasses.
// we only want to hit the db if we absolutely need to.

/*
bool DbPointRecord::isPointAvailable(const string& id, time_t time) {
  if (DB_PR_SUPER::isPointAvailable(id, time)) {
    return true;
  }
  else if ( !(reqRange.first <= time && time <= reqRange.second) ) {
    // force a fetch/cache operation
    time_t margin = 60*60*12;
    vector<Point> points = this->pointsInRange(id, time - margin, time + margin);
  }
  return DB_PR_SUPER::isPointAvailable(id, time);
}
*/
Point DbPointRecord::point(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::point(id, time);
  
  if (!p.isValid) {
    
    // see if we just asked the db for something in this range.
    // if so, and Super couldn't find it, then it's just not here.
    // todo -- check staleness
    
    if (request.contains(id, time)) {
      return Point();
    }
    
    time_t margin = 60*60*12;
    time_t start = time - margin, end = time + margin;
    
    // do the request, and cache the request parameters.
    
    vector<Point> pVec = this->selectRange(id, start, end);
    if (pVec.size() > 0) {
      request = request_t(id, pVec.front().time, pVec.back().time);
    }
    else {
      request = request_t(id,0,0);
    }
    
    
    vector<Point>::const_iterator pIt = pVec.begin();
    while (pIt != pVec.end()) {
      if (pIt->time == time) {
        p = *pIt;
      }
      else if (pIt->time > time) {
        break;
      }
      ++pIt;
    }
    // cache this latest result set
    DB_PR_SUPER::addPoints(id, pVec);
  }
  
  
  
  return p;
}


Point DbPointRecord::pointBefore(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::pointBefore(id, time);
  
  if (!p.isValid) {
    // check if last request covered before
    if (request.contains(id, time-1)) {
      return Point();
    }
    PointRecord::time_pair_t range = DB_PR_SUPER::range(id);
    request = request_t(id, time, time);
    p = this->selectPrevious(id, time);
    // cache it
    if (p.isValid) {
      _cachedPointId = id;
      _cachedPoint = p;
    }
    
    if (range.first <= time && time <= range.second) {
      // then we know this is continuous. add the point.
      DB_PR_SUPER::addPoint(id, p);
    }
    else {
      request = request_t("",0,0);
    }
  }
  
  return p;
}


Point DbPointRecord::pointAfter(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::pointAfter(id, time);
  
  if (!p.isValid) {
    // check if last request covered after
    if (request.contains(id, time+1)) {
      return Point();
    }
    PointRecord::time_pair_t range = DB_PR_SUPER::range(id);
    request = request_t(id, time, time);
    p = this->selectNext(id, time);
    // cache it
    if (p.isValid) {
      _cachedPointId = id;
      _cachedPoint = p;
    }
    
    if (range.first <= time && time <= range.second) {
      // then we know this is continuous. add the point.
      DB_PR_SUPER::addPoint(id, p);
    }
    else {
      request = request_t("",0,0);
    }
  }
  
  return p;
}


std::vector<Point> DbPointRecord::pointsInRange(const string& id, time_t startTime, time_t endTime) {
  
  PointRecord::time_pair_t range = DB_PR_SUPER::range(id);
  
  // if the requested range is not in memcache, then fetch it.
  if ( !(range.first <= startTime && endTime <= range.second) ) {
    vector<Point> left;
    vector<Point> right;
    
    time_t qstart, qend;
    if (startTime < range.first && endTime < range.second  && endTime > range.first) {
      // left-fill query
      qstart = startTime;
      qend = range.first;
      right = DB_PR_SUPER::pointsInRange(id, range.first, endTime);
    }
    else if (range.first < startTime && range.second < endTime && startTime < range.second ) {
      // right-fill query
      qstart = range.second;
      qend = endTime;
      left = DB_PR_SUPER::pointsInRange(id, startTime, range.second);
    }
    else {
      // full overlap -- todo - generate two queries...
      qstart = startTime;
      qend = endTime;
    }
    // db hit
    vector<Point> newPoints = this->selectRange(id, qstart, qend);
    
    vector<Point> merged;
    merged.reserve(newPoints.size() + left.size() + right.size());
    merged.insert(merged.end(), left.begin(), left.end());
    merged.insert(merged.end(), newPoints.begin(), newPoints.end());
    merged.insert(merged.end(), right.begin(), right.end());
    
    request = (merged.size() > 0) ? request_t(id, merged.front().time, merged.back().time) : request_t(id,0,0);
    
    DB_PR_SUPER::addPoints(id, merged);
    
    return merged;
  }
  
  return DB_PR_SUPER::pointsInRange(id, startTime, endTime);
  
}


void DbPointRecord::addPoint(const string& id, Point point) {
  DB_PR_SUPER::addPoint(id, point);
  this->insertSingle(id, point);
}


void DbPointRecord::addPoints(const string& id, std::vector<Point> points) {
  DB_PR_SUPER::addPoints(id, points);
  this->insertRange(id, points);
}


void DbPointRecord::reset() {
  DB_PR_SUPER::reset();
  //this->truncate();
}


void DbPointRecord::reset(const string& id) {
  // deprecate?
  //cout << "Whoops - don't use this" << endl;
  DB_PR_SUPER::reset(id);
  //this->removeRecord(id);
  // wiped out the record completely, so re-initialize it.
  //this->registerAndGetIdentifier(id);
}

/*
Point DbPointRecord::firstPoint(const string& id) {
  
}


Point DbPointRecord::lastPoint(const string& id) {
  
}

*/





/*
// default virtual implementations
void DbPointRecord::fetchRange(const std::string& id, time_t startTime, time_t endTime) {
  reqRange = make_pair(startTime, endTime);
  
  vector<Point> points = selectRange(id, startTime, endTime);
  if (points.size() > 0) {
    DB_PR_SUPER::addPoints(id, points);
  }
}

void DbPointRecord::fetchNext(const std::string& id, time_t time) {
  Point p = selectNext(id, time);
  if (p.isValid) {
    DB_PR_SUPER::addPoint(id, p);
  }
}


void DbPointRecord::fetchPrevious(const std::string& id, time_t time) {
  Point p = selectPrevious(id, time);
  if (p.isValid) {
    DB_PR_SUPER::addPoint(id, p);
  }
}
*/






/*





void DbPointRecord::preFetchRange(const string& id, time_t start, time_t end) {
  // TODO -- performance optimization - caching -- see OdbcPointRecord.cpp for code snippets.
  // get out if we've already hinted this.

  time_t first = DB_PR_SUPER::firstPoint(id).time;
  time_t last = DB_PR_SUPER::lastPoint(id).time;
  if (first <= start && end <= last) {
    return;
  }
  
  // clear out the base-class cache
  //PointRecord::reset(id);
  
  // re-populate base class with new hinted range
  time_t margin = 60*60;
  
  cout << "RTX-DB-FETCH: " << id << " :: " << start << " - " << end << endl;
  
  vector<Point> newPoints = selectRange(id, start - margin, end + margin);

  //cout << "RTX-DB-FETCH: " << id << " :: DONE" << endl;
  
  DB_PR_SUPER::addPoints(id, newPoints);
}

*/