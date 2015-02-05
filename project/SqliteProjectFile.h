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

namespace RTX {
  
  class SqliteProjectFile : public ProjectFile {
  public:
    void loadProjectFile(const string& path);
    void saveProjectFile(const string& path);
    void clear();
    
    RTX_LIST<TimeSeries::_sp> timeSeries();
    RTX_LIST<Clock::_sp> clocks();
    RTX_LIST<PointRecord::_sp> records();
    Model::_sp model();
    
    PointRecord::_sp defaultRecord();
    
    void insertTimeSeries(TimeSeries::_sp ts) {};
    void insertClock(Clock::_sp clock) {};
    void insertRecord(PointRecord::_sp record) {};
    
  private:
    // maps are keyed by uuid from database
    map<int, TimeSeries::_sp> _timeseries;
    map<int, PointRecord::_sp> _records;
    map<int, Clock::_sp> _clocks;
    
    map<int, Element::_sp> _elementUidLookup;
    map<int, std::pair<int, std::string> > _elementOutput;
    
    Model::_sp _model;
    sqlite3 *_dbHandle;
    string _path;
    
    void loadRecordsFromDb();
    void loadClocksFromDb();
    void loadTimeseriesFromDb();
    void loadModelFromDb();
    void setModelInputParameters();
    void setModelOutputSeries();
    
    void loadModelOutputMapping();
    
    void setJunctionParameter(Junction::_sp j, string paramName, TimeSeries::_sp ts);
    void setPipeParameter(Pipe::_sp p, string paramName, TimeSeries::_sp ts);
    TimeSeries::_sp tsPropertyForElementWithKey(Element::_sp, std::string key);
    
    TimeSeries::_sp newTimeseriesWithType(const std::string& type);
    void setBaseProperties(TimeSeries::_sp ts, int uid);
    void setPropertyValuesForTimeSeriesWithType(TimeSeries::_sp ts, const string& type, string key, double value);
    
  };
}

#endif /* defined(__epanet_rtx__SqliteProjectFile__) */
