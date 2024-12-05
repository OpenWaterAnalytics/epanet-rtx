add_executable( epanetrtx-test 
	test_main.cpp
)

set_target_properties(epanetrtx-test PROPERTIES CXX_STANDARD 17)

target_include_directories(epanetrtx-test PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(epanetrtx-test ${rtx_lib_deps} epanetrtx ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_test( NAME epanetrtx-test COMMAND epanetrtx-test )
