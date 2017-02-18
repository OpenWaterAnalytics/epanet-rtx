#ifndef PiPointRecord_h
#define PiPointRecord_h

#include <stdio.h>
#include <map>
#include <mutex>

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>

#include "DbPointRecord.h"

namespace RTX {
  class PiPointRecord : public DbPointRecord {
  public:
    RTX_BASE_PROPS(PiPointRecord);
    
    PiPointRecord();
    virtual ~PiPointRecord();
    
    std::string tagSearchPath;
    
    bool readonly() { return true; };
    void truncate() {};
    
  protected:
    
    // readonly no-ops
    bool insertIdentifierAndUnits(const std::string& id, Units units){ return false; };
    void insertSingle(const std::string& id, Point point) {};
    void insertRange(const std::string& id, std::vector<Point> points) {};
    void removeRecord(const std::string& id) {};
    
    
  private:
    class connectionInfo {
    public:
      // these are set through connection string parser
      std::string proto, host, apiPath, dataServer, user, pass;
      int port;
      // these are determined through a successful connection
      std::string dsWebId;
    };
    connectionInfo _conn;
    
    void parseConnectionString(const std::string& str);
    std::string serializeConnectionString();
    
    bool supportsUnitsColumn() { return false; }; // support not well-defined, so ignore
    void refreshIds();
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time) { return Point(); };
    Point selectPrevious(const std::string& id, time_t time) { return Point(); };
    void doConnect() throw(RtxException);
    
    std::mutex _mtx;
    std::map<std::string, std::string> _webIdLookup; // name->webId
    web::json::value jsonFromRequest(web::http::uri uri, web::http::method withMethod);
    web::http::uri_builder uriBase();
    
  };
}



#endif /* PiPointRecord_h */
