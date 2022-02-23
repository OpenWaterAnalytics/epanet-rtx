//
//  TestClient.h
//  rtx-tests
//
//  Created by Devine, Cameron - Xylem on 2/22/22.
//

#ifndef TestClient_h
#define TestClient_h

#include "oatpp/web/client/ApiClient.hpp"
#include "oatpp/core/macro/codegen.hpp"

/* Begin Api Client code generation */
#include OATPP_CODEGEN_BEGIN(ApiClient)

/**
 * Test API client.
 * Use this client to call application APIs.
 */
class TestClient : public oatpp::web::client::ApiClient {

  API_CLIENT_INIT(TestClient)

  API_CALL("GET", "/", getRoot)

  // TODO - add more client API calls here

};

/* End Api Client code generation */
#include OATPP_CODEGEN_END(ApiClient)

#endif /* TestClient_h */
