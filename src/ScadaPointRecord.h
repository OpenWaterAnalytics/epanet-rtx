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
    virtual std::vector<std::string> identifiers();
    virtual bool isPointAvailable(const string& identifier, time_t time);
    virtual Point point(const string& identifier, time_t time);
    virtual Point pointBefore(const string& identifier, time_t time);
    virtual Point pointAfter(const string& identifier, time_t time);
    virtual void hintAtRange(const string& identifier, time_t start, time_t end);
    virtual std::vector<Point> pointsInRange(const string& identifier, time_t startTime, time_t endTime);
    virtual void addPoint(const string& identifier, Point point) {};
    virtual void addPoints(const string& identifier, std::vector<Point> points) {};
    virtual void reset() {};
  protected:
    virtual std::ostream& toStream(std::ostream &stream);
    
  private:
    bool _connectionOk;
    std::deque<Point> pointsWithStatement(const string& identifier, SQLHSTMT statement, time_t startTime, time_t endTime = 0);
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
    std::string _dataPointQuery, _dataRangeQuery, _dataLowerBoundQuery, _dataUpperBoundQuery, _timeQuery;
    ScadaRecord _tempRecord;
    ScadaQuery _query;
    // convenience class for syntax
    class ScadaSyntax_t {
    public:
      std::string table;
      std::string date;
      std::string tag;
      std::string value;
      std::string quality;
    };
    ScadaSyntax_t _syntax;
    // convenience class for the range statement optmization
    class Hint_t {
    public:
      Hint_t();
      void clear();
      std::string identifier;
      std::pair<time_t, time_t> range;
      std::deque<Point> cache;
    };
    Hint_t _hint;
    
    
    void bindOutputColumns(SQLHSTMT statement, ScadaRecord* record);
    SQL_TIMESTAMP_STRUCT sqlTime(time_t unixTime);
    time_t unixTime(SQL_TIMESTAMP_STRUCT sqlTime);
    SQLRETURN SQL_CHECK(SQLRETURN retVal, std::string function, SQLHANDLE handle, SQLSMALLINT type) throw(std::string);
    std::string extract_error(std::string function, SQLHANDLE handle, SQLSMALLINT type);
    time_t time_to_epoch ( const struct tm *ltm, int utcdiff );
  };

  
}

#endif
