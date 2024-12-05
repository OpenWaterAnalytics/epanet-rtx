#  ________  _______    ______   __    __  ________  ________
# |        \|       \  /      \ |  \  |  \|        \|        \
# | $$$$$$$$| $$$$$$$\|  $$$$$$\| $$\ | $$| $$$$$$$$ \$$$$$$$$
# | $$__    | $$__/ $$| $$__| $$| $$$\| $$| $$__       | $$
# | $$  \   | $$    $$| $$    $$| $$$$\ $$| $$  \      | $$
# | $$$$$   | $$$$$$$ | $$$$$$$$| $$\$$ $$| $$$$$      | $$
# | $$_____ | $$      | $$  | $$| $$ \$$$$| $$_____    | $$
# | $$     \| $$      | $$  | $$| $$  \$$$| $$     \   | $$
#  \$$$$$$$$ \$$       \$$   \$$ \$$   \$$ \$$$$$$$$    \$$
#
#
#
#              _______  ________  __    __
#             |       \|        \|  \  |  \
#             | $$$$$$$\\$$$$$$$$| $$  | $$
#             | $$__| $$  | $$    \$$\/  $$
#             | $$    $$  | $$     >$$  $$
#             | $$$$$$$\  | $$    /  $$$$\
#             | $$  | $$  | $$   |  $$ \$$\
#             | $$  | $$  | $$   | $$  | $$
#              \$$   \$$   \$$    \$$   \$$
#
#


cmake_minimum_required (VERSION 3.15)

project (EPANET-RTX)
SET (CMAKE_C_FLAGS "-std=c99")
SET (CMAKE_CXX_FLAGS "-Wall -std=c++17")
SET (CMAKE_POSITION_INDEPENDENT_CODE ON)
add_definitions(-DRTX_NO_MYSQL)

message("prefix: ${CMAKE_FIND_LIBRARY_PREFIXES}")
message("suffix: ${CMAKE_FIND_LIBRARY_SUFFIXES}")

SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

IF(APPLE)
   FIND_LIBRARY(SECURITY_FRAMEWORK Security)
   FIND_LIBRARY(CORE_FRAMEWORK CoreFoundation)
   SET(EXTRA_LIBS ${SECURITY_FRAMEWORK} ${CORE_FRAMEWORK})
ENDIF (APPLE)

find_package(CURL REQUIRED)
find_package(epanet REQUIRED)
find_package(tsflib REQUIRED)
find_package(Boost REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(sqlite_modern_cpp REQUIRED)
find_package(oatpp REQUIRED)
find_package(oatpp-openssl REQUIRED)
find_package(nlohmann_json REQUIRED)

# the rtx library
include_directories(
	./src
	)

add_library(epanetrtx
./src/Dma.cpp
./src/Element.cpp
./src/EpanetModel.cpp
./src/EpanetModelExporter.cpp
./src/EpanetSyntheticModel.cpp
./src/InpTextPattern.cpp
./src/Junction.cpp
./src/Link.cpp
./src/Model.cpp
./src/Node.cpp
./src/Pipe.cpp
./src/Pump.cpp
./src/Reservoir.cpp
./src/RtxObject.cpp
./src/Tank.cpp
./src/Valve.cpp
)

set_target_properties(
  epanetrtx PROPERTIES
  CXX_STANDARD 17
)

target_compile_definitions(epanetrtx PRIVATE MAXFLOAT=3.40282347e+38F)
set(rtx_lib_deps
	CURL::libcurl
	epanet::epanet
	tsflib::tsflib
	boost::boost
	SQLite::SQLite3
	sqlite_modern_cpp::sqlite_modern_cpp
	oatpp::oatpp
	oatpp::oatpp-openssl
	nlohmann_json::nlohmann_json
	)


target_link_libraries(
	epanetrtx
	${rtx_lib_deps}
)


add_executable(rtx_test 
  ./test/test_main.cpp
	./test/test_element.cpp
)

target_link_libraries(rtx_test
	epanetrtx
	${rtx_lib_deps}
)


add_test(rtx_test rtx_test)


install(DIRECTORY ./src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(TARGETS epanetrtx DESTINATION lib)
