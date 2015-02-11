//
//  TempoDbPointRecord.h
//  epanet-rtx
//
//  Created by Sam Hatchett on 5/8/14.
//
//

#ifndef __epanet_rtx__TempoDbPointRecord__
#define __epanet_rtx__TempoDbPointRecord__

#include <iostream>
#include "DbPointRecord.h"
#include <rapidjson/document.h>
#include <boost/asio.hpp>

namespace RTX {
  class TempoDbPointRecord : public DbPointRecord {
    
  public:
    RTX_SHARED_POINTER(TempoDbPointRecord);
    typedef boost::shared_ptr<rapidjson::Document> JsonDocPtr;
    
    virtual void dbConnect() throw(RtxException);
    virtual bool isConnected() {return _connected;};
    virtual std::string registerAndGetIdentifier(std::string recordName);
    virtual std::vector<std::string> identifiers();
    virtual void truncate() {}; // specific implementation must override this
    virtual time_pair_t range(const string& id);
    
    std::string baseUrl, host, user, pass, db;
    int port;
    
    virtual bool supportsBoundedQueries() {return false;};
    
  protected:
    virtual std::vector<Point> selectRange(const std::string& id, time_t startTime, time_t endTime);
    virtual Point selectNext(const std::string& id, time_t time);
    virtual Point selectPrevious(const std::string& id, time_t time);
    
    virtual void insertSingle(const std::string& id, Point point);
    virtual void insertRange(const std::string& id, std::vector<Point> points);
    virtual void removeRecord(const std::string& id);
    
    
  private:
    bool _connected;
    std::string encode64(const std::string& str);
    std::vector<Point> pointsFromUrl(const std::string& url);
    
  };
}

#endif /* defined(__epanet_rtx__TempoDbPointRecord__) */
