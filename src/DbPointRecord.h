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

#include "MapPointRecord.h"
#include "BufferPointRecord.h"
#include "rtxExceptions.h"


namespace RTX {
  
  /*! \class DbPointRecord
   \brief A persistence layer for databases
   
   Base class for database-connected PointRecord classes.
   
   */
  
  class DbPointRecord : public DB_PR_SUPER {
  public:
    
    RTX_SHARED_POINTER(DbPointRecord);
    DbPointRecord();
    virtual ~DbPointRecord() {};
    
    // end of the road for these guys; no virtuals.
    Point point(const string& id, time_t time);
    Point pointBefore(const string& id, time_t time);
    Point pointAfter(const string& id, time_t time);
    std::vector<Point> pointsInRange(const string& id, time_t startTime, time_t endTime);
    
    void addPoint(const string& id, Point point);
    void addPoints(const string& id, std::vector<Point> points);
    void reset();
    void reset(const string& id);
    virtual void invalidate(const string& identifier);
    virtual void truncate()=0; // specific implementation must override this
    
    virtual std::vector<std::pair<std::string, Units> >availableData() {std::vector<std::pair<std::string,Units> > blank; return blank;};
    
    virtual void dbConnect() throw(RtxException){};
    virtual bool isConnected(){return false;} // abstract base can't have a connection;
    
    virtual bool supportsBoundedQueries();
    
    // db searching prefs
    void setSearchDistance(time_t time);
    time_t searchDistance();
    
    
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
    
  protected:
    // fetch means cache the results
    // these have obvious default implementations, but you can override them also.
    //virtual void fetchRange(const std::string& id, time_t startTime, time_t endTime);
    //virtual void fetchNext(const std::string& id, time_t time);
    //virtual void fetchPrevious(const std::string& id, time_t time);
    
    // select just returns the results
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime)=0;
    virtual Point selectNext(const std::string& id, time_t time)=0;
    virtual Point selectPrevious(const std::string& id, time_t time)=0;
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point)=0;
    virtual void insertRange(const std::string& id, std::vector<Point> points)=0;
    virtual void removeRecord(const std::string& id)=0;
    
    
    
    
    class request_t {
    public:
      PointRecord::time_pair_t range;
      std::string id;
      request_t(std::string id, time_t start, time_t end);
      bool contains(std::string id, time_t t);
    };
    request_t request;
    
    
  private:
    std::string _connectionString;
    time_t _searchDistance;
    
    
    
  };

  
}

#endif
