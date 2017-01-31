//
//  DbPointRecord.cpp
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//


#include <iostream>
#include <sstream>
#include <set>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/join.hpp>
#include <string>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/signals2/mutex.hpp>

using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

#include "DbPointRecord.h"


using namespace RTX;
using namespace std;


std::string DbPointRecord::Query::selectStr() {
  stringstream ss;
  ss << "SELECT ";
  if (this->select.size() == 0) {
    ss << "*";
  }
  else {
    ss << boost::algorithm::join(this->select,", ");
  }
  
  ss << " FROM " << this->nameAndWhereClause();
  
  if (this->order.length() > 0) {
    ss << " ORDER BY " << this->order;
  }
  
  return ss.str();
}

std::string DbPointRecord::Query::nameAndWhereClause() {
  stringstream ss;
  ss << this->from;
  
  if (this->where.size() > 0) {
    ss << " WHERE " << boost::algorithm::join(this->where," AND ");
  }
  return ss.str();
}

DbPointRecord::request_t::request_t(string id, TimeRange r_range) : range(r_range), id(id) {
  
}

bool DbPointRecord::request_t::contains(std::string id, time_t t) {
  if (this->range.start <= t && t <= this->range.end && RTX_STRINGS_ARE_EQUAL(id, this->id)) {
    return true;
  }
  return false;
}

void DbPointRecord::request_t::clear() {
  this->range = TimeRange();
  this->id = "";
}


DbPointRecord::DbPointRecord() : _last_request("",TimeRange()) {
  _searchDistance = 60*60*24*7; // 1-week
  errorMessage = "Not Connected";
  _readOnly = false;
  _filterType = OpcPassThrough;
  
  iterativeSearchMaxIterations = 8;
  iterativeSearchStride = 3*60*60;
  _db_mutex.reset(new boost::signals2::mutex);
  
  useTransactions = false;
  maxTransactionInserts = 1000;
  transactionInsertCount = 0;

}



bool DbPointRecord::readonly() {
  return _readOnly;
}

void DbPointRecord::setReadonly(bool readOnly) {
  _readOnly = readOnly;
}

bool DbPointRecord::registerAndGetIdentifierForSeriesWithUnits(string name, Units units) {
  
  scoped_lock<boost::signals2::mutex> lock(*_db_mutex);
  
  if (name.length() == 0) {
    return false;
  }
  
  bool nameExists = false;
  bool unitsMatch = false;
  Units existingUnits = RTX_NO_UNITS;
  
  if (!this->isConnected()) {
    this->dbConnect();
  }
  
  auto match = this->identifiersAndUnits().doesHaveIdUnits(name,units);
  nameExists = match.first;
  unitsMatch = match.second;
  
  
  if (this->readonly()) {
    // handle a read-only database.
    if (nameExists && (unitsMatch || !this->supportsUnitsColumn()) ) {
      // everything is awesome. name matches, units match (or we don't support it and therefore don't care).
      // make a cache and return affirmative.
      DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
      return true;
    }
    else {
      // SPECIAL CASE FOR OLD RECORDS: we can update the units field if no units are specified.
      if (this->canAssignUnits() && existingUnits == RTX_NO_UNITS) {
        this->assignUnitsToRecord(name, units);
        DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
        return true;
      }
      // names don't match (or units prevent us from using this record) and we can't write to this db. fail.
      return false;
    }
  }
  else {
    // not a readonly db.
    if (nameExists && !unitsMatch) {
      // two possibilities: the units actually don't match, or my units haven't ever been set.
      if (existingUnits == RTX_NO_UNITS) {
        if (this->canAssignUnits()) {
          // aha. update my units then.
          return this->assignUnitsToRecord(name, units);
        }
      }
      else {
        // must remove the old record. units don't match for real.
        this->removeRecord(name);
        return DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
      }
    }
    else if ( ( !nameExists || !unitsMatch ) && this->insertIdentifierAndUnits(name, units) && DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units) ) {
      // this will either insert a new record name, or ignore because it's already there.
      return true;
    }
    else if (nameExists && unitsMatch) {
      return DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
    }
  }
  return false;
}

bool DbPointRecord::canAssignUnits() {
  return false;
}

bool DbPointRecord::assignUnitsToRecord(const std::string& name, const Units& units) {
  // nothing
  return false;
}

PointRecord::IdentifierUnitsList DbPointRecord::identifiersAndUnits() {
  return _identifiersAndUnitsCache;
}


void DbPointRecord::setSearchDistance(time_t time) {
  _searchDistance = time;
}

time_t DbPointRecord::searchDistance() {
  return _searchDistance;
}


Point DbPointRecord::point(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::point(id, time);
  
  if (!p.isValid) {
    
    // see if we just asked the db for something in this range.
    // if so, and Super couldn't find it, then it's just not here.
    // todo -- check staleness
    
    if (_last_request.contains(id, time)) {
      return Point();
    }
    
    time_t margin = 60*60*12;
    time_t start = time - margin, end = time + margin;
    
    // do the request, and cache the request parameters.
    
    vector<Point> pVec = this->selectRange(id, TimeRange(start, end));
    pVec = this->pointsWithOpcFilter(pVec);
    
    if (pVec.size() > 0) {
      _last_request = request_t(id, TimeRange(pVec.front().time, pVec.back().time));
    }
    else {
      _last_request = request_t(id,TimeRange());
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
  
  // available in circular buffer?
  Point p = DB_PR_SUPER::pointBefore(id, time);
  if (p.isValid) {
    return p;
  }
  
  // if it's not buffered, but the last request covered this range, then there is no point here.
  if (_last_request.contains(id, time-1)) {
    return Point();
  }
  
  // should i search iteratively?
  if (this->shouldSearchIteratively()) {
    p = this->searchPreviousIteratively(id, time);
    if (p.isValid) {
      return p;
    }
  }
  
  
  // try a singly-bounded query
  if (this->supportsSinglyBoundedQueries()) {
    p = this->selectPrevious(id, time);
  }
  if (p.isValid) {
    return this->pointWithOpcFilter(p);
  }

  
  return p;
}


Point DbPointRecord::pointAfter(const string& id, time_t time) {
  // buffered?
  Point p = DB_PR_SUPER::pointAfter(id, time);
  if (p.isValid) {
    return p;
  }
  
  // last request covered this already?
  if (_last_request.contains(id, time + 1)) {
    return Point();
  }
  
  if (this->shouldSearchIteratively()) {
    p = this->searchNextIteratively(id, time);
  }
  if (p.isValid) {
    return p;
  }
  
  // singly bounded?
  if (this->supportsSinglyBoundedQueries()) {
    p = this->selectNext(id, time);
  }
  if (p.isValid) {
    return this->pointWithOpcFilter(p);
  }
  
  return p;
}


Point DbPointRecord::searchPreviousIteratively(const string& id, time_t time) {
  int lookBehindLimit = iterativeSearchMaxIterations;
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    vector<Point> points;
    // iterative lookbehind is faster than unbounded lookup
    TimeRange r;
    r.start = time - iterativeSearchStride;
    r.end = time - 1;
    while (points.size() == 0 && lookBehindLimit > 0) {
      points = this->pointsInRange(id, r);
      r.end   -= iterativeSearchStride;
      r.start -= iterativeSearchStride;
      --lookBehindLimit;
    }
    if (points.size() > 0) {
      return points.back();
    }
  }
  return Point();
}

Point DbPointRecord::searchNextIteratively(const string& id, time_t time) {
  int lookAheadLimit = iterativeSearchMaxIterations;
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    vector<Point> points;
    // iterative lookbehind is faster than unbounded lookup
    TimeRange r;
    r.start = time + 1;
    r.end = time + iterativeSearchStride;
    while (points.size() == 0 && lookAheadLimit > 0) {
      points = this->pointsInRange(id, r);
      r.start += iterativeSearchStride;
      r.end += iterativeSearchStride;
      --lookAheadLimit;
    }
    if (points.size() > 0) {
      return points.front();
    }
  }
  return Point();
}


std::vector<Point> DbPointRecord::pointsInRange(const string& id, TimeRange qrange) {
  scoped_lock<boost::signals2::mutex> lock(*_db_mutex);
  // limit double-queries
  if (_last_request.range.containsRange(qrange) && _last_request.id == id) {
    return DB_PR_SUPER::pointsInRange(id, qrange);
  }
  
  TimeRange range = DB_PR_SUPER::range(id);
  TimeRange::intersect_type intersect = range.intersection(qrange);

  // if the requested range is not in memcache, then fetch it.
  if ( intersect == TimeRange::intersect_other_internal ) {
    return DB_PR_SUPER::pointsInRange(id, qrange);
  }
  else {
    vector<Point> left, middle, right;
    TimeRange n_range;
    
    if (intersect == TimeRange::intersect_left) {
      // left-fill query
      n_range.start = qrange.start;
      n_range.end = range.start;
      middle = this->pointsWithOpcFilter(this->selectRange(id, n_range));
      right = DB_PR_SUPER::pointsInRange(id, TimeRange(range.start, qrange.end));
    }
    else if (intersect == TimeRange::intersect_right) {
      // right-fill query
      n_range.start = range.end;
      n_range.end = qrange.end;
      left = DB_PR_SUPER::pointsInRange(id, TimeRange(qrange.start, range.end));
      middle = this->pointsWithOpcFilter(this->selectRange(id, n_range));
    }
    else if (intersect == TimeRange::intersect_other_external){
      // query overlaps but extends on both sides
      TimeRange q_left, q_right;
      
      q_left.start = qrange.start;
      q_left.end = range.start;
      q_right.start = range.end;
      q_right.end = qrange.end;
      
      left = this->pointsWithOpcFilter(this->selectRange(id, q_left));
      middle = DB_PR_SUPER::pointsInRange(id, range);
      right = this->pointsWithOpcFilter(this->selectRange(id, q_right));
    }
    else {
      middle = this->pointsWithOpcFilter(this->selectRange(id, qrange));
    }
    // db hit
    
    vector<Point> merged;
    merged.reserve(middle.size() + left.size() + right.size());
    merged.insert(merged.end(), left.begin(), left.end());
    merged.insert(merged.end(), middle.begin(), middle.end());
    merged.insert(merged.end(), right.begin(), right.end());

    set<time_t> addedTimes;
    vector<Point> deDuped;
    deDuped.reserve(merged.size());
    for(const Point& p : merged) {
      if (addedTimes.count(p.time) == 0) {
        addedTimes.insert(p.time);
        if (qrange.start <= p.time && p.time <= qrange.end) {
          deDuped.push_back(p);
        }
      }
    }
    
    _last_request = (deDuped.size() > 0) ? request_t(id, qrange) : request_t(id,TimeRange());
    DB_PR_SUPER::addPoints(id, deDuped);
    return deDuped;
  }
}


void DbPointRecord::addPoint(const string& id, Point point) {
  scoped_lock<boost::signals2::mutex> lock(*_db_mutex);
  if (!this->readonly()) {
    DB_PR_SUPER::addPoint(id, point);
    this->insertSingle(id, point);
  }
}


void DbPointRecord::addPoints(const string& id, std::vector<Point> points) {
  scoped_lock<boost::signals2::mutex> lock(*_db_mutex);
  if (!this->readonly()) {
    DB_PR_SUPER::addPoints(id, points);
    this->insertRange(id, points);
  }
}


void DbPointRecord::reset() {
  scoped_lock<boost::signals2::mutex> lock(*_db_mutex);
  if (!this->readonly()) {
    DB_PR_SUPER::reset();
    //this->truncate();
  }
}


void DbPointRecord::reset(const string& id) {
  scoped_lock<boost::signals2::mutex> lock(*_db_mutex);
  if (!this->readonly()) {
    // deprecate?
    //cout << "Whoops - don't use this" << endl;
    DB_PR_SUPER::reset(id);
    _last_request.clear();
    //this->removeRecord(id);
    // wiped out the record completely, so re-initialize it.
    //this->registerAndGetIdentifier(id);
  }
}

void DbPointRecord::invalidate(const string &identifier) {
  if (!this->readonly()) {
    this->removeRecord(identifier);
    this->reset(identifier);
  }
}


#pragma mark - opc filter list

void DbPointRecord::setOpcFilterType(OpcFilterType type) {
  if (_filterType != type) {
    BufferPointRecord::reset(); // mem cache
    _filterType = type;
    this->dbConnect();
  }
}

DbPointRecord::OpcFilterType DbPointRecord::opcFilterType() {
  return _filterType;
}

std::set<unsigned int> DbPointRecord::opcFilterList() {
  return _opcFilterCodes;
}

void DbPointRecord::clearOpcFilterList() {
  _opcFilterCodes.clear();
  BufferPointRecord::reset(); // mem cache
  this->dbConnect();
}

void DbPointRecord::addOpcFilterCode(unsigned int code) {
  _opcFilterCodes.insert(code);
  BufferPointRecord::reset(); // mem cache
  this->dbConnect();
}

void DbPointRecord::removeOpcFilterCode(unsigned int code) {
  if (_opcFilterCodes.count(code) > 0) {
    _opcFilterCodes.erase(code);
    BufferPointRecord::reset(); // mem cache
    this->dbConnect();
  }
}



vector<Point> DbPointRecord::pointsWithOpcFilter(std::vector<Point> points) {
  vector<Point> out;
  
  BOOST_FOREACH(Point p, points) {
    Point outPoint = this->pointWithOpcFilter(p);
    if (outPoint.isValid) {
      out.push_back(outPoint);
    }
  }
  
  return out;
}

Point DbPointRecord::pointWithOpcFilter(Point p) {
  Point pOut = p;
  
  switch (this->opcFilterType()) {
    case OpcPassThrough:
      break;
    case OpcWhiteList:
      if (this->opcFilterList().count(p.quality) > 0) {
        pOut = Point(p.time, p.value, Point::opc_rtx_override, p.confidence);
      }
      else {
        pOut = Point();
      }
      break;
    case OpcBlackList:
      if (this->opcFilterList().count(p.quality)) {
        pOut = Point();
      }
      else {
        pOut = Point(p.time, p.value, Point::opc_rtx_override, p.confidence);
      }
      break;
    case OpcCodesToValues:
    {
      pOut = Point(p.time, (double)p.quality, Point::opc_rtx_override, p.confidence);
    }
      break;
    case OpcCodesToConfidence:
    {
      pOut = Point(p.time, p.value, Point::opc_rtx_override, (double)p.quality);
    }
      break;
    default:
      break;
  }
  
  return pOut;
  
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
