#include "OdbcAdapter.h"



using namespace std;
using namespace RTX;

using boost::local_time::posix_time_zone;
using boost::local_time::time_zone_ptr;

const time_t _rtx_odbc_connect_timeout(5);


#define RTX_ODBC_MAX_RETRY 5

string __extract_error(string function, SQLHANDLE handle, SQLSMALLINT type);
string __extract_error(string function, SQLHANDLE handle, SQLSMALLINT type) {
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
    //errorMessage = std::string((char*)text);
    // check if it's a connection issue
    if (strncmp((char*)state, "SCADA_CONNECTION_ISSSUE", 2) == 0) {
      msg += "::Connection Issue::";
      return msg;
    }
  }
  while( ret == SQL_SUCCESS );
  return msg;
}


SQLRETURN __SQL_CHECK(SQLRETURN retVal, string function, SQLHANDLE handle, SQLSMALLINT type) throw(string);
SQLRETURN __SQL_CHECK(SQLRETURN retVal, string function, SQLHANDLE handle, SQLSMALLINT type) throw(string) {
  if(!SQL_SUCCEEDED(retVal)) {
    throw __extract_error(function, handle, type);
  }
  return retVal;
}





OdbcAdapter::OdbcAdapter( errCallback_t cb ) : DbAdapter(cb) {
  _isConnecting = false;
  std::string nyc("EST-5EDT,M4.1.0,M10.5.0");
  _specifiedTimeZoneString = nyc;
  _specifiedTimeZone.reset(new posix_time_zone(nyc));
  
  _timeFormat = PointRecordTime::UTC;
  _handles.SCADAenv = NULL;
  _handles.SCADAdbc = NULL;
  
  // sample queries as default
  _querySyntax.rangeSelect = "SELECT date_col, value_col, quality_col FROM data_tbl WHERE tagname_col = ? AND date_col >= ? AND date_col <= ? ORDER BY date_col ASC";
  _querySyntax.metaSelect = "SELECT tagname_col FROM tag_list_tbl ORDER BY tagname_col ASC";
  
  SQLRETURN sqlRet;
  
  /* Allocate an environment handle */
  __SQL_CHECK(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_handles.SCADAenv), "SQLAllocHandle", _handles.SCADAenv, SQL_HANDLE_ENV);
  /* We want ODBC 3 support */
  __SQL_CHECK(SQLSetEnvAttr(_handles.SCADAenv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0), "SQLSetEnvAttr", _handles.SCADAenv, SQL_HANDLE_ENV);
  /* Allocate a connection handle */
  __SQL_CHECK(SQLAllocHandle(SQL_HANDLE_DBC, _handles.SCADAenv, &_handles.SCADAdbc), "SQLAllocHandle", _handles.SCADAenv, SQL_HANDLE_ENV);
  /* Connect to the DSN, checking for connectivity */
  //"Attempting to Connect to SCADA..."
  
  // readonly
  SQLUINTEGER mode = SQL_MODE_READ_ONLY;
  __SQL_CHECK(SQLSetConnectAttr(_handles.SCADAdbc, SQL_ATTR_ACCESS_MODE, &mode, SQL_IS_UINTEGER), "SQLSetConnectAttr", _handles.SCADAdbc, SQL_HANDLE_DBC);
  
  // timeouts
  SQLUINTEGER timeout = 5;
  sqlRet = __SQL_CHECK(SQLSetConnectAttr(_handles.SCADAdbc, SQL_ATTR_LOGIN_TIMEOUT, &timeout, 0/*ignored*/), "SQLSetConnectAttr", _handles.SCADAdbc, SQL_HANDLE_DBC);
  sqlRet = __SQL_CHECK(SQLSetConnectAttr(_handles.SCADAdbc, SQL_ATTR_CONNECTION_TIMEOUT, &timeout, 0/*ignored*/), "SQLSetConnectAttr", _handles.SCADAdbc, SQL_HANDLE_DBC);
  
  this->initDsnList();

}

OdbcAdapter::~OdbcAdapter() {
  if (_connectFuture.valid()) {
    _connectFuture.wait();
  }
  // make sure handles are free
  if (_handles.SCADAdbc != NULL) {
    SQLDisconnect(_handles.SCADAdbc);
  }
  //  SQLFreeStmt(_handles.rangeStatement, SQL_CLOSE);
  SQLFreeHandle(SQL_HANDLE_DBC, _handles.SCADAdbc);
  SQLFreeHandle(SQL_HANDLE_ENV, _handles.SCADAenv);
}

void OdbcAdapter::initDsnList() {
  SQLRETURN sqlRet;
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
      cerr << "\tdata truncation\n" << endl;
    }
  }
}

const DbAdapter::adapterOptions OdbcAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.supportsUnitsColumn = false;
  o.canAssignUnits = false;
  o.searchIteratively = true;
  o.supportsSinglyBoundQuery = false;
  o.implementationReadonly = true;
  
  return o;
}


std::string OdbcAdapter::driver() {
  return _connection.driver;
}
void OdbcAdapter::setDriver(const std::string& driver) {
  _connection.driver = driver;
}

std::string OdbcAdapter::connectionString() {
  return _connection.connectionString;
}

void OdbcAdapter::setConnectionString(const std::string& con) {
  _connection.connectionString = con;
}

std::string OdbcAdapter::metaQuery() {
  return _querySyntax.metaSelect;
}

void OdbcAdapter::setMetaQuery(const std::string& meta) {
  _querySyntax.metaSelect = meta;
}

std::string OdbcAdapter::rangeQuery() {
  return _querySyntax.rangeSelect;
}

void OdbcAdapter::setRangeQuery(const std::string& range) {
  _querySyntax.rangeSelect = range;
}


void OdbcAdapter::doConnect() {
  _RTX_DB_SCOPED_LOCK;
  
  if (_connected) {
    SQLDisconnect(_handles.SCADAdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, _handles.SCADAdbc);
    /* Allocate a connection handle */
    __SQL_CHECK(SQLAllocHandle(SQL_HANDLE_DBC, _handles.SCADAenv, &_handles.SCADAdbc), "SQLAllocHandle", _handles.SCADAenv, SQL_HANDLE_ENV);
    SQLUINTEGER mode = SQL_MODE_READ_ONLY;
    __SQL_CHECK(SQLSetConnectAttr(_handles.SCADAdbc, SQL_ATTR_ACCESS_MODE, &mode, SQL_IS_UINTEGER), "SQLSetConnectAttr", _handles.SCADAdbc, SQL_HANDLE_DBC);
  }
  
  _connected = false;
  if (RTX_STRINGS_ARE_EQUAL(_connection.driver, "") ||
      RTX_STRINGS_ARE_EQUAL(_connection.connectionString, "") ) {
    _errCallback("Incomplete Connection Information");
    return;
  }
  
  // clean-up potentially inconsistent state
  if (_connectFuture.valid() && !_isConnecting) {
    _connectFuture.get(); // release state & mark invalid
  }
  
  if (_connectFuture.valid()) {
    // future is still working, but here we are. TIMEOUT occurred.
    _errCallback("Connection Timeout. Try Later.");
    return;
  }
  else {
    // first connection attempt. wait for "timeout" seconds
    // async connection timeout
    _connectFuture = std::async(launch::async, [&]() -> bool {
      string connStr = "DRIVER=" + _connection.driver + ";" + _connection.connectionString;
      SQLCHAR outConStr[1024];
      SQLSMALLINT outConStrLen;
      
      _isConnecting = true;
      bool connection_ok = false;
      try {
        SQLRETURN connectRet = SQLDriverConnect(_handles.SCADAdbc, NULL, (SQLCHAR*)connStr.c_str(), strlen(connStr.c_str()), outConStr, 1024, &outConStrLen, SQL_DRIVER_COMPLETE);
        __SQL_CHECK(connectRet, "SQLDriverConnect", _handles.SCADAdbc, SQL_HANDLE_DBC);
        connection_ok = true;
        _errCallback("Connected");
      } catch (string err) {
        connection_ok = false;
        cerr << err << endl;
        _errCallback(err);
      }
      _isConnecting = false;
      return connection_ok;
    });
    
    chrono::seconds timeout(_rtx_odbc_connect_timeout);
    
    future_status stat = _connectFuture.wait_for(timeout);
    if (stat == future_status::timeout) {
      // timed out. bad connection.
      _connected = false;
      _errCallback("Timeout");
    }
    else if (stat == future_status::ready) {
      _connected = _connectFuture.get();
    }
  }
  
  // to-do :: query for sql time offset / ensure correct adjustments.
}


IdentifierUnitsList OdbcAdapter::idUnitsList() {
  _RTX_DB_SCOPED_LOCK;
  SQLHSTMT getIdsStmt = 0;
  IdentifierUnitsList ids;
  
  string metaQ = _querySyntax.metaSelect;
  
  // FIXME
  // add handling for units column - output col 2
  
  SQLCHAR tagName[512];
  SQLLEN tagLengthInd;
  SQLRETURN retcode;
  
  retcode = SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &getIdsStmt);
  retcode = SQLExecDirect(getIdsStmt, (SQLCHAR*)metaQ.c_str(), SQL_NTS);
  
  if (!SQL_SUCCEEDED(retcode)) {
    _errCallback("Could not read Tag table");
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
  
  return ids;
}


// TRANSACTIONS
void OdbcAdapter::beginTransaction() {
  // nope
}

void OdbcAdapter::endTransaction() {
  // nope
}


// READ
std::vector<Point> OdbcAdapter::selectRange(const std::string& id, TimeRange range) {
  // construct the static query text
  string q = this->stringQueryForRange(id, range);
  vector<Point> points;
  SQLHSTMT rangeStmt = 0;
  
  bool fetchSuccess = false;
  int iFetchAttempt = 0;
  do {
    // execute the query and get a result set
    {
      _RTX_DB_SCOPED_LOCK;
      if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, _handles.SCADAdbc, &rangeStmt))) {
        // FIXME - do something like return a range of bad points?
        cerr << "could not allocate sql handle" << endl;
        _errCallback(__extract_error("SQLAllocHandle", _handles.SCADAdbc, SQL_HANDLE_STMT));
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
        _RTX_DB_SCOPED_LOCK;
        cerr << __extract_error("SQLExecDirect", rangeStmt, SQL_HANDLE_STMT) << endl;
        cerr << "query did not succeed: " << q << endl;
      }
      // do something more intelligent here. re-check connection?
      this->doConnect();
    }
    ++iFetchAttempt;
  } while (!fetchSuccess && iFetchAttempt < RTX_ODBC_MAX_RETRY);
  
  return points;

}

Point OdbcAdapter::selectNext(const std::string& id, time_t time) {
  return Point(); // unsupported
}

Point OdbcAdapter::selectPrevious(const std::string& id, time_t time) {
  return Point(); // unsupported
}


// CREATE
bool OdbcAdapter::insertIdentifierAndUnits(const std::string& id, Units units) {
  return false; // unsupported
}

void OdbcAdapter::insertSingle(const std::string& id, Point point) {
  return; // unsupported
}

void OdbcAdapter::insertRange(const std::string& id, std::vector<Point> points) {
  return; // unsupported
}


// UPDATE
bool OdbcAdapter::assignUnitsToRecord(const std::string& name, const Units& units) {
  return false; // unsupported
}


// DELETE
void OdbcAdapter::removeRecord(const std::string& id) {
  return; // unsupported
}

void OdbcAdapter::removeAllRecords() {
  return; // unsupported
}



// odbc-specific methods
std::list<std::string> OdbcAdapter::listDrivers() {
  list<string> drivers;
  SQLRETURN sqlRet;
  SQLCHAR driverChar[256];
  SQLCHAR driverAttr[1256];
  SQLSMALLINT driver_ret, attr_ret;
  SQLUSMALLINT direction = SQL_FETCH_FIRST;
  SQLHENV env;
  sqlRet = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
  while(SQL_SUCCEEDED(sqlRet = SQLDrivers(env, direction, driverChar, 256, &driver_ret, driverAttr, 1256, &attr_ret))) {
    direction = SQL_FETCH_NEXT;
    string thisDriver = string((char*)driverChar);
    string thisAttr = string((char*)driverAttr);
    drivers.push_back(thisDriver);
  }
  SQLFreeHandle(SQL_HANDLE_ENV, env);
  return drivers;
}

void OdbcAdapter::setTimeFormat(PointRecordTime::time_format_t timeFormat) {
  _timeFormat = timeFormat;
}

PointRecordTime::time_format_t OdbcAdapter::timeFormat() {
  return _timeFormat;
}

std::string OdbcAdapter::timeZoneString() {
  return _specifiedTimeZoneString;
}

void OdbcAdapter::setTimeZoneString(const std::string& tzStr) {
  try {
    time_zone_ptr newTimeZone(new posix_time_zone(tzStr));
    if (newTimeZone) {
      _specifiedTimeZoneString = tzStr;
      _specifiedTimeZone = newTimeZone;
    }
  } catch (...) {
    _errCallback("Invalid TZ");
    cerr << "could not set time zone string" << endl;
  }
}



std::string OdbcAdapter::stringQueryForRange(const std::string& id, TimeRange range) {
  
  string query = _querySyntax.rangeSelect;
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


std::vector<Point> OdbcAdapter::pointsFromStatement(SQLHSTMT statement) {
  vector<ScadaRecord> records;
  vector<Point> points;
  
  // make sure output columns are bound. we're not sure where this statement is coming from.
  this->bindOutputColumns(statement, &_tempRecord);
  
  try {
    if (statement == NULL) {
      throw string("Connection not initialized.");
    }
    while (SQL_SUCCEEDED(SQLFetch(statement))) {
      records.push_back(_tempRecord);
    }
  }
  catch(string errorMessage) {
    cerr << errorMessage << endl;
    cerr << "Could not get data from db connection\n";
    cerr << "Attempting to reconnect..." << endl;
    this->doConnect();
    cerr << "Connection returned " << this->adapterConnected() << endl;
  }
  
  __SQL_CHECK(SQLFreeStmt(statement, SQL_UNBIND), "SQL_UNBIND", statement, SQL_HANDLE_STMT);
  
  for(const ScadaRecord& record : records) {
    Point p;
    time_t t;
    if (_timeFormat == PointRecordTime::UTC) {
      t = PointRecordTime::time(record.time);
    }
    else {
      t = PointRecordTime::timeFromZone(record.time, _specifiedTimeZone);
    }
    double v = record.value;
    Point::PointQuality q = Point::opc_good;
    q = (Point::PointQuality)record.quality;
    
    if (record.valueInd > 0) {
      // ok
      p = Point(t, v, q, 0.);
      points.push_back(p);
    }
    else {
      // nothing
    }
  }
  
  // make sure the points are sorted
  std::sort(points.begin(), points.end(), &Point::comparePointTime);
  return points;
}


void OdbcAdapter::bindOutputColumns(SQLHSTMT statement, ScadaRecord* record) {
  __SQL_CHECK(SQLBindCol(statement, 1, SQL_TYPE_TIMESTAMP, &(record->time), NULL, &(record->timeInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  __SQL_CHECK(SQLBindCol(statement, 2, SQL_DOUBLE, &(record->value), 0, &(record->valueInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
  __SQL_CHECK(SQLBindCol(statement, 3, SQL_INTEGER, &(record->quality), 0, &(record->qualityInd) ), "SQLBindCol", statement, SQL_HANDLE_STMT);
}




