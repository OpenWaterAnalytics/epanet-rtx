//
//  MyClientTest.hpp
//  rtx-tests
//
//  Created by Devine, Cameron - Xylem on 2/22/22.
//
#include "MyClientTest.hpp"

#include <stdio.h>

#include "oatpp-test/UnitTest.hpp"

#include "TestController.h"
#include "TestClient.h"

#include "ConcreteDbRecords.h"
#include "Components.hpp"

#include "oatpp/web/client/HttpRequestExecutor.hpp"

#include "oatpp-test/web/ClientServerTestRunner.hpp"

void MyClientTest::onRun() {

  /* Register test components */
  Components component;

  /* Create client-server test runner */
  oatpp::test::web::ClientServerTestRunner runner;

  /* Add TestController endpoints to the router of the test server */
  runner.addController(std::make_shared<TestController>());

  /* Run test */
  runner.run([this, &runner] {

    /* Get client connection provider for Api Client */
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ClientConnectionProvider>, clientConnectionProvider);

    /* Get object mapper component */
    OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);

    /* Create http request executor for Api Client */
    auto requestExecutor = oatpp::web::client::HttpRequestExecutor::createShared(clientConnectionProvider);

    /* Create Test API client */
    auto client = TestClient::createShared(requestExecutor, objectMapper);

    /* Call server API */
    /* Call root endpoint of MyController */
    auto response = client->getRoot();

    /* Assert that server responds with 200 */
    OATPP_ASSERT(response->getStatusCode() == 0);

    /* Read response body as MessageDto */
    auto message = response->readBodyToDto<oatpp::Object<MyDto>>(objectMapper.get());

    /* Assert that received message is as expected */
    OATPP_ASSERT(message);
    OATPP_ASSERT(message->statusCode == 400);
    OATPP_ASSERT(message->message == "Hello World!");

  }, std::chrono::minutes(10) /* test timeout */);

  /* wait all server threads finished */
  std::this_thread::sleep_for(std::chrono::seconds(1));

}
