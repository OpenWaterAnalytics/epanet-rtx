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

DbPointRecord::Hint_t::Hint_t() {
  this->clear();
}

void DbPointRecord::Hint_t::clear() {
  //cout << "clearing scada point cache" << endl;
  identifier = "";
  range.first = 0;
  range.second = 0;
}


vector<Point> DbPointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  vector<Point> pointVector;
  
  // see if it's not already cached.
  if ( !(_hint.range.first <= startTime && _hint.range.second >= endTime && RTX_STRINGS_ARE_EQUAL(identifier, _hint.identifier)) ) {
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
  _hint.clear();
  
  // re-populate base class with new hinted range
  time_t margin = 60*60;
  vector<Point> newPoints = selectRange(identifier, start - margin, end + margin);
  if (newPoints.size() > 0) {
    _hint.identifier = identifier;
    const vector<Point>::iterator firstPoint = newPoints.begin();
    const vector<Point>::reverse_iterator lastPoint = newPoints.rbegin();
    _hint.range.first = firstPoint->time();
    _hint.range.second = lastPoint->time();
  }

  
  PointRecord::addPoints(identifier, newPoints);
}