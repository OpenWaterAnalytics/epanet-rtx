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

DbPointRecord::DbPointRecord() {
  _timeFormat = UTC;
}




vector<Point> DbPointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  vector<Point> pointVector;
  
  // see if it's not already cached.
  if ( !(DB_PR_SUPER::firstPoint(identifier).time() <= startTime && DB_PR_SUPER::lastPoint(identifier).time() >= endTime) ) {
    preFetchRange(identifier, startTime, endTime);
  }
  
  pointVector = DB_PR_SUPER::pointsInRange(identifier, startTime, endTime);
  
  return pointVector;
}


void DbPointRecord::preFetchRange(const string& identifier, time_t start, time_t end) {
  
  
  
  
  // TODO -- performance optimization - caching -- see scadapointrecord.cpp for code snippets.
  // get out if we've already hinted this.

  time_t first = DB_PR_SUPER::firstPoint(identifier).time();
  time_t last = DB_PR_SUPER::lastPoint(identifier).time();
  if (first <= start && end <= last) {
    return;
  }
  
  // clear out the base-class cache
  //PointRecord::reset(identifier);
  
  // re-populate base class with new hinted range
  time_t margin = 60*60;
  
  cout << "RTX-DB-FETCH: " << identifier << " :: " << start << " - " << end << endl;
  
  vector<Point> newPoints = selectRange(identifier, start - margin, end + margin);

  //cout << "RTX-DB-FETCH: " << identifier << " :: DONE" << endl;
  
  DB_PR_SUPER::addPoints(identifier, newPoints);
}