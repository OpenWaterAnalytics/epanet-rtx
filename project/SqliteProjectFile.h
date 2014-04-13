//
//  SqliteProjectFile.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
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
    
    void insertTimeSeries(TimeSeries::sharedPointer ts) {};
    void insertClock(Clock::sharedPointer clock) {};
    void insertRecord(PointRecord::sharedPointer record) {};
    
  private:
    // maps are keyed by uuid from database
    map<int, TimeSeries::sharedPointer> _timeseries;
    map<int, PointRecord::sharedPointer> _records;
    map<int, Clock::sharedPointer> _clocks;
    Model::sharedPointer _model;
    sqlite3 *_dbHandle;
    string _path;
    
    void loadRecordsFromDb();
    void loadTimeseriesFromDb();
    void loadModelFromDb();
        
    TimeSeries::sharedPointer newTimeseriesWithType(const std::string& type);
    void setBaseProperties(TimeSeries::sharedPointer ts, int uid);
    void setPropertyValuesForTimeSeriesWithType(TimeSeries::sharedPointer ts, const string& type, string key, double value);
    
  };
}

#endif /* defined(__epanet_rtx__SqliteProjectFile__) */
