#ifndef InfluxUdpPointRecord_h
#define InfluxUdpPointRecord_h

#include <stdio.h>
#include <future>

#include "rtxMacros.h"
#include "InfluxDbPointRecord.h"

namespace RTX {
  class InfluxUdpPointRecord : public I_InfluxDbPointRecord {
  public:
    RTX_BASE_PROPS(InfluxUdpPointRecord);
    InfluxUdpPointRecord();
    void dbConnect() throw(RtxException);
    std::string connectionString();
    
  protected:
    void sendPointsWithString(const std::string& content);
    std::string formatTimestamp(time_t t);

  private:
    std::future<void> _sendFuture;
  };
}

#endif /* InfluxUdpPointRecord_h */
