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
    typedef enum { LOCAL, UTC } time_format_t;
    
    typedef enum {
      NO_CONNECTOR,
      wonderware_mssql,
      oracle,
      mssql
    } Sql_Connector_t;
    
    class OdbcQuery {
    public:
      std::string connectorName, singleSelect, rangeSelect, upperBound, lowerBound, timeQuery;
      std::map<int,Point::Qual_t> qualityMap;
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
      SQLHSTMT SCADAstmt, rangeStatement, lowerBoundStatement, upperBoundStatement, SCADAtimestmt;
    };
    
    static std::map<Sql_Connector_t, OdbcQuery> queryTypes();
    static Sql_Connector_t typeForName(const std::string& connector);
    
    // shared pointer and ctor/dtor
    RTX_SHARED_POINTER(OdbcPointRecord);
    OdbcPointRecord();
    virtual ~OdbcPointRecord();
    
    // public methods
    
    vector<string>dsnList();
    
    string dsn();
    void setDsn(string dsn);
    
    string uid();
    void setUid(string uid);
    
    string pwd();
    void setPwd(string pwd);
    
    string dataTable();
    void setDataTable(string table);
    
    string dataDateColumn();
    void setDataDateColumn(string col);
    
    string dataNameColumn();
    void setDataNameColumn(string col);
    
    string dataValueColumn();
    void setDataValueColumn(string col);
    
    string dataQualityColumn();
    void setDataQualityColumn(string col);
    
    string tagTable();
    void setTagTable(string table);
    
    string tagNameColumn();
    void setTagNameColumn(string col);
    
    string tagUnitsColumn();
    void setTagUnitsColumn(string col);
    
    Sql_Connector_t connectorType();
    void setConnectorType(Sql_Connector_t connectorType);
    
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);
    virtual std::vector<std::string> identifiers();
    virtual std::ostream& toStream(std::ostream &stream);
    
    void setTimeFormat(time_format_t timeFormat) { _timeFormat = timeFormat;};
    time_format_t timeFormat() { return _timeFormat; };
    
    
  protected:
    void initDsnList();
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    virtual void truncate();
    
  private:
    
    vector<string>_dsnList;
    void rebuildQueries();
    bool _connectionOk;
    std::vector<Point> pointsWithStatement(const string& id, SQLHSTMT statement, time_t startTime, time_t endTime = 0);
    
    class ScadaRecord {
    public:
      //SQLCHAR tagName[MAX_SCADA_TAG];
      SQL_TIMESTAMP_STRUCT time;
      double value;
      int quality;
      SQLLEN /*tagNameInd,*/ timeInd, valueInd, qualityInd;
      bool compareRecords(const ScadaRecord& left, const ScadaRecord& right);
    };
    
    typedef struct {
      SQL_TIMESTAMP_STRUCT start;
      SQL_TIMESTAMP_STRUCT end;
      char tagName[MAX_SCADA_TAG];
      SQLLEN startInd, endInd, tagNameInd;
    } ScadaQuery;
    
    OdbcConnection _connection;
    OdbcQuery      _querySyntax;
    OdbcTableDescription _tableDescription;
    OdbcSqlHandle _handles;
    time_format_t _timeFormat;

    ScadaRecord _tempRecord;
    ScadaQuery _query;
    Sql_Connector_t _connectorType;
    
    void bindOutputColumns(SQLHSTMT statement, ScadaRecord* record);
    SQLRETURN SQL_CHECK(SQLRETURN retVal, std::string function, SQLHANDLE handle, SQLSMALLINT type) throw(std::string);
    std::string extract_error(std::string function, SQLHANDLE handle, SQLSMALLINT type);
    // time methods
    SQL_TIMESTAMP_STRUCT sqlTime(time_t unixTime);
    time_t sql_to_time_t ( const SQL_TIMESTAMP_STRUCT& sqlTime );
    time_t boost_convert_tm_to_time_t(const struct tm &tmStruct);
  };

  
}

#endif
