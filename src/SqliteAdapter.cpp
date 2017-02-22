#include "SqliteAdapter.h"

#include <set>
#include <sstream>

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



SqliteAdapter::SqliteAdapter( errCallback_t cb ) : DbAdapter(cb) {
  _path = "";
  _inTransaction = false;
  _transactionStackCount = 0;
  _maxTransactionStackCount = 50000;
}
SqliteAdapter::~SqliteAdapter() {
  
}

const DbAdapter::adapterOptions SqliteAdapter::options() const {
  DbAdapter::adapterOptions o;
  
  o.canAssignUnits = true;
  o.supportsUnitsColumn = true;
  o.searchIteratively = true;
  o.supportsSinglyBoundQuery = true;
  o.implementationReadonly = false;
  
  return o;
}

std::string SqliteAdapter::connectionString() {
  return _path;
}
void SqliteAdapter::setConnectionString(const std::string& con) {
  if (this->adapterConnected()) {
    
  }
  _path = con;
}

void SqliteAdapter::doConnect() {
  _RTX_DB_SCOPED_LOCK;
  
  if (RTX_STRINGS_ARE_EQUAL(_path, "")) {
    _errCallback("No File Specified");
    return;
  }
  
  
  sqlite3* _rawDb;
  int returnCode;
  returnCode = sqlite3_open_v2(_path.c_str(), &_rawDb, SQLITE_OPEN_READWRITE, NULL); // only if exists
  if (returnCode == SQLITE_CANTOPEN) {
    // attempt to create the db.
    returnCode = sqlite3_open_v2(_path.c_str(), &_rawDb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (returnCode == SQLITE_OK) {
      shared_ptr<sqlite3> dbHandle = shared_ptr<sqlite3>(_rawDb, [=](sqlite3* ptr) { sqlite3_close_v2(ptr); });
      _db.reset(new sqlite::database(dbHandle));
      if (!this->initTables()) {
        throw runtime_error("could not initialize tables in db. check permissions.");
      }
    }
    else {
      throw runtime_error("could not open the db file.");
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
  IdentifierUnitsList ids;
  _metaCache.clear();
    
  _dbq << _selectNamesStr
  >> [&](int uid, string name, std::unique_ptr<string> unitStr) {
    Units units(RTX_NO_UNITS);
    if (unitStr != nullptr) {
      units = Units::unitOfType(*unitStr);
    }
    ids.set(name,units);
    _metaCache[name] = uid;
  };
    
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
    _RTX_DB_SCOPED_LOCK;
    _dbq << _selectRangeStr << id << (int)range.start << (int)range.end
    >> [&](int t, double v, int q, double c) {
      points.push_back(Point((time_t)t, v, Point::PointQuality( q ), c));
    };
  }
  return points;
}
Point SqliteAdapter::selectNext(const std::string& id, time_t time) {
  vector<Point> points;
  _RTX_DB_SCOPED_LOCK;
  
  _dbq << _selectNextStr << id << (int)time
  >> [&](int t, double v, int q, double c) {
    points.push_back(Point((time_t)t, v, Point::PointQuality( q ), c));
  };
  
  if (points.size() > 0) {
    return points.front();
  }
  return Point();
}
Point SqliteAdapter::selectPrevious(const std::string& id, time_t time) {
  
  vector<Point> points;
  _RTX_DB_SCOPED_LOCK;
  
  _dbq << _selectPreviousStr << id << (int)time
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
  
  // has to be connected, otherwise there may not be a row for this guy in the meta table.
  // TODO -- check for errors when inserting data??
  if (!this->adapterConnected()) {
    this->doConnect();
  }
  
  if (this->adapterConnected()) {
    _RTX_DB_SCOPED_LOCK;
    
    _dbq << "insert or ignore into meta (name,units) values (?,?)"
    << id << units.to_string();
    
    int uid = (int)_db->last_insert_rowid();    
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
    int tsUid = _metaCache[id];
    _dbq << _insertSingleStr << (int)point.time << tsUid << point.value << (int)point.quality << point.confidence;
  }
}


void SqliteAdapter::insertRange(const std::string& id, std::vector<Point> points) {
  if (!adapterConnected()) {
    doConnect();
  }
  if (adapterConnected()) {
    
    {
      checkTransactions(true); // any tranactions currently? tell them to quit.
      _RTX_DB_SCOPED_LOCK;
      _dbq << "begin exclusive transaction";
    }
    
    // this is always within a transaction
    for(const Point &p : points) {
      this->insertSingleInTransaction(id, p);
    }
    {
      _RTX_DB_SCOPED_LOCK;
      _dbq << "end transaction";
    }
  }

}

// UPDATE
bool SqliteAdapter::assignUnitsToRecord(const std::string& name, const Units& units) {
  
  // has to be connected, otherwise there may not be a row for this guy in the meta table.
  // TODO -- check for errors when inserting data??
  if (!this->adapterConnected()) {
    this->doConnect();
  }
  
  if (this->adapterConnected()) {
    _RTX_DB_SCOPED_LOCK;
    string q = "update meta set units = ? where name = \'" + name + "\'";
    _dbq << q << units.to_string();
  }
  
  return true;
}

// DELETE
void SqliteAdapter::removeRecord(const std::string& id) {
  _RTX_DB_SCOPED_LOCK;
  
  if (!_connected) {
    return;
  }
  string sqlStr = "delete from points where series_id = (SELECT series_id FROM meta where name = \'" + id + "\'); delete from meta where name = \'" + id + "\'";
  _dbq << sqlStr;
}
void SqliteAdapter::removeAllRecords() {
  if (!_connected) {
    return;
  }
  _RTX_DB_SCOPED_LOCK;
  _dbq << "delete from points";
}



#pragma mark private

bool SqliteAdapter::initTables() {
  
  _dbq << initTablesStr;
  
  return true;
}

int SqliteAdapter::dbSchemaVersion() {
  int vers = -1;
  _dbq << "PRAGMA user_version;" >> vers;
  return vers;
}
void SqliteAdapter::setDbSchemaVersion(int v) {
  stringstream ss;
  ss << "PRAGMA user_version = " << v;
  _dbq << ss.str();
}


bool SqliteAdapter::updateSchema() {
  
  int currentVersion = this->dbSchemaVersion();
  
  while (currentVersion >= 0 && currentVersion < sqlitePointRecordCurrentDbVersion) {
    
    switch (currentVersion) {
      case 0:
      case 1:
      {
        // migrate 0,1->2
        _dbq << 
        "BEGIN TRANSACTION; "
        "UPDATE points SET quality = 128; "
        "END TRANSACTION; ";
        
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



void SqliteAdapter::checkTransactions(bool forceEndTranaction) {
  _RTX_DB_SCOPED_LOCK;
  // forcing to end?
  if (forceEndTranaction) {
    if (_inTransaction) {
      _transactionStackCount = 0;
      _dbq << "END";
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
      _dbq << "END";
      _inTransaction = false;
    }
    else {
      // increment the stack count.
      ++_transactionStackCount;
    }
  }
  else {
    _dbq << "BEGIN";
    _inTransaction = true;
  }
  
}




