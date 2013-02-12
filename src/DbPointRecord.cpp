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
  if ( !(PointRecord::firstPoint(identifier).time() <= startTime && PointRecord::lastPoint(identifier).time() >= endTime) ) {
    preFetchRange(identifier, startTime, endTime);
  }
  
  pointVector = PointRecord::pointsInRange(identifier, startTime, endTime);
  
  return pointVector;
}


void DbPointRecord::preFetchRange(const string& identifier, time_t start, time_t end) {
  
  cout << "RTX-DB-FETCH: " << identifier << " :: " << start << " - " << end << endl;
  
  
  // TODO -- performance optimization - caching -- see scadapointrecord.cpp for code snippets.
  // get out if we've already hinted this.

  if (start >= PointRecord::firstPoint(identifier).time() && end <= PointRecord::lastPoint(identifier).time()) {
    return;
  }
  
  // clear out the base-class cache
  //PointRecord::reset(identifier);
  
  // re-populate base class with new hinted range
  time_t margin = 60*60;
  vector<Point> newPoints = selectRange(identifier, start - margin, end + margin);

  
  PointRecord::addPoints(identifier, newPoints);
}