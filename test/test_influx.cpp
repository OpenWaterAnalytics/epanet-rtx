//
//  test_influx.cpp
//  rtx-tests
//
//  Created by Devine, Cameron - Xylem on 2/22/22.
//
#include "test_main.h"
#include "ConcreteDbRecords.h"
#include "MyClientTest.hpp"
#include "oatpp-test/UnitTest.hpp"

#include "TestController.h"
#include "TestClient.h"
#include "InfluxClient.hpp"
#include "InfluxAdapter.h"

#include "ConcreteDbRecords.h"
#include "Components.hpp"

#include "oatpp/web/client/HttpRequestExecutor.hpp"

#include "oatpp-test/web/ClientServerTestRunner.hpp"
using namespace RTX;
using namespace std;

#include <iostream>
BOOST_AUTO_TEST_SUITE(influx)


BOOST_AUTO_TEST_CASE(influx_basic) {
  
  const string recordName("test record");
  const string connection("proto=HTTP&host=HOST&port=8086&db=DB&u=USER&p=PASS&validate=1");
  
  // create demo record
  DbPointRecord::_sp record(new InfluxDbPointRecord);
  record->setName(recordName);
  record->setConnectionString(connection);
  
  BOOST_CHECK_EQUAL(record->name(), recordName);
  BOOST_CHECK_EQUAL(record->connectionString(), connection);
  BOOST_CHECK_EQUAL(oatpp::base::Environment::getObjectsCount(), 0);
}


BOOST_AUTO_TEST_CASE(influx_client_test){
  oatpp::base::Environment::init();
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
    auto client = InfluxClient::createShared(requestExecutor, objectMapper);

    /* Call server API */
    /* Call root endpoint of MyController */
    auto response = client->getRoot();

    /* Assert that server responds with 200 */
    BOOST_CHECK_EQUAL(response->getStatusCode(), 200);

    /* Read response body as MessageDto */
    auto message = response->readBodyToDto<oatpp::Object<MyDto>>(objectMapper.get());

    /* Assert that received message is as expected */
    //BOOST_ASSERT(message);
    const string msg("Hello World!");
    BOOST_CHECK_EQUAL(message->statusCode,200);
    BOOST_CHECK_EQUAL(message->message.getValue(""), msg);
    
    //Check that the auth part works
    string q("SHOW%20MEASUREMENTS%20LIMIT%201");
    response = client->doCreate("", q);
    BOOST_CHECK_EQUAL(response->getStatusCode(), 200);
    
    auto _errCB = [&](const std::string& msg)->void {
      string errorMessage = msg;
    };
    InfluxTcpAdapter adapter(_errCB, client);
    adapter.doConnect();

    //Authenticated
    //response = client->doQuery("USER:PASS", "db", q);
    //BOOST_CHECK_EQUAL(response->getStatusCode(), 200);

  }, std::chrono::minutes(10) /* test timeout */);

  /* wait all server threads finished */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  
}

BOOST_AUTO_TEST_CASE(influx_id_list){
  //oatpp::base::Environment::init();
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
    auto client = InfluxClient::createShared(requestExecutor, objectMapper);

    
    auto _errCB = [&](const std::string& msg)->void {
      string errorMessage = msg;
    };
    InfluxTcpAdapter adapter(_errCB, client);
    auto list = adapter.idUnitsList();
    BOOST_CHECK_GT(list.count(), 0);

    //Authenticated
    //response = client->doQuery("USER:PASS", "db", q);
    //BOOST_CHECK_EQUAL(response->getStatusCode(), 200);

  }, std::chrono::minutes(2) /* test timeout */);

  /* wait all server threads finished */
  std::this_thread::sleep_for(std::chrono::seconds(1));
  
}
BOOST_AUTO_TEST_SUITE_END()
