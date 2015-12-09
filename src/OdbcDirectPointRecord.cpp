#include "OdbcDirectPointRecord.h"

#include <boost/algorithm/string.hpp>

#include <iostream>

#define RTX_ODBCDIRECT_MAX_RETRY 5

using namespace RTX;
using namespace std;

using boost::signals2::mutex;


OdbcDirectPointRecord::OdbcDirectPointRecord() {
  
}

OdbcDirectPointRecord::~OdbcDirectPointRecord() {
  
}


void OdbcDirectPointRecord::dbConnect() throw(RtxException) {
  OdbcPointRecord::dbConnect();
}

const std::map<std::string,Units> OdbcDirectPointRecord::identifiersAndUnits() {
  scoped_lock<boost::signals2::mutex> lock(_odbcMutex);
  SQLHSTMT getIdsStmt = 0;

  std::map<std::string,Units> ids;
  if (!isConnected()) {
    return ids;
  }
  
  time_t now = time(NULL);
  time_t stale = now - _lastIdRequest;
  _lastIdRequest = now;
  
  if (stale < 5 && !_identifiersAndUnitsCache.empty()) {
    return DbPointRecord::identifiersAndUnits();
  }
  
  
  string tagQuery("");
  tagQuery += "SELECT " + tableDescription.tagNameCol;
  if (!RTX_STRINGS_ARE_EQUAL(tableDescription.tagUnitsCol,"")) {
    tagQuery += ", " + tableDescription.tagUnitsCol;
  }
  tagQuery += " FROM " + tableDescription.tagTable + " ORDER BY " + tableDescription.tagNameCol;
  
  SQLCHAR tagName[512];
  SQLLEN tagLengthInd;
  SQLRETURN retcode;
  
  retcode = SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &getIdsStmt);
  retcode = SQLExecDirect(getIdsStmt, (SQLCHAR*)tagQuery.c_str(), SQL_NTS);
  
  if (!SQL_SUCCEEDED(retcode)) {
    errorMessage = "Could not read Tag table";
  }
  else {
    while (SQL_SUCCEEDED(SQLFetch(getIdsStmt))) {
      SQLGetData(getIdsStmt, 1, SQL_C_CHAR, tagName, 512, &tagLengthInd);
      string newTag((char*)tagName);
      ids[newTag] = RTX_DIMENSIONLESS;
    }
  }
  
  SQLFreeStmt(getIdsStmt, SQL_CLOSE);
  SQLFreeHandle(SQL_HANDLE_STMT, getIdsStmt);
  
  _identifiersAndUnitsCache = ids;
  return ids;
}



std::vector<Point> OdbcDirectPointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  
  this->checkConnected();
  
  // construct the static query text
  string q = this->stringQueryForRange(id, startTime, endTime);
  vector<Point> points;
  SQLHSTMT rangeStmt = 0;
  
  bool fetchSuccess = false;
  int iFetchAttempt = 0;
  do {
    // execute the query and get a result set
    {
      scoped_lock<boost::signals2::mutex> lock(_odbcMutex);
      SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &rangeStmt);
      if (SQL_SUCCEEDED(SQLExecDirect(rangeStmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
        fetchSuccess = true;
        points = this->pointsFromStatement(rangeStmt);
      }
      SQLFreeStmt(rangeStmt, SQL_CLOSE);
      SQLFreeHandle(SQL_HANDLE_STMT, rangeStmt);
    }
    
    if(!fetchSuccess) {
      {
        scoped_lock<boost::signals2::mutex> lock(_odbcMutex);
        cerr << extract_error("SQLExecDirect", rangeStmt, SQL_HANDLE_STMT) << endl;
        cerr << "query did not succeed: " << q << endl;
      }
      // do something more intelligent here. re-check connection?
      this->dbConnect();
    }
    ++iFetchAttempt;
  } while (!fetchSuccess && iFetchAttempt < RTX_ODBCDIRECT_MAX_RETRY);
  
  
  
  
  
  return points;
}

Point OdbcDirectPointRecord::selectNext(const std::string& id, time_t time) {
  scoped_lock<boost::signals2::mutex> lock(_odbcMutex);
  this->checkConnected();
  SQLHSTMT rangeStmt = 0;
  
  if (this->supportsBoundedQueries()) {
    vector<Point> points;
    
    bool fetchSuccess = false;
    int iFetchAttempt = 0;

    do {
      SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &rangeStmt);
      string q = stringQueryForSinglyBoundedRange(id, time, OdbcQueryBoundLower);
      if (SQL_SUCCEEDED(SQLExecDirect(rangeStmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
        fetchSuccess = true;
        points = pointsFromStatement(rangeStmt);
      }
      else {
        cerr << "query did not succeed: " << q << endl;
        this->dbConnect();
      }
      
      ++iFetchAttempt;
      SQLFreeStmt(rangeStmt, SQL_CLOSE);
      SQLFreeHandle(SQL_HANDLE_STMT, rangeStmt);

    } while (!fetchSuccess && iFetchAttempt < RTX_ODBCDIRECT_MAX_RETRY);
    
    
    
    if (points.size() > 0) {
      return points.back();
    }
    else {
      cerr << "no points found for " << id << endl;
    }
    
  }
  else {
    return this->selectNextIteratively(id, time);
  }
  
  return Point();
}

Point OdbcDirectPointRecord::selectNextIteratively(const std::string &id, time_t time) {
  Point p;
  vector<Point> points;
  time_t margin = 60*60*12;
  time_t max_margin = this->searchDistance();
  time_t lookahead = time;
  
  SQLHSTMT rangeStmt = 0;
  string q;
  
  SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &rangeStmt);
  
  while (points.size() == 0 && lookahead < time + max_margin) {
    
    q = this->stringQueryForRange(id, lookahead, lookahead+margin);
    
    if (SQL_SUCCEEDED(SQLExecDirect(rangeStmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
      points = pointsFromStatement(rangeStmt);
    }
    else {
      cerr << "query did not succeed: " << q << endl;
    }
    lookahead += margin;
  }
  // cancel and then fre handle?
  SQLFreeStmt(rangeStmt, SQL_CLOSE);
  SQLFreeHandle(SQL_HANDLE_STMT, rangeStmt);
  
  
  if (points.size() > 0) {
    p = points.front();
    int i = 0;
    while (p.time <= time && i < points.size()) {
      p = points.at(i);
      ++i;
    }
  }
  else {
    //cerr << "no points found for " << id << " :: range " << time - 1 << " - " << lookahead + margin << endl;
  }
  
  return p;
}

Point OdbcDirectPointRecord::selectPrevious(const std::string& id, time_t time) {
  scoped_lock<boost::signals2::mutex> lock(_odbcMutex);
  this->checkConnected();
  SQLHSTMT rangeStmt = 0;

  if (this->supportsBoundedQueries()) {
    vector<Point> points;
    
    SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &rangeStmt);
    string q = stringQueryForSinglyBoundedRange(id, time, OdbcQueryBoundUpper);
    if (SQL_SUCCEEDED(SQLExecDirect(rangeStmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
      points = pointsFromStatement(rangeStmt);
    }
    else {
      cerr << "query did not succeed: " << q << endl;
    }
    
    if (points.size() > 0) {
      return points.back();
    }
    else {
      cerr << "no points found for " << id << endl;
    }
    
    SQLFreeStmt(rangeStmt, SQL_CLOSE);
    SQLFreeHandle(SQL_HANDLE_STMT, rangeStmt);
  }
  else {
    
    return this->selectPreviousIteratively(id, time);
    
  }
  
  return Point();
}

Point OdbcDirectPointRecord::selectPreviousIteratively(const std::string &id, time_t time) {
  Point p;
  vector<Point> points;
  time_t margin = 60*60*12;
  time_t max_margin = this->searchDistance();
  time_t lookBehind = time;
  SQLHSTMT rangeStmt = 0;

  string q;
  
  SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &rangeStmt);
  
  while (points.size() == 0 && lookBehind > time - max_margin) {
    
    q = this->stringQueryForRange(id, lookBehind-margin, lookBehind+1);
    
    if (SQL_SUCCEEDED(SQLExecDirect(rangeStmt, (SQLCHAR*)q.c_str(), SQL_NTS))) {
      points = pointsFromStatement(rangeStmt);
    }
    else {
      cerr << "query did not succeed: " << q << endl;
    }
    lookBehind -= margin;
  }
  
  SQLFreeStmt(rangeStmt, SQL_CLOSE);
  SQLFreeHandle(SQL_HANDLE_STMT, rangeStmt);
  
  
  if (points.size() > 0) {
    p = points.back();
    while ( p.time >= time && points.size() > 0 ) {
      points.pop_back();
      p = points.back();
    }
  }
  else {
    cerr << "no points found for " << id << endl;
  }
  
  return p;
}


std::string OdbcDirectPointRecord::stringQueryForRange(const std::string& id, time_t start, time_t end) {
  
  string query = _querySyntax.rangeSelect;
  string startStr,endStr;
  
  if (this->timeFormat() == PointRecordTime::UTC) {
    startStr = PointRecordTime::utcDateStringFromUnix(start-1);
    endStr = PointRecordTime::utcDateStringFromUnix(end+1);
  }
  else {
    startStr = PointRecordTime::localDateStringFromUnix(start-1, _specifiedTimeZone);
    endStr = PointRecordTime::localDateStringFromUnix(end+1, _specifiedTimeZone);
  }
  
  string startDateStr = "'" + startStr + "'"; // minus one because of wonderware's bullshit "initial value" in delta retrieval.
  string endDateStr = "'" + endStr + "'"; // because wonderware does fractional seconds
  string idStr = "'" + id + "'";
  
  boost::replace_first(query, "?", idStr);
  boost::replace_first(query, "?", startDateStr);
  boost::replace_first(query, "?", endDateStr);
  
  return query;
  
}

std::string OdbcDirectPointRecord::stringQueryForSinglyBoundedRange(const string& id, time_t bound, OdbcQueryBoundType boundType) {
  
  string query("");
  
  switch (boundType) {
    case OdbcQueryBoundLower:
      query = _querySyntax.lowerBound;
      break;
    case OdbcQueryBoundUpper:
      query = _querySyntax.upperBound;
    default:
      break;
  }
  
  string idStr = "'" + id + "'";
  string boundDateStr = "'" + PointRecordTime::utcDateStringFromUnix(bound) + "'";
  
  boost::replace_first(query, "?", idStr);
  boost::replace_first(query, "?", boundDateStr);
  
  return query;
}


