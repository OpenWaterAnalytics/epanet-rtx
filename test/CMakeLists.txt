find_package(Boost REQUIRED unit_test_framework)

add_executable(rtx_unit_test
    MyClientTest.cpp
    test_main.cpp
    test_record.cpp
    test_influx.cpp
    test_units.cpp
)

target_include_directories(
    rtx_unit_test PUBLIC
    ../src
    ${CMAKE_CURRENT_SOURCE_DIR}
    Boost::unit_test_framework
)

target_link_libraries(
    rtx_unit_test
    epanetrtx
    oatpp::oatpp-test
    Boost::unit_test_framework
)

# target_compile_definitions(
#     rtx_unit_test PRIVATE
#     BOOST_TEST_DYN_LINK
# )

add_test(unit_test rtx_unit_test)
