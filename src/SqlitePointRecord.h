//
//  SqlitePointRecord.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__SqlitePointRecord__
#define __epanet_rtx__SqlitePointRecord__

#include <iostream>
#include <sqlite3.h>

#include "DbPointRecord.h"

#include <boost/signals2/mutex.hpp>

namespace RTX {
  
  class SqlitePointRecord : public DbPointRecord {
  public:
    RTX_SHARED_POINTER(SqlitePointRecord);
    SqlitePointRecord();
    virtual ~SqlitePointRecord();
    
    bool supportsUnitsColumn() { return true; };
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    const virtual std::map<std::string,Units> identifiersAndUnits();
    
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    
    virtual time_pair_t range(const string& id);
    
    std::string path();
    void setPath(std::string path);
    
    virtual bool supportsBoundedQueries();
    virtual void truncate();
    bool canAssignUnits();
    bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    virtual void beginBulkOperation();
    virtual void endBulkOperation();
    
  protected:
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point);
    void insertSingleInTransaction(const std::string &id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    
  private:
    sqlite3 *_dbHandle;
    std::string _selectRangeStr, _selectSingleStr, _selectNamesStr, _selectPreviousStr, _selectNextStr, _insertSingleStr, _selectFirstStr, _selectLastStr;
    
    sqlite3_stmt *_insertSingleStmt;
    
    std::string _path;
    bool _connected;
    
    bool _inTransaction, _inBulkOperation;
    int _transactionStackCount;
    int _maxTransactionStackCount;
    void checkTransactions(bool forceEndTranaction);
    
    boost::shared_ptr<boost::signals2::mutex> _mutex;
    
    bool initTables();
    void logDbError();
    
    Point pointFromStatment(sqlite3_stmt *stmt);
    std::vector<Point> pointsFromPreparedStatement(sqlite3_stmt *stmt);
    
    bool updateSchema();
    int dbSchemaVersion();
    void setDbSchemaVersion(int v);
    
  };
  
}



#endif /* defined(__epanet_rtx__SqlitePointRecord__) */
