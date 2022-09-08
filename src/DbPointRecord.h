//
//  DbPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_DbPointRecord_h
#define epanet_rtx_DbPointRecord_h

#define DB_PR_SUPER BufferPointRecord

#include <set>
#include <shared_mutex>

#include "BufferPointRecord.h"
#include "rtxExceptions.h"
#include "DbAdapter.h"


namespace RTX {
  
  /*! \class DbPointRecord
   \brief A persistence layer for databases
   
   Base class for database-connected PointRecord classes.
   
   */
  
  class DbPointRecord : public DB_PR_SUPER {
    
  public:
    // base defs
    RTX_BASE_PROPS(DbPointRecord);
    
    // ctor/dtor
    DbPointRecord();
    virtual ~DbPointRecord() {};

    // superclass overrides
    //// registration
    bool registerAndGetIdentifierForSeriesWithUnits(std::string name, Units units);    
    IdentifierUnitsList identifiersAndUnits();

    //// lookup
    Point point(const string& id, time_t time);
    Point pointBefore(const string& id, time_t time, WhereClause q = WhereClause());
    Point pointAfter(const string& id, time_t time, WhereClause q = WhereClause());
    std::vector<Point> pointsInRange(const string& id, TimeRange range);
    //// insert
    void addPoint(const string& id, Point point);
    void addPoints(const string& id, std::vector<Point> points);
    //// drop
    void reset();
    void reset(const string& id);
    
    // db-only methods
    std::string errorMessage;
    std::string connectionString();
    void setConnectionString(const std::string& str);
    void invalidate(const string& identifier);
    void dbConnect();
    bool isConnected();
    virtual bool readonly();
    virtual void setReadonly(bool readOnly);
    virtual void truncate(); 
    
    void beginBulkOperation();
    void endBulkOperation();
    
    void willQuery(TimeRange range);
    
    std::vector<Point> pointsWithQuery(const std::string& query, TimeRange range);

    
    
    // helper class/functions
    /*--------------------------------------------*/
    //! OPC quality filter :: blacklist / whitelist
    enum OpcFilterType: unsigned int {
      OpcPassThrough   = 0, //!< all codes
      OpcBlackList     = 1, //!< explicitly exclude codes
      OpcWhiteList     = 2, //!< only allow these codes
      OpcCodesToValues = 3,  //!< convert opc codes to values
      OpcCodesToConfidence = 4, //!< convert opc codes to point confidence field
      OpcNoFilter = 5
    };
    
    void setOpcFilterType(OpcFilterType type);
    OpcFilterType opcFilterType();
    
    std::set<unsigned int> opcFilterList();
    void clearOpcFilterList();
    void addOpcFilterCode(unsigned int code);
    void removeOpcFilterCode(unsigned int code);

    //exceptions specific to this class family
    class RtxDbConnectException : public RtxException {
    public:
      virtual const char* what() const throw()
      { return "Could not connect to database.\n"; }
    };
    class RtxDbRetrievalException : public RtxException {
      virtual const char* what() const throw()
      { return "Could not retrieve data.\n"; }
    };
    
  protected:
    
    DbAdapter *_adapter;
    DbAdapter::errCallback_t _errCB;
    
    Point searchPreviousIteratively(const string& id, time_t time);
    Point searchNextIteratively(const string& id, time_t time);
    
    int iterativeSearchStride;
    int iterativeSearchMaxIterations;

    IdentifierUnitsList _identifiersAndUnitsCache; /// for subs to use
    time_t _lastIdRequest;
    
    class request_t {
    public:
      TimeRange range;
      std::string id;
      request_t(std::string id, TimeRange range);
      bool contains(std::string id, time_t t);
      void clear();
    };
    request_t _last_request;
    
    class WideQueryInfo {
    public:
      const static time_t ttl = 60;
      WideQueryInfo() {_range=TimeRange(); _queryTime=0;};
      WideQueryInfo(TimeRange range) {_range = range; _queryTime = time(NULL);};
      TimeRange range() {return _range;};
      bool valid();
    private:
      time_t _queryTime;
      TimeRange _range;
    };
    
    WideQueryInfo _wideQuery;
    
    
  private:
    bool checkConnected();
    Point pointWithOpcFilter(Point p);
    std::vector<Point> pointsWithOpcFilter(std::vector<Point> points);
    
    bool _readOnly;
    bool _badConnection = false;
    std::chrono::time_point<std::chrono::system_clock> _lastFailedAttempt;
    std::set<unsigned int> _opcFilterCodes;
    OpcFilterType _filterType;
    std::shared_mutex _db_readwrite;
    
    std::function<Point(Point)> _opcFilter;
    
    
    
  };

  
}

#endif
