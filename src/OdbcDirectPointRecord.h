//
//  OdbcDirectPointRecord.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 12/31/13.
//
//

#ifndef __epanet_rtx__OdbcDirectPointRecord__
#define __epanet_rtx__OdbcDirectPointRecord__

#include <iostream>

#include "OdbcPointRecord.h"

namespace RTX {
  class OdbcDirectPointRecord : public OdbcPointRecord {
    
  public:
    RTX_SHARED_POINTER(OdbcDirectPointRecord);
    OdbcDirectPointRecord();
    virtual ~OdbcDirectPointRecord();
    std::vector<std::string> identifiers();
    
  protected:
    void dbConnect() throw(RtxException);
    std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    
  private:
    std::string stringQueryForRange(const std::string& id, time_t start, time_t end);
    std::string stringQueryForIds();
    SQLHSTMT _directStatment;
    
  };
}



#endif /* defined(__epanet_rtx__OdbcDirectPointRecord__) */
