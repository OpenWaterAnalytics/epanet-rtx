#include "SqlitePointRecord.h"
#include <boost/foreach.hpp>

using namespace RTX;
using namespace std;

#include <boost/interprocess/sync/scoped_lock.hpp>

using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

typedef const unsigned char* sqltext;

/******************************************************************************************/
static string initTablesStr = "CREATE TABLE 'meta' ('series_id' INTEGER PRIMARY KEY ASC AUTOINCREMENT, 'name' TEXT UNIQUE ON CONFLICT ABORT, 'units' TEXT, 'regular_period' INTEGER, 'regular_offset' INTEGER); CREATE TABLE 'points' ('time' INTEGER, 'series_id' INTEGER REFERENCES 'meta'('series_id'), 'value' REAL, 'confidence' REAL, 'quality' INTEGER, UNIQUE (series_id, time asc) ON CONFLICT IGNORE)";
static string selectPreamble = "SELECT time, value, quality, confidence FROM points INNER JOIN meta USING (series_id) WHERE name = ? AND ";
static string singleSelectStr = selectPreamble + "time = ? order by time asc";
static string rangeSelectStr = selectPreamble + "time >= ? AND time <= ? order by time asc";
static string nextSelectStr = selectPreamble + "time > ? order by time asc LIMIT 1";
static string prevSelectStr = selectPreamble + "time < ? order by time desc LIMIT 1";
static string singleInsertStr = "INSERT INTO points (time, series_id, value, quality, confidence) SELECT ?,series_id,?,?,? FROM meta WHERE name = ?";
static string firstSelectStr = selectPreamble + "1 order by time asc limit 1";
static string lastSelectStr = selectPreamble + "1 order by time desc limit 1";
static string selectNamesStr = "select name from meta order by name asc";
/******************************************************************************************/



SqlitePointRecord::SqlitePointRecord() {
  _path = "";
  _connected = false;
  
  _inTransaction = false;
  _transactionStackCount = 0;
  _maxTransactionStackCount = 500;
  
  _mutex.reset(new boost::signals2::mutex );
}

SqlitePointRecord::~SqlitePointRecord() {
  this->setPath("");
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
  
  
  
  // prepare the select & insert statments
  
  string statmentStrings[] =          {selectNamesStr,   singleSelectStr,   rangeSelectStr,   prevSelectStr,       nextSelectStr,   singleInsertStr,   firstSelectStr,   lastSelectStr  };
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
  BOOST_FOREACH(const string &name, DB_PR_SUPER::identifiers()) {
    this->registerAndGetIdentifier(name, Units());
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

void SqlitePointRecord::logDbError() {
  const char *zErrMsg = sqlite3_errmsg(_dbHandle);
  errorMessage = string(zErrMsg);
  cerr << "sqlite error: " << errorMessage << endl;
}


bool SqlitePointRecord::isConnected() {
  return _connected;
}

std::string SqlitePointRecord::registerAndGetIdentifier(std::string recordName, Units dataUnits) {
  
  int ret = 0;
  
  // has to be connected, otherwise there may not be a row for this guy in the meta table.
  // TODO -- check for errors when inserting data??
  if (!this->isConnected()) {
    this->dbConnect();
  }
  
  if (this->isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
    // insert a name here.
    // INSERT IGNORE INTO meta (name,units) VALUES (?,?)
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(_dbHandle, "insert or ignore into meta (name,units) values (?,?)", -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, recordName.c_str(), -1, NULL);
    sqlite3_bind_text(stmt, 2, dataUnits.unitString().c_str(), -1, NULL);
    
    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
      logDbError();
    }
    sqlite3_reset(stmt);
  }
  
  
  DB_PR_SUPER::registerAndGetIdentifier(recordName, dataUnits);
  
  return recordName;
}

std::vector<std::string> SqlitePointRecord::identifiers() {
  vector<string> names;
  
  if (this->isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    int ret = sqlite3_step(_selectNamesStmt);
    while (ret == SQLITE_ROW) {
      sqltext row = sqlite3_column_text(_selectNamesStmt, 0);
      names.push_back(string((char*)row));
      ret = sqlite3_step(_selectNamesStmt);
    }
    sqlite3_reset(_selectNamesStmt);
  }
  
  
  
  return names;
}

std::vector<std::pair<std::string, Units> > SqlitePointRecord::availableData() {
  // todo
}


PointRecord::time_pair_t SqlitePointRecord::range(const string& id) {
  Point last,first;
  vector<Point> points;
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
//    checkTransactions(true);
    
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
  
  
  return make_pair(first.time, last.time);
}

std::vector<Point> SqlitePointRecord::selectRange(const std::string& id, time_t startTime, time_t endTime) {
  vector<Point> points;
  
  if (!isConnected()) {
    this->dbConnect();
  }
  if (isConnected()) {
    // SELECT time, value, quality, confidence FROM points INNER JOIN meta USING (series_id) WHERE name = ? AND time >= ? AND time <= ? order by time asc
    scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
//    checkTransactions(true);
    
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
    //scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
//    checkTransactions(true);
    /*
    sqlite3_bind_text(_selectNextStmt, 1, id.c_str(), -1, NULL);
    sqlite3_bind_int(_selectNextStmt, 2, (int)time);
    
    vector<Point> points = pointsFromPreparedStatement(_selectNextStmt);
    */
    
    vector<Point> points;
    time_t searchStartTime = time;
    int lookaheadsLeft = maxLookahead;
    
    while (points.size() == 0 && lookaheadsLeft > 0) {
      points = this->selectRange(id, searchStartTime, searchStartTime + timeMargin);
      searchStartTime += timeMargin;
      --lookaheadsLeft;
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
    //scoped_lock<boost::signals2::mutex> lock(*_mutex);
    
//    checkTransactions(true);
    /*
    sqlite3_bind_text(_selectPreviousStmt, 1, id.c_str(), -1, NULL);
    sqlite3_bind_int(_selectPreviousStmt, 2, (int)time);
    
    vector<Point> points = pointsFromPreparedStatement(_selectPreviousStmt);
    */
    
    
    vector<Point> points;
    time_t searchEndTime = time;
    int lookbehandsLeft = maxLookBehinds;
    
    while (points.size() == 0 && lookbehandsLeft > 0) {
      points = this->selectRange(id, searchEndTime - timeMargin, searchEndTime);
      searchEndTime -= timeMargin;
      --lookbehandsLeft;
    }
    
    
    if (points.size() > 0) {
      return points.front();
    }
    return Point();
  }
  return p;
  
  
}


void SqlitePointRecord::insertSingle(const std::string &id, RTX::Point point) {
  
  this->checkTransactions(false);
  this->insertSingleInTransaction(id, point);
  
  
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
      scoped_lock<boost::signals2::mutex> lock(*_mutex);
      checkTransactions(true); // any tranactions currently? tell them to quit.
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
    
    ret = sqlite3_exec(_dbHandle, "end transaction", NULL, NULL, &errmsg);
    if (ret != SQLITE_OK) {
      logDbError();
      return;
    }
    // end scoped lock
  }
  
}

void SqlitePointRecord::removeRecord(const std::string& id) {
  
  if (!_connected) {
    return;
  }
  char *errmsg;
  string sqlStr = "delete from points where series_id = (SELECT series_id FROM meta where name = \'" + id + "\')";
  const char *sql = sqlStr.c_str();
  int ret = sqlite3_exec(_dbHandle, sql, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    logDbError();
    return;
  }
  
  // INSERT INTO points (time, series_id, value, quality, confidence) SELECT ?,series_id,?,?,? FROM meta WHERE name = ?
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
  Point::Qual_t qual = Point::Qual_t( sqlite3_column_int(stmt, 2) );
  
  return Point(time, value, qual);
}


bool SqlitePointRecord::supportsBoundedQueries() {
  return true;
}


void SqlitePointRecord::checkTransactions(bool forceEndTranaction) {
  
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


