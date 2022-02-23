//
//  MyClientTest.hpp
//  rtx-tests
//
//  Created by Devine, Cameron - Xylem on 2/22/22.
//

#ifndef MyClientTest_hpp
#define MyClientTest_hpp


#include "oatpp-test/UnitTest.hpp"

class MyClientTest : public oatpp::test::UnitTest {
  public:

    MyClientTest() : UnitTest("TEST[MyClientTest]"){}
    void onRun() override;

  };

#endif /* MyClientTest_hpp */
