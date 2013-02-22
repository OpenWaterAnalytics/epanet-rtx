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

DbPointRecord::DbPointRecord() : _timeFormat(UTC) {
  
}



// trying to unify some of the cache-checking code so it's not spread out over the subclasses.
// we only want to hit the db if we absolutely need to.


bool DbPointRecord::isPointAvailable(const string& id, time_t time) {
  if (DB_PR_SUPER::isPointAvailable(id, time)) {
    return true;
  }
  else if ( !(reqRange.first <= time && time <= reqRange.second) ) {
    // force a fetch/cache operation
    time_t margin = 60*60*12;
    this->pointsInRange(id, time - margin, time + margin);
  }
  return DB_PR_SUPER::isPointAvailable(id, time);
}

Point DbPointRecord::point(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::point(id, time);
  
  if (!p.isValid()) {
    time_t margin = 60*60*12;
    this->fetchRange(id, time - margin, time + margin);
    p = DB_PR_SUPER::point(id, time);
  }
  
  return p;
}


Point DbPointRecord::pointBefore(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::pointBefore(id, time);
  
  if (!p.isValid()) {
    this->fetchPrevious(id, time);
    p = DB_PR_SUPER::pointBefore(id, time);
  }
  
  return p;
}


Point DbPointRecord::pointAfter(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::pointAfter(id, time);
  
  if (!p.isValid()) {
    this->fetchNext(id, time);
    p = DB_PR_SUPER::pointAfter(id, time);
  }

  return p;
}


std::vector<Point> DbPointRecord::pointsInRange(const string& id, time_t startTime, time_t endTime) {
  
  PointRecord::time_pair_t range = DB_PR_SUPER::range(id);
  
  if ( !(range.first <= startTime && endTime <= range.second) ) {
    time_t qstart, qend;
    if (startTime < range.first && endTime < range.second  && endTime > range.first) {
      // left-fill query
      qstart = startTime;
      qend = range.first;
    }
    else if (range.first < startTime && range.second < endTime && startTime < range.second ) {
      // right-fill query
      qstart = range.second;
      qend = endTime;
    }
    else {
      // full overlap -- todo - generate two queries...
      qstart = startTime;
      qend = endTime;
    }
    // db hit
    this->fetchRange(id, qstart, qend);
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
  this->truncate();
}


void DbPointRecord::reset(const string& id) {
  DB_PR_SUPER::reset(id);
  this->removeRecord(id);
}

/*
Point DbPointRecord::firstPoint(const string& id) {
  
}


Point DbPointRecord::lastPoint(const string& id) {
  
}

*/






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
  if (p.isValid()) {
    DB_PR_SUPER::addPoint(id, p);
  }
}


void DbPointRecord::fetchPrevious(const std::string& id, time_t time) {
  Point p = selectPrevious(id, time);
  if (p.isValid()) {
    DB_PR_SUPER::addPoint(id, p);
  }
}







/*





void DbPointRecord::preFetchRange(const string& id, time_t start, time_t end) {
  // TODO -- performance optimization - caching -- see scadapointrecord.cpp for code snippets.
  // get out if we've already hinted this.

  time_t first = DB_PR_SUPER::firstPoint(id).time();
  time_t last = DB_PR_SUPER::lastPoint(id).time();
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