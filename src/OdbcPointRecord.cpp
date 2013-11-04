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
#include <boost/date_time/posix_time/posix_time.hpp>


using namespace RTX;
using namespace std;

boost::posix_time::ptime _pTimeEpoch;

OdbcPointRecord::OdbcPointRecord() : _timeFormat(UTC){
  _connectionOk = false;
  _connectorType = NO_CONNECTOR;
  _tableName = "#TABLENAME#";
  _dateCol = "#DATECOL";
  _tagCol = "#TAGCOL";
  _valueCol = "#VALUECOL";
  _qualityCol = "#QUALITYCOL#";
  _SCADAdbc = NULL;
  _SCADAenv = NULL;
  _rangeStatement = NULL;
  _pTimeEpoch = boost::posix_time::ptime(boost::gregorian::date(1970,1,1));
}


OdbcPointRecord::~OdbcPointRecord() {
  // make sure handles are free
  if (_SCADAdbc != NULL) {
    SQL_CHECK(SQLDisconnect(_SCADAdbc), "SQLDisconnect", _SCADAdbc, SQL_HANDLE_DBC);
  }
  SQLFreeStmt(_rangeStatement, SQL_CLOSE);
  SQLFreeHandle(SQL_HANDLE_DBC, _SCADAdbc);
  SQLFreeHandle(SQL_HANDLE_ENV, _SCADAenv);
}


///! templates for selection queries
map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::odbc_query_t> OdbcPointRecord::queryTypes() {
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::odbc_query_t> list;
  
  odbc_query_t wwQueries;
  wwQueries.connectorName = "wonderware_mssql";
  wwQueries.singleSelect = "SELECT #DATECOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE (#DATECOL# = ?) AND #TAGCOL# = ? AND wwTimeZone = 'UTC'";
  //wwQueries.rangeSelect =  "SELECT #DATECOL#, #TAGCOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE (#DATECOL# >= ?) AND (#DATECOL# <= ?) AND #TAGCOL# = ? AND wwTimeZone = 'UTC' ORDER BY #DATECOL# asc"; // experimentally, ORDER BY is much slower. wonderware always returns rows ordered by DateTime ascending, so this is not really necessary.
  wwQueries.rangeSelect =  "SELECT #DATECOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE (#DATECOL# > ?) AND (#DATECOL# < ?) AND #TAGCOL# = ? AND wwTimeZone = 'UTC'";
  wwQueries.lowerBound = "";
  wwQueries.upperBound = "";
  wwQueries.timeQuery = "SELECT CONVERT(datetime, GETDATE()) AS DT";
  map<int, Point::Qual_t> wwQualMap;
  wwQualMap[192] = Point::good;
  wwQualMap[0] = Point::missing;
  wwQueries.qualityMap = wwQualMap;
  
  odbc_query_t oraQueries;
  oraQueries.connectorName = "oracle";
  oraQueries.singleSelect = "";
  oraQueries.rangeSelect = "SELECT #DATECOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE (#DATECOL# > ?) AND (#DATECOL# < ?) AND #TAGCOL# = ? ORDER BY #DATECOL# asc";
  oraQueries.lowerBound = "";
  oraQueries.upperBound = "";
  oraQueries.timeQuery = "select sysdate from dual";
  map<int, Point::Qual_t> oraQualMap;
  oraQualMap[0] = Point::good;            // 00000000000
  oraQualMap[128] = Point::questionable;  // 00010000000
  oraQualMap[192] = Point::questionable;  // 00011000000
  oraQualMap[256] = Point::questionable;  // 00100000000
  oraQualMap[768] = Point::questionable;  // 01100000000
  oraQualMap[32]   = Point::missing;      // 00000100000
  oraQualMap[1024] = Point::missing;      // 10000000000
  
  oraQueries.qualityMap = oraQualMap;
  
  /*
  oraQueries.rangeSelect = "\
  select time, name, value, quality from \
  (select SCADA.CTPLUS_TB_POINTDATA.EVENTTIMEUTC time, SCADA.CTPLUS_TB_POINTDATA.POINTID id, SCADA.CTPLUS_TB_POINTDATA.VALUE value, SCADA.CTPLUS_TB_POINTDATA.QUALITY quality \
   from SCADA.CTPLUS_TB_POINTDATA \
   where (SCADA.CTPLUS_TB_POINTDATA.EVENTTIMEUTC > ?) \
   AND (SCADA.CTPLUS_TB_POINTDATA.EVENTTIMEUTC < ?)) \
   right inner join (select SCADA.CTPLUS_TB_POINTID.ID pointID, SCADA.CTPLUS_TB_POINTID.NAME name from SCADA.CTPLUS_TB_POINTID where name = ?) \
   on id = pointId ORDER BY time asc";
  */
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

OdbcPointRecord::Sql_Connector_t OdbcPointRecord::connectorType() {
  return _connectorType;
}

void OdbcPointRecord::setConnectorType(Sql_Connector_t connectorType) {
  
  _connectorType = connectorType;
  
  if (connectorType == NO_CONNECTOR) {
    return;
  }
  
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::odbc_query_t> qTypes = queryTypes();
  if (qTypes.find(connectorType) == qTypes.end()) {
    cerr << "OdbcPointRecord could not find the specified connector type" << endl;
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
  _qualityMap = queries.qualityMap;

}


void OdbcPointRecord::dbConnect() throw(RtxException) {
  _connectionOk = false;
  if (RTX_STRINGS_ARE_EQUAL(this->connectionString(), "")) {
    return;
  }
  
  
  SQLRETURN sqlRet;
  
  if (_SCADAdbc != NULL) {
    SQL_CHECK(SQLDisconnect(_SCADAdbc), "SQLDisconnect", _SCADAdbc, SQL_HANDLE_DBC);
  }
  
  
  try {
    // make sure handles are free
    sqlRet = SQLFreeStmt(_rangeStatement, SQL_CLOSE);
    sqlRet = SQLFreeHandle(SQL_HANDLE_DBC, _SCADAdbc);
    sqlRet = SQLFreeHandle(SQL_HANDLE_ENV, _SCADAenv);
    
    /* Allocate an environment handle */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_SCADAenv), "SQLAllocHandle", _SCADAenv, SQL_HANDLE_ENV);
    /* We want ODBC 3 support */
    SQL_CHECK(SQLSetEnvAttr(_SCADAenv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0), "SQLSetEnvAttr", _SCADAenv, SQL_HANDLE_ENV);
    /* Allocate a connection handle */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_DBC, _SCADAenv, &_SCADAdbc), "SQLAllocHandle", _SCADAenv, SQL_HANDLE_ENV);
    /* Connect to the DSN, checking for connectivity */
    //"Attempting to Connect to SCADA..."
    
    // readonly
    SQLUINTEGER mode = SQL_MODE_READ_ONLY;
    SQL_CHECK(SQLSetConnectAttr(_SCADAdbc, SQL_ATTR_ACCESS_MODE, &mode, SQL_IS_UINTEGER), "SQLSetConnectAttr", _SCADAdbc, SQL_HANDLE_DBC);
    
    // timeouts
    SQLUINTEGER timeout = 5;
    sqlRet = SQL_CHECK(SQLSetConnectAttr(_SCADAdbc, SQL_ATTR_LOGIN_TIMEOUT, &timeout, 0/*ignored*/), "SQLSetConnectAttr", _SCADAdbc, SQL_HANDLE_DBC);
    sqlRet = SQL_CHECK(SQLSetConnectAttr(_SCADAdbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, 0/*ignored*/), "SQLSetConnectAttr", _SCADAdbc, SQL_HANDLE_DBC);
    //sqlRet = SQL_CHECK(SQLSetConnectAttr(_SCADAdbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, SQL_IS_UINTEGER), "SQLSetConnectAttr", _SCADAdbc, SQL_HANDLE_DBC);
    //SQLINTEGER confirm = 0, nBytes = 0;
    //sqlRet = SQL_CHECK(SQLGetConnectAttr(_SCADAdbc, SQL_ATTR_CONNECTION_TIMEOUT, &confirm, 0 /* ignored */, &nBytes), "SQLGetConnectAttr", _SCADAdbc, SQL_HANDLE_DBC);
    //if (confirm != timeout) {
    //  cerr << "timeout not set correctly" << endl;
    //}
    
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
    
    // query timeout
    SQL_CHECK(SQLSetStmtAttr(_rangeStatement, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0), "SQLSetStmtAttr", _rangeStatement, SQL_HANDLE_STMT);
    
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
    //SQL_CHECK(SQLPrepare(_SCADAstmt, (SQLCHAR*)singleSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _SCADAstmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_rangeStatement, (SQLCHAR*)rangeSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_SCADAtimestmt, (SQLCHAR*)timeQuery().c_str(), SQL_NTS), "SQLPrepare", _SCADAtimestmt, SQL_HANDLE_STMT);
    //SQL_CHECK(SQLPrepare(_lowerBoundStatement, (SQLCHAR*)loweBoundSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _lowerBoundStatement, SQL_HANDLE_STMT);
    //SQL_CHECK(SQLPrepare(_upperBoundStatement, (SQLCHAR*)upperBoundSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _upperBoundStatement, SQL_HANDLE_STMT);
    
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

string OdbcPointRecord::registerAndGetIdentifier(string recordName, Units dataUnits) {
  return DB_PR_SUPER::registerAndGetIdentifier(recordName, dataUnits);
}

vector<string> OdbcPointRecord::identifiers() {
  vector<string> ids;
  if (!isConnected()) {
    return ids;
  }
  return ids;
  
  
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
    //cerr << "no points found for " << id << " :: range " << time - 1 << " - " << lookahead + margin << endl;
  }
  return p;
}


Point OdbcPointRecord::selectPrevious(const string& id, time_t time) {
  Point p;
  time_t margin = 60*60*24*7; // one week?
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
    //cerr << "no points found for " << id << " :: range " << lookbehind-margin << " - " << time+1 << endl;
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
  
  // set up query-bound variables
  _query.start = sqlTime(startTime-1);
  _query.end = sqlTime(endTime+1); // add one second to get fractional times included
  strcpy(_query.tagName, id.c_str());
  vector<ScadaRecord> records;
  
  try {
    if (statement == NULL) {
      throw string("Connection not initialized.");
    }
    SQL_CHECK(SQLExecute(statement), "SQLExecute", statement, SQL_HANDLE_STMT);    
    while (SQL_SUCCEEDED(SQLFetch(statement))) {
      records.push_back(_tempRecord);
    }
    SQL_CHECK(SQLFreeStmt(statement, SQL_CLOSE), "SQLCancel", statement, SQL_HANDLE_STMT);
  }
  catch(string errorMessage) {
    cerr << errorMessage << endl;
    cerr << "Could not get data from db connection\n";
    cerr << "Attempting to reconnect..." << endl;
    this->dbConnect();
    cerr << "Connection returned " << this->isConnected() << endl;
  }
  
  
  BOOST_FOREACH(const ScadaRecord& record, records) {
    Point p;
    //time_t t = unixTime(record.time);
    time_t t = sql_to_time_t(record.time);
    double v = record.value;
    int qu = record.quality;
    Point::Qual_t q = Point::missing;
    
    // map to rtx quality types
    if (_qualityMap.count(qu) > 0) {
      q = _qualityMap[qu];
    }
    else {
      q = Point::bad;
    }
    
    if (record.valueInd > 0 && q != Point::missing) {
      // ok
      p = Point(t, v, q);
      points.push_back(p);
    }
    else {
      // nothing
      //cout << "skipped invalid point. quality = " << _tempRecord.quality << endl;
    }

  }
  
  
  if (points.size() == 0) {
    //cerr << "no points found" << endl;
  }
  else {
    // make sure the request paramter cache is current. (super uses this)
    request = request_t(id,points.front().time, points.back().time);
  }
  
  return points;
}



void OdbcPointRecord::bindOutputColumns(SQLHSTMT statement, ScadaRecord* record) {
  SQL_CHECK(SQLBindCol(statement, 1, SQL_C_TYPE_TIMESTAMP, &(record->time), NULL, &(record->timeInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  //SQL_CHECK(SQLBindCol(statement, 2, SQL_C_CHAR, record->tagName, MAX_SCADA_TAG, &(record->tagNameInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 2, SQL_C_DOUBLE, &(record->value), 0, &(record->valueInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 3, SQL_INTEGER, &(record->quality), 0, &(record->qualityInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
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
	
	sqlTimestamp.year = pTMstruct->tm_year + 1900;
	sqlTimestamp.month = pTMstruct->tm_mon + 1;
	sqlTimestamp.day = pTMstruct->tm_mday;
	sqlTimestamp.hour = pTMstruct->tm_hour;
  sqlTimestamp.minute = pTMstruct->tm_min;
	sqlTimestamp.second = pTMstruct->tm_sec;
	sqlTimestamp.fraction = (SQLUINTEGER)0;
  
  
  
  return sqlTimestamp;
}


time_t OdbcPointRecord::sql_to_time_t( const SQL_TIMESTAMP_STRUCT& sqlTime ) {
  
  time_t uTime;
  struct tm tmTime;
  
  tmTime.tm_year = sqlTime.year - 1900;
  tmTime.tm_mon = sqlTime.month -1;
  tmTime.tm_mday = sqlTime.day;
  tmTime.tm_hour = sqlTime.hour;
  tmTime.tm_min = sqlTime.minute;
  tmTime.tm_sec = sqlTime.second;
  
  // Portability note: mktime is essentially universally available. timegm is rather rare. fortunately, boost has some capabilities here.
  // uTime = timegm(&tmTime);
  
  uTime = boost_convert_tm_to_time_t(tmTime);
  
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
   
  
  return uTime;
}


time_t OdbcPointRecord::boost_convert_tm_to_time_t(const struct tm &tmStruct) {  
  boost::posix_time::ptime pt = boost::posix_time::ptime_from_tm(tmStruct);
  boost::posix_time::time_duration::sec_type x = (pt - _pTimeEpoch).total_seconds();
  return time_t(x);
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


