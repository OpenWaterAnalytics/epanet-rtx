#include "SqliteProjectFile.h"

#ifndef RTX_NO_ODBC
#include "OdbcDirectPointRecord.h"
#endif
#ifndef RTX_NO_MYSQL
#include "MysqlPointRecord.h"
#endif
#include "SqlitePointRecord.h"
#include "InfluxDbPointRecord.h"

#include "TimeSeries.h"
#include "ConstantTimeSeries.h"
#include "SineTimeSeries.h"
#include "TimeSeriesFilter.h"
#include "MovingAverage.h"
#include "FirstDerivative.h"
#include "AggregatorTimeSeries.h"
#include "FirstDerivative.h"
#include "OffsetTimeSeries.h"
#include "CurveFunction.h"
#include "ThresholdTimeSeries.h"
#include "ValidRangeTimeSeries.h"
#include "GainTimeSeries.h"
#include "MultiplierTimeSeries.h"
#include "InversionTimeSeries.h"
#include "FailoverTimeSeries.h"
#include "LagTimeSeries.h"
#include "MathOpsTimeSeries.h"
//#include "WarpingTimeSeries.h"
#include "StatsTimeSeries.h"
#include "OutlierExclusionTimeSeries.h"

#include "rtxMacros.h"

#include "EpanetModel.h"

#include <boost/range/adaptors.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <iostream>
#include <fstream>

#define typeEquals(x) RTX_STRINGS_ARE_EQUAL(type,x)


typedef const unsigned char* sqltext;

using namespace RTX;
using namespace std;




/////////////////////////////////////////////////////////////
static int sqlRequiredUserVersion = 17;

//---------------------------------------------------------//
static string sqlSelectRecords =    "select id,name,type from db_records";
static string sqlSelectRecordProperties = "select key,value from db_record_properties where record_id = ?";
static string sqlSelectClocks = "select id,name,type,period,offset from clocks";
static string sqlSelectTimeseries = "select id,type from time_series";
static string sqlGetTsById =        "select name,units,record,clock from time_series where id=?";
//static string sqlGetTsModularById = "select * from time_series_modular left join time_series_extended using (id) where id=?";
//static string sqlGetTsExtendedById = "select type,value_1,value_2,value_3 from time_series left join time_series_extended using (id) where id=?";
static string sqlGetTsPropertiesById = "select key,value from time_series_properties where ref_table is null and id=?";
static string sqlGetTsSourceById = "select key,value from time_series_properties where ref_table=\"time_series\" and id=?";
static string sqlGetTsClockParameterById = "select key,value from time_series_properties where ref_table=\"clocks\" and id=?";
static string sqlGetAggregatorSourcesById = "select time_series,multiplier from time_series_aggregator where aggregator_id=?";
static string sqlGetCurveCoordinatesByTsId = "select x,y from ( select curve,name,curve_id,time_series_curves.id as tsId from time_series_curves left join curves on (curve = curve_id) ) left join curve_data using (curve_id) where tsId=?";
static string sqlGetModelElementParams = "select time_series, model_element, parameter from element_parameters";
static string sqlGetModelNameAndTypeByUid = "select name,type from model_elements where uid=?";

static string selectModelFileStr = "select value from meta where key = \"model_contents\"";

//---------------------------------------------------------//
static string dbTimeseriesName =  "TimeSeries";
static string dbConstantName =   "Constant";
static string dbSineName = "Sine";
static string dbResamplerName =  "Resampler";
static string dbMovingaverageName = "MovingAverage";
static string dbAggregatorName = "Aggregator";
static string dbDerivativeName = "FirstDerivative";
static string dbOffsetName =     "Offset";
static string dbCurveName =      "Curve";
static string dbThresholdName =  "Threshold";
static string dbValidrangeName = "ValidRange";
static string dbGainName =       "Gain";
static string dbMultiplierName = "Multiplier";
static string dbInversionName = "Inversion";
static string dbFailoverName = "Failover";
static string dbLagName = "Lag";
static string dbWarpName = "Warp";
static string dbMathOpsName = "MathOps";
static string dbStatsName = "Stats";
static string dbOutlierName = "OutlierExclusion";

//---------------------------------------------------------//
static string dbOdbcRecordName =  "odbc";
static string dbMysqlRecordName = "mysql";
static string dbSqliteRecordName = "sqlite";
static string dbInfluxRecordName = "influx";
//---------------------------------------------------------//
/////////////////////////////////////////////////////////////

static map<int,int> mapping() {
  map<int,int> m;
  m[1] = 1;
  return m;
}

// local convenience classes
typedef struct {
  TimeSeries::_sp ts;
  int uid;
  string type;
} tsListEntry;

typedef struct {
  PointRecord::_sp record;
  int uid;
  string type;
} pointRecordEntity;

typedef struct {
  int tsUid;
  int modelUid;
  string param;
} modelInputEntry;

enum SqliteModelParameterType {
  ParameterTypeJunction  = 0,
  ParameterTypeTank      = 1,
  ParameterTypeReservoir = 2,
  ParameterTypePipe      = 3,
  ParameterTypePump      = 4,
  ParameterTypeValve     = 5
};


// optional compile-in databases
namespace RTX {
  class PointRecordFactory {
  public:
    static PointRecord::_sp createSqliteRecordFromRow(sqlite3_stmt *stmt);
    static PointRecord::_sp createCsvPointRecordFromRow(sqlite3_stmt *stmt);
    static PointRecord::_sp createInfluxRecordFromRow(sqlite3_stmt *stmt);
#ifndef RTX_NO_ODBC
    static PointRecord::_sp createOdbcRecordFromRow(sqlite3_stmt *stmt);
#endif
#ifndef RTX_NO_MYSQL
    static PointRecord::_sp createMysqlRecordFromRow(sqlite3_stmt *stmt);
#endif
  };
}



bool SqliteProjectFile::loadProjectFile(const string& path) {
  boost::filesystem::path p(path);
  _path = boost::filesystem::absolute(p).string();
//  char *zErrMsg = 0;
  int returnCode;
  returnCode = sqlite3_open_v2(_path.c_str(), &_dbHandle, SQLITE_OPEN_READONLY, NULL);
  if( returnCode ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_dbHandle));
    sqlite3_close(_dbHandle);
    return false;
  }
  
  
  // check schema version
  int databaseVersion = -1;
  sqlite3_stmt *stmt_version;
  if(sqlite3_prepare_v2(_dbHandle, "PRAGMA user_version;", -1, &stmt_version, NULL) == SQLITE_OK) {
    while(sqlite3_step(stmt_version) == SQLITE_ROW) {
      databaseVersion = sqlite3_column_int(stmt_version, 0);
    }
  }
  
  if (databaseVersion < sqlRequiredUserVersion) {
    cerr << "Config Database Schema version not compatible. Require version " << sqlRequiredUserVersion << " or greater." << endl;
    return false;
  }
  
  
  
  // load everything
  loadRecordsFromDb();
  loadClocksFromDb();
  loadModelFromDb();
  loadModelOutputMapping(); // for output series
  
  loadTimeseriesFromDb();
  
  setModelInputParameters();
  
  
  // close db
  sqlite3_close(_dbHandle);
  return true;
}

bool SqliteProjectFile::saveProjectFile(const string& path) {
  // nope
  return false;
}

void SqliteProjectFile::clear() {
  
}


PointRecord::_sp SqliteProjectFile::defaultRecord() {
  PointRecord::_sp rec;
  return rec;
}

Model::_sp SqliteProjectFile::model() {
  return _model;
}

RTX_LIST<TimeSeries::_sp> SqliteProjectFile::timeSeries() {
  RTX_LIST<TimeSeries::_sp> list;
  BOOST_FOREACH(TimeSeries::_sp ts, _timeseries | boost::adaptors::map_values) {
    list.push_back(ts);
  }
  return list;
}

RTX_LIST<Clock::_sp> SqliteProjectFile::clocks() {
  RTX_LIST<Clock::_sp> list;
  BOOST_FOREACH(Clock::_sp c, _clocks | boost::adaptors::map_values) {
    list.push_back(c);
  }
  return list;
}

RTX_LIST<PointRecord::_sp> SqliteProjectFile::records() {
  RTX_LIST<PointRecord::_sp> list;
  BOOST_FOREACH(PointRecord::_sp pr, _records | boost::adaptors::map_values) {
    list.push_back(pr);
  }
  return list;
}


string SqliteProjectFile::metaValueForKey(const std::string& key) {
  
  sqlite3_stmt *stmt;
  sqlite3_prepare_v2(_dbHandle, "select value from meta where key=?", -1, &stmt, NULL);
  sqlite3_bind_text(stmt, 1, key.c_str(), -1, NULL);
  
  if (sqlite3_step(stmt) != SQLITE_ROW) {
    cerr << "could not get meta key-value" << endl;
    return string("");
  }
  
  sqltext value = sqlite3_column_text(stmt, 0);
  string v = string((char*)value);
  
  sqlite3_reset(stmt);
  sqlite3_finalize(stmt);
  
  return v;
}



#pragma mark - private methods

void SqliteProjectFile::loadRecordsFromDb() {
  
  RTX_LIST<pointRecordEntity> recordEntities;
  
  // set up the function pointers for creating the new records.
  typedef PointRecord::_sp (*PointRecordFp)(sqlite3_stmt*);
  map<string,PointRecordFp> prCreators;
  
#ifndef RTX_NO_ODBC
  prCreators[dbOdbcRecordName] = PointRecordFactory::createOdbcRecordFromRow;
#endif
#ifndef RTX_NO_MYSQL
  prCreators[dbMysqlRecordName] = PointRecordFactory::createMysqlRecordFromRow;
#endif
  
  prCreators[dbSqliteRecordName] = PointRecordFactory::createSqliteRecordFromRow;
  prCreators[dbInfluxRecordName] = PointRecordFactory::createInfluxRecordFromRow;
  
  sqlite3_stmt *stmt;
  
  int retCode = sqlite3_prepare_v2(_dbHandle, sqlSelectRecords.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlSelectRecords << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    // we have a row ready
    int uid = sqlite3_column_int(stmt, 0);
    string name = string((char*)sqlite3_column_text(stmt, 1));
    string type = string((char*)sqlite3_column_text(stmt, 2));
    
//    cout << "record: " << uid << " \"" << name << "\" (" << type << ")" << endl;
    
    // find the right function to create this type of record
    PointRecordFp creator = prCreators[type];
    PointRecord::_sp record;
    if (creator) {
      record = creator(stmt);
    }
    
    if (record) {
      record->setName(name);
      _records[uid] = record;
      pointRecordEntity entity;
      entity.record = record;
      entity.type = type;
      entity.uid = uid;
      recordEntities.push_back(entity);
    }
    else {
      cerr << "could not create point record. type \"" << type << "\" not supported." << endl;
    }
    
  }
  sqlite3_reset(stmt);
  sqlite3_finalize(stmt);
  
  // now load up each point record's connection attributes.
  boost::filesystem::path projPath(_path);
  BOOST_FOREACH(const pointRecordEntity& entity, recordEntities) {
    
    // collect the k-v pairs
    map<string, string> kvMap;
    sqlite3_prepare_v2(_dbHandle, sqlSelectRecordProperties.c_str(), -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, entity.uid);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      string key = string((char*)sqlite3_column_text(stmt, 0));
      string value = string((char*)sqlite3_column_text(stmt, 1));
      kvMap[key] = value;
    }
    sqlite3_reset(stmt);
    sqlite3_finalize(stmt);
    
    
    if (kvMap.count("filterString")) {
      string value = kvMap["filterString"];
      vector<string> parts;
      boost::split(parts, value, boost::is_any_of(":"));
      string filterTypeString = parts[0];
      
      DbPointRecord::OpcFilterType filterType;
      if (boost::iequals(filterTypeString, "blacklist")) {
        filterType = DbPointRecord::OpcFilterType::OpcBlackList;
      }
      else if (boost::iequals(filterTypeString, "whitelist")) {
        filterType = DbPointRecord::OpcFilterType::OpcWhiteList;
      }
      else if (boost::iequals(filterTypeString, "codes")) {
        filterType = DbPointRecord::OpcFilterType::OpcCodesToValues;
      }
      else {
        filterType = DbPointRecord::OpcFilterType::OpcPassThrough;
      }
      boost::dynamic_pointer_cast<DbPointRecord>(entity.record)->setOpcFilterType(filterType);
      
      if (parts.size() > 1) {
        vector<string> codeStrings;
        boost::split(codeStrings, parts[1], boost::is_any_of(","));
        BOOST_FOREACH(string s, codeStrings) {
          unsigned int code = boost::lexical_cast<int>(s);
          boost::dynamic_pointer_cast<DbPointRecord>(entity.record)->addOpcFilterCode(code);
        }
      }
    }
    
    if(kvMap.count("readonly")) {
      string value = kvMap["readonly"];
      // string one or zero (1,0)
      bool readOnly = boost::lexical_cast<bool>(value);
      boost::dynamic_pointer_cast<DbPointRecord>(entity.record)->setReadonly(readOnly);
    }
    
    if (kvMap.count("connectionString")) {
      if (RTX_STRINGS_ARE_EQUAL(entity.type, "sqlite")) {
        boost::filesystem::path dbPath(kvMap["connectionString"]);
        string absDbPath = boost::filesystem::absolute(dbPath,projPath.parent_path()).string();
        boost::dynamic_pointer_cast<DbPointRecord>(entity.record)->setConnectionString(absDbPath);
      }
      else {
        string conn = kvMap["connectionString"];
        boost::dynamic_pointer_cast<DbPointRecord>(entity.record)->setConnectionString(conn);
      }
    }
    
    
    
  }
}

void SqliteProjectFile::loadClocksFromDb() {
  
  sqlite3_stmt *stmt;
  int retCode = sqlite3_prepare_v2(_dbHandle, sqlSelectClocks.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlSelectClocks << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    //static string sqlSelectClocks = "select id,name,type,period,offset from clocks";
    int uid = sqlite3_column_int(stmt, 0);
    string name = string((char*)sqlite3_column_text(stmt, 1));
    string type = string((char*)sqlite3_column_text(stmt, 2));
    int period = sqlite3_column_int(stmt, 3);
    int offset = sqlite3_column_int(stmt, 4);
    
    if (RTX_STRINGS_ARE_EQUAL(type, "Regular")) {
      Clock::_sp clock( new Clock(period,offset) );
      clock->setName(name);
      _clocks[uid] = clock;
    }
    
  }
  
}

void SqliteProjectFile::loadTimeseriesFromDb() {
  
  RTX_LIST<tsListEntry> tsList;
  sqlite3_stmt *stmt;
  
  int retCode = sqlite3_prepare_v2(_dbHandle, sqlSelectTimeseries.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlSelectTimeseries << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  
  // CREATE ALL THE TIME SERIES
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    // we have a row ready
    int uid = sqlite3_column_int(stmt, 0);
    sqltext typeChar = sqlite3_column_text(stmt, 1);
    string type((char*)typeChar);
    TimeSeries::_sp ts = newTimeseriesWithType(type);
    if (ts) {
      tsListEntry entry;
      entry.ts = ts;
      entry.uid = uid;
      entry.type = type;
      tsList.push_back(entry);
    }
    
    else if (RTX_STRINGS_ARE_EQUAL(type, "element_output")) {
      
      // this time series already exists. it's an output of a model element.
      // we should already have the info to find it in _elementOutput
      
      if (_elementOutput.find(uid) == _elementOutput.end()) {
        cerr << "Warning: could not find output element specifed with time series UID " << uid << " - check that the ts exists in model_element_storage table." << endl;
        continue;
      }
      
      pair<int,string> elementUidKeyName = _elementOutput[uid];
      int modelElementUid = elementUidKeyName.first;
      string modelElementKey = elementUidKeyName.second;
      
      Element::_sp modelElement = _elementUidLookup[modelElementUid];
      
      if (!modelElement) {
        cerr << "loading time series: could not find model element : " << modelElementUid << endl;
      }
      
      TimeSeries::_sp elementProp = tsPropertyForElementWithKey(modelElement, modelElementKey);
      if (!elementProp) {
        cerr << "could not retrieve element property: " << modelElementKey << endl;
        continue;
      }
      
      // we have found the model element, and retrieved the appropriate output parameter as a time series. save it for future use.
      tsListEntry entry;
      entry.ts = elementProp;
      entry.uid = uid;
      entry.type = type;
      tsList.push_back(entry);
      
    }
    
  }
  sqlite3_finalize(stmt);
  
  // cache them, keyed by uid
  BOOST_FOREACH(tsListEntry entry, tsList) {
    _timeseries[entry.uid] = entry.ts;
  }
  
  // setting units, record, clock...
  BOOST_FOREACH(tsListEntry entry, tsList) {
    this->setBaseProperties(entry.ts, entry.uid);
  }
  
  
  
  // SET PROPERTIES
  retCode = sqlite3_prepare_v2(_dbHandle, sqlGetTsPropertiesById.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlGetTsPropertiesById << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  BOOST_FOREACH(tsListEntry entry, tsList) {
    sqlite3_bind_int(stmt, 1, entry.uid);
    // get k-v pairs and use a setter function
    retCode = sqlite3_step(stmt);
    while (retCode == SQLITE_ROW) {
      string key((char*)sqlite3_column_text(stmt, 0));
      double val = sqlite3_column_double(stmt, 1);
      this->setPropertyValuesForTimeSeriesWithType(entry.ts, entry.type, key, val);
      retCode = sqlite3_step(stmt);
    }
    sqlite3_reset(stmt);
  }
  retCode = sqlite3_finalize(stmt);
  
  // SET SOURCES
  /*** ---> TODO ::
   need to figure out a smart way to ensure that downstream modules are affected by upstream setters.
   that is, make sure we're working from left to right.
   do we need a responder chain?
   
   generally we would expect all this to be ordered by id, with id increasing from "left to right"
   ***/
  
  // get time series sources and secondary sources, for modular types.
  retCode = sqlite3_prepare_v2(_dbHandle, sqlGetTsSourceById.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlGetTsSourceById << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  
  BOOST_FOREACH(tsListEntry entry, tsList) {
    // if it doesn't take a source, then move on.
    TimeSeriesFilter::_sp filter = boost::dynamic_pointer_cast<TimeSeriesFilter>(entry.ts);
    if (!filter) {
      continue;
    }
    
    // query for any upstream sources / secondary sources
    sqlite3_bind_int(stmt, 1, entry.uid);
    // for each row, find out what key it's setting and set it on this time series.
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      string key = (char*)sqlite3_column_text(stmt, 0);
      int idx = sqlite3_column_int(stmt, 1);
      TimeSeries::_sp upstreamTs = _timeseries[idx];
      if (!upstreamTs) {
        continue;
      }
      
      // possibilites for key: source, failover, multiplier, warp
      if (RTX_STRINGS_ARE_EQUAL(key, "source")) {
        filter->setSource(upstreamTs);
      }
      else if (RTX_STRINGS_ARE_EQUAL(key, "failover")) {
        boost::dynamic_pointer_cast<FailoverTimeSeries>(filter)->setFailoverTimeseries(upstreamTs);
      }
      else if (RTX_STRINGS_ARE_EQUAL(key, "multiplier")) {
        boost::dynamic_pointer_cast<MultiplierTimeSeries>(filter)->setMultiplier(upstreamTs);
      }
      else if (RTX_STRINGS_ARE_EQUAL(key, "warp")) {
//        boost::dynamic_pointer_cast<WarpingTimeSeries>(mod)->setWarp(upstreamTs);
      }
      else {
        cerr << "unknown key: " << key << endl;
      }
    }
    sqlite3_reset(stmt); // get ready for next bind
  }
  retCode = sqlite3_finalize(stmt);
  
  
  
  // get parameterized clocks
  retCode = sqlite3_prepare_v2(_dbHandle, sqlGetTsClockParameterById.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlGetTsClockParameterById << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  BOOST_FOREACH(tsListEntry entry, tsList) {
    // query for any upstream sources / secondary sources
    sqlite3_bind_int(stmt, 1, entry.uid);
    // if this returns a row, then this time series has a parameterized clock
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      string key = (char*)sqlite3_column_text(stmt, 0);
      int idx = sqlite3_column_int(stmt, 1);
      Clock::_sp clock = _clocks[idx];
      if (!clock) {
        continue;
      }
      if (RTX_STRINGS_ARE_EQUAL(key, "samplingWindow")) {
        BaseStatsTimeSeries::_sp bs = boost::dynamic_pointer_cast<BaseStatsTimeSeries>(entry.ts);
        bs->setWindow(clock);
      }
    }
    sqlite3_reset(stmt);
  }
  retCode = sqlite3_finalize(stmt);
  
  
  
  
  // ANY AGGREGATORS?
  sqlite3_prepare_v2(_dbHandle, sqlGetAggregatorSourcesById.c_str(), -1, &stmt, NULL);
  BOOST_FOREACH(tsListEntry entry, tsList) {
    if (RTX_STRINGS_ARE_EQUAL(entry.type, "Aggregator")) {
      AggregatorTimeSeries::_sp agg = boost::dynamic_pointer_cast<AggregatorTimeSeries>(entry.ts);
      if (!agg) {
        cerr << "not an aggregator" << endl;
        continue;
      }
      // get sources.
      sqlite3_bind_int(stmt, 1, entry.uid);
      while (sqlite3_step(stmt) == SQLITE_ROW) {
        int idx = sqlite3_column_int(stmt, 0);
        double multiplier = sqlite3_column_double(stmt, 1);
        TimeSeries::_sp upstream = _timeseries[idx];
        agg->addSource(upstream, multiplier);
      }
      sqlite3_reset(stmt);
    }
    
  }
  
  
  // ANY CURVE TIME SERIES?
  BOOST_FOREACH(tsListEntry tsEntry, tsList) {
    if (RTX_STRINGS_ARE_EQUAL(tsEntry.type, "Curve")) {
      
      CurveFunction::_sp curveTs = boost::dynamic_pointer_cast<CurveFunction>(tsEntry.ts);
      if (!curveTs) {
        cerr << "not an actual curve function object" << endl;
        continue;
      }
      
      // get the curve id
      sqlite3_stmt *getCurveIdStmt;
      sqlite3_prepare_v2(_dbHandle, sqlGetCurveCoordinatesByTsId.c_str(), -1, &getCurveIdStmt, NULL);
      sqlite3_bind_int(getCurveIdStmt, 1, tsEntry.uid);
      while (sqlite3_step(getCurveIdStmt) == SQLITE_ROW) {
        double x = sqlite3_column_double(getCurveIdStmt, 0);
        double y = sqlite3_column_double(getCurveIdStmt, 1);
        curveTs->addCurveCoordinate(x, y);
      }
      sqlite3_finalize(getCurveIdStmt);
    }
    
    
  }
  
  
  retCode = sqlite3_finalize(stmt);
  
  
}



void SqliteProjectFile::loadModelFromDb() {
  int ret;
  string modelContents;
  boost::filesystem::path tempFile = boost::filesystem::temp_directory_path();
  
  // de-serialize the model file from meta.key="model_contents"
  sqlite3_stmt *stmt;
  ret = sqlite3_prepare_v2(_dbHandle, selectModelFileStr.c_str(), -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    cerr << "can't prepare statement: " << selectModelFileStr << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
  }
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    modelContents = string((char*)sqlite3_column_text(stmt, 0));
    try {
      boost::filesystem::path tempNameHash = boost::filesystem::unique_path();
      const std::string tempstr    = tempNameHash.native();  // optional
      
      tempFile /= tempNameHash;
      tempFile.replace_extension(".inp");
      
      // dump contents into file.
      
      ofstream modelFileStream;
      modelFileStream.open(tempFile.string());
      modelFileStream << modelContents;
      modelFileStream.close();
      
      
    } catch (std::exception &e) {
      cerr << e.what();
    }
    
  }
  else {
    std::cerr << "can't step sqlite or no model found: " << sqlite3_errmsg(_dbHandle) << endl;
    sqlite3_finalize(stmt);
    return;
  }
  sqlite3_finalize(stmt);
  
  EpanetModel::_sp enModel( new EpanetModel( tempFile.string() ) );
  _model = enModel;
  
  
  // sorry, not done yet. we have to get a mapping of model element UID to the element pointer.
  // this is in case the configuration uses these model elements as output.
  
  
  
  
  static string sqlSelectModelElements = "select uid,name,type from model_elements";
  
  int retCode = sqlite3_prepare_v2(_dbHandle, sqlSelectModelElements.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlSelectModelElements << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    
    int e_uid = sqlite3_column_int(stmt, 0);
    string e_name = string((char*)sqlite3_column_text(stmt, 1));
    int e_type = sqlite3_column_int(stmt, 2);
    
    switch (e_type) {
      case 0:
      case 1:
      case 2:
      {
        // junction, tank, reservoir
        Node::_sp n = _model->nodeWithName(e_name);
        if (n) {
          _elementUidLookup[e_uid] = n;
        }
      }
        break;
        
      case 3:
      case 4:
      case 5:
      {
        // pipe, valve, pump
        Link::_sp l = _model->linkWithName(e_name);
        if (l) {
          _elementUidLookup[e_uid] = l;
        }
      }
        break;
      default:
        break;
    }
    
  }
  
  sqlite3_finalize(stmt);

  
  static string sqlSelectNodeCoords = "select uid,latitude,longitude from model_junctions";
  retCode = sqlite3_prepare_v2(_dbHandle, sqlSelectNodeCoords.c_str(), -1, &stmt, NULL);
  if (retCode != SQLITE_OK) {
    cerr << "can't prepare statement: " << sqlSelectNodeCoords << " -- error: " << sqlite3_errmsg(_dbHandle) << endl;
    return;
  }
  
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int e_uid = sqlite3_column_int(stmt, 0);
    double e_lat = sqlite3_column_double(stmt, 1);
    double e_lon = sqlite3_column_double(stmt, 2);
    
    Node::_sp theNode = boost::dynamic_pointer_cast<Node>(_elementUidLookup[e_uid]);
    if (theNode) {
      theNode->setCoordinates(e_lon, e_lat);
    }
  }

}




#pragma mark - point record creators
#ifndef RTX_NO_ODBC
PointRecord::_sp PointRecordFactory::createOdbcRecordFromRow(sqlite3_stmt *stmt) {
  OdbcPointRecord::_sp pr( new OdbcDirectPointRecord );
  return pr;
}
#endif
#ifndef RTX_NO_MYSQL
PointRecord::_sp PointRecordFactory::createMysqlRecordFromRow(sqlite3_stmt *stmt) {
  MysqlPointRecord::_sp pr( new MysqlPointRecord );
  return pr;
}
#endif
PointRecord::_sp PointRecordFactory::createSqliteRecordFromRow(sqlite3_stmt *stmt) {
  SqlitePointRecord::_sp pr( new SqlitePointRecord );
  return pr;
}
PointRecord::_sp PointRecordFactory::createInfluxRecordFromRow(sqlite3_stmt *stmt) {
  InfluxDbPointRecord::_sp pr( new InfluxDbPointRecord );
  return pr;
}



#pragma mark - timeseries creators

TimeSeries::_sp SqliteProjectFile::newTimeseriesWithType(const string& type) {
  
  if (typeEquals(dbTimeseriesName)) {
    // just the base class
    TimeSeries::_sp ts(new TimeSeries);
    return ts;
  }
  else if (typeEquals(dbConstantName)) {
    ConstantTimeSeries::_sp ts(new ConstantTimeSeries);
    return ts;
  }
  else if (typeEquals(dbSineName)) {
    SineTimeSeries::_sp ts(new SineTimeSeries);
    return ts;
  }
  else if (typeEquals(dbResamplerName)) {
    TimeSeriesFilter::_sp ts(new TimeSeriesFilter);
    return ts;
  }
  else if (typeEquals(dbMovingaverageName)) {
    MovingAverage::_sp ts(new MovingAverage);
    return ts;
  }
  else if (typeEquals(dbAggregatorName)) {
    AggregatorTimeSeries::_sp ts(new AggregatorTimeSeries);
    return ts;
  }
  else if (typeEquals(dbDerivativeName)) {
    FirstDerivative::_sp ts(new FirstDerivative);
    return ts;
  }
  else if (typeEquals(dbOffsetName)) {
    OffsetTimeSeries::_sp ts(new OffsetTimeSeries);
    return ts;
  }
  else if (typeEquals(dbCurveName)) {
    CurveFunction::_sp ts(new CurveFunction);
    return ts;
  }
  else if (typeEquals(dbThresholdName)) {
    ThresholdTimeSeries::_sp ts(new ThresholdTimeSeries);
    return ts;
  }
  else if (typeEquals(dbValidrangeName)) {
    ValidRangeTimeSeries::_sp ts(new ValidRangeTimeSeries);
    return ts;
  }
  else if (typeEquals(dbGainName)) {
    GainTimeSeries::_sp ts(new GainTimeSeries);
    return ts;
  }
  else if (typeEquals(dbMultiplierName)) {
    MultiplierTimeSeries::_sp ts(new MultiplierTimeSeries);
    return ts;
  }
  else if (typeEquals(dbInversionName)) {
    InversionTimeSeries::_sp ts(new InversionTimeSeries);
    return ts;
  }
  else if (typeEquals(dbFailoverName)) {
    FailoverTimeSeries::_sp ts(new FailoverTimeSeries);
    return ts;
  }
  else if (typeEquals(dbLagName)) {
    LagTimeSeries::_sp ts(new LagTimeSeries);
    return ts;
  }
  else if (typeEquals(dbWarpName)) {
//    WarpingTimeSeries::_sp ts(new WarpingTimeSeries);
//    return ts;
  }
  else if (typeEquals(dbMathOpsName)) {
    MathOpsTimeSeries::_sp ts(new MathOpsTimeSeries);
    return ts;
  }
  else if (typeEquals(dbStatsName)) {
    StatsTimeSeries::_sp ts(new StatsTimeSeries);
    return ts;
  }
  else if (typeEquals(dbOutlierName)) {
    OutlierExclusionTimeSeries::_sp ts(new OutlierExclusionTimeSeries);
    return ts;
  }
  

  cerr << "Did not recognize type: " << type << endl;
  return TimeSeries::_sp(); // nada
}

#pragma mark TimeSeries setters

// sets name, clock, record, units
void SqliteProjectFile::setBaseProperties(TimeSeries::_sp ts, int uid) {
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
  
  // name
  ts->setName(string((char*)name));
  
  // units
  string unitsStr((char*)unitsText);
  Units theUnits = Units::unitOfType(unitsStr);
  ts->setUnits(theUnits);
  
  // clock
  if (clockUid > 0) {
    Clock::_sp clock = _clocks[clockUid];
    if (clock) {
      ts->setClock(clock);
    }
    else {
      cerr << "could not find clock uid " << clockUid << endl;
    }
  }
  
  // record
  // set the record last, since any other changes will invalidate points that are currently stored there.
  if (recordUid > 0) { // zero is a null value; column autoincrement starts at 1
    PointRecord::_sp pr = _records[recordUid];
    if (pr) {
      ts->setRecord(pr);
    }
  }
  
  
  

  
  sqlite3_finalize(stmt);
  
}
/*
 void SqliteProjectFile::setExtendedProperties(TimeSeries::_sp ts, int uid) {
 sqlite3_stmt *stmt;
 //  sqlite3_prepare_v2(_dbHandle, sqlGetTsExtendedById.c_str(), -1, &stmt, NULL);
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
 */



void SqliteProjectFile::setPropertyValuesForTimeSeriesWithType(TimeSeries::_sp ts, const string& type, string key, double val) {
  
  
  /*** stupidly long chain of if-else statements to set k-v properties ***/
  /*** broken down first by type name, then by key name ***/
  if (typeEquals(dbTimeseriesName)) {
    // base class keys... none.
  }
  else if (typeEquals(dbConstantName)) {
    // keys: value
    if (RTX_STRINGS_ARE_EQUAL(key, "value")) {
      boost::dynamic_pointer_cast<ConstantTimeSeries>(ts)->setValue(val);
    }
  }
  else if (typeEquals(dbSineName)) {
    SineTimeSeries::_sp sine = boost::dynamic_pointer_cast<SineTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "magnitude")) {
      sine->setMagnitude(val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "period")) {
      sine->setPeriod((time_t)val);
    }
  }
  else if (typeEquals(dbResamplerName)) {
    TimeSeriesFilter::_sp rs = boost::dynamic_pointer_cast<TimeSeriesFilter>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "interpolationMode")) {
      rs->setResampleMode((TimeSeries::TimeSeriesResampleMode)val);
    }
  }
  else if (typeEquals(dbMovingaverageName)) {
    MovingAverage::_sp ma = boost::dynamic_pointer_cast<MovingAverage>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "window")) {
      ma->setWindowSize((int)val);
    }
  }
  else if (typeEquals(dbAggregatorName)) {
    // nothing
  }
  else if (typeEquals(dbDerivativeName)) {
    // nothing
  }
  else if (typeEquals(dbOffsetName)) {
    OffsetTimeSeries::_sp os = boost::dynamic_pointer_cast<OffsetTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "offset")) {
      os->setOffset(val);
    }
  }
  else if (typeEquals(dbCurveName)) {
  }
  else if (typeEquals(dbThresholdName)) {
    ThresholdTimeSeries::_sp th = boost::dynamic_pointer_cast<ThresholdTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "threshold")) {
      th->setThreshold(val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "value")) {
      th->setValue(val);
    }
  }
  else if (typeEquals(dbValidrangeName)) {
    ValidRangeTimeSeries::_sp vr = boost::dynamic_pointer_cast<ValidRangeTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "mode")) {
      vr->setMode((ValidRangeTimeSeries::filterMode_t)val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "max")) {
      vr->setRange(vr->range().first, val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "min")) {
      vr->setRange(val, vr->range().second);
    }
  }
  else if (typeEquals(dbGainName)) {
    GainTimeSeries::_sp gn = boost::dynamic_pointer_cast<GainTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "gain")) {
      gn->setGain(val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "gainUnits")) {
      unsigned int idx = 0;
      unsigned int desiredValue = (unsigned int)val;
      map<string,Units> uMap = Units::unitStringMap;
      map<string,Units>::const_iterator it;
      for(it=uMap.begin();it!=uMap.end();it++) {
        if (idx == desiredValue) { // indices match
          gn->setGainUnits(it->second);
          break; // map iteration
        }
        ++idx;
      }
    }
  }
  else if (typeEquals(dbMultiplierName)) {
    //
  }
  else if (typeEquals(dbInversionName)) {
    //
  }
  else if (typeEquals(dbFailoverName)) {
    FailoverTimeSeries::_sp fo = boost::dynamic_pointer_cast<FailoverTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "maximumStaleness")) {
      fo->setMaximumStaleness((time_t)val);
    }
  }
  else if (typeEquals(dbLagName)) {
    LagTimeSeries::_sp os = boost::dynamic_pointer_cast<LagTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "lag")) {
      os->setOffset((time_t)val);
    }
  }
  else if (typeEquals(dbWarpName)) {
    //
  }
  else if (typeEquals(dbStatsName)) {
    StatsTimeSeries::_sp st = boost::dynamic_pointer_cast<StatsTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "samplingMode")) {
      st->setSamplingMode((StatsTimeSeries::StatsSamplingMode_t)val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "statsMode")) {
      st->setStatsType((StatsTimeSeries::StatsTimeSeriesType)val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "arbitraryPercentile")) {
      st->setArbitraryPercentile(val);
    }
  }
  else if (typeEquals(dbOutlierName)) {
    OutlierExclusionTimeSeries::_sp outl = boost::dynamic_pointer_cast<OutlierExclusionTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "samplingMode")) {
      outl->setSamplingMode((StatsTimeSeries::StatsSamplingMode_t)val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "exclusionMode")) {
      outl->setExclusionMode((OutlierExclusionTimeSeries::exclusion_mode_t)val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "outlierMultiplier")) {
      outl->setOutlierMultiplier(val);
    }
  }
  else if (typeEquals(dbMathOpsName)) {
    MathOpsTimeSeries::_sp mo = boost::dynamic_pointer_cast<MathOpsTimeSeries>(ts);
    if (RTX_STRINGS_ARE_EQUAL(key, "opsType")) {
      mo->setMathOpsType((MathOpsTimeSeries::MathOpsTimeSeriesType)val);
    }
    else if (RTX_STRINGS_ARE_EQUAL(key, "argument")) {
      mo->setArgument(val);
    }
  }
  else {
    cerr << "unknown type name: " << type << endl;
  }
  
}



void SqliteProjectFile::setModelInputParameters() {
  
  // input params are defined by matched timeseries db-uid and model db-uid, so get that info first.
  vector<modelInputEntry> modelInputs;
  
  sqlite3_stmt *stmt;
  int ret = sqlite3_prepare_v2(_dbHandle, sqlGetModelElementParams.c_str(), -1, &stmt, NULL);
  
  if (ret != SQLITE_OK) {
    cerr << "could not prepare statement: " << sqlGetModelElementParams << endl;
    return;
  }
  
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    modelInputEntry entry;
    entry.tsUid = sqlite3_column_int(stmt, 0);
    entry.modelUid = sqlite3_column_int(stmt, 1);
    entry.param = string((char*)sqlite3_column_text(stmt, 2));
    modelInputs.push_back(entry);
  }
  sqlite3_finalize(stmt);
  
  ret = sqlite3_prepare_v2(_dbHandle, sqlGetModelNameAndTypeByUid.c_str(), -1, &stmt, NULL);
  if (ret != SQLITE_OK) {
    cerr << "could not prepare statement: " << sqlGetModelNameAndTypeByUid << endl;
    return;
  }
  
  BOOST_FOREACH(const modelInputEntry& entry, modelInputs) {
    // first, check if it's a valid time series
    TimeSeries::_sp ts = _timeseries[entry.tsUid];
    if (!ts) {
      cerr << "Invalid time series: " << entry.tsUid << endl;
      continue;
    }
    
    // get a row for this model element
    sqlite3_bind_int(stmt, 1, entry.modelUid);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
      cerr << "could not get model element " << entry.modelUid << endl;
      continue;
    }
    // fetch the name and type of element.
    string elementName = string((char*) sqlite3_column_text(stmt, 0));
    SqliteModelParameterType elementType = (SqliteModelParameterType)sqlite3_column_int(stmt, 1);
    
    switch ((int)elementType) {
      case ParameterTypeJunction:
      case ParameterTypeTank:
      case ParameterTypeReservoir:
      {
        /// do junctioney things
        Junction::_sp j = boost::dynamic_pointer_cast<Junction>(this->model()->nodeWithName(elementName));
        this->setJunctionParameter(j, entry.param, ts);
        break;
      }
      case ParameterTypePipe:
      case ParameterTypePump:
      case ParameterTypeValve:
      {
        // do pipey things
        Pipe::_sp p = boost::dynamic_pointer_cast<Pipe>(this->model()->linkWithName(elementName));
        this->setPipeParameter(p, entry.param, ts);
        break;
      }
      default:
        break;
    }
    
    sqlite3_reset(stmt);
  }
  sqlite3_finalize(stmt);
  
}


void SqliteProjectFile::loadModelOutputMapping() {
  
  static string sqlGetModelOutputMapping = "select ts_id, model_id, key from model_element_storage";
  
  // find model element storage data
  sqlite3_stmt *stmt;
  int ret = sqlite3_prepare_v2(_dbHandle, sqlGetModelOutputMapping.c_str(), -1, &stmt, NULL);
  
  if (ret != SQLITE_OK) {
    cerr << "could not prepare statement: " << sqlGetModelOutputMapping << endl;
    return;
  }
  
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    
    int tsUid = sqlite3_column_int(stmt, 0);
    int modelUid = sqlite3_column_int(stmt, 1);
    string key = string((char*)sqlite3_column_text(stmt, 2));
    
    _elementOutput[tsUid] = make_pair(modelUid,key);
  }
  sqlite3_finalize(stmt);
  
}



void SqliteProjectFile::setJunctionParameter(Junction::_sp j, string paramName, TimeSeries::_sp ts) {
  
  if (RTX_STRINGS_ARE_EQUAL(paramName, "demandBoundary")) {
    j->setBoundaryFlow(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "headBoundary")) {
    j->setHeadMeasure(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "qualityBoundary")) {
    j->setQualitySource(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "pressureMeasure")) {
    j->setPressureMeasure(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "headMeasure")) {
    j->setHeadMeasure(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "levelMeasure")) {
    boost::dynamic_pointer_cast<Tank>(j)->setLevelMeasure(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "qualityMeasure")) {
    j->setQualityMeasure(ts);
  }
  else {
    cerr << "Unknown parameter type: " << paramName << endl;
  }
  
}

void SqliteProjectFile::setPipeParameter(Pipe::_sp p, string paramName, TimeSeries::_sp ts) {
  
  if (RTX_STRINGS_ARE_EQUAL(paramName, "statusBoundary")) {
    p->setStatusParameter(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "settingBoundary")) {
    p->setSettingParameter(ts);
  }
  else if (RTX_STRINGS_ARE_EQUAL(paramName, "flowMeasure")) {
    p->setFlowMeasure(ts);
  }
  else {
    cerr << "Unknown parameter type: " << paramName << endl;
  }
  
}


TimeSeries::_sp SqliteProjectFile::tsPropertyForElementWithKey(Element::_sp element, std::string key) {
  
  TimeSeries::_sp blank;
  
  // @[@"flow",@"level",@"head",@"tankFlow",@"demand",@"quality"]
  
  // TO_DO :: add support for the full set of read-only timeseries keys
  
  switch (element->type()) {
    case Element::JUNCTION:
    case Element::TANK:
    case Element::RESERVOIR:
    {
      if (RTX_STRINGS_ARE_EQUAL(key, "tankFlow")) {
        return boost::static_pointer_cast<Tank>(element)->flowMeasure();
      }
      else if (RTX_STRINGS_ARE_EQUAL(key, "level")) {
        return boost::static_pointer_cast<Tank>(element)->level();
      }
      else if (RTX_STRINGS_ARE_EQUAL(key, "head")) {
        return boost::static_pointer_cast<Junction>(element)->head();
      }
      else if (RTX_STRINGS_ARE_EQUAL(key, "pressure")) {
        return boost::static_pointer_cast<Junction>(element)->pressure();
      }
      else if (RTX_STRINGS_ARE_EQUAL(key, "quality")) {
        return boost::static_pointer_cast<Junction>(element)->quality();
      }
      else {
        cerr << "Warning unknown Node key: " << key << endl;
        return blank;
      }
    }
      break;
      
    case Element::PIPE:
    case Element::PUMP:
    case Element::VALVE:
    {
      if (RTX_STRINGS_ARE_EQUAL(key, "flow")) {
        return boost::static_pointer_cast<Pipe>(element)->flow();
      }
      else {
        cerr << "Warning unknown Link key: " << key << endl;
        return blank;
      }
    }
      break;
      
    default:
    {
      // unknown element class
      cerr << "Warning unknown element class" << endl;
      return blank;
    }
      break;
  }
}



