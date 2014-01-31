//
//  OdbcPreparedPointRecord.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/31/13.
//
//

#include "OdbcPreparedPointRecord.h"

#include <boost/foreach.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using namespace RTX;
using namespace std;

boost::posix_time::ptime _pTimeEpoch;

OdbcPreparedPointRecord::OdbcPreparedPointRecord() : _timeFormat(UTC){
  _connectionOk = false;
  _connectorType = NO_CONNECTOR;
  
  _handles.SCADAenv = NULL;
  _handles.SCADAdbc = NULL;
  
  //
  //  _tableName = "#TABLENAME#";
  //  _dateCol = "#DATECOL";
  //  _tagCol = "#TAGCOL";
  //  _valueCol = "#VALUECOL";
  //  _qualityCol = "#QUALITYCOL#";
  //  _SCADAdbc = NULL;
  //  _SCADAenv = NULL;
  //  _rangeStatement = NULL;
  _pTimeEpoch = boost::posix_time::ptime(boost::gregorian::date(1970,1,1));
  
  
  this->initDsnList();
  
}


OdbcPreparedPointRecord::~OdbcPreparedPointRecord() {
  // make sure handles are free
  if (_handles.SCADAdbc != NULL) {
    SQLDisconnect(_handles.SCADAdbc);
  }
  //  SQLFreeStmt(_handles.rangeStatement, SQL_CLOSE);
  SQLFreeHandle(SQL_HANDLE_DBC, _handles.SCADAdbc);
  SQLFreeHandle(SQL_HANDLE_ENV, _handles.SCADAenv);
}

void OdbcPreparedPointRecord::initDsnList() {
  
  SQLRETURN sqlRet;
  
  sqlRet = SQLFreeHandle(SQL_HANDLE_DBC, _handles.SCADAdbc);
  sqlRet = SQLFreeHandle(SQL_HANDLE_ENV, _handles.SCADAenv);
  
  /* Allocate an environment handle */
  SQL_CHECK(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_handles.SCADAenv), "SQLAllocHandle", _handles.SCADAenv, SQL_HANDLE_ENV);
  /* We want ODBC 3 support */
  SQL_CHECK(SQLSetEnvAttr(_handles.SCADAenv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0), "SQLSetEnvAttr", _handles.SCADAenv, SQL_HANDLE_ENV);
  /* Allocate a connection handle */
  SQL_CHECK(SQLAllocHandle(SQL_HANDLE_DBC, _handles.SCADAenv, &_handles.SCADAdbc), "SQLAllocHandle", _handles.SCADAenv, SQL_HANDLE_ENV);
  /* Connect to the DSN, checking for connectivity */
  //"Attempting to Connect to SCADA..."
  
  // readonly
  SQLUINTEGER mode = SQL_MODE_READ_ONLY;
  SQL_CHECK(SQLSetConnectAttr(_handles.SCADAdbc, SQL_ATTR_ACCESS_MODE, &mode, SQL_IS_UINTEGER), "SQLSetConnectAttr", _handles.SCADAdbc, SQL_HANDLE_DBC);
  
  // timeouts
  SQLUINTEGER timeout = 5;
  sqlRet = SQL_CHECK(SQLSetConnectAttr(_handles.SCADAdbc, SQL_ATTR_LOGIN_TIMEOUT, &timeout, 0/*ignored*/), "SQLSetConnectAttr", _handles.SCADAdbc, SQL_HANDLE_DBC);
  sqlRet = SQL_CHECK(SQLSetConnectAttr(_handles.SCADAdbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, 0/*ignored*/), "SQLSetConnectAttr", _handles.SCADAdbc, SQL_HANDLE_DBC);
  
  
  _dsnList.clear();
  
  SQLCHAR dsnChar[256];
  SQLCHAR desc[256];
  SQLSMALLINT dsn_ret;
  SQLSMALLINT desc_ret;
  SQLUSMALLINT direction = SQL_FETCH_FIRST;
  while(SQL_SUCCEEDED(sqlRet = SQLDataSources(_handles.SCADAenv, direction, dsnChar, sizeof(dsnChar), &dsn_ret, desc, sizeof(desc), &desc_ret))) {
    direction = SQL_FETCH_NEXT;
    string thisDsn = string((char*)dsnChar);
    _dsnList.push_back(thisDsn);
    if (sqlRet == SQL_SUCCESS_WITH_INFO) {
      printf("\tdata truncation\n");
    }
  }
  
  
  
}

///! templates for selection queries
map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::OdbcQuery> OdbcPreparedPointRecord::queryTypes() {
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::OdbcQuery> list;
  
  OdbcQuery wwQueries;
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
  
  /***************************************************/
  
  OdbcQuery oraQueries;
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
  
  
  // "regular" mssql db...
  OdbcQuery mssqlQueries = wwQueries;
  mssqlQueries.connectorName = "mssql";
  mssqlQueries.rangeSelect =  "SELECT #DATECOL#, #VALUECOL#, #QUALITYCOL# FROM #TABLENAME# WHERE #TAGCOL# = ? AND (#DATECOL# > ?) AND (#DATECOL# < ?)";// ORDER BY #DATECOL# asc";
  mssqlQueries.qualityMap = oraQualMap;
  
  
  list[mssql] = mssqlQueries;
  list[wonderware_mssql] = wwQueries;
  list[oracle] = oraQueries;
  
  return list;
}


OdbcPointRecord::Sql_Connector_t OdbcPreparedPointRecord::typeForName(const string& connector) {
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::OdbcQuery> list = queryTypes();
  
  BOOST_FOREACH(Sql_Connector_t connType, list | boost::adaptors::map_keys) {
    if (RTX_STRINGS_ARE_EQUAL(connector, list[connType].connectorName)) {
      return connType;
    }
  }
  
  cerr << "could not resolve connector type: " << connector << endl;
  return NO_CONNECTOR;
}


vector<string> OdbcPreparedPointRecord::dsnList() {
  return _dsnList;
}




void OdbcPreparedPointRecord::rebuildQueries() {
  
  map<OdbcPointRecord::Sql_Connector_t, OdbcPointRecord::OdbcQuery> qTypes = queryTypes();
  if (qTypes.find(this->connectorType()) == qTypes.end()) {
    cerr << "Could not find the specified connector type" << endl;
    return;
  }
  
  _querySyntax = qTypes[this->connectorType()];
  
  {
    // do some string replacement to construct the sql queries from my type's template queries.
    vector<string*> querystrings;
    querystrings.push_back(&_querySyntax.singleSelect);
    querystrings.push_back(&_querySyntax.rangeSelect);
    querystrings.push_back(&_querySyntax.upperBound);
    querystrings.push_back(&_querySyntax.lowerBound);
    
    BOOST_FOREACH(string* str, querystrings) {
      boost::replace_all(*str, "#TABLENAME#",  _tableDescription.dataTable);
      boost::replace_all(*str, "#DATECOL#",    _tableDescription.dataDateCol);
      boost::replace_all(*str, "#TAGCOL#",     _tableDescription.dataNameCol);
      boost::replace_all(*str, "#VALUECOL#",   _tableDescription.dataValueCol);
      boost::replace_all(*str, "#QUALITYCOL#", _tableDescription.dataQualityCol);
    }
  }
  
  
  _query.tagNameInd = SQL_NTS;
  
}

void OdbcPreparedPointRecord::dbConnect() throw(RtxException) {
  _connectionOk = false;
  if (RTX_STRINGS_ARE_EQUAL(this->dsn(), "") ||
      RTX_STRINGS_ARE_EQUAL(this->uid(), "") ||
      RTX_STRINGS_ARE_EQUAL(this->pwd(), "") ) {
    errorMessage = "Incomplete Login";
    return;
  }
  
  
  this->rebuildQueries();
  
  SQLRETURN sqlRet;
  
  if (_handles.SCADAdbc != NULL) {
    SQLDisconnect(_handles.SCADAdbc);
  }
  
  
  try {
    // make sure handles are free
    //sqlRet = SQLFreeStmt(_handles.rangeStatement, SQL_CLOSE);
    
    // "DRIVER=TDS;SERVER=192.168.239.131;UID=EPA;PWD=rtAgent;DATABASE=Runtime;TDS_Version=8.0;Port=1433;"
    //sqlRet = SQLDriverConnect(_SCADAdbc, NULL, (SQLCHAR*)(this->connectionString()).c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_PROMPT);
    //sqlRet = SQLConnect(_SCADAdbc, (SQLCHAR*)(this->connectionString()).c_str(), SQL_NTS, NULL, 0, NULL, 0);
    //    sqlRet = SQLConnect(_SCADAdbc, (SQLCHAR*)"TDS", SQL_NTS, (SQLCHAR*)"rtxagent", SQL_NTS, (SQLCHAR*)"rtxagentpw", SQL_NTS);
    //    sqlRet = SQLConnect(_SCADAdbc, (SQLCHAR*)"TDS", SQL_NTS, (SQLCHAR*)"rtx_user", SQL_NTS, (SQLCHAR*)"rtx_db_agent", SQL_NTS);
    
    
    sqlRet = SQLConnect(_handles.SCADAdbc, (SQLCHAR*)this->dsn().c_str(), SQL_NTS, (SQLCHAR*)this->uid().c_str(), SQL_NTS, (SQLCHAR*)this->pwd().c_str(), SQL_NTS);
    
    
    SQL_CHECK(sqlRet, "SQLDriverConnect", _handles.SCADAdbc, SQL_HANDLE_DBC);
    
    /* allocate the statement handles for data aquisition */
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &_handles.SCADAstmt), "SQLAllocHandle", _handles.SCADAstmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &_handles.rangeStatement), "SQLAllocHandle", _handles.rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &_handles.lowerBoundStatement), "SQLAllocHandle", _handles.lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &_handles.upperBoundStatement), "SQLAllocHandle", _handles.upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &_handles.SCADAtimestmt), "SQLAllocHandle", _handles.SCADAtimestmt, SQL_HANDLE_STMT);
    
    // bindings for single point statement
    /* bind tempRecord members to SQL return columns */
    bindOutputColumns(_handles.SCADAstmt, &_tempRecord);
    // bind input parameters, so we can easily change them when we want to make requests.
    sqlRet = SQLBindParameter(_handles.SCADAstmt, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd);
    sqlRet = SQLBindParameter(_handles.SCADAstmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd);
    
    // bindings for the range statement
    bindOutputColumns(_handles.rangeStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_handles.rangeStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _handles.rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_handles.rangeStatement, 3, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _handles.rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_handles.rangeStatement, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _handles.rangeStatement, SQL_HANDLE_STMT);
    
    SQLUINTEGER timeout = 5;
    // query timeout
    SQL_CHECK(SQLSetStmtAttr(_handles.rangeStatement, SQL_ATTR_QUERY_TIMEOUT, &timeout, 0), "SQLSetStmtAttr", _handles.rangeStatement, SQL_HANDLE_STMT);
    
    // bindings for lower bound statement
    bindOutputColumns(_handles.lowerBoundStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_handles.lowerBoundStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _handles.lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_handles.lowerBoundStatement, 3, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _handles.lowerBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_handles.lowerBoundStatement, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _handles.lowerBoundStatement, SQL_HANDLE_STMT);
    
    // bindings for upper bound statement
    bindOutputColumns(_handles.upperBoundStatement, &_tempRecord);
    SQL_CHECK(SQLBindParameter(_handles.upperBoundStatement, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.start, sizeof(SQL_TIMESTAMP_STRUCT), &_query.startInd), "SQLBindParameter", _handles.upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_handles.upperBoundStatement, 3, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0, &_query.end, sizeof(SQL_TIMESTAMP_STRUCT), &_query.endInd), "SQLBindParameter", _handles.upperBoundStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLBindParameter(_handles.upperBoundStatement, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, MAX_SCADA_TAG, 0, _query.tagName, 0, &_query.tagNameInd), "SQLBindParameter", _handles.upperBoundStatement, SQL_HANDLE_STMT);
    
    
    // prepare the statements
    //SQL_CHECK(SQLPrepare(_SCADAstmt, (SQLCHAR*)singleSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _SCADAstmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_handles.rangeStatement, (SQLCHAR*)_querySyntax.rangeSelect.c_str(), SQL_NTS), "SQLPrepare", _handles.rangeStatement, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(_handles.SCADAtimestmt, (SQLCHAR*)_querySyntax.timeQuery.c_str(), SQL_NTS), "SQLPrepare", _handles.SCADAtimestmt, SQL_HANDLE_STMT);
    //SQL_CHECK(SQLPrepare(_lowerBoundStatement, (SQLCHAR*)lowerBoundSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _lowerBoundStatement, SQL_HANDLE_STMT);
    //SQL_CHECK(SQLPrepare(_upperBoundStatement, (SQLCHAR*)upperBoundSelectQuery().c_str(), SQL_NTS), "SQLPrepare", _upperBoundStatement, SQL_HANDLE_STMT);
    
    // if we made it this far...
    _connectionOk = true;
    
    errorMessage = "OK";
    
  } catch (string err) {
    errorMessage = err;
    cerr << "Initialize failed: " << err << "\n";
    _connectionOk = false;
    //throw DbPointRecord::RtxDbConnectException();
  }
  
  // todo -- check time offset (dst?)
  
}




vector<string> OdbcPreparedPointRecord::identifiers() {
  vector<string> ids;
  if (!isConnected()) {
    return ids;
  }
  
  string tagQuery("");
  tagQuery += "SELECT " + _tableDescription.tagNameCol;
  if (!RTX_STRINGS_ARE_EQUAL(_tableDescription.tagUnitsCol,"")) {
    tagQuery += ", " + _tableDescription.tagUnitsCol;
  }
  tagQuery += " FROM " + _tableDescription.tagTable + " ORDER BY " + _tableDescription.tagNameCol;
  
  SQLCHAR tagName[512];
  SQLHSTMT tagStmt;
  SQLRETURN retCode;
  SQLLEN tagLengthInd;
  
  try {
    SQL_CHECK(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &tagStmt), "SQLAllocHandle", _handles.SCADAstmt, SQL_HANDLE_STMT);
    SQL_CHECK(SQLPrepare(tagStmt, (SQLCHAR*)tagQuery.c_str(), SQL_NTS), "identifiers", _handles.SCADAdbc, SQL_HANDLE_DBC);
    SQL_CHECK(SQLExecute(tagStmt), "SQLExecute", tagStmt, SQL_HANDLE_STMT);
  } catch (string &e) {
    return ids;
  }
  
  
  
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



// select just returns the results (no caching)
vector<Point> OdbcPreparedPointRecord::selectRange(const string& id, time_t startTime, time_t endTime) {
  return pointsWithStatement(id, _handles.rangeStatement, startTime, endTime);
}


Point OdbcPreparedPointRecord::selectNext(const string& id, time_t time) {
  Point p;
  time_t margin = 60*60*12;
  vector<Point> points = pointsWithStatement(id, _handles.rangeStatement, time-1, time + margin);
  
  time_t max_margin = this->searchDistance();
  time_t lookahead = time;
  while (points.size() == 0 && lookahead < time + max_margin) {
    //cout << "scada lookahead" << endl;
    lookahead += margin;
    points = pointsWithStatement(id, _handles.rangeStatement, time-1, lookahead+margin);
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


Point OdbcPreparedPointRecord::selectPrevious(const string& id, time_t time) {
  Point p;
  time_t margin = 60*60*24*7; // one week?
  vector<Point> points = pointsWithStatement(id, _handles.rangeStatement, time - margin, time+1);
  
  time_t max_margin = this->searchDistance();
  time_t lookbehind = time;
  while (points.size() == 0 && lookbehind > time - max_margin) {
    //cout << "scada lookbehind" << endl;
    lookbehind -= margin;
    points = pointsWithStatement(id, _handles.rangeStatement, lookbehind-margin, time+1);
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
void OdbcPreparedPointRecord::insertSingle(const string& id, Point point) {
  
}


void OdbcPreparedPointRecord::insertRange(const string& id, vector<Point> points) {
  
}


void OdbcPreparedPointRecord::removeRecord(const string& id) {
  
}


void OdbcPreparedPointRecord::truncate() {
  
}





#pragma mark - Protected

ostream& OdbcPreparedPointRecord::toStream(ostream &stream) {
  stream << "ODBC Scada Point Record" << endl;
  // todo - stream extra info
  return stream;
}



#pragma mark - Internal (private) methods


vector<Point> OdbcPointRecord::pointsWithStatement(const string& id, SQLHSTMT statement, time_t startTime, time_t endTime) {
  
  if (!_connectionOk) {
    this->dbConnect();
  }
  
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
  list<ScadaRecord> records;
  
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
    if (_querySyntax.qualityMap.count(qu) > 0) {
      q = _querySyntax.qualityMap[qu];
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
  
  // make sure the points are sorted
  std::sort(points.begin(), points.end(), &Point::comparePointTime);
  
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
  SQL_CHECK(SQLBindCol(statement, 1, SQL_TYPE_TIMESTAMP, &(record->time), NULL, &(record->timeInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  //SQL_CHECK(SQLBindCol(statement, 2, SQL_C_CHAR, record->tagName, MAX_SCADA_TAG, &(record->tagNameInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  SQL_CHECK(SQLBindCol(statement, 2, SQL_DOUBLE, &(record->value), 0, &(record->valueInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
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
    errorMessage = std::string((char*)text);
    // check if it's a connection issue
    if (strncmp((char*)state, "SCADA_CONNECTION_ISSSUE", 2) == 0) {
      msg += "::Connection Issue::";
      return msg;
    }
  }
  while( ret == SQL_SUCCESS );
	return msg;
}


