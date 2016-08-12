//
//  OdbcDirectPointRecord.h
//  epanet-rtx
//
//  Open Water Analytics [wateranalytics.org]
//  See README.md and license.txt for more information
//

#ifndef __epanet_rtx__OdbcDirectPointRecord__
#define __epanet_rtx__OdbcDirectPointRecord__

#include <iostream>

#include "OdbcPointRecord.h"

namespace RTX {
  class OdbcDirectPointRecord : public OdbcPointRecord {
    
  public:
    RTX_BASE_PROPS(OdbcDirectPointRecord);
    OdbcDirectPointRecord();
    virtual ~OdbcDirectPointRecord();
    IdentifierUnitsList identifiersAndUnits();
    
  protected:
    void dbConnect() throw(RtxException);
    std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    
  private:
    typedef enum {OdbcQueryBoundLower,OdbcQueryBoundUpper} OdbcQueryBoundType;
    std::string stringQueryForRange(const std::string& id, time_t start, time_t end);
    std::string stringQueryForSinglyBoundedRange(const std::string& id, time_t bound, OdbcQueryBoundType boundType);
    std::string stringQueryForIds();
//    SQLHSTMT _directTagQueryStmt, _directRangeQueryStmt;
    
  };
}



#endif /* defined(__epanet_rtx__OdbcDirectPointRecord__) */
