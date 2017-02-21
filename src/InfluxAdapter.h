#ifndef InfluxAdapter_hpp
#define InfluxAdapter_hpp

#include <stdio.h>
#include <future>
#include <cpprest/uri.h>

#include "DbAdapter.h"

namespace RTX {
  class InfluxAdapter : public DbAdapter {
  public:
    InfluxAdapter( errCallback_t cb );
    virtual ~InfluxAdapter();
        
    void setConnectionString(const std::string& con);
    
    
    // TRANSACTIONS
    void beginTransaction();
    void endTransaction();
    
    
    // CREATE
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    void insertSingle(const std::string& id, Point point);
    void insertRange(const std::string& id, std::vector<Point> points);
    
    // UPDATE
    bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    
    // utility only for influx
    void sendInfluxString(time_t time, const std::string& seriesId, const std::string& values);
    
  protected:
    // compulsory overrides
    virtual std::string formatTimestamp(time_t t) = 0;
    virtual void sendPointsWithString(const std::string& content) = 0;
    virtual size_t maxTransactionLines() = 0;
    
    // sub types
    class connectionInfo {
    public:
      connectionInfo();
      std::string proto, host, user, pass, db;
      int port;
    };
    connectionInfo conn;
    
    

    std::vector<std::string> insertionLinesFromPoints(const std::string& tsName, std::vector<Point> points);
    std::string influxIdForTsId(const std::string& id);
    std::vector<std::string> _transactionLines;
    IdentifierUnitsList _idCache;
    bool _inTransaction;
    
  private:
    void commitTransactionLines();
    
    
  };
  
  class ITaskWrapper {
    // empty implementation for async task sub
  };
  
  class InfluxTcpAdapter : public InfluxAdapter {
  public:
    InfluxTcpAdapter( errCallback_t cb );
    ~InfluxTcpAdapter();
    
    const adapterOptions options() const;
    std::string connectionString();
    void doConnect();
    IdentifierUnitsList idUnitsList();
    
    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    // DELETE
    void removeRecord(const std::string& id);
    void removeAllRecords();
    
  protected:
    size_t maxTransactionLines();
    void sendPointsWithString(const std::string& content);
    std::string formatTimestamp(time_t t);
    
  private:
    class Query {
    public:
      std::vector<std::string> select,where;
      std::string from,order;
      std::string selectStr();
      std::string nameAndWhereClause();
    };
    
    std::shared_ptr<ITaskWrapper> _sendTask;
    web::uri uriForQuery(const std::string& query, bool withTimePrecision = true);
    Query queryPartsFromMetricId(const std::string& name);
  };
  
  
  class InfluxUdpAdapter : public InfluxAdapter {
  public:
    InfluxUdpAdapter( errCallback_t cb );
    ~InfluxUdpAdapter();
    
    const adapterOptions options() const;
    std::string connectionString();
    void doConnect();
    IdentifierUnitsList idUnitsList();
    
    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    // DELETE
    void removeRecord(const std::string& id);
    void removeAllRecords();
    
  protected:
    size_t maxTransactionLines();
    void sendPointsWithString(const std::string& content);
    std::string formatTimestamp(time_t t);
    
  private:
    std::future<void> _sendFuture;
  };
  
  
  
}



#endif /* InfluxAdapter_hpp */
