#ifndef I_InfluxDbPointRecord_h
#define I_InfluxDbPointRecord_h

#include <stdio.h>
#include <boost/atomic.hpp>
#include <thread>
#include <mutex>

#include "DbPointRecord.h"

namespace RTX {
  class I_InfluxDbPointRecord : public DbPointRecord {
  public:

    // public non-override
    I_InfluxDbPointRecord();
    bool isConnected() {return _connected;};
    void beginBulkOperation();
    void endBulkOperation();
    void commitTransactionLines();
    virtual void truncate() {};
    
    virtual size_t maxTransactionLines() { return 1; };
    
    void sendInfluxString(time_t time, const std::string& seriesId, const std::string& values);
    
    void insertSingle(const std::string& id, Point point);
    void insertRange(const std::string& id, std::vector<Point> points);
    
    // optional overrides
    virtual bool supportsSinglyBoundedQueries() {return true;};
    virtual bool shouldSearchIteratively() {return false;};
    virtual bool supportsUnitsColumn() {return true;};
    virtual bool insertIdentifierAndUnits(const std::string& id, Units units);
    virtual IdentifierUnitsList identifiersAndUnits();
    
    
    
    
  protected:
    // sub types
    class connectionInfo {
    public:
      connectionInfo();
      std::string proto, host, user, pass, db;
      int port;
    };
    connectionInfo conn;
    
    class Query {
    public:
      std::vector<std::string> select,where;
      std::string from,order;
      std::string selectStr();
      std::string nameAndWhereClause();
    };
    
    
    virtual void parseConnectionString(const std::string& str);
    virtual std::string serializeConnectionString();
    // instance method interfaces - optional override
    virtual std::vector<Point> selectRange(const std::string& id, TimeRange range) {return std::vector<Point>();};
    virtual Point selectNext(const std::string& id, time_t time) {return Point();};
    virtual Point selectPrevious(const std::string& id, time_t time) {return Point();};
    virtual void removeRecord(const std::string& id) {};
    virtual std::string formatTimestamp(time_t t) { return std::to_string(t); };
    
    // must override
    virtual void sendPointsWithString(const std::string& content) = 0;
    
    // static methods
    std::vector<std::string> insertionLinesFromPoints(const std::string& tsName, std::vector<Point> points);
    
    // utility
    std::string influxIdForTsId(const std::string& id);
    
    // protected ivars, thread locking, & async transactions
    boost::atomic<bool> _connected;
    std::mutex _influxMutex;
    std::vector<std::string> _transactionLines;
    
  private:
    
    
  };
}


#endif /* I_InfluxDbPointRecord_h */
