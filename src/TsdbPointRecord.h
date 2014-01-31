//
//  TsdbPointRecord.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 6/28/13.
//
//

#ifndef __epanet_rtx__TsdbPointRecord__
#define __epanet_rtx__TsdbPointRecord__

#include <iostream>

#include "DbPointRecord.h"


namespace RTX {
  
  class TsdbPointRecord : public DbPointRecord {
  public:
    RTX_SHARED_POINTER(TsdbPointRecord);
    TsdbPointRecord() {};
    virtual ~TsdbPointRecord() {};
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected();
    virtual std::string registerAndGetIdentifier(std::string recordName, Units dataUnits);
    virtual std::vector<std::string> identifiers();
    virtual time_pair_t range(const string& id);
    virtual std::ostream& toStream(std::ostream &stream);
    
    
  protected:
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    // insertions or alterations may choose to ignore / deny
    virtual void insertSingle(const std::string& id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id) {};
    virtual void truncate() {};
    
  private:
    int _sock;
  };
  
}



#endif /* defined(__epanet_rtx__TsdbPointRecord__) */
