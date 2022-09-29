//
//  InfluxClient.hpp
//  epanet-rtx
//
//  Created by Devine, Cameron - Xylem on 2/14/22.
//

#ifndef InfluxClient_h
#define InfluxClient_h

#include "oatpp/web/client/ApiClient.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(ApiClient)

class InfluxClient : public oatpp::web::client::ApiClient {

  
private:
  constexpr static const char* HTTP_GET = "GET";
  constexpr static const char* HTTP_POST = "POST";
  constexpr static const char* HTTP_PUT = "PUT";
  constexpr static const char* HTTP_DELETE = "DELETE";
public:
  API_CLIENT_INIT(InfluxClient)
  
  //-----------------------------------------------------------------------------------------------
  // Synchronous calls
  //
  API_CALL(HTTP_GET, "query", doCreate, AUTHORIZATION_BASIC(String, authString), QUERY(String, q, "q"))
  API_CALL(HTTP_GET, "query", doQuery, AUTHORIZATION_BASIC(String, authString), QUERY(String, db, "db"), QUERY(String, q, "q"))
  API_CALL(HTTP_GET, "query", doQueryWithTimePrecision, AUTHORIZATION_BASIC(String, authString), QUERY(String, db, "db"), QUERY(String, q, "q"), QUERY(String, epoch, "epoch"))
  API_CALL(HTTP_POST, "query", removeRecord, AUTHORIZATION_BASIC(String, authString), QUERY(String, q, "q"))
  API_CALL(HTTP_POST, "write", sendPoints, AUTHORIZATION_BASIC(String, authString), HEADER(String, contentEncoding, "Content-Encoding"), QUERY(String, db, "db"), QUERY(String, precision, "precision"), BODY_STRING(String, data))


  // FOR TESTING
  API_CALL("GET", "/", getRoot)
  //
#include OATPP_CODEGEN_END(ApiClient)
};


#endif /* InfluxClient_h */
