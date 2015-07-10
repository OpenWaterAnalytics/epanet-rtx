#include "SqlitePointRecord.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

#include <boost/interprocess/sync/scoped_lock.hpp>

using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

static int sqlitePointRecordCurrentDbVersion = 2;

typedef const unsigned char* sqltext;

/******************************************************************************************/
static string initTablesStr = "CREATE TABLE 'meta' ('series_id' INTEGER PRIMARY KEY ASC AUTOINCREMENT, 'name' TEXT UNIQUE ON CONFLICT ABORT, 'units' TEXT, 'regular_period' INTEGER, 'regular_offset' INTEGER); CREATE TABLE 'points' ('time' INTEGER, 'series_id' INTEGER REFERENCES 'meta'('series_id'), 'value' REAL, 'confidence' REAL, 'quality' INTEGER, UNIQUE (series_id, time asc) ON CONFLICT IGNORE); PRAGMA user_version = 2";
/******************************************************************************************/



SqlitePointRecord::SqlitePointRecord() {
  _path = "";
  _connected = false;
  
  _inTransaction = false;
  _inBulkOperation = false;
  _transactionStackCount = 0;
  _maxTransactionStackCount = 5000;
  
  _mutex.reset(new boost::signals2::mutex );
  
  _insertSingleStmt = NULL;
  _selectFirstStmt = NULL;
  _selectLastStmt = NULL;
  _selectNamesStmt = NULL;
  _selectNextStmt = NULL;
  _selectPreviousStmt = NULL;
  _selectRangeStmt = NULL;
  _selectSingleStmt = NULL;
  _dbHandle = NULL;
  
}

SqlitePointRecord::~SqlitePointRecord() {
  this->setPath("");
  
  sqlite3_finalize(_insertSingleStmt);
  sqlite3_finalize(_selectFirstStmt);
  sqlite3_finalize(_selectLastStmt);
  sqlite3_finalize(_selectNamesStmt);
  sqlite3_finalize(_selectNextStmt);
  sqlite3_finalize(_selectPreviousStmt);
  sqlite3_finalize(_selectRangeStmt);
  sqlite3_finalize(_selectSingleStmt);
  
  sqlite3_close(_dbHandle);
  
}


string SqlitePointRecord::path() {
  return _path;
}

void SqlitePointRecord::setPath(std::string path) {
  if (this->isConnected()) {
    sqlite3_close(_dbHandle);
  }
  _path = path;
}


void SqlitePointRecord::dbConnect() throw(RtxException) {

  { // lock mutex
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    if (RTX_STRINGS_ARE_EQUAL(_path, "")) {
      errorMessage = "No File Specified";
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
      this->logDbError();
      sqlite3_close(_dbHandle);
      return;
    }
    
    // check schema
    int databaseVersion = this->dbSchemaVersion();
    if (databaseVersion < sqlitePointRecordCurrentDbVersion) {
      cerr << "Point Record Database Schema version not compatible. Require version " << sqlitePointRecordCurrentDbVersion << " or greater. Updating." << endl;
      bool ok = this->updateSchema();
    }
    
    // prepare the select & insert statments
    string selectPreamble = "SELECT time, value, quality, confidence FROM points INNER JOIN meta USING (series_id) WHERE name = ? AND ";
    string singleSelectStr = selectPreamble + "time = ? order by time asc";
    
    // unpack the filtering clause incase there is a black/white list
    string qualClause = "";
    if ((this->opcFilterType() == OpcBlackList || this->opcFilterType() == OpcWhiteList) && this->opcFilterList().size() > 0) {
      stringstream filterSS;
      filterSS << " AND quality";
      if (this->opcFilterType() == OpcBlackList) {
        filterSS << " NOT";
      }
      filterSS << " IN (";
      set<unsigned int> codes = this->opcFilterList();
      set<unsigned int>::const_iterator it = codes.begin();
      filterSS << *it;
      ++it;
      while (it != codes.end()) {
        filterSS << "," << *it;
        ++it;
      }
      filterSS << ")";
      qualClause = filterSS.str();
    }
    
    
    string rangeSelectStr = selectPreamble + "time >= ? AND time <= ?" + qualClause + " order by time asc";
    string nextSelectStr = selectPreamble + "time > ?" + qualClause + " order by time asc LIMIT 1";
    string prevSelectStr = selectPreamble + "time < ?" + qualClause + " order by time desc LIMIT 1";
    string singleInsertStr = "INSERT INTO points (time, series_id, value, quality, confidence) SELECT ?,series_id,?,?,? FROM meta WHERE name = ?";
    string firstSelectStr = selectPreamble + "1 order by time asc limit 1";
    string lastSelectStr = selectPreamble + "1 order by time desc limit 1";
    string selectNamesStr = "select name,units from meta order by name asc";
    
    
    
    string statmentStrings[] =           {selectNamesStr,    singleSelectStr,    rangeSelectStr,    prevSelectStr,        nextSelectStr,    singleInsertStr,    firstSelectStr,    lastSelectStr  };
    sqlite3_stmt** preparedStatments[] = {&_selectNamesStmt, &_selectSingleStmt, &_selectRangeStmt, &_selectPreviousStmt, &_selectNextStmt, &_insertSingleStmt, &_selectFirstStmt, &_selectLastStmt};
    int nStmts = 8;
    
    for (int iStmt = 0; iStmt < nStmts; ++iStmt) {
      sqlite3_stmt **stmt = preparedStatments[iStmt];
      returnCode = sqlite3_prepare_v2(_dbHandle, statmentStrings[iStmt].c_str(), -1, stmt, NULL);
      if (returnCode != SQLITE_OK) {
        this->logDbError();
        return;
      }
    }
    errorMessage = "OK";
    _connected = true;
  }
  BOOST_FOREACH(const nameUnitsPair p, DB_PR_SUPER::identifiersAndUnits()) {
    this->registerAndGetIdentifierForSeriesWithUnits(p.first,p.second);
  }
}


bool SqlitePointRecord::initTables() {
  
  
  char *errmsg;
  
  int errCode;
  errCode = sqlite3_exec(_dbHandle, initTablesStr.c_str(), NULL, NULL, &errmsg);
  if (errCode != SQLITE_OK) {
    errorMessage = string(errmsg);
    return false;
  }
  
  return true;
}

int SqlitePointRecord::dbSchemaVersion() {
  int vers = -1;
  sqlite3_stmt *stmt_version;
  if(sqlite3_prepare_v2(_dbHandle, "PRAGMA user_version;", -1, &stmt_version, NULL) == SQLITE_OK) {
    while(sqlite3_step(stmt_version) == SQLITE_ROW) {
      vers = sqlite3_column_int(stmt_version, 0);
    }
  }
  return vers;
}
void SqlitePointRecord::setDbSchemaVersion(int v) {
  stringstream ss;
  ss << "PRAGMA user_version = " << v;
  sqlite3_exec(_dbHandle, ss.str().c_str(), NULL, NULL, NULL);
}


bool SqlitePointRecord::updateSchema() {
  
  int currentVersion = this->dbSchemaVersion();
  
  while (currentVersion < sqlitePointRecordCurrentDbVersion) {
    
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




void SqlitePointRecord::logDbError() {
  const char *zErrMsg = sqlite3_errmsg(_dbHandle);
  errorMessage = string(zErrMsg);
  cerr << "sqlite error: " << errorMessage << endl;
}


bool SqlitePointRecord::isConnected() {
  return _connected;
}


bool SqlitePointRecord::registerAndGetIdentifierForSeriesWithUnits(string name, Units units) {
  // mostly a copy of DbPointRecord implementation. but we allow for upgrading an "old" database with new units.
  
  bool nameExists = false;
  bool unitsMatch = false;
  Units existingUnits;
  
  vector< pair<string,Units> > existing = this->identifiersAndUnits();
  typedef pair<string,Units> sup_t;
  BOOST_FOREACH(const sup_t p, existing) {
    string n = p.first;
    if ( RTX_STRINGS_ARE_EQUAL_CS(n, name)) {
      existingUnits = p.second;
      nameExists = true;
      if (existingUnits == units) {
        unitsMatch = true;
      }
      break;
    }
  }
  
  if (this->readonly()) {
    // handle a read-only database.
    if (nameExists && (unitsMatch || !this->supportsUnitsColumn()) ) {
      // everything is awesome. name matches, units match (or we don't support it and therefore don't care).
      // make a cache and return affirmative.
      DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
      return true;
    }
    else {
      
      // SPECIAL CASE FOR OLD RECORDS: we can update the units field if no units are specified.
      if (existingUnits == RTX_NO_UNITS) {
        this->assignUnitsToRecord(name, units);
        DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units);
        return true;
      }
      
      
      // names don't match (or units prevent us from using this record) and we can't write to this db. fail.
      return false;
    }
  }
  else {
    // not a readonly db.
    if (nameExists && !unitsMatch) {
      // two possibilities: the units actually don't match, or my units haven't ever been set.
      if (existingUnits == RTX_NO_UNITS) {
        // aha. update my units then.
        this->assignUnitsToRecord(name, units);
      }
      else {
        // must remove the old record. units don't match for real.
        this->removeRecord(name);
      }
    }
    if (this->insertIdentifierAndUnits(name, units) && DB_PR_SUPER::registerAndGetIdentifierForSeriesWithUnits(name, units) ) {
      // this will either insert a new record name, or ignore because it's already there.
      return true;
    }
  }
  return false;
}

bool SqlitePointRecord::assignUnitsToRecord(const std::string &name, RTX::Units units) {
  
  int ret = 0;
  bool success = false;
  
  // has to be connected, otherwise there may not be a row for this guy in the meta table.
  // TODO -- check for errors when inserting data??
  if (!this->isConnected()) {
    this->dbConnect();
  }
  
  if (this->isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    string unitsStr = units.unitString();
    sqlite3_stmt *stmt;
    string q = "update meta set units = ? where name = \'" + name + "\'";
    sqlite3_prepare_v2(_dbHandle, q.c_str(), -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, unitsStr.c_str(), -1, NULL);
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      logDbError();
    }
    else {
      success = true;
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    
  }
  
  return success;
  
}

bool SqlitePointRecord::insertIdentifierAndUnits(const std::string &id, RTX::Units units) {
  
  int ret = 0;
  bool success = false;
  
  // has to be connected, otherwise there may not be a row for this guy in the meta table.
  // TODO -- check for errors when inserting data??
  if (!this->isConnected()) {
    this->dbConnect();
  }
  
  if (this->isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    // insert a name here.
    // INSERT IGNORE INTO meta (name,units) VALUES (?,?)
    string unitsStr = units.unitString();
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(_dbHandle, "insert or ignore into meta (name,units) values (?,?)", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 2, unitsStr.c_str(), -1, NULL);
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      logDbError();
    }
    else {
      success = true;
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    
  }
  
  return success;
}


std::vector< PointRecord::nameUnitsPair > SqlitePointRecord::identifiersAndUnits() {
  vector<nameUnitsPair> ids;
  
  if (!this->isConnected()) {
    this->dbConnect();
  }
  
  if (this->isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    int ret = sqlite3_step(_selectNamesStmt);
    while (ret == SQLITE_ROW) {
      string name = string((char*)sqlite3_column_text(_selectNamesStmt, 0));
      int type = sqlite3_column_type(_selectNamesStmt, 1);
      Units units;
      if (type == SQLITE_NULL) {
        units = RTX_NO_UNITS;
      }
      else {
        string unitsStr = string((char*)sqlite3_column_text(_selectNamesStmt, 1));
        units = Units::unitOfType(unitsStr);
      }
      ids.push_back(make_pair(name, units));
      ret = sqlite3_step(_selectNamesStmt);
    }
    sqlite3_reset(_selectNamesStmt);
  }
  
  return ids;
}


PointRecord::time_pair_t SqlitePointRecord::range(const string& id) {
  Point last,first;
  vector<Point> points;
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    // some optimizations here.
    const bool optimized(true);
    
    if (optimized) {
      time_t minTime = 0, maxTime = 0;
      int ret;
      sqlite3_stmt *selectFirstStmt;
      string selectFirst("select min(time) from points where series_id=(select series_id from meta where name=?)");
      ret = sqlite3_prepare_v2(_dbHandle, selectFirst.c_str(), -1, &selectFirstStmt, NULL);
      ret = sqlite3_bind_text(selectFirstStmt, 1, id.c_str(), -1, NULL);
      if (sqlite3_step(selectFirstStmt) == SQLITE_ROW) {
        minTime = (time_t)sqlite3_column_int(selectFirstStmt, 0);
      }
      sqlite3_reset(selectFirstStmt);
      sqlite3_finalize(selectFirstStmt);
      
      sqlite3_stmt *selectLastStmt;
      string selectLast("select max(time) from points where series_id=(select series_id from meta where name=?)");
      ret = sqlite3_prepare_v2(_dbHandle, selectLast.c_str(), -1, &selectLastStmt, NULL);
      ret = sqlite3_bind_text(selectLastStmt, 1, id.c_str(), -1, NULL);
      if (sqlite3_step(selectLastStmt) == SQLITE_ROW) {
        maxTime = (time_t)sqlite3_column_int(selectLastStmt, 0);
      }
      sqlite3_reset(selectLastStmt);
      sqlite3_finalize(selectLastStmt);
      
      return make_pair(minTime, maxTime);
    }
    
    // non-optimized code -- deprecate
    else {
      sqlite3_bind_text(_selectFirstStmt, 1, id.c_str(), -1, NULL);
      points = pointsFromPreparedStatement(_selectFirstStmt);
      if (points.size() > 0) {
        first = points.front();
      }
      
      sqlite3_bind_text(_selectLastStmt, 1, id.c_str(), -1, NULL);
      points = pointsFromPreparedStatement(_selectLastStmt);
      if (points.size() > 0) {
        last = points.front();
      }
    }
    /*
     
     */
    
  }
  
  
  //  return make_pair(first.time, last.time);
  return make_pair(0, 0);
}

std::vector<Point> SqlitePointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  vector<Point> points;
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    // SELECT time, value, quality, confidence FROM points INNER JOIN meta USING (series_id) WHERE name = ? AND time >= ? AND time <= ? order by time asc
    
    //    checkTransactions(true);
    
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    sqlite3_bind_text(_selectRangeStmt, 1, id.c_str(), -1, NULL);
    sqlite3_bind_int(_selectRangeStmt, 2, (int)startTime);
    sqlite3_bind_int(_selectRangeStmt, 3, (int)endTime);
    
    return pointsFromPreparedStatement(_selectRangeStmt);
  }
  return points;
}

Point SqlitePointRecord::selectNext(const std::string& id, time_t time) {
  Point p;
  
  const time_t timeMargin = 60*60*24;
  const int maxLookahead = 100; // 100 days
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    
    vector<Point> points;
    
    // iterative lookahead, faster than unbounded query
    time_t searchStartTime = time + 1;
    int lookaheadsLeft = maxLookahead;
    while (points.size() == 0 && lookaheadsLeft > 0) {
      points = this->selectRange(id, searchStartTime, searchStartTime + timeMargin);
      searchStartTime += timeMargin;
      --lookaheadsLeft;
    }
    
    // if iterative search fails, there still may be points here.
    // suffer then long execution of the unbounded query.
    if (points.size() == 0) {
      // slow query
      sqlite3_bind_text(_selectNextStmt, 1, id.c_str(), -1, NULL);
      sqlite3_bind_int(_selectNextStmt, 2, (int)time);
      points = pointsFromPreparedStatement(_selectNextStmt);
    }
    
    
    if (points.size() > 0) {
      return points.front();
    }
    return Point();
  }
  return p;
  
  
}

Point SqlitePointRecord::selectPrevious(const std::string& id, time_t time) {
  
  Point p;
  
  const time_t timeMargin = 60*60*24;
  const int maxLookBehinds = 100; // 100 days
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    
    vector<Point> points;
    
    
    // iterative lookbehind is faster than unbounded lookup
    time_t searchEndTime = time - 1;
    int lookbehandsLeft = maxLookBehinds;
    while (points.size() == 0 && lookbehandsLeft > 0) {
      points = this->selectRange(id, searchEndTime - timeMargin, searchEndTime);
      searchEndTime -= timeMargin;
      --lookbehandsLeft;
    }
    
    // if the iterative lookbehind did not work
    if (points.size() == 0) {
      // slow query
      sqlite3_bind_text(_selectPreviousStmt, 1, id.c_str(), -1, NULL);
      sqlite3_bind_int(_selectPreviousStmt, 2, (int)time);
      points = pointsFromPreparedStatement(_selectPreviousStmt);
    }
    
    if (points.size() > 0) {
      return points.back();
    }
    return Point();
  }
  return p;
  
  
}


void SqlitePointRecord::beginBulkOperation() {
  if (_inBulkOperation) {
    return;
  }
  this->checkTransactions(false);
  _inBulkOperation = true;
}
void SqlitePointRecord::endBulkOperation() {
  if (!_inBulkOperation) {
    return;
  }
  this->checkTransactions(true);
  _inBulkOperation = false;
}

void SqlitePointRecord::insertSingle(const std::string &id, RTX::Point point) {
  
  if (_inBulkOperation) { // caller has promised to end the bulk operation eventually.
    insertSingleInTransaction(id, point);
    this->checkTransactions(false); // only flush if we've reached the stride size.
  }
  
  else {
    this->checkTransactions(false);
    this->insertSingleInTransaction(id, point);
    this->checkTransactions(true);
  }
  
}

// insertions or alterations may choose to ignore / deny
void SqlitePointRecord::insertSingleInTransaction(const std::string& id, Point point) {
  
  if (!isConnected()) {
    dbConnect();
  }
  if (isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    int ret;
    
    // INSERT INTO points (time, series_id, value, quality, confidence) SELECT ?,series_id,?,?,? FROM meta WHERE name = ?
    
    ret = sqlite3_bind_int(    _insertSingleStmt, 1, (int)point.time      );
    ret = sqlite3_bind_double( _insertSingleStmt, 2, point.value          );
    ret = sqlite3_bind_int(    _insertSingleStmt, 3, point.quality        );
    ret = sqlite3_bind_double( _insertSingleStmt, 4, point.confidence     );
    ret = sqlite3_bind_text(   _insertSingleStmt, 5, id.c_str(), -1, NULL );
    
    ret = sqlite3_step(_insertSingleStmt);
    if (ret != SQLITE_DONE) {
      logDbError();
    }
    sqlite3_reset(_insertSingleStmt);
  }
  
  return;
}

void SqlitePointRecord::insertRange(const std::string& id, std::vector<Point> points) {
  
  if (!isConnected()) {
    dbConnect();
  }
  if (isConnected()) {
    
    int ret;
    char *errmsg;
    
    {
      checkTransactions(true); // any tranactions currently? tell them to quit.
      scoped_lock<boost::signals2::mutex> lock(*_mutex);
      ret = sqlite3_exec(_dbHandle, "begin exclusive transaction", NULL, NULL, &errmsg);
      if (ret != SQLITE_OK) {
        logDbError();
        return;
      }
    }
    
    // this is always within a transaction
    BOOST_FOREACH(const Point &p, points) {
      this->insertSingleInTransaction(id, p);
    }
    {
      scoped_lock<boost::signals2::mutex> lock(*_mutex);
      ret = sqlite3_exec(_dbHandle, "end transaction", NULL, NULL, &errmsg);
      if (ret != SQLITE_OK) {
        logDbError();
        return;
      }
    }
    
    
  }
  
}

void SqlitePointRecord::removeRecord(const std::string& id) {
  scoped_lock<boost::signals2::mutex> lock(*_mutex);
  
  if (!_connected) {
    return;
  }
  char *errmsg;
  string sqlStr = "delete from points where series_id = (SELECT series_id FROM meta where name = \'" + id + "\'); delete from meta where name = \'" + id + "\'";
  const char *sql = sqlStr.c_str();
  int ret = sqlite3_exec(_dbHandle, sql, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    logDbError();
    return;
  }
}

void SqlitePointRecord::truncate() {
  if (!_connected) {
    return;
  }
  char *errmsg;
  int ret = sqlite3_exec(_dbHandle, "delete from points", NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    logDbError();
    return;
  }
}


std::vector<Point> SqlitePointRecord::pointsFromPreparedStatement(sqlite3_stmt *stmt) {
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

Point SqlitePointRecord::pointFromStatment(sqlite3_stmt *stmt) {
  // SELECT time, value, quality, confidence
  time_t time = sqlite3_column_int(stmt, 0);
  double value = sqlite3_column_double(stmt, 1);
  int q = sqlite3_column_int(stmt, 2);
  double c = sqlite3_column_double(stmt, 3);
  Point::PointQuality qual = Point::PointQuality( q );
  
  return Point(time, value, qual, c);
}


bool SqlitePointRecord::supportsBoundedQueries() {
  return true;
}


void SqlitePointRecord::checkTransactions(bool forceEndTranaction) {
  scoped_lock<boost::signals2::mutex> lock(*_mutex);
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

