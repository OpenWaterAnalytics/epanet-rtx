#ifndef OpcPointRecord_hpp
#define OpcPointRecord_hpp

#include <stdio.h>

#include <ua_client.h>

#include "DbPointRecord.h"

namespace RTX {
  class OpcPointRecord : public DbPointRecord {
  public:
    RTX_BASE_PROPS(OpcPointRecord);
    OpcPointRecord();
    
    std::string connectionString();
    void setConnectionString(const std::string& string);
    
    void dbConnect() throw(RtxException);
    bool isConnected();
    bool supportsUnitsColumn() {return false;};
    const std::map<std::string, Units> identifiersAndUnits();
    
    // stubs
    void truncate() {};
    bool supportsSinglyBoundedQueries() {return false;};
    bool shouldSearchIteratively() {return false;};
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time) {};
    Point selectPrevious(const std::string& id, time_t time) {};
    
    // insertions or alterations: may choose to ignore / deny
    void insertSingle(const std::string& id, Point point) {};
    void insertRange(const std::string& id, std::vector<Point> points) {};
    void removeRecord(const std::string& id) {};
    bool insertIdentifierAndUnits(const std::string& id, Units units) {};
    
  private:
    std::string _endpoint;
    UA_Client *_client;
    bool _connected;
//    std::map<string,OpcUa::Node> _nodes;
  };
}

#endif /* OpcPointRecord_hpp */
