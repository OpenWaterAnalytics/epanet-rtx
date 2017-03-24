#ifndef SqliteAdapter_hpp
#define SqliteAdapter_hpp

#include <stdio.h>

#include "DbAdapter.h"

#include <sqlite_modern_cpp.h>
#include <sqlite3.h>


namespace RTX {
  class SqliteAdapter : public DbAdapter {
  public:
    SqliteAdapter( errCallback_t cb );
    ~SqliteAdapter();
    
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
    
    std::string basePath;
    
  private:
    std::shared_ptr<sqlite::database> _db;        
    std::string _path;
    
    bool _inTransaction;
    int _transactionStackCount;
    int _maxTransactionStackCount;
    void checkTransactions();
    void commit();
    
    bool initTables();
    void insertSingleInTransaction(const std::string &id, Point point);
    
    bool updateSchema();
    int dbSchemaVersion();
    void setDbSchemaVersion(int v);
    
    std::map<std::string,int> _metaCache;

    
  };
}





#endif /* SqliteAdapter_hpp */
