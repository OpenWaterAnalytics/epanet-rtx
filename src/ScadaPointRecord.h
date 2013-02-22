//
//  ScadaPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_ScadaPointRecord_h
#define epanet_rtx_ScadaPointRecord_h

#define MAX_SCADA_TAG 50

#include "DbPointRecord.h"

#include <deque>

#include <sql.h>
#include <sqlext.h>

namespace RTX {
  
  /*! \class ScadaPointRecord
   \brief A persistence class for SCADA databases
   
   Primarily to be used for data acquisition. Polls an ODBC-based SCADA connection for data and creates Points from that data.
   
   */
  
  class ScadaPointRecord : public DbPointRecord {
  public:
    RTX_SHARED_POINTER(ScadaPointRecord);
    ScadaPointRecord();
    virtual ~ScadaPointRecord();
    
    void setSyntax(const string& table, const string& dateCol, const string& tagCol, const string& valueCol, const string& qualityCol);
    virtual void connect() throw(RtxException);
    virtual bool isConnected();
    virtual std::string registerAndGetIdentifier(std::string recordName);
    virtual std::vector<std::string> identifiers();
    virtual std::ostream& toStream(std::ostream &stream);
    
  protected:
    // fetch means cache the results
    virtual void fetchRange(const std::string& id, time_t startTime, time_t endTime);
    virtual void fetchNext(const std::string& id, time_t time);
    virtual void fetchPrevious(const std::string& id, time_t time);
    
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
    std::vector<Point> pointsWithStatement(const string& id, SQLHSTMT statement, time_t startTime, time_t endTime = 0);
    typedef struct {
      SQLCHAR tagName[MAX_SCADA_TAG];
      SQL_TIMESTAMP_STRUCT time;
      double value;
      int quality;
      SQLLEN tagNameInd, timeInd, valueInd, qualityInd;
    } ScadaRecord;
    typedef struct {
      SQL_TIMESTAMP_STRUCT start;
      SQL_TIMESTAMP_STRUCT end;
      char tagName[MAX_SCADA_TAG];
      SQLLEN startInd, endInd, tagNameInd;
    } ScadaQuery;
    
    SQLHENV _SCADAenv;
    SQLHDBC _SCADAdbc;
    SQLHSTMT _SCADAstmt, _rangeStatement, _lowerBoundStatement, _upperBoundStatement, _SCADAtimestmt;
    std::string _dsn;
    ScadaRecord _tempRecord;
    ScadaQuery _query;
    
    void bindOutputColumns(SQLHSTMT statement, ScadaRecord* record);
    SQL_TIMESTAMP_STRUCT sqlTime(time_t unixTime);
    time_t unixTime(SQL_TIMESTAMP_STRUCT sqlTime);
    SQLRETURN SQL_CHECK(SQLRETURN retVal, std::string function, SQLHANDLE handle, SQLSMALLINT type) throw(std::string);
    std::string extract_error(std::string function, SQLHANDLE handle, SQLSMALLINT type);
    time_t time_to_epoch ( const struct tm *ltm, int utcdiff );
  };

  
}

#endif
