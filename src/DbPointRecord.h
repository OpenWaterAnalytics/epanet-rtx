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

#include "BufferPointRecord.h"
#include "rtxExceptions.h"


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
    IdentifierUnitsList identifiersAndUnits(); /// simple cache

    //// lookup
    Point point(const string& id, time_t time);
    Point pointBefore(const string& id, time_t time);
    Point pointAfter(const string& id, time_t time);
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
    void dbConnect() throw(RtxException);
    bool isConnected();
    virtual bool readonly();
    virtual void setReadonly(bool readOnly);
    virtual void truncate() { }; 
    
    
    // helper class/functions
    /*--------------------------------------------*/
    //! OPC quality filter :: blacklist / whitelist
    enum OpcFilterType: unsigned int {
      OpcPassThrough   = 0, //!< all codes
      OpcBlackList     = 1, //!< explicitly exclude codes
      OpcWhiteList     = 2, //!< only allow these codes
      OpcCodesToValues = 3,  //!< convert opc codes to values
      OpcCodesToConfidence = 4 //!< convert opc codes to point confidence field
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
    
    class DbOptions {
    public:
      DbOptions(bool supportsUnitsCol, bool assignUnits, bool searchIteratively, bool singlyBoundQueries);
      bool supportsUnitsColumn;
      bool canAssignUnits;
      bool searchIteratively;
      bool supportsSinglyBoundQueries;
    };
    DbOptions _dbOptions;
    
    // subclass handy flag vars
    bool _connected;
    
    // virtuals, with default no-ops for base class opt-in support
    virtual void doConnect() throw(RtxException) = 0;
    virtual void parseConnectionString(const std::string& str) = 0;
    virtual std::string serializeConnectionString() = 0;
    virtual void refreshIds() = 0;
    
    virtual bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    Point searchPreviousIteratively(const string& id, time_t time);
    Point searchNextIteratively(const string& id, time_t time);
    
    int iterativeSearchStride;
    int iterativeSearchMaxIterations;
    
    
    // db searching prefs
    void setSearchDistance(time_t time);
    time_t searchDistance();
    
        
    
    
    
    
    
    /*--------------------------------------------*/
    
    
    
    
    
    
    bool useTransactions;
    int maxTransactionInserts;
    int transactionInsertCount;
        
    // select statements
    virtual std::vector<Point> selectRange(const std::string& id, TimeRange range)=0;
    virtual Point selectNext(const std::string& id, time_t time)=0;
    virtual Point selectPrevious(const std::string& id, time_t time)=0;
    
    // insertions or alterations: may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point)=0;
    virtual void insertRange(const std::string& id, std::vector<Point> points)=0;
    virtual void removeRecord(const std::string& id)=0;
    virtual bool insertIdentifierAndUnits(const std::string& id, Units units)=0;
    
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
    
    
  private:
    Point pointWithOpcFilter(Point p);
    std::vector<Point> pointsWithOpcFilter(std::vector<Point> points);
    
    time_t _searchDistance;
    bool _readOnly;
    std::set<unsigned int> _opcFilterCodes;
    OpcFilterType _filterType;
    std::shared_ptr<boost::signals2::mutex> _db_mutex;
    
  };

  
}

#endif
