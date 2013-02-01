//
//  DbPointRecord.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 1/31/13.
//
//

#include "DbPointRecord.h"

using namespace RTX;

DbPointRecord::DbPointRecord() {
  _timeFormat = UTC;
}

DbPointRecord::Hint_t::Hint_t() {
  this->clear();
}

void DbPointRecord::Hint_t::clear() {
  //std::cout << "clearing scada point cache" << std::endl;
  identifier = "";
  range.first = 0;
  range.second = 0;
}


std::vector<Point> DbPointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point> pointVector;
  
  // see if it's not already cached.
  if ( !(_hint.range.first <= startTime && _hint.range.second >= endTime && RTX_STRINGS_ARE_EQUAL(identifier, _hint.identifier)) ) {
    preFetchRange(identifier, startTime, endTime);
  }
  
  pointVector = PointRecord::pointsInRange(identifier, startTime, endTime);
  
  return pointVector;
}


void DbPointRecord::preFetchRange(const string& identifier, time_t start, time_t end) {
  // TODO -- performance optimization - caching -- see scadapointrecord.cpp for code snippets.
  // get out if we've already hinted this.
  if (identifier == _hint.identifier && start >= _hint.range.first && end <= _hint.range.second) {
    return;
  }
  
  // clear out the base-class cache
  PointRecord::reset(identifier);
  _hint.clear();
  
  // re-populate base class with new hinted range
  time_t margin = 120;
  std::vector<Point> newPoints = selectRange(identifier, start - margin, end + margin);
  if (newPoints.size() > 0) {
    _hint.identifier = identifier;
    const std::vector<Point>::iterator firstPoint = newPoints.begin();
    const std::vector<Point>::reverse_iterator lastPoint = newPoints.rbegin();
    _hint.range.first = firstPoint->time();
    _hint.range.second = lastPoint->time();
  }

  
  PointRecord::addPoints(identifier, newPoints);
}