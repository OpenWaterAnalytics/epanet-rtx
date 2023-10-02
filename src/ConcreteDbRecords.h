#ifndef __epanet_rtx__ConcreteDbRecords__
#define __epanet_rtx__ConcreteDbRecords__

#include <iostream>
#include <list>

#include "DbPointRecord.h"
#include "PointRecordTime.h"

namespace RTX {
  
  class SqlitePointRecord : public DbPointRecord {
  public:
    RTX_BASE_PROPS(SqlitePointRecord);
    SqlitePointRecord();
    virtual ~SqlitePointRecord();
    
    std::string basePath();
    void setBasePath(const std::string& path);
    
    bool supportsQualifiedQuery() { return true; };
  };
  
  
  
  class InfluxDbPointRecord : public DbPointRecord {
  public:
    RTX_BASE_PROPS(InfluxDbPointRecord);
    InfluxDbPointRecord();
    ~InfluxDbPointRecord();
    bool supportsQualifiedQuery() { return true; };
    void sendInfluxString(time_t time, const string& seriesId, const string& values);
  };
  
  
  class InfluxUdpPointRecord : public DbPointRecord {
  public:
    RTX_BASE_PROPS(InfluxUdpPointRecord);
    InfluxUdpPointRecord();
    ~InfluxUdpPointRecord();
    
    void sendInfluxString(time_t time, const string& seriesId, const string& values);
  };
  
}

#endif /* defined(__epanet_rtx__ConcreteDbRecords__) */
