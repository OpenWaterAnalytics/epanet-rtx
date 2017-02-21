#ifndef DbAdapter_h
#define DbAdapter_h

#include <boost/atomic.hpp>
#include <mutex>
#include <vector>

#include "Point.h"
#include "Units.h"
#include "TimeRange.h"
#include "IdentifierUnitsList.h"

// virtual interface for database adapters. includes configuration object which 
// declares what the implementation supports.

namespace RTX {
  
#define _RTX_DB_SCOPED_LOCK std::lock_guard<std::mutex> lock(_dbMtx)
  
  class DbAdapter {
  public:
    // utility types
    struct adapterOptions {
      bool supportsUnitsColumn;
      bool canAssignUnits;
      bool searchIteratively;
      bool supportsSinglyBoundQuery;
      bool implementationReadonly;
    };
    
    typedef std::function<void(const std::string)> errCallback_t;
    
    DbAdapter( errCallback_t cb ) : _errCallback(cb) { };
    virtual ~DbAdapter() { };
    
    virtual const adapterOptions options() const = 0;
    
    virtual std::string connectionString() = 0;
    virtual void setConnectionString(const std::string& con) = 0;
    
    virtual void doConnect() = 0;
    bool adapterConnected() { return _connected; };
    
    virtual IdentifierUnitsList idUnitsList() = 0;
    
    // TRANSACTIONS
    virtual void beginTransaction() = 0;
    virtual void endTransaction() = 0;
    
    // READ
    virtual std::vector<Point> selectRange(const std::string& id, TimeRange range) = 0;
    virtual Point selectNext(const std::string& id, time_t time) = 0;
    virtual Point selectPrevious(const std::string& id, time_t time) = 0;
    
    // CREATE
    virtual bool insertIdentifierAndUnits(const std::string& id, Units units) = 0;
    virtual void insertSingle(const std::string& id, Point point) = 0;
    virtual void insertRange(const std::string& id, std::vector<Point> points) = 0;
    
    // UPDATE
    virtual bool assignUnitsToRecord(const std::string& name, const Units& units) = 0;
    
    // DELETE
    virtual void removeRecord(const std::string& id) = 0;
    virtual void removeAllRecords() = 0;
    
    
  protected:
    boost::atomic<bool> _connected;
    std::mutex _dbMtx;
    std::function<void(const std::string errMsg)> _errCallback;
    
    
  };
}


#endif /* DbAdapter_h */
