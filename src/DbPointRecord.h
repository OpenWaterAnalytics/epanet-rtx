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
    
    class Query {
    public:
      std::vector<std::string> select,where;
      std::string from,order;
      std::string selectStr();
      std::string nameAndWhereClause();
    };
    
    
    RTX_BASE_PROPS(DbPointRecord);
    DbPointRecord();
    virtual ~DbPointRecord() {};
    
    virtual std::string connectionString() {return "";};
    virtual void setConnectionString(const std::string& string) {};
    
    virtual bool readonly();
    virtual void setReadonly(bool readOnly);
    
    virtual bool supportsUnitsColumn() = 0; /// subs must implement!
    
    virtual bool registerAndGetIdentifierForSeriesWithUnits(std::string name, Units units);
    virtual IdentifierUnitsList identifiersAndUnits(); /// simple cache
    
    // end of the road for these guys; no virtuals.
    Point point(const string& id, time_t time);
    Point pointBefore(const string& id, time_t time);
    Point pointAfter(const string& id, time_t time);
    std::vector<Point> pointsInRange(const string& id, TimeRange range);
    
    void addPoint(const string& id, Point point);
    void addPoints(const string& id, std::vector<Point> points);
    void reset();
    void reset(const string& id);
    virtual void invalidate(const string& identifier);
    virtual void truncate()=0; // specific implementation must override this
        
    virtual void dbConnect() throw(RtxException){};
    virtual bool isConnected(){return false;} // abstract base can't have a connection;
    
    virtual bool canAssignUnits();
    virtual bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    virtual bool supportsSinglyBoundedQueries() = 0; //! override for dbs that support LIMIT
    virtual bool shouldSearchIteratively()  = 0;     //! override for dbs with slow LIMIT queries or no support.
    Point searchPreviousIteratively(const string& id, time_t time);
    Point searchNextIteratively(const string& id, time_t time);
    
    int iterativeSearchStride;
    int iterativeSearchMaxIterations;
    
    
    // db searching prefs
    void setSearchDistance(time_t time);
    time_t searchDistance();
    
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
    
    Point pointWithOpcFilter(Point p);
    std::vector<Point> pointsWithOpcFilter(std::vector<Point> points);
    
    
    
    
    /*--------------------------------------------*/
    
    
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
    
    std::string errorMessage;
    
    bool useTransactions;
    int maxTransactionInserts;
    int transactionInsertCount;
    
  protected:
    
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
    time_t _searchDistance;
    bool _readOnly;
    std::set<unsigned int> _opcFilterCodes;
    OpcFilterType _filterType;
    std::shared_ptr<boost::signals2::mutex> _db_mutex;
    
  };

  
}

#endif
