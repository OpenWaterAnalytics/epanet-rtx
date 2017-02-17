#ifndef __epanet_rtx__InfluxDbPointRecord__
#define __epanet_rtx__InfluxDbPointRecord__

#include <boost/asio.hpp>
#include <boost/signals2/mutex.hpp>

#include <iostream>
#include <cpprest/base_uri.h>


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
    IdentifierUnitsList identifiersAndUnits(); // class -specific override
    
    size_t maxTransactionLines() {return 1000;};
    
    void truncate();
    
  protected:
    // overrides -> read methods
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time);
    Point selectPrevious(const std::string& id, time_t time);
    
    // overrides -> write methods
    
    void removeRecord(const std::string& id);
    
  private:
    void doConnect() throw(RtxException);
    void sendPointsWithString(const std::string& content);
    I_InfluxDbPointRecord::Query queryPartsFromMetricId(const std::string& name);
    std::shared_ptr<ITaskWrapper> _sendTask;
    
    web::uri uriForQuery(const std::string& query, bool withTimePrecision = true);

    
  };
}

#endif /* defined(__epanet_rtx__InfluxDbPointRecord__) */
