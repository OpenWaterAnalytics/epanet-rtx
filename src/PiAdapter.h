#ifndef PiAdapter_hpp
#define PiAdapter_hpp

#include <stdio.h>

#include "DbAdapter.h"

#include <cpprest/uri.h>
#include <cpprest/json.h>
#include <cpprest/http_client.h>

namespace RTX {
  class PiAdapter : public DbAdapter {
  public:
    PiAdapter(errCallback_t cb);
    ~PiAdapter();
    
    const adapterOptions options() const;
    
    std::string connectionString();
    void setConnectionString(const std::string& con);
    
    void doConnect();
    
    IdentifierUnitsList idUnitsList();
    
    // TRANSACTIONS
    void beginTransaction();
    void endTransaction();
    
    // READ
    std::vector<Point> selectRange(const std::string& id, TimeRange range);
    Point selectNext(const std::string& id, time_t time, WhereClause q = WhereClause());
    Point selectPrevious(const std::string& id, time_t time, WhereClause q = WhereClause());
    
    // CREATE
    bool insertIdentifierAndUnits(const std::string& id, Units units);
    void insertSingle(const std::string& id, Point point);
    void insertRange(const std::string& id, std::vector<Point> points);
    
    // UPDATE
    bool assignUnitsToRecord(const std::string& name, const Units& units);
    
    // DELETE
    void removeRecord(const std::string& id);
    void removeAllRecords();
    
    
    // PI_SPECIFIC details
    std::string tagSearchPath;
    std::string valueConversions; /// use for text-field conversions to numerical type... "Active=1&Inactive=0".
    
    
    
  private:
    struct connectionInfo {
      // these are set through connection string parser
      std::string proto, host, apiPath, dataServer, user, pass;
      int port;
      // these are determined through a successful connection
      std::string dsWebId;
    };
    connectionInfo _conn;
    
    std::map<std::string, std::string> _webIdLookup; // name->webId
    std::map<std::string, double> _conversions;
    web::json::value jsonFromRequest(web::http::uri uri, web::http::method withMethod);
    web::http::uri_builder uriBase();

    std::map<std::string,std::string> _kvFromDelimited(const std::string& str);
    Point _pointFromJson(const web::json::value& j);
    
  };
}

#endif /* PiAdapter_hpp */
