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
    
    class odbc_query_t {
    public:
      std::string connectorName, singleSelect, rangeSelect, upperBound, lowerBound, timeQuery;
      std::map<int,Point::Qual_t> qualityMap;
    };
    
    static std::map<Sql_Connector_t, odbc_query_t> queryTypes();
    static Sql_Connector_t typeForName(const std::string& connector);
    
    // shared pointer and ctor/dtor
    RTX_SHARED_POINTER(OdbcPointRecord);
    OdbcPointRecord();
    virtual ~OdbcPointRecord();
    
    // public methods
    void setTableColumnNames(const std::string& table, const std::string& dateCol, const std::string& tagCol, const std::string& valueCol, const std::string& qualityCol);
    void setConnectorType(Sql_Connector_t connectorType);
    Sql_Connector_t connectorType();
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);
    virtual std::vector<std::string> identifiers();
    virtual std::ostream& toStream(std::ostream &stream);
    
    void setSingleSelectQuery(const std::string& query) {_singleSelect = query;};
    void setRangeSelectQuery(const std::string& query) {_rangeSelect = query;};
    void setLowerBoundSelectQuery(const std::string& query) {_lowerBoundSelect = query;};
    void setUpperBoundSelectQuery(const std::string& query) {_upperBoundSelect = query;};
    void setTimeQuery(const std::string& query) {_timeQuery = query;};
    
    void setTimeFormat(time_format_t timeFormat) { _timeFormat = timeFormat;};
    time_format_t timeFormat() { return _timeFormat; };
    
    std::string singleSelectQuery() {return _singleSelect;};
    std::string rangeSelectQuery() {return _rangeSelect;};
    std::string loweBoundSelectQuery() {return _lowerBoundSelect;};
    std::string upperBoundSelectQuery() {return _upperBoundSelect;};
    std::string timeQuery() {return _timeQuery;};
    
  protected:
    // fetch means cache the results
    //virtual void fetchRange(const std::string& id, time_t startTime, time_t endTime);
    //virtual void fetchNext(const std::string& id, time_t time);
    //virtual void fetchPrevious(const std::string& id, time_t time);
    
    // select just returns the results (no caching)
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    virtual void truncate();
    
  private:
    bool _connectionOk;
    std::map<int,Point::Qual_t> _qualityMap;
    std::vector<Point> pointsWithStatement(const string& id, SQLHSTMT statement, time_t startTime, time_t endTime = 0);
    typedef struct {
      //SQLCHAR tagName[MAX_SCADA_TAG];
      SQL_TIMESTAMP_STRUCT time;
      double value;
      int quality;
      SQLLEN /*tagNameInd,*/ timeInd, valueInd, qualityInd;
    } ScadaRecord;
    typedef struct {
      SQL_TIMESTAMP_STRUCT start;
      SQL_TIMESTAMP_STRUCT end;
      char tagName[MAX_SCADA_TAG];
      SQLLEN startInd, endInd, tagNameInd;
    } ScadaQuery;
    
    std::string _tableName, _dateCol, _tagCol, _valueCol, _qualityCol;
    std::string _singleSelect, _rangeSelect, _upperBoundSelect, _lowerBoundSelect, _timeQuery;
    time_format_t _timeFormat;
    SQLHENV _SCADAenv;
    SQLHDBC _SCADAdbc;
    SQLHSTMT _SCADAstmt, _rangeStatement, _lowerBoundStatement, _upperBoundStatement, _SCADAtimestmt;
    std::string _dsn;
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
