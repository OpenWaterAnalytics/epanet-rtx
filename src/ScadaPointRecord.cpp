//
//  ScadaPointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include "ScadaPointRecord.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

ScadaPointRecord::ScadaPointRecord() {
  _connectionOk = false;
}


ScadaPointRecord::~ScadaPointRecord() {
  
}

#pragma mark -
#pragma mark Initialization

void ScadaPointRecord::setSyntax(const string& table, const string& dateCol, const string& tagCol, const string& valueCol, const string& qualityCol) {
  string dataPointQuery, dataRangeQuery, dataLowerBoundQuery, dataUpperBoundQuery, dataTimeQuery;
  
  // defaults are good for MSSQL-style syntax. override these defaults for other schemas.
  
  dataPointQuery = "SELECT " + dateCol + ", " + tagCol + ", " + valueCol + ", " + qualityCol +
                  " FROM " + table +
                  " WHERE (" + dateCol + " = ?) AND " + tagCol + " = ?";
  
  dataRangeQuery = "SELECT " + dateCol + ", " + tagCol + ", " + valueCol + ", " + qualityCol +
                  " FROM " + table +
                  " WHERE (" + dateCol + " >= ?) AND (" + dateCol + " <= ?) AND " + tagCol + " = ? ORDER BY " + dateCol + " asc";
  
  dataLowerBoundQuery =  "SELECT TOP 2 " + dateCol + ", " + tagCol + ", " + valueCol + ", " + qualityCol +
                        " FROM " + table +
                        " WHERE (" + dateCol + " > ?) AND (" + dateCol + " <= ?) AND " + tagCol + " = ?" +
                        " ORDER BY " + dateCol + " DESC";
  
  dataUpperBoundQuery =  "SELECT TOP 2 " + dateCol + ", " + tagCol + ", " + valueCol + ", " + qualityCol +
                        " FROM " + table +
                        " WHERE (" + dateCol + " > ?) AND (" + dateCol + " <= ?) AND " + tagCol + " = ?" +
                        " ORDER BY " + dateCol + " ASC";
  
  dataTimeQuery = "SELECT CONVERT(datetime, GETDATE()) AS DT";
  
  
  _query.tagNameInd = SQL_NTS;
  
  
  setSingleSelectQuery(dataPointQuery);
  setRangeSelectQuery(dataRangeQuery);
  setUpperBoundSelectQuery(dataUpperBoundQuery);
  setLowerBoundSelectQuery(dataLowerBoundQuery);
  setTimeQuery(dataTimeQuery);
  
}


void ScadaPointRecord::connect() throw(RtxException) {
  _connectionOk = false;
  if (RTX_STRINGS_ARE_EQUAL(this->connectionString(), "")) {
    return;
  }
  
  
  SQLRETURN sqlRet;
  
  try {
    /* Allocate an environment handle */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_SCADAenv), "SQLAllocHandle", _SCADAenv, SQL_HANDLE_ENV);
    /* We want ODBC 3 support */
    SQL_CHECK(SQLSetEnvAttr(_SCADAenv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0), "SQLSetEnvAttr", _SCADAenv, SQL_HANDLE_ENV);
    /* Allocate a connection handle */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_DBC, _SCADAenv, &_SCADAdbc), "SQLAllocHandle", _SCADAenv, SQL_HANDLE_ENV);
    /* Connect to the DSN, checking for connectivity */
    //"Attempting to Connect to SCADA..."
    
    SQLSMALLINT returnLen;
    //SQL_CHECK(SQLDriverConnect(_SCADAdbc, NULL, (SQLCHAR*)(this->connectionString()).c_str(), SQL_NTS, NULL, 0, &returnLen, SQL_DRIVER_COMPLETE), "SQLDriverConnect", _SCADAdbc, SQL_HANDLE_DBC);
    sqlRet = SQLDriverConnect(_SCADAdbc, NULL, (SQLCHAR*)(this->connectionString()).c_str(), SQL_NTS, NULL, 0, &returnLen, SQL_DRIVER_COMPLETE);

    SQL_CHECK(sqlRet, "SQLDriverConnect", _SCADAdbc, SQL_HANDLE_DBC);
    
    /* allocate the statement handles for data aquisition */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _SCADAdbc, &_SCADAstmt), "SQLAllocHandle", _SCADAstmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _SCADAdbc, &_rangeStatement), "SQLAllocHandle", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _SCADAdbc, &_lowerBoundStatement), "SQLAllocHandle", _lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _SCADAdbc, &_upperBoundStatement), "SQLAllocHandle", _upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _SCADAdbc, &_SCADAtimestmt), "SQLAllocHandle", _SCADAtimestmt, SQL_HANDLE_STMT);
    
    // bindings for single point statement
    /* bind tempRecord members to SQL return columns */
    bindOutputColumns(_SCADAstmt, &_tempRecord);
    // bind input parameters, so we can easily change them when we want to make requests.
    sqlRet = SQLBindParameter(_SCADAstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd);
    sqlRet = SQLBindParameter(_SCADAstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd);
    
    // bindings for the range statement
    bindOutputColumns(_rangeStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_rangeStatement, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_rangeStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_rangeStatement, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _rangeStatement, SQL_HANDLE_STMT);
    
    // bindings for lower bound statement
    bindOutputColumns(_lowerBoundStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_lowerBoundStatement, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_lowerBoundStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_lowerBoundStatement, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _lowerBoundStatement, SQL_HANDLE_STMT);
    
    // bindings for upper bound statement
    bindOutputColumns(_upperBoundStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_upperBoundStatement, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_upperBoundStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_upperBoundStatement, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _upperBoundStatement, SQL_HANDLE_STMT);
    
    
    // prepare the statements
    SQL_CHECK(SQLPrepare(_SCADAstmt, (SQLCHAR*)singleSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _SCADAstmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_rangeStatement, (SQLCHAR*)rangeSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_SCADAtimestmt, (SQLCHAR*)timeQuery().c_str(), SQL_NTS), "SQLPrepare", _SCADAtimestmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_lowerBoundStatement, (SQLCHAR*)loweBoundSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_upperBoundStatement, (SQLCHAR*)upperBoundSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _upperBoundStatement, SQL_HANDLE_STMT);
    
    // if we made it this far...
    _connectionOk = true;
    
  } catch (std::string errorMessage) {
    std::cerr << "Initialize failed: " << errorMessage << "\n";
    _connectionOk = false;
    //throw DbPointRecord::RtxDbConnectException();
  }
  
  // todo -- check time offset (dst?)

}

bool ScadaPointRecord::isConnected() {
  return _connectionOk;
}

std::string ScadaPointRecord::registerAndGetIdentifier(std::string recordName) {
  return DB_PR_SUPER::registerAndGetIdentifier(recordName);
}

std::vector<std::string> ScadaPointRecord::identifiers() {
  std::vector<std::string> ids;
  if (!isConnected()) {
    return ids;
  }
  
  // get tag names from db.
  std::string tagQuery = "SELECT TagName FROM Tag ORDER BY TagName";
  SQLCHAR tagName[512];
  SQLHSTMT tagStmt;
  SQLRETURN retCode;
  SQLLEN tagLengthInd;
  
  SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _SCADAdbc, &tagStmt), "SQLAllocHandle", _SCADAstmt, SQL_HANDLE_STMT);
  SQL_CHECK(SQLPrepare(tagStmt, (SQLCHAR*)tagQuery.c_str(), SQL_NTS), "identifiers", _SCADAdbc, SQL_HANDLE_DBC);
  SQL_CHECK(SQLExecute(tagStmt), "SQLExecute", tagStmt, SQL_HANDLE_STMT);
  
  while (true) {
    retCode = SQLFetch(tagStmt);
    if (retCode == SQL_ERROR || retCode == SQL_SUCCESS_WITH_INFO) {
      //show_error();
    }
    if (retCode == SQL_SUCCESS || retCode == SQL_SUCCESS_WITH_INFO){
      SQLGetData(tagStmt, 1, SQL_C_CHAR, tagName, 512, &tagLengthInd);
      std::string newTag((char*)tagName);
      ids.push_back(newTag);
    } else {
      break;
    }
  }
  
  return ids;
}



#pragma mark -


// fetch means cache the results
void ScadaPointRecord::fetchRange(const std::string& id, time_t startTime, time_t endTime) {
  // just call super
  DbPointRecord::fetchRange(id, startTime, endTime);
}

void ScadaPointRecord::fetchNext(const std::string& id, time_t time) {
  time_t margin = 60*60*12;
  fetchRange(id, time-1, time+margin);
}


void ScadaPointRecord::fetchPrevious(const std::string& id, time_t time) {
  time_t margin = 60*60*12;
  fetchRange(id, time-margin, time+1);
}



// select just returns the results (no caching)
std::vector<Point> ScadaPointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  return pointsWithStatement(id, _rangeStatement, startTime, endTime);
}


Point ScadaPointRecord::selectNext(const std::string& id, time_t time) {
  Point p;
  time_t margin = 60*60*12;
  vector<Point> points = pointsWithStatement(id, _rangeStatement, time, time + margin);
  
  time_t max_margin = 60*60*24*2; // 2-day lookahead max
  time_t lookahead = time;
  while (points.size() == 0 && lookahead < time + max_margin) {
    cout << "scada lookahead" << endl;
    lookahead += margin;
    points = pointsWithStatement(id, _rangeStatement, lookahead, lookahead+margin);
  }
  
  if (points.size() > 0) {
    p = points.front();
  }
  return p;
}


Point ScadaPointRecord::selectPrevious(const std::string& id, time_t time) {
  Point p;
  time_t margin = 60*60*12;
  vector<Point> points = pointsWithStatement(id, _rangeStatement, time - margin, time);
  
  time_t max_margin = 60*60*24*2; // 2-day lookahead max
  time_t lookbehind = time;
  while (points.size() == 0 && lookbehind < time + max_margin) {
    cout << "scada lookbehind" << endl;
    lookbehind -= margin;
    points = pointsWithStatement(id, _rangeStatement, lookbehind-margin, lookbehind);
  }
  
  if (points.size() > 0) {
    p = points.back();
  }
  return p;
}



// insertions or alterations may choose to ignore / deny
void ScadaPointRecord::insertSingle(const std::string& id, Point point) {
  
}


void ScadaPointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  
}


void ScadaPointRecord::removeRecord(const std::string& id) {
  
}


void ScadaPointRecord::truncate() {
  
}





#pragma mark - Protected

std::ostream& ScadaPointRecord::toStream(std::ostream &stream) {
  stream << "ODBC Scada Point Record" << endl;
  // todo - stream extra info
  return stream;
}



#pragma mark - Internal (private) methods


std::vector<Point> ScadaPointRecord::pointsWithStatement(const string& id, SQLHSTMT statement, time_t startTime, time_t endTime) {
  std::vector< Point > points;
  points.clear();
  
  cout << "hit SCADA" << endl;
  
  // set up query-bound variables
  _query.start = sqlTime(startTime);
  _query.end = sqlTime(endTime);
  strcpy(_query.tagName, id.c_str());
  
  try {
    SQL_CHECK(SQLExecute(statement), "SQLExecute", statement, SQL_HANDLE_STMT);
    while (SQL_SUCCEEDED(SQLFetch(statement))) {
      // add the point to the return vector
      // todo
      // check data quality value
      Point aPoint( unixTime(_tempRecord.time), _tempRecord.value, Point::good);
      if(aPoint.isValid()) {
        points.push_back(aPoint);
      }
    }
    SQL_CHECK(SQLFreeStmt(statement, SQL_CLOSE), "SQLCancel", statement, SQL_HANDLE_STMT);
  }
  catch(std::string errorMessage) {
    std::cerr << errorMessage;
    std::cerr << "Could not get data from db connection\n";
  }
  
  if (points.size() == 0) {
    cerr << "no points found" << endl;
  }
  
  
  cout << "done SCADA" << endl;
  
  return points;
}



void ScadaPointRecord::bindOutputColumns(SQLHSTMT statement, ScadaRecord* record) {
  SQL_CHECK(SQLBindCol(statement, 1, SQL_C_TYPE_TIMESTAMP, &(record->time), NULL, &(record->timeInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 2, SQL_C_CHAR, record->tagName, MAX_SCADA_TAG, &(record->tagNameInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 3, SQL_C_DOUBLE, &(record->value), 0, &(record->valueInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 4, SQL_C_ULONG, &(record->quality), 0, &(record->qualityInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
}




SQL_TIMESTAMP_STRUCT ScadaPointRecord::sqlTime(time_t unixTime) {
  SQL_TIMESTAMP_STRUCT sqlTimestamp;
  struct tm myTMstruct;
  struct tm* pTMstruct = &myTMstruct;
  
  // time format (local/utc)
  if (timeFormat() == UTC) {
    pTMstruct = gmtime(&unixTime);
  }
  else if (timeFormat() == LOCAL) {
    pTMstruct = localtime(&unixTime);
  }
  
	if (pTMstruct->tm_isdst == 1) {
    pTMstruct->tm_hour -= 1;
  }
  
  // fix any negative hour field
  mktime(pTMstruct);
	
	sqlTimestamp.year = pTMstruct->tm_year + 1900;
	sqlTimestamp.month = pTMstruct->tm_mon + 1;
	sqlTimestamp.day = pTMstruct->tm_mday;
	sqlTimestamp.hour = pTMstruct->tm_hour;
  sqlTimestamp.minute = pTMstruct->tm_min;
	sqlTimestamp.second = pTMstruct->tm_sec;
	sqlTimestamp.fraction = (SQLUINTEGER)0;
  
  
  
  return sqlTimestamp;
}

time_t ScadaPointRecord::unixTime(SQL_TIMESTAMP_STRUCT sqlTime) {
  time_t myUnixTime;
  struct tm tmTimestamp;
  tmTimestamp.tm_isdst = 0;
  struct tm* pTimestamp = &tmTimestamp;
  const time_t timestamp = time(NULL);
  pTimestamp = localtime(&timestamp);
  
  tmTimestamp.tm_year = sqlTime.year - 1900;
  tmTimestamp.tm_mon = sqlTime.month -1;
  tmTimestamp.tm_mday = sqlTime.day;
  tmTimestamp.tm_hour = sqlTime.hour;
  tmTimestamp.tm_min = sqlTime.minute;
  tmTimestamp.tm_sec = sqlTime.second;
  
  
  //myUnixTime = mktime(&tmTimestamp);

  
  
  
  // todo -- we can speed up mktime, but this private method is borked so fix it.
  myUnixTime = time_to_epoch(&tmTimestamp, 0);
  
  
  if (timeFormat() == UTC) {
    myUnixTime += pTimestamp->tm_gmtoff;
  }
  
  return myUnixTime;
}

time_t ScadaPointRecord::time_to_epoch ( const struct tm *ltm, int utcdiff ) {
  const int mon_days [] =
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  long tyears, tdays, leaps, utc_hrs;
  int i;
  
  tyears = ltm->tm_year - 70; // tm->tm_year is from 1900.
  leaps = (tyears + 2) / 4; // no of next two lines until year 2100.
                            //i = (ltm->tm_year – 100) / 100;
                            //leaps -= ( (i/4)*3 + i%4 );
  tdays = 0;
  for (i=0; i < ltm->tm_mon; i++) tdays += mon_days[i];
  
  tdays += ltm->tm_mday-1; // days of month passed.
  tdays = tdays + (tyears * 365) + leaps;
  
  utc_hrs = ltm->tm_hour + utcdiff; // for your time zone.
  return (tdays * 86400) + (utc_hrs * 3600) + (ltm->tm_min * 60) + ltm->tm_sec;
}


SQLRETURN ScadaPointRecord::SQL_CHECK(SQLRETURN retVal, std::string function, SQLHANDLE handle, SQLSMALLINT type) throw(std::string)
{
	if(!SQL_SUCCEEDED(retVal)) {
    std::string errorMessage;
		errorMessage = extract_error(function, handle, type);
    throw errorMessage;
	}
	return retVal;
}


std::string ScadaPointRecord::extract_error(std::string function, SQLHANDLE handle, SQLSMALLINT type)
{
  SQLINTEGER	 i = 0;
  SQLINTEGER	 native;
  SQLCHAR	 state[ 7 ];
  SQLCHAR	 text[256];
  SQLSMALLINT	 len;
  SQLRETURN	 ret;
  std::string msg("");
  
  do
  {
    ret = SQLGetDiagRec(type, handle, ++i, state, &native, text, sizeof(text), &len );
    if (SQL_SUCCEEDED(ret)) {
      msg += (char*)state;
      msg += "::";
      msg += (long int)i;
      msg += "::";
      msg += (long int)native;
      msg += "::";
      msg += (char*)text;
    }
    
    // check if it's a connection issue
    if (strncmp((char*)state, "SCADA_CONNECTION_ISSSUE", 2) == 0) {
      msg += "::Connection Issue::";
      return msg;
    }
  }
  while( ret == SQL_SUCCESS );
	return msg;
}


