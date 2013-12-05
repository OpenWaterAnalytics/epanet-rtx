//
//  SqliteProjectFile.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 9/24/13.
//
//

#ifndef __epanet_rtx__SqliteProjectFile__
#define __epanet_rtx__SqliteProjectFile__

#include <iostream>
#include <sqlite3.h>

#include "ProjectFile.h"

#include "ModularTimeSeries.h"
#include "AggregatorTimeSeries.h"



namespace RTX {
  
  class SqliteProjectFile : public ProjectFile {
  public:
    void loadProjectFile(const string& path);
    void saveProjectFile(const string& path);
    void clear();
    
    RTX_LIST<TimeSeries::sharedPointer> timeSeries();
    RTX_LIST<Clock::sharedPointer> clocks();
    RTX_LIST<PointRecord::sharedPointer> records();
    Model::sharedPointer model();
    
    PointRecord::sharedPointer defaultRecord();
    
    
  private:
    // maps are keyed by uuid from database
    map<int, TimeSeries::sharedPointer> _timeseries;
    map<int, PointRecord::sharedPointer> _records;
    map<int, Clock::sharedPointer> _clocks;
    Model::sharedPointer _model;
    sqlite3 *_dbHandle;
    string _path;
    
    void loadRecordsFromDb(sqlite3 *db);
    void loadTimeseriesFromDb(sqlite3 *db);
    
    static PointRecord::sharedPointer createOdbcRecordFromRow(sqlite3_stmt *stmt);
    static PointRecord::sharedPointer createMysqlRecordFromRow(sqlite3_stmt *stmt);
    
    void setBaseProperties(TimeSeries::sharedPointer ts, int uid);
    void setExtendedProperties(TimeSeries::sharedPointer ts, int uid);
    void setModularProperties(ModularTimeSeries::sharedPointer mts, int uid);
    void setAggregatorProperties(AggregatorTimeSeries::sharedPointer agg, int uid);
    TimeSeries::sharedPointer createTimeSeriesWithUid(int uid);
    TimeSeries::sharedPointer createResamplerWithUid(int uid);
    
  };
  
}

#endif /* defined(__epanet_rtx__SqliteProjectFile__) */
