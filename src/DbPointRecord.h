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

#include "BufferPointRecord.h"
#include "rtxExceptions.h"

namespace RTX {
  
  /*! \class DbPointRecord
   \brief A persistence layer for databases
   
   Base class for database-connected PointRecord classes.
   
   */
  
  class DbPointRecord : public DB_PR_SUPER {
  public:
    typedef enum { LOCAL, UTC } time_format_t;
    RTX_SHARED_POINTER(DbPointRecord);
    DbPointRecord();
    virtual ~DbPointRecord() {};
    
    
    
    // end of the road for these guys
    bool isPointAvailable(const string& id, time_t time);
    Point point(const string& id, time_t time);
    Point pointBefore(const string& id, time_t time);
    Point pointAfter(const string& id, time_t time);
    std::vector<Point> pointsInRange(const string& id, time_t startTime, time_t endTime);
    void addPoint(const string& id, Point point);
    void addPoints(const string& id, std::vector<Point> points);
    void reset();
    void reset(const string& id);
    //Point firstPoint(const string& id);
    //Point lastPoint(const string& id);
    
    
    
    // pointRecord methods to override
    virtual std::string registerAndGetIdentifier(std::string recordName)=0;
    virtual std::vector<std::string> identifiers()=0;
    
    // db connection
    virtual void connect() throw(RtxException)=0;
    virtual bool isConnected()=0;
    
    
    
    // getters & setters
    
    void setTimeFormat(time_format_t timeFormat) { _timeFormat = timeFormat;};
    time_format_t timeFormat() { return _timeFormat; };
    
    std::string singleSelectQuery() {return _singleSelect;};
    std::string rangeSelectQuery() {return _rangeSelect;};
    std::string loweBoundSelectQuery() {return _lowerBoundSelect;};
    std::string upperBoundSelectQuery() {return _upperBoundSelect;};
    std::string timeQuery() {return _timeQuery;};
    
    void setSingleSelectQuery(const std::string& query) {_singleSelect = query;};
    void setRangeSelectQuery(const std::string& query) {_rangeSelect = query;};
    void setLowerBoundSelectQuery(const std::string& query) {_lowerBoundSelect = query;};
    void setUpperBoundSelectQuery(const std::string& query) {_upperBoundSelect = query;};
    void setTimeQuery(const std::string& query) {_timeQuery = query;};
    
    
    
    
    
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
    // fetch means cache the results
    // these have obvious default implementations, but you can override them also.
    virtual void fetchRange(const std::string& id, time_t startTime, time_t endTime);
    virtual void fetchNext(const std::string& id, time_t time);
    virtual void fetchPrevious(const std::string& id, time_t time);
    
    // select just returns the results (no caching)
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime)=0;
    virtual Point selectNext(const std::string& id, time_t time)=0;
    virtual Point selectPrevious(const std::string& id, time_t time)=0;
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point)=0;
    virtual void insertRange(const std::string& id, std::vector<Point> points)=0;
    virtual void removeRecord(const std::string& id)=0;
    virtual void truncate()=0;
    
    PointRecord::time_pair_t reqRange;
    
  private:
    std::string _singleSelect, _rangeSelect, _upperBoundSelect, _lowerBoundSelect, _timeQuery;
    time_format_t _timeFormat;
    
  };

  
}

#endif
