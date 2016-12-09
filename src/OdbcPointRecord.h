//
//  OdbcPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_OdbcPointRecord_h
#define epanet_rtx_OdbcPointRecord_h

#define MAX_SCADA_TAG 50

#include "DbPointRecord.h"

#include <deque>
#include <list>

#ifdef _WIN32
  #include <Windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <future>
#include <boost/atomic.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
using boost::signals2::mutex;
using boost::interprocess::scoped_lock;

#include "PointRecordTime.h"

namespace RTX {
  
  /*! \class OdbcPointRecord
   \brief A persistence class for SCADA databases
   
   Primarily to be used for data acquisition. Polls an ODBC-based SCADA connection for data and creates Points from that data.
   
   */
  
  using std::string;
  using std::vector;
  
  class OdbcPointRecord : public DbPointRecord {
  public:
    // types
    
    class OdbcQuery {
    public:
      std::string metaSelect, rangeSelect;
    };
    
    class OdbcConnection {
    public:
      string driver;
      string conInformation;
    };
    
    class OdbcSqlHandle {
    public:
      SQLHENV SCADAenv;
      SQLHDBC SCADAdbc;
    };
    
    // shared pointer and ctor/dtor
    RTX_BASE_PROPS(OdbcPointRecord);
    OdbcPointRecord();
    virtual ~OdbcPointRecord();
    
    // public methods
    static std::list<std::string>driverList();
    
    
    
    // public class ivars for connection and syntax info. call dbConnect() after changing any of these properites.
    OdbcConnection connection;
    OdbcQuery querySyntax;
    
    std::string connectionString() {return connection.conInformation;};
    void setConnectionString(const std::string& con) {connection.conInformation = con;};

    virtual bool readonly() {return true;};
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    
    virtual IdentifierUnitsList identifiersAndUnits();
    virtual std::ostream& toStream(std::ostream &stream);
    
    void setTimeFormat(PointRecordTime::time_format_t timeFormat);
    PointRecordTime::time_format_t timeFormat();
    
    std::string timeZoneString();
    void setTimeZoneString(const std::string& tzStr);
    
    virtual bool shouldSearchIteratively() { return true; };
    virtual bool supportsSinglyBoundedQueries();
    bool supportsUnitsColumn() { return false; };
    
  protected:
    void initDsnList();
    virtual bool insertIdentifierAndUnits(const std::string& id, Units units){ return false; };
    // abstract stubs
    virtual void rebuildQueries(); // must call base
    virtual std::vector<Point> selectRange(const std::string& id, TimeRange range)=0;
    virtual Point selectNext(const std::string& id, time_t time)=0;
    virtual Point selectPrevious(const std::string& id, time_t time)=0;
    
    
    
    // insertions or alterations may choose to ignore / deny
    // pseudo-abstract base is no-op
    virtual void insertSingle(const std::string& id, Point point) {};
    virtual void insertRange(const std::string& id, std::vector<Point> points) {};
    virtual void removeRecord(const std::string& id) {};
    virtual void truncate() {};
    
    bool checkConnected();
    
    // time methods
    //SQL_TIMESTAMP_STRUCT sqlTime(time_t unixTime);
    //time_t sql_to_time_t ( const SQL_TIMESTAMP_STRUCT& sqlTime );
    //time_t boost_convert_tm_to_time_t(const struct tm &tmStruct);
    
    typedef struct {
      SQL_TIMESTAMP_STRUCT start;
      SQL_TIMESTAMP_STRUCT end;
      char tagName[MAX_SCADA_TAG];
      SQLLEN startInd, endInd, tagNameInd;
    } ScadaQueryParameters;
    
    class ScadaRecord {
    public:
      //SQLCHAR tagName[MAX_SCADA_TAG];
      SQL_TIMESTAMP_STRUCT time;
      double value;
      int quality;
      SQLLEN timeInd, valueInd, qualityInd;
      bool compareRecords(const ScadaRecord& left, const ScadaRecord& right);
    };
    
    
    ScadaQueryParameters    _query;
    OdbcSqlHandle           _handles;
    ScadaRecord             _tempRecord;
    
    void bindOutputColumns(SQLHSTMT statement, ScadaRecord* record);
    
    std::vector<Point> pointsFromStatement(SQLHSTMT statement);
    std::string extract_error(std::string function, SQLHANDLE handle, SQLSMALLINT type);
    
    
    
    boost::signals2::mutex _odbcMutex;
    
    PointRecordTime::time_format_t _timeFormat;
    boost::local_time::time_zone_ptr _specifiedTimeZone;
    std::string _specifiedTimeZoneString;
    
  private:
    vector<string> _dsnList;
    bool _connectionOk;
    SQLRETURN SQL_CHECK(SQLRETURN retVal, std::string function, SQLHANDLE handle, SQLSMALLINT type) throw(std::string);
    boost::atomic<bool> _isConnecting;
    std::future<bool> _connectFuture;
   
  };

  
}

#endif
