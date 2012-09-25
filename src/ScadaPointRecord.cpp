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
using std::endl;

ScadaPointRecord::ScadaPointRecord() {
  _hint.clear();
  _connectionOk = false;
}


ScadaPointRecord::~ScadaPointRecord() {
  
}

#pragma mark -
#pragma mark Initialization

void ScadaPointRecord::setSyntax(const string& table, const string& dateCol, const string& tagCol, const string& valueCol, const string& qualityCol) {
  _syntax.table =   table;
  _syntax.date =    dateCol;
  _syntax.tag =     tagCol;
  _syntax.value =   valueCol;
  _syntax.quality = qualityCol;
}


void ScadaPointRecord::connect() throw(RtxException) {
  _connectionOk = false;
  if (RTX_STRINGS_ARE_EQUAL(this->connectionString(), "")) {
    return;
  }
  _dataPointQuery = "SELECT " + _syntax.date + ", " + _syntax.tag + ", " + _syntax.value + ", " + _syntax.quality +
                    " FROM " + _syntax.table +
                    " WHERE (" + _syntax.date + " = ?) AND " + _syntax.tag + " = ?";
  
  _dataRangeQuery = "SELECT " + _syntax.date + ", " + _syntax.tag + ", " + _syntax.value + ", " + _syntax.quality +
                    " FROM " + _syntax.table +
                    " WHERE (" + _syntax.date + " >= ?) AND (" + _syntax.date + " <= ?) AND " + _syntax.tag + " = ?";
  
  _dataLowerBoundQuery =  "SELECT TOP 2 " + _syntax.date + ", " + _syntax.tag + ", " + _syntax.value + ", " + _syntax.quality +
                          " FROM " + _syntax.table +
                          " WHERE (" + _syntax.date + " > ?) AND (" + _syntax.date + " <= ?) AND " + _syntax.tag + " = ?" +
                          " ORDER BY " + _syntax.date + " DESC";
  
  _dataUpperBoundQuery =  "SELECT TOP 2 " + _syntax.date + ", " + _syntax.tag + ", " + _syntax.value + ", " + _syntax.quality +
                          " FROM " + _syntax.table +
                          " WHERE (" + _syntax.date + " > ?) AND (" + _syntax.date + " <= ?) AND " + _syntax.tag + " = ?" +
                          " ORDER BY " + _syntax.date + " ASC";
  
  _timeQuery = "SELECT CONVERT(datetime, GETDATE()) AS DT";
  _query.tagNameInd = SQL_NTS;
  
  SQLRETURN ret;
  
  try {
    /* Allocate an environment handle */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_SCADAenv), "SQLAllocHandle", _SCADAenv, SQL_HANDLE_ENV);
    /* We want ODBC 3 support */
    SQL_CHECK(SQLSetEnvAttr(_SCADAenv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0), "SQLSetEnvAttr", _SCADAenv, SQL_HANDLE_ENV);
    /* Allocate a connection handle */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_DBC, _SCADAenv, &_SCADAdbc), "SQLAllocHandle", _SCADAenv, SQL_HANDLE_ENV);
    /* Connect to the DSN, checking for connectivity */
    //"Attempting to Connect to SCADA..."
    
    SQL_CHECK(SQLDriverConnect(_SCADAdbc, NULL, (SQLCHAR*)(this->connectionString()).c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT), "SQLDriverConnect", _SCADAdbc, SQL_HANDLE_DBC);
    
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
    ret = SQLBindParameter(_SCADAstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd);
    ret = SQLBindParameter(_SCADAstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd);
    
    // bindings for the range statement
    bindOutputColumns(_rangeStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_rangeStatement, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_rangeStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_rangeStatement, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _rangeStatement, SQL_HANDLE_STMT);
    
    // bindings for lower bound statement
    bindOutputColumns(_lowerBoundStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_lowerBoundStatement, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_lowerBoundStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_lowerBoundStatement, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _lowerBoundStatement, SQL_HANDLE_STMT);
    
    // bindings for upper bound statement
    bindOutputColumns(_upperBoundStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_upperBoundStatement, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_upperBoundStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_upperBoundStatement, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _upperBoundStatement, SQL_HANDLE_STMT);
    
    
    // prepare the statements
    SQL_CHECK(SQLPrepare(_SCADAstmt, (SQLCHAR*)_dataPointQuery.c_str(), SQL_NTS), "SQLPrepare", _SCADAstmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_rangeStatement, (SQLCHAR*)_dataRangeQuery.c_str(), SQL_NTS), "SQLPrepare", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_SCADAtimestmt, (SQLCHAR*)_timeQuery.c_str(), SQL_NTS), "SQLPrepare", _SCADAtimestmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_lowerBoundStatement, (SQLCHAR*)_dataLowerBoundQuery.c_str(), SQL_NTS), "SQLPrepare", _lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_upperBoundStatement, (SQLCHAR*)_dataUpperBoundQuery.c_str(), SQL_NTS), "SQLPrepare", _upperBoundStatement, SQL_HANDLE_STMT);
    
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



#pragma mark - Retrieval

bool ScadaPointRecord::isPointAvailable(const string& identifier, time_t time)  {
  Point::sharedPointer aPoint = pointBefore(identifier, time);
  if (!aPoint) {
    return false;
  }
  if (aPoint->time() == time) {
    return true;
  }
  else if (pointAfter(identifier, time) && pointAfter(identifier, time)->time() == time) {
    return true;
  }
  else {
    return false;
  }
}

// get a single point, rely on scada system's interpolation...
Point::sharedPointer ScadaPointRecord::point(const string& identifier, time_t time) {
  
  Point::sharedPointer thePoint;
  
  // see if the requested point is within my cache
  if ( (time >= _hint.range.first) && (time <= _hint.range.second) && (RTX_STRINGS_ARE_EQUAL(identifier, _hint.identifier)) ) {
    BOOST_FOREACH(Point::sharedPointer point, _hint.cache) {
      if (point->time() == time) {
        thePoint = point;
      }
    }
  }
  
  else {
    thePoint = (pointsWithStatement(identifier, _SCADAstmt, time)).front();
  }
  
  return thePoint;
}

void ScadaPointRecord::hintAtRange(const string& identifier, time_t start, time_t end) {
  // simple optimization for read-only pointrecord.
  if (start < _hint.range.first || end > _hint.range.second || !RTX_STRINGS_ARE_EQUAL(identifier, _hint.identifier)) {
    //std::cout << "hinting at range (" << identifier << "): " << start << " - " << end << std::endl;
    _hint.cache = pointsWithStatement(identifier, _rangeStatement, start, end);
    _hint.identifier = identifier;
    _hint.range.first = start;
    _hint.range.second = end;
  }
  else {
    //std::cout << "range has already been hinted" << std::endl;
  }
  
}

// get a range of points, native resolution
std::vector<Point::sharedPointer> ScadaPointRecord::pointsInRange(const string& identifier, time_t startTime, time_t endTime) {
  std::vector<Point::sharedPointer> pointVector;
  
  // see if it's already cached.
  if ( !(_hint.range.first <= startTime && _hint.range.second >= endTime && RTX_STRINGS_ARE_EQUAL(identifier, _hint.identifier)) ) {
    hintAtRange(identifier, startTime, endTime);
  }
  
  for (unsigned int i = 0; i < _hint.cache.size(); i++) {
    pointVector.push_back(_hint.cache.at(i));
  }
  
  return pointVector;
}

Point::sharedPointer ScadaPointRecord::pointBefore(const string& identifier, time_t time) {
  
  Point::sharedPointer thePoint;
  
  // see if the requested point is within my cache
  if ( (time >= _hint.range.first) && (time <= _hint.range.second) && (RTX_STRINGS_ARE_EQUAL(identifier, _hint.identifier)) ) {
    // assuming strictly ascending order...
    // todo - clean this up? cache class?
    BOOST_FOREACH(Point::sharedPointer point, _hint.cache) {
      if (point->time() <= time) {
        thePoint = point;
      }
    }
  }
  
  else {
    // reset the cache, it's obviously no good.
    _hint.clear();
    
    // add one second to the upper range so that we catch fractional times.
    std::deque<Point::sharedPointer> points = pointsWithStatement(identifier, _lowerBoundStatement, time - (3600 * 4), (time + 1) );
    if (points.front()->time() > time) {
      points.pop_front();
    }
    
    thePoint = (points.front());
  }
  
  return thePoint;
}

Point::sharedPointer ScadaPointRecord::pointAfter(const string& identifier, time_t time) {
  
  Point::sharedPointer thePoint;

  if ( (time >= _hint.range.first) && (time <= _hint.range.second) && (RTX_STRINGS_ARE_EQUAL(identifier, _hint.identifier)) ) {
    // assuming strictly ascending order...
    // todo - clean this up? cache class?
    BOOST_FOREACH(Point::sharedPointer point, _hint.cache) {
      if (point->time() > time) {
        thePoint = point;
        break;
      }
    }
  }
  
  else {
    // reset the cache, it's obviously no good.
    _hint.clear();
    
    std::deque<Point::sharedPointer> points = pointsWithStatement(identifier, _upperBoundStatement, time, time + (3600 * 4));
    if (points.size() > 0 && points.front()) {
      if (points.front()->time() <= time) {
        points.pop_front();
      }
      thePoint = (points.front());
    }
    
  }
    
  return thePoint;
}


#pragma mark - Protected

std::ostream& ScadaPointRecord::toStream(std::ostream &stream) {
  stream << "ODBC Scada Point Record" << endl;
  // todo - stream extra info
  return stream;
}



#pragma mark - Internal (private) methods


std::deque<Point::sharedPointer> ScadaPointRecord::pointsWithStatement(const string& identifier, SQLHSTMT statement, time_t startTime, time_t endTime) {
  std::deque< Point::sharedPointer > points;
  points.clear();
  
  //std::cout << "querying scada: " << startTime << " - " << endTime << std::endl;
  
  // set up query-bound variables
  _query.start = sqlTime(startTime);
  _query.end = sqlTime(endTime);
  
  strcpy(_query.tagName, identifier.c_str());
  try {
    SQL_CHECK(SQLExecute(statement), "SQLExecute", statement, SQL_HANDLE_STMT);
    while (SQL_SUCCEEDED(SQLFetch(statement))) {
      // add the point to the return vector
      // todo
      // check data quality value
      Point::sharedPointer aPoint( new Point(unixTime(_tempRecord.time), _tempRecord.value, Point::good));
      if(aPoint) {
        points.push_back(aPoint);
      }
    }
    SQL_CHECK(SQLFreeStmt(statement, SQL_CLOSE), "SQLCancel", statement, SQL_HANDLE_STMT);
  }
  catch(std::string errorMessage) {
    std::cerr << errorMessage;
    std::cerr << "Could not get data from db connection\n";
  }
  
  return points;
}



void ScadaPointRecord::bindOutputColumns(SQLHSTMT statement, ScadaRecord* record) {
  SQL_CHECK(SQLBindCol(statement, 1, SQL_C_TYPE_TIMESTAMP, &(record->time), NULL, &(record->timeInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 2, SQL_C_CHAR, record->tagName, MAX_SCADA_TAG, &(record->tagNameInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 3, SQL_C_DOUBLE, &(record->value), 0, &(record->valueInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 4, SQL_C_ULONG, &(record->quality), 0, &(record->qualityInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
}




SQL_TIMESTAMP_STRUCT ScadaPointRecord::sqlTime(time_t unixTime) {
  SQL_TIMESTAMP_STRUCT mySQLtime;
  struct tm myTMstruct;
  struct tm* pTMstruct = &myTMstruct;
  pTMstruct = localtime(&unixTime);
	if (pTMstruct->tm_isdst == 1) {
    pTMstruct->tm_hour -= 1;
  }
  
  // fix any negative hour field
  mktime(pTMstruct);
	
	mySQLtime.year = pTMstruct->tm_year + 1900;
	mySQLtime.month = pTMstruct->tm_mon + 1;
	mySQLtime.day = pTMstruct->tm_mday;
	mySQLtime.hour = pTMstruct->tm_hour;
  mySQLtime.minute = pTMstruct->tm_min;
	mySQLtime.second = pTMstruct->tm_sec;
	mySQLtime.fraction = (SQLUINTEGER)0;
  
  
  
  return mySQLtime;
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
  
  myUnixTime = mktime(&tmTimestamp);
  return myUnixTime;
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




ScadaPointRecord::Hint_t::Hint_t() {
  clear();
}

void ScadaPointRecord::Hint_t::clear() { 
  //std::cout << "clearing scada point cache" << std::endl;
  identifier = ""; 
  range.first = 0; 
  range.second = 0; 
  cache.clear(); 
}
