#ifndef OpcAdapter_hpp
#define OpcAdapter_hpp

#include <stdio.h>
#include <vector>
#include <list>
#include <string>
#include <future>
#include <boost/atomic.hpp>

#include "DbAdapter.h"
#include "PointRecordTime.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>

#include <open62541.h>

namespace RTX {
  class OpcAdapter : public DbAdapter {
  public:
    OpcAdapter( errCallback_t cb );
    ~OpcAdapter();
    
    const adapterOptions options() const;
    
    std::string connectionString();
    void setConnectionString(const std::string& con);
    
    void doConnect();
    
    IdentifierUnitsList idUnitsList();
    
    // TRANSACTIONS
    void beginTransaction();
    void endTransaction();
    
    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    // CREATE
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    void insertSingle(const std::string& id, Point point);
    void insertRange(const std::string& id, std::vector<Point> points);
    
    // UPDATE
    bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    // DELETE
    void removeRecord(const std::string& id);
    void removeAllRecords();
    
    
    // opc-specific methods
    
  private:
    class connectionInfo {
    public:
      connectionInfo();
      std::string proto, host, user, pass;
      int port;
      bool validate;
    };
    connectionInfo _conn;
    
    UA_Client *_client;
    
    std::map<std::string, UA_NodeId> _nodes;
    
  };
}



#endif /* OpcAdapter_hpp */
