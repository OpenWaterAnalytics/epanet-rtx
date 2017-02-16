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
    
    class connectionInfo {
    public:
      // these are set through connection string parser
      std::string proto, host, apiPath, dataServer, user, pass;
      int port;
      // these are determined through a successful connection
      std::string dsWebId;
    };
    
    PiPointRecord();
    bool isConnected();
    
    std::string connectionString();
    void setConnectionString(const std::string& str);
    
    void dbConnect() throw(RtxException);
    
    bool supportsUnitsColumn() { return false; }; // support not well-defined, so ignore
    bool readonly() { return true; };
    bool supportsSinglyBoundedQueries() { return false; };
    bool shouldSearchIteratively() { return true; };
    
    IdentifierUnitsList identifiersAndUnits();
    
    connectionInfo conn;
    
  protected:
    bool insertIdentifierAndUnits(const std::string& id, Units units){ return false; };
    
    virtual std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time) { return Point(); };
    Point selectPrevious(const std::string& id, time_t time) { return Point(); };
    
  private:
    bool _isConnected;
    std::mutex _mtx;
    std::map<std::string, std::string> _webIdLookup; // name->webId
    web::json::value jsonFromRequest(web::http::uri uri, web::http::method withMethod);
    web::http::uri_builder uriBase();
    
  };
}



#endif /* PiPointRecord_h */
