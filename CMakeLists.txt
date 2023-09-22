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


find_package(epanet REQUIRED)
find_package(Boost REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(sqlite_modern_cpp REQUIRED)
find_package(oatpp REQUIRED)
find_package(oatpp-openssl REQUIRED)
find_package(nlohmann_json REQUIRED)

# the rtx library
include_directories(
	./src
	/usr/local/include
	/usr/local/include/iODBC
	/usr/include
	/usr/include/iodbc
	/opt/homebrew/include
	)

link_directories(
	/usr/lib
	/usr/local/lib
	)

add_library(epanetrtx
./src/AggregatorTimeSeries.cpp
./src/BaseStatsTimeSeries.cpp
./src/BufferPointRecord.cpp
./src/Clock.cpp
./src/ConcreteDbRecords.cpp
./src/ConstantTimeSeries.cpp
./src/CorrelatorTimeSeries.cpp
./src/Curve.cpp
./src/CurveFunction.cpp
./src/DbPointRecord.cpp
./src/Dma.cpp
./src/Element.cpp
./src/EpanetModel.cpp
./src/EpanetModelExporter.cpp
./src/EpanetSyntheticModel.cpp
./src/FailoverTimeSeries.cpp
./src/FirstDerivative.cpp
./src/GainTimeSeries.cpp
./src/IdentifierUnitsList.cpp
./src/InfluxAdapter.cpp
./src/InpTextPattern.cpp
./src/IntegratorTimeSeries.cpp
./src/InversionTimeSeries.cpp
./src/Junction.cpp
./src/LagTimeSeries.cpp
./src/Link.cpp
./src/LogicTimeSeries.cpp
./src/MathOpsTimeSeries.cpp
./src/MetaTimeSeries.cpp
./src/MetricInfo.cpp
./src/Model.cpp
./src/MovingAverage.cpp
./src/MultiplierTimeSeries.cpp
./src/Node.cpp
./src/OdbcAdapter.cpp
./src/OffsetTimeSeries.cpp
./src/OutlierExclusionTimeSeries.cpp
./src/Pipe.cpp
./src/Point.cpp
./src/PointCollection.cpp
./src/PointRecord.cpp
./src/PointRecordTime.cpp
./src/Pump.cpp
./src/Reservoir.cpp
./src/SineTimeSeries.cpp
./src/SquareWaveTimeSeries.cpp
./src/SqliteAdapter.cpp
./src/StatsTimeSeries.cpp
./src/Tank.cpp
./src/ThresholdTimeSeries.cpp
./src/TimeRange.cpp
./src/TimeSeries.cpp
./src/TimeSeriesFilter.cpp
./src/TimeSeriesFilterSecondary.cpp
./src/TimeSeriesFilterSinglePoint.cpp
./src/TimeSeriesLowess.cpp
./src/TimeSeriesQuery.cpp
./src/TimeSeriesSynthetic.cpp
./src/Units.cpp
./src/ValidRangeTimeSeries.cpp
./src/Valve.cpp
./src/WhereClause.cpp
)

set_target_properties(
  epanetrtx PROPERTIES
  CXX_STANDARD 17
)

target_compile_definitions(epanetrtx PRIVATE MAXFLOAT=3.40282347e+38F)
set(rtx_lib_deps
	epanet
	curl
	iodbc
	m
	z
	pthread
	${epanet_LIBRARIES}
	${Boost_LIBRARIES}
	${SQLite3_LIBRARIES}
	${sqlite_modern_cpp_LIBRARIES}
	${oatpp_LIBRARIES}
	${oatpp-openssl_LIBRARIES}
	${nlohmann_json_LIBRARIES}
	)


target_link_libraries(
	epanetrtx
	${rtx_lib_deps}
)


add_executable(rtx_test 
  ./test/test_main.cpp
	./test/test_units.cpp
	./test/test_record.cpp
	./test/test_influx.cpp
	./test/test_element.cpp
)

target_link_libraries(rtx_test
	epanetrtx
	${rtx_lib_deps}
)


add_test(rtx_test rtx_test)


install(DIRECTORY ./src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
install(TARGETS epanetrtx DESTINATION lib)
