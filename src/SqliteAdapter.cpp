#include "SqliteAdapter.h"

#include <set>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "WhereClause.h"

using namespace std;
using namespace RTX;

static int sqlitePointRecordCurrentDbVersion = 2;
typedef const unsigned char* sqltext;

/******************************************************************************************/
const string initTablesStr = "CREATE TABLE 'meta' ('series_id' INTEGER PRIMARY KEY ASC AUTOINCREMENT, 'name' TEXT UNIQUE ON CONFLICT ABORT, 'units' TEXT, 'regular_period' INTEGER, 'regular_offset' INTEGER); CREATE TABLE 'points' ('time' INTEGER, 'series_id' INTEGER REFERENCES 'meta'('series_id'), 'value' REAL, 'confidence' REAL, 'quality' INTEGER, UNIQUE (series_id, time asc) ON CONFLICT IGNORE); PRAGMA user_version = 2";
/******************************************************************************************/

#define _dbq (*(_db.get()))

const string _selectSingleStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? AND time = ? order by time asc";
const string _selectRangeStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? AND time >= ? AND time <= ? order by time asc";
const string _selectNextStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? AND time > ? order by time asc LIMIT 1";
const string _selectPreviousStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? AND time < ? order by time desc LIMIT 1";
const string _insertSingleStr = "INSERT INTO points(time,series_id,value,quality,confidence) VALUES (?,?,?,?,?)";
const string _selectFirstStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? order by time asc limit 1";
const string _selectLastStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? order by time desc limit 1";
const string _selectNamesStr = "select series_id,name,units from meta order by name asc";

const string _selectNextWhereValueStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? AND time > ? [#] order by time asc LIMIT 1";
const string _selectPreviousWhereValueStr = "SELECT time,value,quality,confidence FROM points INNER JOIN meta USING(series_id) WHERE name = ? AND time < ? [#] order by time desc LIMIT 1";

const string _makeSelectStr(const std::string& tpl, WhereClause q);
const string _makeSelectStr(const std::string& tpl, WhereClause q) {
  vector<string> clauses;
  for (auto const& i : q.clauses) {
    switch (i.first) {
      case WhereClause::gte:
        clauses.push_back(">= " + std::to_string(i.second));
        break;
      case WhereClause::gt:
        clauses.push_back("> " + std::to_string(i.second));
        break;
      case WhereClause::lte:
        clauses.push_back("<= " + std::to_string(i.second));
        break;
      case WhereClause::lt:
        clauses.push_back("< " + std::to_string(i.second));
        break;
      default:
        break;
    }
  }
  string whereClause = "AND value " + boost::algorithm::join(clauses, " AND value ");
  string stmt = tpl;
  boost::replace_all(stmt, "[#]", whereClause);
  return stmt;
}

SqliteAdapter::SqliteAdapter( errCallback_t cb ) : DbAdapter(cb) {
  _path = "";
  basePath = "";
  _inTransaction = false;
  _transactionStackCount = 0;
  _maxTransactionStackCount = 50000;
  _connected = false;
}
SqliteAdapter::~SqliteAdapter() {
  
}

const DbAdapter::adapterOptions SqliteAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.canAssignUnits = true;
  o.supportsUnitsColumn = true;
  o.searchIteratively = false;
  o.supportsSinglyBoundQuery = true;
  o.implementationReadonly = false;
  o.canDoWideQuery = false;
  
  return o;
}

std::string SqliteAdapter::connectionString() {
  return _path;
}
void SqliteAdapter::setConnectionString(const std::string& con) {
  _path = con;
}

void SqliteAdapter::doConnect() {
  _RTX_DB_SCOPED_LOCK;

  if (RTX_STRINGS_ARE_EQUAL(_path, "")) {
    _errCallback("No File Specified");
    return;
  }
  
  boost::filesystem::path dbPath(this->basePath);
  dbPath /= _path;
  auto realPath = dbPath.make_preferred().string();
  
  sqlite3* _rawDb;
  int returnCode;
  returnCode = sqlite3_open_v2(realPath.c_str(), &_rawDb, SQLITE_OPEN_READWRITE, NULL); // only if exists
  if (returnCode == SQLITE_CANTOPEN) {
    // attempt to create the db.
    returnCode = sqlite3_open_v2(realPath.c_str(), &_rawDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (returnCode == SQLITE_OK) {
      shared_ptr<sqlite3> dbHandle = shared_ptr<sqlite3>(_rawDb, [=](sqlite3* ptr) { sqlite3_close_v2(ptr); });
      _db.reset(new sqlite::database(dbHandle));
      if (!this->initTables()) {
        throw runtime_error("could not initialize tables in db. check permissions.");
      }
    }
    else {
      string err = "could not open the db file: " + realPath;
      throw runtime_error(err);
    }
  }
  else if (returnCode == SQLITE_OK) {
    shared_ptr<sqlite3> dbHandle = shared_ptr<sqlite3>(_rawDb, [=](sqlite3* ptr) { sqlite3_close_v2(ptr); });
    _db.reset(new sqlite::database(dbHandle));
  }
  if( returnCode != SQLITE_OK ){
    sqlite3_close(_rawDb);
    throw runtime_error("could not open or create database");
  }
    

  // check schema
  bool updateSuccess = true;
  int databaseVersion = this->dbSchemaVersion();
  if (databaseVersion < 0) {
    // db was opened, but we could not get the user_version from the db's internal data.
    // that sucks, so it's gotta be corrupt.
    _errCallback("Corrupt Database");
    _connected = false;
    return;
  }
  if (databaseVersion < sqlitePointRecordCurrentDbVersion) {
    cerr << "Point Record Database Schema version not compatible. Require version " << sqlitePointRecordCurrentDbVersion << " or greater. Updating." << endl;
    updateSuccess = this->updateSchema();
  }
  
  _errCallback("OK");
  _connected = true;
}


IdentifierUnitsList SqliteAdapter::idUnitsList() {
  _RTX_DB_SCOPED_LOCK;
  
  if (_inTransaction) {
    return _idCache;
  }
  
  _metaCache.clear();
  _idCache.clear();
    
  _dbq << _selectNamesStr
  >> [&](int uid, string name, std::unique_ptr<string> unitStr) {
    Units units(RTX_NO_UNITS);
    if (unitStr != nullptr) {
      units = Units::unitOfType(*unitStr);
    }
    _idCache.set(name,units);
    _metaCache[name] = uid;
  };
    
  return _idCache;
}

// TRANSACTIONS
void SqliteAdapter::beginTransaction() {
  if (!_inTransaction) {
    _RTX_DB_SCOPED_LOCK;
    _dbq << "begin;";
    _inTransaction = true;
  }
}
void SqliteAdapter::endTransaction() {
  if (_inTransaction) {
    this->commit();
    //_inTransaction = false;
  }
}

// READ
std::vector<Point> SqliteAdapter::selectRange(const std::string& id, TimeRange range) {
  vector<Point> points;
  
  _RTX_DB_SCOPED_LOCK;
  _dbq << _selectRangeStr << id << (int)range.start << (int)range.end
  >> [&](int t, double v, int q, double c) {
    points.push_back(Point((time_t)t, v, Point::PointQuality( q ), c));
  };
  
  return points;
}

Point SqliteAdapter::selectNext(const std::string& id, time_t time, WhereClause q) {
  vector<Point> points;
  _RTX_DB_SCOPED_LOCK;
  
  auto selectStr = (q.clauses.empty()) ? _selectNextStr : _makeSelectStr(_selectNextWhereValueStr, q);
  
  _dbq << selectStr << id << (int)time
  >> [&](int t, double v, int q, double c) {
    points.push_back(Point((time_t)t, v, Point::PointQuality( q ), c));
  };
  
  if (points.size() > 0) {
    return points.front();
  }
  return Point();
}

Point SqliteAdapter::selectPrevious(const std::string& id, time_t time, WhereClause q) {
  
  vector<Point> points;
  _RTX_DB_SCOPED_LOCK;
  
  auto selectStr = (q.clauses.empty()) ? _selectPreviousStr : _makeSelectStr(_selectPreviousWhereValueStr, q);
  
  _dbq << selectStr << id << (int)time
  >> [&](int t, double v, int q, double c) {
    points.push_back(Point((time_t)t, v, Point::PointQuality( q ), c));
  };
  
  if (points.size() > 0) {
    return points.front();
  }
  return Point();
}

// CREATE
bool SqliteAdapter::insertIdentifierAndUnits(const std::string& id, Units units) {
  bool success = false;
  {
    _RTX_DB_SCOPED_LOCK;
    try {
      _dbq << "insert or ignore into meta (name,units) values (?,?)"
      << id << units.to_string();
    
      int uid = (int)_db->last_insert_rowid();    
      // add to the cache.
      _metaCache[id] = uid;
      _idCache.set(id, units);
      success = true;
    }
    catch (exception& e) {
      cerr << "could not create series" << endl;
    }
  }
  this->checkTransactions(); // allow flushing if we've exceeded max transaction stack size
  
  return success;
}

void SqliteAdapter::insertSingle(const std::string& id, Point point) {
  if (_inTransaction) { // caller has promised to end the bulk operation eventually.
    insertSingleInTransaction(id, point);
    this->checkTransactions();
  }
  
  else {
    this->beginTransaction();
    this->insertSingleInTransaction(id, point);
    this->endTransaction();
  }

}

void SqliteAdapter::insertSingleInTransaction(const std::string& id, Point point) {
  
  _RTX_DB_SCOPED_LOCK; 
  int tsUid = _metaCache[id];
  _dbq << _insertSingleStr << (int)point.time << tsUid << point.value << (int)point.quality << point.confidence;
  
}


void SqliteAdapter::insertRange(const std::string& id, std::vector<Point> points) {
  
  this->commit(); // commit any transactions in progress
  
  this->beginTransaction();
  for(const Point &p : points) {
    this->insertSingleInTransaction(id, p);
  }
  this->endTransaction();
}

// UPDATE
bool SqliteAdapter::assignUnitsToRecord(const std::string& name, const Units& units) {
  
  _RTX_DB_SCOPED_LOCK;
  string q = "update meta set units = ? where name = \'" + name + "\'";
  _dbq << q << units.to_string();
  
  return true;
}

// DELETE
void SqliteAdapter::removeRecord(const std::string& id) {
  _RTX_DB_SCOPED_LOCK;
  _dbq << "delete from points where series_id = (SELECT series_id FROM meta where name = \'" + id + "\');";
  _dbq << "delete from meta where name = \'" + id + "\'";
}
void SqliteAdapter::removeAllRecords() {
  _RTX_DB_SCOPED_LOCK;
  _dbq << "delete from points";
}



#pragma mark private

bool SqliteAdapter::initTables() {
  auto err = sqlite3_exec(_db->connection().get(), initTablesStr.c_str(), nullptr, nullptr, nullptr);
  return (err == SQLITE_OK);
}

int SqliteAdapter::dbSchemaVersion() {
  int vers = -1;
  _dbq << "PRAGMA user_version;" >> vers;
  return vers;
}
void SqliteAdapter::setDbSchemaVersion(int v) {
  stringstream ss;
  ss << "PRAGMA user_version = " << v << ";";
  auto q = ss.str();
  _dbq << q;
}


bool SqliteAdapter::updateSchema() {
  
  int currentVersion = this->dbSchemaVersion();
  
  while (currentVersion >= 0 && currentVersion < sqlitePointRecordCurrentDbVersion) {
    
    switch (currentVersion) {
      case 0:
      case 1:
      {
        // migrate 0,1->2
        _dbq << "UPDATE points SET quality = 128";
        this->setDbSchemaVersion(2);
      }
        break;
        
      default:
        break;
    }
    currentVersion = this->dbSchemaVersion();
  }
  
  if (currentVersion == sqlitePointRecordCurrentDbVersion) {
    return true;
  }
  else {
    return false;
  }
}



void SqliteAdapter::checkTransactions() {
  if (_inTransaction) {
    if (_transactionStackCount >= _maxTransactionStackCount) {
      // reset the stack, commit the transaction
      this->endTransaction();
      this->beginTransaction();
    }
    else {
      // increment the stack count.
      _RTX_DB_SCOPED_LOCK;
      ++_transactionStackCount;
    }
  }
}


void SqliteAdapter::commit() {
  _RTX_DB_SCOPED_LOCK;
  if (_inTransaction) {
    _dbq << "end;";
    _transactionStackCount = 0;
    _inTransaction = false;
  }
}


