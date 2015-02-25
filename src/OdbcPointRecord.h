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

#ifdef _WIN32
  #include <Windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

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
    
    
    typedef enum {
      NO_CONNECTOR,
      wonderware_mssql,
      oracle,
      mssql
    } Sql_Connector_t;
    
    class OdbcQuery {
    public:
      std::string connectorName, singleSelect, rangeSelect, upperBound, lowerBound, timeQuery;
    };
    
    class OdbcTableDescription {
    public:
      string dataTable, dataDateCol, dataNameCol, dataValueCol, dataQualityCol;
      string tagTable, tagNameCol, tagUnitsCol;
    };
    
    class OdbcConnection {
    public:
      string dsn;
      string pwd;
      string uid;
    };
    
    class OdbcSqlHandle {
    public:
      SQLHENV SCADAenv;
      SQLHDBC SCADAdbc;
    };
    
    static std::map<Sql_Connector_t, OdbcQuery> queryTypes();
    static Sql_Connector_t typeForName(const std::string& connector);
    
    // shared pointer and ctor/dtor
    RTX_SHARED_POINTER(OdbcPointRecord);
    OdbcPointRecord();
    virtual ~OdbcPointRecord();
    
    // public methods
    
    vector<string>dsnList();
    
    // public class ivars for connection and syntax info. call dbConnect() after changing any of these properites.
    OdbcConnection connection;
    OdbcTableDescription tableDescription;
    
    Sql_Connector_t connectorType();
    void setConnectorType(Sql_Connector_t connectorType);
    
    virtual bool readonly() {return true;};
    
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    
    virtual bool registerAndGetIdentifier(std::string recordName);
    virtual std::vector<std::string> identifiers();
    virtual std::ostream& toStream(std::ostream &stream);
    
    void setTimeFormat(PointRecordTime::time_format_t timeFormat) { _timeFormat = timeFormat;};
    PointRecordTime::time_format_t timeFormat() { return _timeFormat; };
    
    virtual bool supportsBoundedQueries();
    
  protected:
    void initDsnList();
    virtual bool insertIdentifier(const std::string& id) { return false; };
    // abstract stubs
    virtual void rebuildQueries(); // must call base
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime)=0;
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
    } ScadaQuery;
    
    class ScadaRecord {
    public:
      //SQLCHAR tagName[MAX_SCADA_TAG];
      SQL_TIMESTAMP_STRUCT time;
      double value;
      int quality;
      SQLLEN timeInd, valueInd, qualityInd;
      bool compareRecords(const ScadaRecord& left, const ScadaRecord& right);
    };
    
    OdbcQuery     _querySyntax;
    ScadaQuery    _query;
    OdbcSqlHandle _handles;
    ScadaRecord   _tempRecord;
    
    void bindOutputColumns(SQLHSTMT statement, ScadaRecord* record);
    
    std::vector<Point> pointsFromStatement(SQLHSTMT statement);
    std::string extract_error(std::string function, SQLHANDLE handle, SQLSMALLINT type);
    
    
    
    boost::signals2::mutex _odbcMutex;
    
  private:
    vector<string> _dsnList;
    bool _connectionOk;
    
    PointRecordTime::time_format_t _timeFormat;
    Sql_Connector_t _connectorType;
    
    
    
    SQLRETURN SQL_CHECK(SQLRETURN retVal, std::string function, SQLHANDLE handle, SQLSMALLINT type) throw(std::string);
    
    
   
  };

  
}

#endif
