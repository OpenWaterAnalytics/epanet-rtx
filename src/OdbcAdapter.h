#ifndef OdbcAdapter_hpp
#define OdbcAdapter_hpp

#include <stdio.h>
#include <vector>
#include <list>
#include <string>
#include <future>
#include <atomic>

#include "DbAdapter.h"
#include "PointRecordTime.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

namespace RTX {
  class OdbcAdapter : public DbAdapter {
  public:
    OdbcAdapter( errCallback_t cb );
    ~OdbcAdapter();
    
    const adapterOptions options() const;
    
    std::string connectionString();
    void setConnectionString(const std::string& con);
    
    void doConnect();
    
    IdentifierUnitsList idUnitsList();
    
    // TRANSACTIONS
    void beginTransaction();
    void endTransaction();
    bool inTransaction() {return _inTransaction;};
    
    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time, WhereClause q = WhereClause());
    Point selectPrevious(const std::string& id, time_t time, WhereClause q = WhereClause());
    
    // CREATE
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    void insertSingle(const std::string& id, Point point);
    void insertRange(const std::string& id, std::vector<Point> points);
    
    // UPDATE
    bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    // DELETE
    void removeRecord(const std::string& id);
    void removeAllRecords();
    
    
    // odbc-specific methods
    static std::list<std::string> listDrivers();
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
    
  private:
    //** types **//
    class OdbcQuery {
    public:
      std::string metaSelect, rangeSelect;
    };
    
    class OdbcConnection {
    public:
      std::string driver;
      std::string connectionString;
    };
    
    class OdbcSqlHandle {
    public:
      SQLHENV SCADAenv;
      SQLHDBC SCADAdbc;
    };
    
    class ScadaRecord {
    public:
      //SQLCHAR tagName[MAX_SCADA_TAG];
      SQL_TIMESTAMP_STRUCT time;
      double value;
      int quality;
      SQLLEN timeInd, valueInd, qualityInd;
      bool compareRecords(const ScadaRecord& left, const ScadaRecord& right);
    };
    
    
    //** ivars **//
    OdbcConnection _connection;
    OdbcQuery _querySyntax;
    OdbcSqlHandle _handles;
    ScadaRecord _tempRecord;
    std::atomic<bool> _isConnecting;
    std::future<bool> _connectFuture;
    std::vector<std::string> _dsnList;
    std::string _specifiedTimeZoneString;
    boost::local_time::time_zone_ptr _specifiedTimeZone;
    PointRecordTime::time_format_t _timeFormat;
    std::atomic<bool> _inTransaction;
    
    //** methods **//
    void initDsnList();
    std::string stringQueryForRange(const std::string& id, TimeRange range);
    std::vector<Point> pointsFromStatement(SQLHSTMT statement);
    void bindOutputColumns(SQLHSTMT statement, ScadaRecord* record);
  };
}




#endif /* OdbcAdapter_hpp */
