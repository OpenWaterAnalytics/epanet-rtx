//
//  OdbcPointRecord.cpp
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  


#include "OdbcPointRecord.h"
#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>

using namespace RTX;
using namespace std;

OdbcPointRecord::OdbcPointRecord() : _timeFormat(UTC){
  _connectionOk = false;
  
  _tableName = "#TABLENAME#";
  _dateCol = "#DATECOL";
  _tagCol = "#TAGCOL";
  _valueCol = "#VALUECOL";
  _qualityCol = "#QUALITYCOL#";
}


OdbcPointRecord::~OdbcPointRecord() {
  
}


///! templates for selection queries
map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::odbc_query_t> OdbcPointRecord::queryTypes() {
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::odbc_query_t> list;
  
  odbc_query_t wwQueries;
  wwQueries.connectorName = "wonderware_mssql";
  wwQueries.singleSelect = "SELECT #DATECOL#, #TAGCOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE (#DATECOL# = ?) AND #TAGCOL# = ? AND wwTimeZone = 'UTC'";
  //wwQueries.rangeSelect =  "SELECT #DATECOL#, #TAGCOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE (#DATECOL# >= ?) AND (#DATECOL# <= ?) AND #TAGCOL# = ? AND wwTimeZone = 'UTC' ORDER BY #DATECOL# asc"; // experimentally, ORDER BY is much slower. wonderware always returns rows ordered by DateTime ascending, so this is not really necessary.
  wwQueries.rangeSelect =  "SELECT #DATECOL#, #TAGCOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE (#DATECOL# >= ?) AND (#DATECOL# <= ?) AND #TAGCOL# = ? AND wwTimeZone = 'UTC'";
  wwQueries.lowerBound = "";
  wwQueries.upperBound = "";
  wwQueries.timeQuery = "SELECT CONVERT(datetime, GETDATE()) AS DT";
  
  
  odbc_query_t oraQueries;
  oraQueries.connectorName = "oracle";
  oraQueries.singleSelect = "";
  oraQueries.rangeSelect = "";
  oraQueries.lowerBound = "";
  oraQueries.upperBound = "";
  oraQueries.timeQuery = "select sysdate from dual";
  
  
  list[wonderware_mssql] = wwQueries;
  list[oracle] = oraQueries;
  
  return list;
}


OdbcPointRecord::Sql_Connector_t OdbcPointRecord::typeForName(const string& connector) {
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::odbc_query_t> list = queryTypes();
  
  BOOST_FOREACH(Sql_Connector_t connType, list | boost::adaptors::map_keys) {
    if (RTX_STRINGS_ARE_EQUAL(connector, list[connType].connectorName)) {
      return connType;
    }
  }

  cerr << "could not resolve connector type: " << connector << endl;
  return NO_CONNECTOR;
}


#pragma mark -

void OdbcPointRecord::setTableColumnNames(const string& table, const string& dateCol, const string& tagCol, const string& valueCol, const string& qualityCol) {
  
  _tableName = table;
  _dateCol = dateCol;
  _tagCol = tagCol;
  _valueCol = valueCol;
  _qualityCol = qualityCol;
  
}

void OdbcPointRecord::setConnectorType(Sql_Connector_t connectorType) {
  
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::odbc_query_t> qTypes = queryTypes();
  if (qTypes.find(connectorType) == qTypes.end()) {
    cerr << "could not find the specified connector type" << endl;
    return;
  }
  
  OdbcPointRecord::odbc_query_t queries = qTypes[connectorType];
  
  
  {
    // do some string replacement to construct the sql queries from my type's template queries.
    vector<string*> querystrings;
    querystrings.push_back(&queries.singleSelect);
    querystrings.push_back(&queries.rangeSelect);
    querystrings.push_back(&queries.upperBound);
    querystrings.push_back(&queries.lowerBound);
    
    BOOST_FOREACH(string* str, querystrings) {
      boost::replace_all(*str, "#TABLENAME#", _tableName);
      boost::replace_all(*str, "#DATECOL#", _dateCol);
      boost::replace_all(*str, "#TAGCOL#", _tagCol);
      boost::replace_all(*str, "#VALUECOL#", _valueCol);
      boost::replace_all(*str, "#QUALITYCOL#", _qualityCol);
    }
  }
  
  setSingleSelectQuery(queries.singleSelect);
  setRangeSelectQuery(queries.rangeSelect);
  setUpperBoundSelectQuery(queries.upperBound);  // todo
  setLowerBoundSelectQuery(queries.lowerBound);  // todo
  setTimeQuery(queries.timeQuery);
  
  _query.tagNameInd = SQL_NTS;

}


void OdbcPointRecord::connect() throw(RtxException) {
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
    
  } catch (string errorMessage) {
    cerr << "Initialize failed: " << errorMessage << "\n";
    _connectionOk = false;
    //throw DbPointRecord::RtxDbConnectException();
  }
  
  // todo -- check time offset (dst?)

}

bool OdbcPointRecord::isConnected() {
  return _connectionOk;
}

string OdbcPointRecord::registerAndGetIdentifier(string recordName) {
  return DB_PR_SUPER::registerAndGetIdentifier(recordName);
}

vector<string> OdbcPointRecord::identifiers() {
  vector<string> ids;
  if (!isConnected()) {
    return ids;
  }
  
  // get tag names from db.
  string tagQuery = "SELECT TagName FROM Tag ORDER BY TagName";
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
      string newTag((char*)tagName);
      ids.push_back(newTag);
    } else {
      break;
    }
  }
  
  return ids;
}



#pragma mark -


// fetch means cache the results
/*
void OdbcPointRecord::fetchRange(const string& id, time_t startTime, time_t endTime) {
  // just call super
  DbPointRecord::fetchRange(id, startTime, endTime);
}

void OdbcPointRecord::fetchNext(const string& id, time_t time) {
  time_t margin = 60*60*12;
  fetchRange(id, time-1, time+margin);
}


void OdbcPointRecord::fetchPrevious(const string& id, time_t time) {
  time_t margin = 60*60*12;
  fetchRange(id, time-margin, time+1);
}
*/


// select just returns the results (no caching)
vector<Point> OdbcPointRecord::selectRange(const string& id, time_t startTime, time_t endTime) {
  return pointsWithStatement(id, _rangeStatement, startTime, endTime);
}


Point OdbcPointRecord::selectNext(const string& id, time_t time) {
  Point p;
  time_t margin = 60*60*12;
  vector<Point> points = pointsWithStatement(id, _rangeStatement, time-1, time + margin);
  
  time_t max_margin = this->searchDistance();
  time_t lookahead = time;
  while (points.size() == 0 && lookahead < time + max_margin) {
    //cout << "scada lookahead" << endl;
    lookahead += margin;
    points = pointsWithStatement(id, _rangeStatement, time-1, lookahead+margin);
  }
  
  if (points.size() > 0) {
    p = points.front();
    int i = 0;
    while (p.time <= time && i < points.size()) {
      p = points.at(i);
      ++i;
    }
  }
  if (points.empty()) {
    cerr << "no points found for " << id << " :: range " << time - 1 << " - " << lookahead + margin << endl;
  }
  return p;
}


Point OdbcPointRecord::selectPrevious(const string& id, time_t time) {
  Point p;
  time_t margin = 60*60*12;
  vector<Point> points = pointsWithStatement(id, _rangeStatement, time - margin, time+1);
  
  time_t max_margin = this->searchDistance();
  time_t lookbehind = time;
  while (points.size() == 0 && lookbehind > time - max_margin) {
    //cout << "scada lookbehind" << endl;
    lookbehind -= margin;
    points = pointsWithStatement(id, _rangeStatement, lookbehind-margin, time+1);
  }
  // sanity
  while (!points.empty() && points.back().time > time) {
    points.pop_back();
  }
  
  if (points.size() > 0) {
    p = points.back();
  }
  if (points.empty()) {
    cerr << "no points found for " << id << " :: range " << lookbehind-margin << " - " << time+1 << endl;
  }
  return p;
}



// insertions or alterations may choose to ignore / deny
void OdbcPointRecord::insertSingle(const string& id, Point point) {
  
}


void OdbcPointRecord::insertRange(const string& id, vector<Point> points) {
  
}


void OdbcPointRecord::removeRecord(const string& id) {
  
}


void OdbcPointRecord::truncate() {
  
}





#pragma mark - Protected

ostream& OdbcPointRecord::toStream(ostream &stream) {
  stream << "ODBC Scada Point Record" << endl;
  // todo - stream extra info
  return stream;
}



#pragma mark - Internal (private) methods


vector<Point> OdbcPointRecord::pointsWithStatement(const string& id, SQLHSTMT statement, time_t startTime, time_t endTime) {
  vector< Point > points;
  points.clear();
  
  
  if (startTime == 0 || endTime == 0) {
    // something smells
    return points;
  }
  
  // make sure the request paramter cache is current. (super uses this)
  request = request_t(id,startTime, endTime);
  
  // set up query-bound variables
  _query.start = sqlTime(startTime-1);
  _query.end = sqlTime(endTime+1); // add one second to get fractional times included
  strcpy(_query.tagName, id.c_str());
  
  try {
    //cout << "scada: " << id << " : " << startTime << " - " << endTime << endl;
    SQL_CHECK(SQLExecute(statement), "SQLExecute", statement, SQL_HANDLE_STMT);
    while (SQL_SUCCEEDED(SQLFetch(statement))) {
      Point p;
      //time_t t = unixTime(_tempRecord.time);
      time_t t = sql_to_tm(_tempRecord.time);
      double v = _tempRecord.value;
      int qu = _tempRecord.quality;
      Point::Qual_t q = Point::Qual_t::good; // todo -- map to rtx quality types
      
      if (_tempRecord.valueInd > 0 && qu == 0) {
        // ok
        p = Point(t, v, q);
        points.push_back(p);
      }
      else {
        // nothing
        //cout << "skipped invalid point. quality = " << _tempRecord.quality << endl;
      }
      
    }
    SQL_CHECK(SQLFreeStmt(statement, SQL_CLOSE), "SQLCancel", statement, SQL_HANDLE_STMT);
  }
  catch(string errorMessage) {
    cerr << errorMessage << endl;
    cerr << "Could not get data from db connection\n";
    cerr << "Attempting to reconnect..." << endl;
    this->connect();
    cerr << "Connection returned " << this->isConnected() << endl;
  }
  
  if (points.size() == 0) {
    //cerr << "no points found" << endl;
  }
  
  return points;
}



void OdbcPointRecord::bindOutputColumns(SQLHSTMT statement, ScadaRecord* record) {
  SQL_CHECK(SQLBindCol(statement, 1, SQL_C_TYPE_TIMESTAMP, &(record->time), NULL, &(record->timeInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 2, SQL_C_CHAR, record->tagName, MAX_SCADA_TAG, &(record->tagNameInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 3, SQL_C_DOUBLE, &(record->value), 0, &(record->valueInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 4, SQL_C_ULONG, &(record->quality), 0, &(record->qualityInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
}




SQL_TIMESTAMP_STRUCT OdbcPointRecord::sqlTime(time_t unixTime) {
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
  /*
	if (pTMstruct->tm_isdst == 1) {
    pTMstruct->tm_hour -= 1;
  }
  */
  // fix any negative hour field
  // not needed.
  //mktime(pTMstruct);
	
	sqlTimestamp.year = pTMstruct->tm_year + 1900;
	sqlTimestamp.month = pTMstruct->tm_mon + 1;
	sqlTimestamp.day = pTMstruct->tm_mday;
	sqlTimestamp.hour = pTMstruct->tm_hour;
  sqlTimestamp.minute = pTMstruct->tm_min;
	sqlTimestamp.second = pTMstruct->tm_sec;
	sqlTimestamp.fraction = (SQLUINTEGER)0;
  
  
  
  return sqlTimestamp;
}

time_t OdbcPointRecord::unixTime(SQL_TIMESTAMP_STRUCT sqlTime) {
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
  // mktime sets to local time zone, but the passed-in structure should be UTC
  if (timeFormat() == UTC) {
    myUnixTime += pTimestamp->tm_gmtoff;
  }
  if (tmTimestamp.tm_isdst == 1) {
    myUnixTime -= 3600;
  }
  
  // todo -- we can speed up mktime, but this private method is borked so fix it.
  //myUnixTime = time_to_epoch(&tmTimestamp, 0);
  
  
  
  
  return myUnixTime;
}

time_t OdbcPointRecord::sql_to_tm( const SQL_TIMESTAMP_STRUCT& sqlTime ) {
  
  /*
  const int mon_days [] =
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  long tyears, tdays, leaps, utc_hrs, utc_mins, utc_secs;
  int i;
  
  tyears = sqlTime.year - 1970; // = ltm->tm_year - 70; // tm->tm_year is from 1900.
  leaps = (tyears + 2) / 4; // no of next two lines until year 2100.
  //i = (ltm->tm_year – 100) / 100;
  //leaps -= ( (i/4)*3 + i%4 );
  tdays = 0;
  for (i=0; i < sqlTime.month-1; i++) {
    tdays += mon_days[i];
  }
  
  tdays += sqlTime.day - 1; // days of month passed.
  tdays = tdays + (tyears * 365) + leaps;
  
  utc_hrs = sqlTime.hour;
  utc_mins = sqlTime.minute;
  utc_secs = sqlTime.second;
  time_t uTime = (tdays * 86400) + (utc_hrs * 3600) + (utc_mins * 60) + utc_secs;
  */
  
  time_t uTime;
  struct tm tmTime;
  
  tmTime.tm_year = sqlTime.year - 1900;
  tmTime.tm_mon = sqlTime.month -1;
  tmTime.tm_mday = sqlTime.day;
  tmTime.tm_hour = sqlTime.hour;
  tmTime.tm_min = sqlTime.minute;
  tmTime.tm_sec = sqlTime.second;
  
  // Portability note: mktime is essentially universally available. timegm is rather rare. For the most portable conversion from a UTC broken-down time to a simple time, set the TZ environment variable to UTC, call mktime, then set TZ back.
  
  uTime = timegm(&tmTime);
  /*
  // back convert for a check
  SQL_TIMESTAMP_STRUCT sqlTimeCheck = this->sqlTime(uTime);
  if ( sqlTimeCheck.year == sqlTime.year &&
       sqlTimeCheck.month == sqlTime.month &&
       sqlTimeCheck.day == sqlTime.day &&
       sqlTimeCheck.hour == sqlTime.hour &&
       sqlTimeCheck.minute == sqlTime.minute &&
       sqlTimeCheck.second == sqlTime.second) {
    
  }
  else {
    cerr << "time not formed correctly" << endl;
  }
   */
  
  return uTime;
}


time_t OdbcPointRecord::time_to_epoch ( const struct tm *ltm, int utcdiff ) {
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


SQLRETURN OdbcPointRecord::SQL_CHECK(SQLRETURN retVal, string function, SQLHANDLE handle, SQLSMALLINT type) throw(string)
{
	if(!SQL_SUCCEEDED(retVal)) {
    string errorMessage;
		errorMessage = extract_error(function, handle, type);
    throw errorMessage;
	}
	return retVal;
}


string OdbcPointRecord::extract_error(string function, SQLHANDLE handle, SQLSMALLINT type)
{
  SQLINTEGER	 i = 0;
  SQLINTEGER	 native;
  SQLCHAR	 state[ 7 ];
  SQLCHAR	 text[256];
  SQLSMALLINT	 len;
  SQLRETURN	 ret;
  string msg("");
  
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


