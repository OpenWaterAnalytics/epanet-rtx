//
//  DbPointRecord.h
//  epanet-rtx
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef epanet_rtx_DbPointRecord_h
#define epanet_rtx_DbPointRecord_h

#include "PointRecord.h"
#include "rtxExceptions.h"

namespace RTX {
  
  /*! \class DbPointRecord
   \brief A persistence layer for databases
   
   Base class for database-connected PointRecord classes.
   
   */
  
  class DbPointRecord : public PointRecord {
  public:
    RTX_SHARED_POINTER(DbPointRecord);
    DbPointRecord() {};
    virtual ~DbPointRecord() {};
    
    string singleSelectQuery() {return _singleSelect;};
    string rangeSelectQuery() {return _rangeSelect;};
    string loweBoundSelectQuery() {return _lowerBoundSelect;};
    string upperBoundSelectQuery() {return _upperBoundSelect;};
    string timeQuery() {return _timeQuery;};
    
    void setSingleSelectQuery(string query) {_singleSelect = query;};
    void setRangeSelectQuery(string query) {_rangeSelect = query;};
    void setLowerBoundSelectQuery(string query) {_lowerBoundSelect = query;};
    void setUpperBoundSelectQuery(string query) {_upperBoundSelect = query;};
    void setTimeQuery(string query) {_timeQuery = query;};
    
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
    
  private:
    string _singleSelect, _rangeSelect, _upperBoundSelect, _lowerBoundSelect, _timeQuery;
    
  };

  
}

#endif
