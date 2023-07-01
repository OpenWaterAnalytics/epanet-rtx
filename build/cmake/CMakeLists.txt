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


cmake_minimum_required (VERSION 3.16)

project (EPANET-RTX LANGUAGES CXX)
SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_definitions(RTX_NO_MYSQL)
IF (MSVC)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
    add_compile_options(/W3 /wd4068)
ELSE()
    add_compile_options(-Wno-fatal-errors -Wall -Wpedantic -Wno-unknown-pragmas)
ENDIF()

option(BUILD_EXAMPLES "Build the examples" ON)
option(BUILD_TESTS "Build the tests" OFF)
option(BUILD_WITH_MSX "Build with EPANET MSX Library" OFF)
option(BUILD_WITH_OPC "Build with open62541 Library" OFF)

include_directories (${Boost_INCLUDE_DIRS})

SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

IF(APPLE)
   FIND_LIBRARY(SECURITY_FRAMEWORK Security)
   FIND_LIBRARY(CORE_FRAMEWORK CoreFoundation)
   SET(EXTRA_LIBS ${SECURITY_FRAMEWORK} ${CORE_FRAMEWORK})
ENDIF (APPLE)

include(/opt/src/epanet-rtx/deps/conanbuildinfo.cmake)
conan_basic_setup()

# the rtx library
include_directories(
	../../src
	/usr/local/include
	/usr/local/include/iODBC
	/usr/include
	/usr/include/iodbc
	../../../EPANET/src
	/usr/local/include/oatpp-1.3.0/oatpp
	/usr/local/include/oatpp-1.3.0/oatpp-openssl
	)

link_directories(
	/usr/lib
	/usr/local/lib
	)

SET (RTX_SOURCES
../../src/AggregatorTimeSeries.cpp
../../src/BaseStatsTimeSeries.cpp
../../src/BufferPointRecord.cpp
../../src/Clock.cpp
../../src/ConcreteDbRecords.cpp
../../src/ConstantTimeSeries.cpp
../../src/CorrelatorTimeSeries.cpp
../../src/Curve.cpp
../../src/CurveFunction.cpp
../../src/DbPointRecord.cpp
../../src/Dma.cpp
../../src/Element.cpp
../../src/EpanetModel.cpp
../../src/EpanetModelExporter.cpp
../../src/EpanetSyntheticModel.cpp
../../src/FailoverTimeSeries.cpp
../../src/FirstDerivative.cpp
../../src/GainTimeSeries.cpp
../../src/IdentifierUnitsList.cpp
../../src/InfluxAdapter.cpp
../../src/InpTextPattern.cpp
../../src/IntegratorTimeSeries.cpp
../../src/InversionTimeSeries.cpp
../../src/Junction.cpp
../../src/LagTimeSeries.cpp
../../src/Link.cpp
../../src/LogicTimeSeries.cpp
../../src/MathOpsTimeSeries.cpp
../../src/MetaTimeSeries.cpp
../../src/MetricInfo.cpp
../../src/Model.cpp
../../src/MovingAverage.cpp
../../src/MultiplierTimeSeries.cpp
../../src/Node.cpp
../../src/OdbcAdapter.cpp
../../src/OffsetTimeSeries.cpp
../../src/OutlierExclusionTimeSeries.cpp
../../src/PiAdapter.cpp
../../src/Pipe.cpp
../../src/Point.cpp
../../src/PointCollection.cpp
../../src/PointRecord.cpp
../../src/PointRecordTime.cpp
../../src/Pump.cpp
../../src/Reservoir.cpp
../../src/SineTimeSeries.cpp
../../src/SqliteAdapter.cpp
../../src/SquareWaveTimeSeries.cpp
../../src/StatsTimeSeries.cpp
../../src/Tank.cpp
../../src/ThresholdTimeSeries.cpp
../../src/TimeRange.cpp
../../src/TimeSeries.cpp
../../src/TimeSeriesFilter.cpp
../../src/TimeSeriesFilterSecondary.cpp
../../src/TimeSeriesFilterSinglePoint.cpp
../../src/TimeSeriesLowess.cpp
../../src/TimeSeriesQuery.cpp
../../src/TimeSeriesSynthetic.cpp
../../src/Units.cpp
../../src/ValidRangeTimeSeries.cpp
../../src/Valve.cpp
../../src/WhereClause.cpp
)

if (BUILD_WITH_MSX)
    list(APPEND RTX_SOURCES ../../src/EpanetMsxModel.cpp)
elseif (BUILD_WITH_OPC)
    list(APPEND RTX_SOURCES ../../src/OpcAdapter.cpp ../../src/OpcPointRecord.cpp)
endif()

add_library(epanetrtx ${RTX_SOURCES})

set_target_properties(
  epanetrtx PROPERTIES
  CXX_STANDARD 17
)

target_compile_definitions(epanetrtx PRIVATE MAXFLOAT=3.40282347e+38F)
set(rtx_lib_deps
	epanet
	curl
	iodbc
	sqlite3
	m
	z
	pthread
	)


target_link_libraries(
	epanetrtx
	${rtx_lib_deps}
  ${CONAN_LIBS}
	)



install(DIRECTORY ../../src/ DESTINATION include/rtx FILES_MATCHING PATTERN "*.h")
install(TARGETS epanetrtx DESTINATION lib)

IF (BUILD_EXAMPLES)
    add_subdirectory(../../examples/conceptual ../conceptual)
    IF (NOT MSVC)
        add_subdirectory(../../examples/LINK/service ../LINK/service)
    ENDIF()
ENDIF()

IF (BUILD_TESTS)
    enable_testing()
    add_subdirectory(../../test/ ../test/)
ENDIF()
