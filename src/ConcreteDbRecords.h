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
  
  
  class PiPointRecord : public DbPointRecord {
  public:
    RTX_BASE_PROPS(PiPointRecord);
    PiPointRecord();
    ~PiPointRecord();
    void setTagSearchPath(const std::string& path);
    std::string tagSearchPath();
    void setConversions(const std::string& conversions);
    std::string conversions();
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
  
  
  class OdbcPointRecord : public DbPointRecord {
  public:
    RTX_BASE_PROPS(OdbcPointRecord);
    OdbcPointRecord();
    ~OdbcPointRecord();
    
    static std::list<std::string> driverList();
    
    void setTimeFormat(PointRecordTime::time_format_t timeFormat);
    PointRecordTime::time_format_t timeFormat();
    
    std::string timeZoneString();
    void setTimeZoneString(const std::string& tzStr);
    
    std::string driver();
    void setDriver(const std::string& driver);
    
    std::string metaQuery();
    void setMetaQuery(const std::string& meta);
    
    std::string rangeQuery();
    void setRangeQuery(const std::string& range);
    
  };
  
}

#endif /* defined(__epanet_rtx__ConcreteDbRecords__) */
