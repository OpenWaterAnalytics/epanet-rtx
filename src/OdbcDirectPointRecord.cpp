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

PointRecord::IdentifierUnitsList OdbcDirectPointRecord::identifiersAndUnits() {
  if (!this->isConnected()) {
    this->dbConnect();
  }
  
  scoped_lock<boost::signals2::mutex> lock(_odbcMutex);
  SQLHSTMT getIdsStmt = 0;

  IdentifierUnitsList ids;
  if (!this->isConnected()) {
    return ids;
  }
  
  time_t now = time(NULL);
  time_t stale = now - _lastIdRequest;
  _lastIdRequest = now;
  
  if (stale < 5 && !_identifiersAndUnitsCache.empty()) {
    return DbPointRecord::identifiersAndUnits();
  }
  
  string metaQ = querySyntax.metaSelect;
  
  // FIXME
  // add handling for units column - output col 2
  
  SQLCHAR tagName[512];
  SQLLEN tagLengthInd;
  SQLRETURN retcode;
  
  retcode = SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &getIdsStmt);
  retcode = SQLExecDirect(getIdsStmt, (SQLCHAR*)metaQ.c_str(), SQL_NTS);
  
  if (!SQL_SUCCEEDED(retcode)) {
    errorMessage = "Could not read Tag table";
  }
  else {
    while (SQL_SUCCEEDED(SQLFetch(getIdsStmt))) {
      SQLGetData(getIdsStmt, 1, SQL_C_CHAR, tagName, 512, &tagLengthInd);
      string newTag((char*)tagName);
      ids.set(newTag,RTX_DIMENSIONLESS);
    }
  }
  
  SQLFreeStmt(getIdsStmt, SQL_CLOSE);
  SQLFreeHandle(SQL_HANDLE_STMT, getIdsStmt);
  
  _identifiersAndUnitsCache = ids;
  return _identifiersAndUnitsCache;
}



std::vector<Point> OdbcDirectPointRecord::selectRange(const std::string& id, TimeRange range) {
  
  this->checkConnected();
  
  // construct the static query text
  string q = this->stringQueryForRange(id, range);
  vector<Point> points;
  SQLHSTMT rangeStmt = 0;
  
  bool fetchSuccess = false;
  int iFetchAttempt = 0;
  do {
    // execute the query and get a result set
    {
      scoped_lock<boost::signals2::mutex> lock(_odbcMutex);
      if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &rangeStmt))) {
        // FIXME - do something like return a range of bad points?
        cerr << "could not allocate sql handle" << endl;
        errorMessage = extract_error("SQLAllocHandle", _handles.SCADAdbc, SQL_HANDLE_STMT);
        return points;
      }
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

/* selectNext and selectPrevious will not be called since this class does not support "singly-bounded queries" */

Point OdbcDirectPointRecord::selectNext(const std::string& id, time_t time) {
  Point p;
  return p;
}
Point OdbcDirectPointRecord::selectPrevious(const std::string& id, time_t time) {
  Point p;
  return p;
}



std::string OdbcDirectPointRecord::stringQueryForRange(const std::string& id, TimeRange range) {
  
  string query = querySyntax.rangeSelect;
  string startStr,endStr;
  
  if (this->timeFormat() == PointRecordTime::UTC) {
    startStr = PointRecordTime::utcDateStringFromUnix(range.start-1);
    endStr = PointRecordTime::utcDateStringFromUnix(range.end+1);
  }
  else {
    startStr = PointRecordTime::localDateStringFromUnix(range.start-1, _specifiedTimeZone);
    endStr = PointRecordTime::localDateStringFromUnix(range.end+1, _specifiedTimeZone);
  }
  
  string startDateStr = "'" + startStr + "'"; // minus one because of wonderware's silly "initial value" in delta retrieval.
  string endDateStr = "'" + endStr + "'"; // because wonderware does fractional seconds
  string idStr = "'" + id + "'";
  
  boost::replace_first(query, "?", idStr);
  boost::replace_first(query, "?", startDateStr);
  boost::replace_first(query, "?", endDateStr);
  
  return query;
  
}

std::string OdbcDirectPointRecord::stringQueryForSinglyBoundedRange(const string& id, time_t bound, OdbcQueryBoundType boundType) {
  
  string query("");
  return query;
}


