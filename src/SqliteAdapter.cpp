#include "SqliteAdapter.h"

#include <set>
#include <sstream>

using namespace std;
using namespace RTX;


static int sqlitePointRecordCurrentDbVersion = 2;

typedef const unsigned char* sqltext;

/******************************************************************************************/
static string initTablesStr = "CREATE TABLE 'meta' ('series_id' INTEGER PRIMARY KEY ASC AUTOINCREMENT, 'name' TEXT UNIQUE ON CONFLICT ABORT, 'units' TEXT, 'regular_period' INTEGER, 'regular_offset' INTEGER); CREATE TABLE 'points' ('time' INTEGER, 'series_id' INTEGER REFERENCES 'meta'('series_id'), 'value' REAL, 'confidence' REAL, 'quality' INTEGER, UNIQUE (series_id, time asc) ON CONFLICT IGNORE); PRAGMA user_version = 2";
/******************************************************************************************/



SqliteAdapter::SqliteAdapter( errCallback_t cb ) : DbAdapter(cb) {
  _path = "";
  _inTransaction = false;
  _transactionStackCount = 0;
  _maxTransactionStackCount = 50000;
  _dbHandle = NULL;
  _insertSingleStmt = NULL;
}
SqliteAdapter::~SqliteAdapter() {
  sqlite3_finalize(_insertSingleStmt);
  sqlite3_close(_dbHandle);
}

const DbAdapter::adapterOptions SqliteAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.canAssignUnits = true;
  o.supportsUnitsColumn = true;
  o.supportsSinglyBoundQuery = true;
  o.implementationReadonly = false;
  
  return o;
}

std::string SqliteAdapter::connectionString() {
  return _path;
}
void SqliteAdapter::setConnectionString(const std::string& con) {
  if (this->adapterConnected()) {
    sqlite3_close(_dbHandle);
  }
  _path = con;
}

void SqliteAdapter::doConnect() {
  _RTX_DB_SCOPED_LOCK;
  
  if (RTX_STRINGS_ARE_EQUAL(_path, "")) {
    _errCallback("No File Specified");
    return;
  }
  
  int returnCode;
  returnCode = sqlite3_open_v2(_path.c_str(), &_dbHandle, SQLITE_OPEN_READWRITE, NULL); // only if exists
  if (returnCode == SQLITE_CANTOPEN) {
    // attempt to create the db.
    returnCode = sqlite3_open_v2(_path.c_str(), &_dbHandle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (returnCode == SQLITE_OK) {
      if (!this->initTables()) {
        return;
      }
    }
  }
  if( returnCode != SQLITE_OK ){
    this->logDbError(returnCode);
    sqlite3_close(_dbHandle);
    return;
  }
  
  // check schema
  bool updateSuccess = true;
  int databaseVersion = this->dbSchemaVersion();
  if (databaseVersion < 0) {
    // db was opened, but we could not get the user_version from the db's internal data.
    // that sucks, so it's gotta be corrupt.
    _errCallback("Corrupt Database");
    _connected = false;
    sqlite3_close(_dbHandle);
    return;
  }
  if (databaseVersion < sqlitePointRecordCurrentDbVersion) {
    cerr << "Point Record Database Schema version not compatible. Require version " << sqlitePointRecordCurrentDbVersion << " or greater. Updating." << endl;
    updateSuccess = this->updateSchema();
  }
  
  // prepare the select & insert statments
  string selectPreamble = "SELECT time, value, quality, confidence FROM points INNER JOIN meta USING (series_id) WHERE name = ? AND ";
  _selectSingleStr = selectPreamble + "time = ? order by time asc";
  
  
  _selectRangeStr = selectPreamble + "time >= ? AND time <= ?" + " order by time asc";
  _selectNextStr = selectPreamble + "time > ?" + " order by time asc LIMIT 1";
  _selectPreviousStr = selectPreamble + "time < ?" + " order by time desc LIMIT 1";
  _insertSingleStr = "INSERT INTO points (time, series_id, value, quality, confidence) VALUES (?,?,?,?,?)";
  _selectFirstStr = selectPreamble + "1 order by time asc limit 1";
  _selectLastStr = selectPreamble + "1 order by time desc limit 1";
  _selectNamesStr = "select series_id,name,units from meta order by name asc";
  
  // prepare statements for performance
  sqlite3_finalize(_insertSingleStmt);
  sqlite3_prepare_v2(_dbHandle, _insertSingleStr.c_str(), -1, &_insertSingleStmt, NULL);
  
  _errCallback("OK");
  _connected = true;
}


IdentifierUnitsList SqliteAdapter::idUnitsList() {
  _RTX_DB_SCOPED_LOCK;
  IdentifierUnitsList ids;
  _metaCache.clear();
  
  sqlite3_stmt *selectIdsStmt;
  int ret;
  ret = sqlite3_prepare_v2(_dbHandle, _selectNamesStr.c_str(), -1, &selectIdsStmt, NULL);
  ret = sqlite3_step(selectIdsStmt);
  while (ret == SQLITE_ROW) {
    int uid = sqlite3_column_int(selectIdsStmt, 0);
    string name = string((char*)sqlite3_column_text(selectIdsStmt, 1));
    int type = sqlite3_column_type(selectIdsStmt, 2);
    Units units;
    if (type == SQLITE_NULL) {
      units = RTX_NO_UNITS;
    }
    else {
      string unitsStr = string((char*)sqlite3_column_text(selectIdsStmt, 2));
      units = Units::unitOfType(unitsStr);
    }
    ids.set(name,units);
    ret = sqlite3_step(selectIdsStmt);
    _metaCache[name] = uid;
  }
  sqlite3_reset(selectIdsStmt);
  sqlite3_finalize(selectIdsStmt);
  
  return ids;
}

// TRANSACTIONS
void SqliteAdapter::beginTransaction() {
  if (_inTransaction) {
    return;
  }
  this->checkTransactions(false);
  _inTransaction = true;
}
void SqliteAdapter::endTransaction() {
  if (!_inTransaction) {
    return;
  }
  this->checkTransactions(true);
  _inTransaction = false;
}

// READ
std::vector<Point> SqliteAdapter::selectRange(const std::string& id, TimeRange range) {
  vector<Point> points;
  
  if (!adapterConnected()) {
    this->doConnect();
  }
  if (adapterConnected()) {
    // SELECT time, value, quality, confidence FROM points INNER JOIN meta USING (series_id) WHERE name = ? AND time >= ? AND time <= ? order by time asc
    
    //    checkTransactions(true);
    
    _RTX_DB_SCOPED_LOCK;
    
    sqlite3_stmt *s;
    int ret = sqlite3_prepare_v2(_dbHandle, _selectRangeStr.c_str(), -1, &s, NULL);
    
    sqlite3_bind_text(s, 1, id.c_str(), -1, NULL);
    sqlite3_bind_int(s, 2, (int)range.start);
    sqlite3_bind_int(s, 3, (int)range.end);
    
    points = pointsFromPreparedStatement(s);
    
    sqlite3_reset(s);
    sqlite3_finalize(s);
    
  }
  return points;
}
Point SqliteAdapter::selectNext(const std::string& id, time_t time) {
  sqlite3_stmt *s;
  sqlite3_prepare_v2(_dbHandle, _selectNextStr.c_str(), -1, &s, NULL);
  sqlite3_bind_text(s, 1, id.c_str(), -1, NULL);
  sqlite3_bind_int(s, 2, (int)time);
  vector<Point> points = pointsFromPreparedStatement(s);
  sqlite3_finalize(s);
  
  if (points.size() > 0) {
    return points.front();
  }
  return Point();
}
Point SqliteAdapter::selectPrevious(const std::string& id, time_t time) {
  sqlite3_stmt *s;
  sqlite3_prepare_v2(_dbHandle, _selectPreviousStr.c_str(), -1, &s, NULL);
  sqlite3_bind_text(s, 1, id.c_str(), -1, NULL);
  sqlite3_bind_int(s, 2, (int)time);
  vector<Point> points = pointsFromPreparedStatement(s);
  sqlite3_finalize(s);
  
  if (points.size() > 0) {
    return points.back();
  }
  return Point();
}

// CREATE
bool SqliteAdapter::insertIdentifierAndUnits(const std::string& id, Units units) {
  int ret = 0;
  bool success = false;
  
  // has to be connected, otherwise there may not be a row for this guy in the meta table.
  // TODO -- check for errors when inserting data??
  if (!this->adapterConnected()) {
    this->doConnect();
  }
  
  if (this->adapterConnected()) {
    _RTX_DB_SCOPED_LOCK;
    
    // insert a name here.
    // INSERT IGNORE INTO meta (name,units) VALUES (?,?)
    string unitsStr = units.to_string();
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(_dbHandle, "insert or ignore into meta (name,units) values (?,?)", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 2, unitsStr.c_str(), -1, NULL);
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      logDbError(ret);
    }
    else {
      success = true;
    }
    int uid = (int)sqlite3_last_insert_rowid(_dbHandle);
    
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    
    // add to the cache.
    _metaCache[id] = uid;
  }
  if (_inTransaction) {
    this->checkTransactions(false);
  }
  return success;
}

void SqliteAdapter::insertSingle(const std::string& id, Point point) {
  if (_inTransaction) { // caller has promised to end the bulk operation eventually.
    insertSingleInTransaction(id, point);
    this->checkTransactions(false); // only flush if we've reached the stride size.
  }
  
  else {
    this->checkTransactions(false);
    this->insertSingleInTransaction(id, point);
    this->checkTransactions(true);
  }

}

void SqliteAdapter::insertSingleInTransaction(const std::string& id, Point point) {
  
  if (!adapterConnected()) {
    doConnect();
  }
  if (adapterConnected()) {
    _RTX_DB_SCOPED_LOCK;
    
    int ret;
    // INSERT INTO points (time, series_id, value, quality, confidence) VALUES ?,?,?,?,?
    int tsUid = _metaCache[id];
    ret = sqlite3_bind_int(    _insertSingleStmt, 1, (int)point.time      );
    ret = sqlite3_bind_int(    _insertSingleStmt, 2, tsUid                );
    ret = sqlite3_bind_double( _insertSingleStmt, 3, point.value          );
    ret = sqlite3_bind_int(    _insertSingleStmt, 4, point.quality        );
    ret = sqlite3_bind_double( _insertSingleStmt, 5, point.confidence     );
    
    ret = sqlite3_step(_insertSingleStmt);
    if (ret != SQLITE_DONE) {
      logDbError(ret);
    }
    sqlite3_reset(_insertSingleStmt);
  }
  
  return;
}


void SqliteAdapter::insertRange(const std::string& id, std::vector<Point> points) {
  if (!adapterConnected()) {
    doConnect();
  }
  if (adapterConnected()) {
    
    int ret;
    char *errmsg;
    
    {
      checkTransactions(true); // any tranactions currently? tell them to quit.
      _RTX_DB_SCOPED_LOCK;
      ret = sqlite3_exec(_dbHandle, "begin exclusive transaction", NULL, NULL, &errmsg);
      if (ret != SQLITE_OK) {
        logDbError(ret);
        return;
      }
    }
    
    // this is always within a transaction
    for(const Point &p : points) {
      this->insertSingleInTransaction(id, p);
    }
    {
      _RTX_DB_SCOPED_LOCK;
      ret = sqlite3_exec(_dbHandle, "end transaction", NULL, NULL, &errmsg);
      if (ret != SQLITE_OK) {
        logDbError(ret);
        return;
      }
    }
  }

}

// UPDATE
bool SqliteAdapter::assignUnitsToRecord(const std::string& name, const Units& units) {
  int ret = 0;
  bool success = false;
  
  // has to be connected, otherwise there may not be a row for this guy in the meta table.
  // TODO -- check for errors when inserting data??
  if (!this->adapterConnected()) {
    this->doConnect();
  }
  
  if (this->adapterConnected()) {
    _RTX_DB_SCOPED_LOCK;
    
    string unitsStr = units.to_string();
    sqlite3_stmt *stmt;
    string q = "update meta set units = ? where name = \'" + name + "\'";
    sqlite3_prepare_v2(_dbHandle, q.c_str(), -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, unitsStr.c_str(), -1, NULL);
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      logDbError(ret);
    }
    else {
      success = true;
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
  }
  
  return success;
}

// DELETE
void SqliteAdapter::removeRecord(const std::string& id) {
  _RTX_DB_SCOPED_LOCK;
  
  if (!_connected) {
    return;
  }
  
  char *errmsg;
  string sqlStr = "delete from points where series_id = (SELECT series_id FROM meta where name = \'" + id + "\'); delete from meta where name = \'" + id + "\'";
  const char *sql = sqlStr.c_str();
  int ret = sqlite3_exec(_dbHandle, sql, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    logDbError(ret);
    return;
  }
}
void SqliteAdapter::removeAllRecords() {
  if (!_connected) {
    return;
  }
  _RTX_DB_SCOPED_LOCK;
  
  char *errmsg;
  int ret = sqlite3_exec(_dbHandle, "delete from points", NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    logDbError(ret);
    return;
  }
}



#pragma mark private

bool SqliteAdapter::initTables() {
  char *errmsg;
  int errCode;
  errCode = sqlite3_exec(_dbHandle, initTablesStr.c_str(), NULL, NULL, &errmsg);
  if (errCode != SQLITE_OK) {
    _errCallback(string(errmsg));
    return false;
  }
  return true;
}

int SqliteAdapter::dbSchemaVersion() {
  int vers = -1;
  sqlite3_stmt *stmt_version;
  if(sqlite3_prepare_v2(_dbHandle, "PRAGMA user_version;", -1, &stmt_version, NULL) == SQLITE_OK) {
    while(sqlite3_step(stmt_version) == SQLITE_ROW) {
      vers = sqlite3_column_int(stmt_version, 0);
    }
  }
  sqlite3_reset(stmt_version);
  sqlite3_finalize(stmt_version);
  return vers;
}
void SqliteAdapter::setDbSchemaVersion(int v) {
  stringstream ss;
  ss << "PRAGMA user_version = " << v;
  sqlite3_exec(_dbHandle, ss.str().c_str(), NULL, NULL, NULL);
}


bool SqliteAdapter::updateSchema() {
  
  int currentVersion = this->dbSchemaVersion();
  
  while (currentVersion >= 0 && currentVersion < sqlitePointRecordCurrentDbVersion) {
    
    switch (currentVersion) {
      case 0:
      case 1:
      {
        // migrate 0,1->2
        stringstream ss;
        ss << "BEGIN TRANSACTION; ";
        ss << "UPDATE points SET quality = 128; ";
        ss << "END TRANSACTION; ";
        
        sqlite3_exec(_dbHandle, ss.str().c_str(), NULL, NULL, NULL);
        currentVersion = 2;
        this->setDbSchemaVersion(currentVersion);
      }
        break;
        
      default:
        break;
    }
    
  }
  
  if (currentVersion == sqlitePointRecordCurrentDbVersion) {
    return true;
  }
  else {
    return false;
  }
}




void SqliteAdapter::logDbError(int ret) {
  const char *zErrMsg = sqlite3_errmsg(_dbHandle);
  string err = string(zErrMsg);
  _errCallback(err);
  cerr << "sqlite error: " << err << endl;
}




std::vector<Point> SqliteAdapter::pointsFromPreparedStatement(sqlite3_stmt *stmt) {
  int ret;
  vector<Point> points;
  
  ret = sqlite3_step(stmt);
  while (ret == SQLITE_ROW) {
    Point p = pointFromStatment(stmt);
    points.push_back(p);
    ret = sqlite3_step(stmt);
  }
  // finally...
  if (ret != SQLITE_DONE) {
    // something's wrong
    cerr << "sqlite returns " << ret << " -- prepared statement fails" << endl;
  }
  sqlite3_reset(stmt);
  
  return points;
}

Point SqliteAdapter::pointFromStatment(sqlite3_stmt *stmt) {
  // SELECT time, value, quality, confidence
  time_t time = sqlite3_column_int(stmt, 0);
  double value = sqlite3_column_double(stmt, 1);
  int q = sqlite3_column_int(stmt, 2);
  double c = sqlite3_column_double(stmt, 3);
  Point::PointQuality qual = Point::PointQuality( q );
  
  return Point(time, value, qual, c);
}



void SqliteAdapter::checkTransactions(bool forceEndTranaction) {
  _RTX_DB_SCOPED_LOCK;
  // forcing to end?
  if (forceEndTranaction) {
    if (_inTransaction) {
      _transactionStackCount = 0;
      sqlite3_exec(_dbHandle, "END", 0, 0, 0);
      _inTransaction = false;
      return;
    }
    else {
      // do nothing
      return;
    }
  }
  
  
  if (_inTransaction) {
    if (_transactionStackCount >= _maxTransactionStackCount) {
      // reset the stack, commit the stack
      _transactionStackCount = 0;
      sqlite3_exec(_dbHandle, "END", 0, 0, 0);
      _inTransaction = false;
    }
    else {
      // increment the stack count.
      ++_transactionStackCount;
    }
  }
  else {
    sqlite3_exec(_dbHandle, "BEGIN", 0, 0, 0);
    _inTransaction = true;
  }
  
}




