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
#include <string>
#include <thread>
#include <mutex>
#include <shared_mutex>

#include "DbPointRecord.h"
#include "DbAdapter.h"

using namespace RTX;
using namespace std;

#define _DB_MAX_CONNECT_TRY 5
#define SERIES_LIST_TTL 30
#define FUSE_DURATION 1

/************ request type *******************/

DbPointRecord::request_t::request_t(string id, TimeRange r_range) : range(r_range), id(id) { }

bool DbPointRecord::request_t::contains(std::string id, time_t t) {
  if (this->range.start <= t 
      && t <= this->range.end 
      && RTX_STRINGS_ARE_EQUAL(id, this->id)) {
    return true;
  }
  return false;
}

void DbPointRecord::request_t::clear() {
  this->range = TimeRange();
  this->id = "";
}

/************ widequery info *******************/

bool DbPointRecord::WideQueryInfo::valid() {
  if ((time(NULL) - _queryTime) < ttl) {
    return true;
  }
  else {
    return false;
  }
};



/************ impl *******************/

DbPointRecord::DbPointRecord() : _last_request("",TimeRange()) {
  _lastFailedAttempt = std::chrono::time_point<std::chrono::system_clock>();
  _adapter = NULL;
  errorMessage = "Not Connected";
  _readOnly = false;
  _filterType = OpcNoFilter;
  
  iterativeSearchMaxIterations = 8;
  iterativeSearchStride = 3*60*60;
    
  _errCB = [&](const std::string& msg)->void {
    this->errorMessage = msg;
  };
  
  this->setOpcFilterType(OpcPassThrough);
}

void DbPointRecord::setConnectionString(const std::string &str) {
  _adapter->setConnectionString(str);
  _lastFailedAttempt = std::chrono::time_point<std::chrono::system_clock>();
}
string DbPointRecord::connectionString() {
  return _adapter->connectionString();
}


bool DbPointRecord::isConnected() {
  return _adapter->adapterConnected();
}

void DbPointRecord::dbConnect() {
  try {
    if (_adapter != NULL) {
      if(_badConnection == true){
        auto thisAttempt = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = thisAttempt-_lastFailedAttempt;
        if(elapsed_seconds <= std::chrono::minutes(FUSE_DURATION)){
          return;
        }
      }
      _adapter->doConnect();
      _badConnection = false;
    }
  } catch (exception &e) {
    _badConnection = true;
    _lastFailedAttempt = std::chrono::system_clock::now();
    cerr << "could not connect to db: " << _adapter->connectionString() << endl;
    cerr << e.what() << endl;
  }
  
}

bool DbPointRecord::checkConnected() {
  int iTry = 0;
  while (!_adapter->adapterConnected() && ++iTry < _DB_MAX_CONNECT_TRY) {
    this->dbConnect();
  }
  return isConnected();
}

bool DbPointRecord::readonly() {
  if (_adapter->options().implementationReadonly) {
    return true;
  }
  else {
    return _readOnly;
  }
}

void DbPointRecord::setReadonly(bool readOnly) {
  if (_adapter->options().implementationReadonly) {
    _readOnly = false;
    return;
  }
  else {
    _readOnly = readOnly;
  }
}


/******
 
 the logic below is incredibly complex. i've tried to flatten it out by providing multiple exits
 but it still feels a bit  overwrought. 
 
******/
bool DbPointRecord::registerAndGetIdentifierForSeriesWithUnits(string name, Units units) {
  if (name.length() == 0) {
    return false;
  }
  
  bool nameExists = false;
  bool unitsMatch = false;
  Units existingUnits = RTX_NO_UNITS;
  auto match = this->identifiersAndUnits().doesHaveIdUnits(name,units);
  std::lock_guard lock(_db_readwrite); // get a write lock
  
  if (!checkConnected()) {
    return DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
  }
  
  nameExists = match.first;
  unitsMatch = match.second;
  
    
  ///// sucess cases
  
  // ideal case: everything checks out.
  if (nameExists 
      && unitsMatch) 
  {
    return DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
  }
  
  // no matter what happens, we are messing with cache invalidation... 
  if (!_adapter->inTransaction()) {
    _lastIdRequest = 0; // so invalidate the request cache.  
  }

  // almost ideal: name is good, units mismatch, but adaptor doesn't care
  if (nameExists 
      && !unitsMatch 
      && !_adapter->options().supportsUnitsColumn) 
  {
    return DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
  }
  
  // special snowflake case: name is good, existing units don't match and are junk, but adapter can assign. 
  if (nameExists 
      && !unitsMatch 
      && _adapter->options().canAssignUnits 
      && existingUnits == RTX_NO_UNITS) 
  {
    _adapter->assignUnitsToRecord(name, units);
    return DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
  }
  
  //// cases where a series is created in the record
  if (!this->readonly()) {
    
    // name is there, units don't match, and we were not a snowflake.
    if (nameExists && !unitsMatch) {
      // remove old record and register new one
      _adapter->removeRecord(name);
    }
    
    // by now, nothing is there - insert a new series.
    bool inserted = _adapter->insertIdentifierAndUnits(name, units);
    bool cached = DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
    return inserted && cached;
  }
  
  
  //// failure cases
  
  // if nothing above applies, and the record is readonly, then fail.
  if (this->readonly()) {
    return false;
  }
    
  // everything else is some combination of failures
  
  return false;
}


IdentifierUnitsList DbPointRecord::identifiersAndUnits() {
  std::shared_lock lock(_db_readwrite); // get a read lock
  time_t now = time(NULL);
  time_t stale = now - _lastIdRequest;
  
  if ((stale < SERIES_LIST_TTL || _adapter->inTransaction()) && !_identifiersAndUnitsCache.get()->empty()) {
    return _identifiersAndUnitsCache;
  }
  
  if (checkConnected()) {
    _identifiersAndUnitsCache = _adapter->idUnitsList();
    _lastIdRequest = time(NULL);
  }
  return _identifiersAndUnitsCache;
}



void DbPointRecord::beginBulkOperation() {
  if (checkConnected()) {
    _adapter->beginTransaction();
  }
}

void DbPointRecord::endBulkOperation() {
  if (checkConnected()) {
    _adapter->endTransaction();
  }
}

void DbPointRecord::willQuery(RTX::TimeRange range) {
  if (checkConnected()) {
    // make sure the id list is current:
    _lastIdRequest = 0;
    this->identifiersAndUnits();
    
    auto fetch = _adapter->wideQuery(range);
    for (auto res : fetch) {
      DB_PR_SUPER::addPoints(res.first, res.second);
    }
    // optimization: if the adaptor supports wide query then allow queries to bypass db hits
    // so cache the range of that query.
    if (_adapter->options().canDoWideQuery) {
      _wideQuery = WideQueryInfo(range);
    }
  }
}

vector<Point> DbPointRecord::pointsWithQuery(const string& query, TimeRange range) {
  if (checkConnected()) {
    return _adapter->selectWithQuery(query, range);
  }
  return vector<Point>();
}


Point DbPointRecord::point(const string& id, time_t time) {
  
  Point p = DB_PR_SUPER::point(id, time);
  
  if (!checkConnected()) {
    return p;
  }
  
  // if there is a valid ("alive") wide query, and if the time requested is in range, then the point should be there.
  // if it's not there, it doesn't exist (within TTL anyway)
  if (_wideQuery.valid() && _wideQuery.range().contains(time)) {
    return p;
  }
  
  if (!p.isValid) {
    
    // see if we just asked the db for something in this range.
    // if so, and Super couldn't find it, then it's just not here.
    // todo -- check staleness
    
    if (_last_request.contains(id, time)) {
      return Point();
    }
    
    if (_wideQuery.valid() && _wideQuery.range().contains(time)) {
      return Point();
    }
    
    time_t margin = 60*60*12;
    time_t start = time - margin, end = time + margin;
    
    // do the request, and cache the request parameters.
    
    vector<Point> pVec = _adapter->selectRange(id, TimeRange(start, end));
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


Point DbPointRecord::pointBefore(const string& id, time_t time, WhereClause q) {
  
  // available in circular buffer?
  Point p = DB_PR_SUPER::pointBefore(id, time, q);
  if (p.isValid) {
    return p;
  }
  
  // if there is a valid ("alive") wide query, and if the time requested is in range, then the point should be there.
  // The Base buffer class might not catch this, since it depends on a time range based on extant data.
  // so we (partially) reproduce some logic here to get that edge case.
  if (_wideQuery.valid()) {
    // the actual effective range is the superset of the buffered range and the wide query range.
    TimeRange actualRange = TimeRange::unionOf(_wideQuery.range(), DB_PR_SUPER::range(id));
    
    if (actualRange.contains(time)) {
      // last check...
      
      auto existing = DB_PR_SUPER::pointsInRange(id, actualRange);
      // if no existing, definitely no points
      if (existing.size() == 0) {
        return p;
      }
      // existing points could be before/after the desired timestamp...
      // so just check if our buffer's last point is before requested.
      if (existing.back().time < time) {
        return existing.back();
      }
    }
    else {
      return p;
    }
    
  }
  
  
  // if it's not buffered, but the last request covered this range, then there is no point here.
//  if (_last_request.contains(id, time-1)) {
//    return Point();
//  }
  
  if (!checkConnected()) {
    return p;
  }
  
  // should i search iteratively?
  if (_adapter->options().searchIteratively) {
    p = this->searchPreviousIteratively(id, time);
    if (p.isValid) {
      return this->pointWithOpcFilter(p);
    }
  }
  
  
  // try a singly-bounded query
  if (_adapter->options().supportsSinglyBoundQuery) {
    p = _adapter->selectPrevious(id, time, q);
  }
  if (p.isValid) {
    return this->pointWithOpcFilter(p);
  }

  
  return p;
}


Point DbPointRecord::pointAfter(const string& id, time_t time, WhereClause q) {
  // buffered?
  Point p = DB_PR_SUPER::pointAfter(id, time, q);
  if (p.isValid) {
    return p;
  }
  
  // if there is a valid ("alive") wide query, and if the time requested is in range, then the point should be there.
  // if it's not there, it doesn't exist (within TTL anyway)
  if (_wideQuery.valid()) {
    // the actual effective range is the superset of the buffered range and the wide query range.
    TimeRange actualRange = TimeRange::unionOf(_wideQuery.range(), DB_PR_SUPER::range(id));
    
    if (actualRange.contains(time)) {
      // last check...
      
      auto existing = DB_PR_SUPER::pointsInRange(id, actualRange);
      // if no existing, definitely no points
      if (existing.size() == 0) {
        return p;
      }
      // existing points could be before/after the desired timestamp...
      // so just check if our buffer's first point is after requested.
      if (existing.front().time > time) {
        return existing.front();
      }
    }
    else {
      return p;
    }
  }
  
  // last request covered this already?
//  if (_last_request.contains(id, time + 1)) {
//    return Point();
//  }
  
  if (!checkConnected()) {
    return p;
  }
  
  if (_adapter->options().searchIteratively) {
    p = this->searchNextIteratively(id, time);
  }
  if (p.isValid) {
    return this->pointWithOpcFilter(p);
  }
  
  // singly bounded?
  if (_adapter->options().supportsSinglyBoundQuery) {
    p = _adapter->selectNext(id, time, q);
  }
  if (p.isValid) {
    return this->pointWithOpcFilter(p);
  }
  
  return p;
}


Point DbPointRecord::searchPreviousIteratively(const string& id, time_t time) {
  int lookBehindLimit = iterativeSearchMaxIterations;
  
  if (!checkConnected()) {
    return Point();
  }
  
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
  
  return Point();
}

Point DbPointRecord::searchNextIteratively(const string& id, time_t time) {
  int lookAheadLimit = iterativeSearchMaxIterations;
  
  if (!checkConnected()) {
    return Point();
  }
  
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
  
  return Point();
}


std::vector<Point> DbPointRecord::pointsInRange(const string& id, TimeRange qrange) {
  std::shared_lock lock(_db_readwrite); // get a read lock
  
  // limit double-queries
  if (_last_request.range.containsRange(qrange) && _last_request.id == id) {
    return DB_PR_SUPER::pointsInRange(id, qrange);
  }
  
  // wide query optimization
  if (_wideQuery.valid() && _wideQuery.range().intersection(qrange) == TimeRange::intersect_other_internal) {
    return DB_PR_SUPER::pointsInRange(id, qrange);
  }
  else {
    _wideQuery = WideQueryInfo(); // the intersection did not align, so invalidate this wide query marker. Beyond here we may mutate the cache.
  }
  
  
  if (!checkConnected()) {
    return DB_PR_SUPER::pointsInRange(id, qrange);
  }
  
  TimeRange range = DB_PR_SUPER::range(id);
  TimeRange::intersect_type intersect = range.intersection(qrange);

  // if the requested range is not in memcache, then fetch it.
  if ( intersect == TimeRange::intersect_other_internal || intersect == TimeRange::intersect_equal ) {
    return DB_PR_SUPER::pointsInRange(id, qrange);
  }
  else {
    vector<Point> left, middle, right;
    TimeRange n_range;
    
    if (intersect == TimeRange::intersect_left) {
      // left-fill query
      n_range.start = qrange.start;
      n_range.end = range.start;
      middle = this->pointsWithOpcFilter(_adapter->selectRange(id, n_range));
      right = DB_PR_SUPER::pointsInRange(id, TimeRange(range.start, qrange.end));
    }
    else if (intersect == TimeRange::intersect_right) {
      // right-fill query
      n_range.start = range.end;
      n_range.end = qrange.end;
      left = DB_PR_SUPER::pointsInRange(id, TimeRange(qrange.start, range.end));
      middle = this->pointsWithOpcFilter(_adapter->selectRange(id, n_range));
    }
    else if (intersect == TimeRange::intersect_other_external){
      // query overlaps but extends on both sides
      TimeRange q_left, q_right;
      
      q_left.start = qrange.start;
      q_left.end = range.start;
      q_right.start = range.end;
      q_right.end = qrange.end;
      
      left = this->pointsWithOpcFilter(_adapter->selectRange(id, q_left));
      middle = DB_PR_SUPER::pointsInRange(id, range);
      right = this->pointsWithOpcFilter(_adapter->selectRange(id, q_right));
    }
    else {
      middle = this->pointsWithOpcFilter(_adapter->selectRange(id, qrange));
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
    
    // mutation requires getting a write lock...
    lock.unlock();
    {
      std::lock_guard lock2(_db_readwrite);
      _last_request = (deDuped.size() > 0) ? request_t(id, qrange) : request_t(id,TimeRange());
    }
    DB_PR_SUPER::addPoints(id, deDuped);
    return deDuped;
  }
}


void DbPointRecord::addPoint(const string& id, Point point) {
  std::lock_guard lock(_db_readwrite); // get a write lock
  if (!this->readonly() && checkConnected()) {
    DB_PR_SUPER::addPoint(id, point);
    _adapter->insertSingle(id, point);
  }
}


void DbPointRecord::addPoints(const string& id, std::vector<Point> points) {
  std::lock_guard lock(_db_readwrite); // get a write lock
  if (!this->readonly() && checkConnected()) {
    DB_PR_SUPER::addPoints(id, points);
    _adapter->insertRange(id, points);
  }
}


void DbPointRecord::truncate() {
  std::lock_guard lock(_db_readwrite); // get a write lock
  if (!this->readonly() && checkConnected()) {
    DB_PR_SUPER::reset();
    _adapter->removeAllRecords();
  }
}

void DbPointRecord::reset() {
  std::lock_guard lock(_db_readwrite); // get a write lock
  if (!this->readonly() && checkConnected()) {
    DB_PR_SUPER::reset();
    cerr << "deprecated. do not use" << endl;
    //this->truncate();
  }
}


void DbPointRecord::reset(const string& id) {
  std::lock_guard lock(_db_readwrite); // get a write lock
  if (!this->readonly() && checkConnected()) {
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
  if (!this->readonly() && checkConnected()) {
    _adapter->removeRecord(identifier);
    this->reset(identifier);
  }
}


#pragma mark - opc filter list

void DbPointRecord::setOpcFilterType(OpcFilterType type) {
  
  const map< OpcFilterType,function<Point(Point)> > opcFilters({
    { OpcPassThrough , 
      [&](Point p)->Point { 
        return p;
      } },
    { OpcWhiteList ,   
      [&](Point p)->Point { 
        if (this->opcFilterList().count(p.quality) > 0) {
          return Point(p.time, p.value, Point::opc_rtx_override, p.confidence);
        }
        else {
          return Point();
        }
      } },
    { OpcBlackList ,
      [&](Point p)->Point {
        if (this->opcFilterList().count(p.quality)) {
          return Point();
        }
        else {
          return Point(p.time, p.value, Point::opc_rtx_override, p.confidence);
        }
      }
    },
    { OpcCodesToValues ,
      [&](Point p)->Point {
        return Point(p.time, (double)p.quality, Point::opc_rtx_override, p.confidence);
      }
    },
    { OpcCodesToConfidence ,
      [&](Point p)->Point {
        return Point(p.time, p.value, Point::opc_rtx_override, (double)p.quality);
      }
    }
  });

  
  if (_filterType != type) {
    BufferPointRecord::reset(); // mem cache
    _filterType = type;
    _opcFilter = opcFilters.at(type);
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
  if (this->isConnected()) {
    this->dbConnect();
  }
}

void DbPointRecord::addOpcFilterCode(unsigned int code) {
  _opcFilterCodes.insert(code);
  BufferPointRecord::reset(); // mem cache
  if (this->isConnected()) {
    this->dbConnect();
  }
}

void DbPointRecord::removeOpcFilterCode(unsigned int code) {
  if (_opcFilterCodes.count(code) > 0) {
    _opcFilterCodes.erase(code);
    BufferPointRecord::reset(); // mem cache
    if (this->isConnected()) {
      this->dbConnect();
    }
  }
}



vector<Point> DbPointRecord::pointsWithOpcFilter(std::vector<Point> points) {
  vector<Point> out;
  
  for(const Point& p : points) {
    Point outPoint = this->pointWithOpcFilter(p);
    if (outPoint.isValid) {
      out.push_back(outPoint);
    }
  }
  
  return out;
}


Point DbPointRecord::pointWithOpcFilter(Point p) {
  return _opcFilter(p);
}

