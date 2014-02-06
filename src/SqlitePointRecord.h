//
//  SqlitePointRecord.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/4/13.
//
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
    
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);
    virtual std::vector<std::string> identifiers();
    virtual std::vector<std::pair<std::string, Units> >availableData();
    
    virtual time_pair_t range(const string& id);
    
    std::string path();
    void setPath(std::string path);
    
    virtual bool supportsBoundedQueries();

    
  protected:
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point);
    void insertSingleInTransaction(const std::string &id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    virtual void truncate();
    
  private:
    sqlite3 *_dbHandle;
    sqlite3_stmt *_selectRangeStmt, *_selectSingleStmt, *_selectNamesStmt, *_selectPreviousStmt, *_selectNextStmt, *_insertSingleStmt, *_selectFirstStmt, *_selectLastStmt;
    
    std::string _path;
    bool _connected;
    
    bool _inTransaction;
    int _transactionStackCount;
    int _maxTransactionStackCount;
    void checkTransactions(bool forceEndTranaction);
    
    boost::shared_ptr<boost::signals2::mutex> _mutex;
    
    bool initTables();
    void logDbError();
    
    Point pointFromStatment(sqlite3_stmt *stmt);
    std::vector<Point> pointsFromPreparedStatement(sqlite3_stmt *stmt);
    
  };
  
}



#endif /* defined(__epanet_rtx__SqlitePointRecord__) */
