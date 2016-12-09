#ifndef __epanet_rtx__InfluxDbPointRecord__
#define __epanet_rtx__InfluxDbPointRecord__

#include <boost/asio.hpp>
#include <boost/signals2/mutex.hpp>

#include <iostream>

#include "I_InfluxDbPointRecord.h"

namespace RTX {
  class ITaskWrapper {
    // empty implementation for async task sub
  };
  
  class InfluxDbPointRecord : public I_InfluxDbPointRecord {
  public:
    RTX_BASE_PROPS(InfluxDbPointRecord);
    InfluxDbPointRecord();
    
    // obligate overrides
    void dbConnect() throw(RtxException);
    IdentifierUnitsList identifiersAndUnits(); // class -specific override
    
  protected:
    // overrides -> read methods
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    // overrides -> write methods
    void removeRecord(const std::string& id);
    
  private:
    void sendPointsWithString(const std::string& content);
    DbPointRecord::Query queryPartsFromMetricId(const std::string& name);
    std::shared_ptr<ITaskWrapper> _sendTask;
    
  };
}

#endif /* defined(__epanet_rtx__InfluxDbPointRecord__) */
