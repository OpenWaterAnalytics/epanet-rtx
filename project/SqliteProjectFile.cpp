//
//  SqliteProjectFile.cpp
//  epanet-rtx
//
//  Created by Sam Hatchett on 9/24/13.
//
//

#include "SqliteProjectFile.h"

#include "OdbcPointRecord.h"
#include "MysqlPointRecord.h"

#include "OffsetTimeSeries.h"
#include "ConstantTimeSeries.h"
#include "ValidRangeTimeSeries.h"
#include "Resampler.h"

typedef const unsigned char* sqltext;

using namespace RTX;
using namespace std;


/////////////////////////////////////////////////////////////
//---------------------------------------------------------//
static string sqlSelectRecords =    "select id,name,type from point_records";
static string sqlSelectTimeseries = "select id,type time_series";
static string sqlGetTsById =        "select name,units,record,clock from time_series where id=?";
static string sqlGetTsModularById = "select * from time_series_modular left join time_series_extended using (id) where id=?";
static string sqlGetTsExtendedById = "select type,value_1,value_2,value_3 from time_series left join time_series_extended using (id) where id=?";
//---------------------------------------------------------//
static string dbTimseriesName =  "TimeSeries";
static string dbResamplerName =  "Resampler";
static string dbConstantName =   "Constant";
static string dbOffsetName =     "Offset";
static string dbValidrangeName = "ValidRange";
static string dbThresholdName =  "Threshold";
static string dbMultiplierName = "Multiplier";
static string dbGainName =       "Gain";
static string dbCurveName =      "Curve";
static string dbMovingaverageName = "MovingAverage";
static string dbDerivativeName = "Derivative";
static string dbAggregatorName = "Aggregator";
//---------------------------------------------------------//
static string dbOdbcRecordName =  "odbc";
static string dbMysqlRecordName = "mysql";
//---------------------------------------------------------//
/////////////////////////////////////////////////////////////

static map<int,int> mapping() {
  map<int,int> m;
  m[1] = 1;
  return m;
}







void SqliteProjectFile::loadProjectFile(const string& path) {
  _path = path;
  char *zErrMsg = 0;
  int returnCode;
  returnCode = sqlite3_open(_path.c_str(), &_dbHandle);
  if( returnCode ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_dbHandle));
    sqlite3_close(_dbHandle);
    return;
  }
  
  
  // do stuff
  loadRecordsFromDb(_dbHandle);
  loadTimeseriesFromDb(_dbHandle);

  
  //rc = sqlite3_exec(db, argv[2], callback, 0, &zErrMsg);
  if( returnCode != SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  
  // close db
  sqlite3_close(_dbHandle);
}

void SqliteProjectFile::saveProjectFile(const string& path) {
  
}

void SqliteProjectFile::clear() {
  
}


PointRecord::sharedPointer SqliteProjectFile::defaultRecord() {
  
}

Model::sharedPointer SqliteProjectFile::model() {
  
}





#pragma mark - private methods

void SqliteProjectFile::loadRecordsFromDb(sqlite3 *db) {
  
  // set up the function pointers for creating the new records.
  typedef PointRecord::sharedPointer (*PointRecordFp)(sqlite3_stmt*);
  map<string,PointRecordFp> prCreators;
  prCreators[dbOdbcRecordName] = SqliteProjectFile::createOdbcRecordFromRow;
  prCreators[dbMysqlRecordName] = SqliteProjectFile::createMysqlRecordFromRow;
  
  sqlite3_stmt *stmt;
  
  int retCode = sqlite3_prepare_v2(db, sqlSelectRecords.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlSelectRecords << " -- error: " << sqlite3_errmsg(db) << endl;
    return;
  }
  
  retCode = sqlite3_step(stmt);
  while (retCode == SQLITE_ROW) {
    // we have a row ready
    int uid = sqlite3_column_int(stmt, 0);
    sqltext name = sqlite3_column_text(stmt, 1);
    sqltext type = sqlite3_column_text(stmt, 2);
    
    cout << "record: " << uid << " " << name << " " << type << endl;
    
    // find the right function to create this type of record
    PointRecordFp creator = prCreators[(char*)type];
    PointRecord::sharedPointer record = creator(stmt);
    
    if (record) {
      _records[uid] = record;
    }
    else {
      cerr << "could not create point record. check type?" << endl;
    }
    
    // get the next row, or end
    retCode = sqlite3_step(stmt);
  }
  
  if (retCode != SQLITE_DONE) {
    cerr << "prepared statement failed" << endl;
    
  }
}


void SqliteProjectFile::loadTimeseriesFromDb(sqlite3 *db) {
  
  sqlite3_stmt *stmt;
  
  int retCode = sqlite3_prepare_v2(db, sqlSelectTimeseries.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlSelectTimeseries << " -- error: " << sqlite3_errmsg(db) << endl;
    return;
  }
  
  retCode = sqlite3_step(stmt);
  while (retCode == SQLITE_ROW) {
    // we have a row ready
    int uid = sqlite3_column_int(stmt, 0);
    sqltext typeChar = sqlite3_column_text(stmt, 2);
    string type((char*)typeChar);
    
    cout << "ts: " << uid << " " << type << endl;
    
    TimeSeries::sharedPointer ts;
    if (RTX_STRINGS_ARE_EQUAL(type, dbTimseriesName)) {
      ts = createTimeSeriesWithUid(uid);
    }
    else if (RTX_STRINGS_ARE_EQUAL(type, dbResamplerName)) {
      ts = createResamplerWithUid(uid);
    }
    else {
      cerr << "did not recognize type: " << type << " -- timeseries not created" << endl;
    }
    
    if (ts) {
      // store it locally
      _timeseries[uid] = ts;
    }
    
    
    // get the next row, or end
    retCode = sqlite3_step(stmt);
  }
  
  if (retCode != SQLITE_DONE) {
    cerr << "prepared statement failed" << endl;
    
  }
  
  retCode = sqlite3_finalize(stmt);
  
  
  
}





#pragma mark - point record creators

PointRecord::sharedPointer SqliteProjectFile::createOdbcRecordFromRow(sqlite3_stmt *stmt) {
  OdbcPointRecord::sharedPointer pr( new OdbcPointRecord );
  
  return pr;
}

PointRecord::sharedPointer SqliteProjectFile::createMysqlRecordFromRow(sqlite3_stmt *stmt) {
  MysqlPointRecord::sharedPointer pr( new MysqlPointRecord );
  
  return pr;
}

#pragma mark - timeseries creators

// sets name, clock, record, units
void SqliteProjectFile::setBaseProperties(TimeSeries::sharedPointer ts, int uid) {
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(_dbHandle, sqlGetTsById.c_str(), -1, &stmt, NULL);
  sqlite3_bind_int(stmt, 1, uid);
  
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    cerr << "could not get base properties" << endl;
    return;
  }
  
    
  sqltext name =      sqlite3_column_text(stmt, 0);
  sqltext unitsText = sqlite3_column_text(stmt, 1);
  int recordUid =     sqlite3_column_int(stmt, 2);
  int clockUid =      sqlite3_column_int(stmt, 3);
  
  
  ts->setName(string((char*)name));
  
  // record
  if (recordUid > 0) { // zero is a null value; column autoincrement starts at 1
    PointRecord::sharedPointer pr = _records[recordUid];
    if (pr) {
      ts->setRecord(pr);
    }
  }
  
  // units
  string unitsStr((char*)unitsText);
  Units theUnits = Units::unitOfType(unitsStr);
  ts->setUnits(theUnits);
  
  // clock
  if (clockUid > 0) {
    Clock::sharedPointer clock = _clocks[clockUid];
    if (clock) {
      ts->setClock(clock);
    }
  }
  
}

void SqliteProjectFile::setExtendedProperties(TimeSeries::sharedPointer ts, int uid) {
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(_dbHandle, sqlGetTsExtendedById.c_str(), -1, &stmt, NULL);
  sqlite3_bind_int(stmt, 1, uid);
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    cerr << "could not get extended properties" << endl;
    return;
  }
  
  string type = string((char*)sqlite3_column_text(stmt, 0));
  double v1 = sqlite3_column_double(stmt, 1);
  double v2 = sqlite3_column_double(stmt, 2);
  double v3 = sqlite3_column_double(stmt, 3);
  
  //std::function<bool(string)> typeEquals = [=](string x) {return RTX_STRINGS_ARE_EQUAL(type, x);}; // woot c++11
  
  #define typeEquals(x) RTX_STRINGS_ARE_EQUAL(type,x)
  
  if (typeEquals(dbOffsetName)) {
    boost::static_pointer_cast<OffsetTimeSeries>(ts)->setOffset(v1);
  }
  else if (typeEquals(dbConstantName)) {
    boost::static_pointer_cast<ConstantTimeSeries>(ts)->setValue(v1);
  }
  else if (typeEquals(dbValidrangeName)) {
    boost::static_pointer_cast<ValidRangeTimeSeries>(ts)->setRange(v1, v2);
    ValidRangeTimeSeries::filterMode_t mode = (v3 > 0) ? ValidRangeTimeSeries::drop : ValidRangeTimeSeries::saturate;
    boost::static_pointer_cast<ValidRangeTimeSeries>(ts)->setMode(mode);
  }
  else if (typeEquals(dbResamplerName)) {
//    boost::static_pointer_cast<Resampler>(ts)->setMode(mode);
  }
  
}

void SqliteProjectFile::setModularProperties(ModularTimeSeries::sharedPointer mts, int uid) {
  
}

void SqliteProjectFile::setAggregatorProperties(AggregatorTimeSeries::sharedPointer agg, int uid) {
  
}




TimeSeries::sharedPointer SqliteProjectFile::createTimeSeriesWithUid(int uid) {
  TimeSeries::sharedPointer ts(new TimeSeries);
  setBaseProperties(ts, uid);
  
  return ts;
}

TimeSeries::sharedPointer SqliteProjectFile::createResamplerWithUid(int uid) {
  Resampler::sharedPointer ts(new Resampler);
  setBaseProperties(ts, uid);
  setExtendedProperties(ts, uid);
  return ts;
}
