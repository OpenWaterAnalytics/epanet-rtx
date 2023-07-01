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

find_package(Boost REQUIRED filesystem iostreams thread)
find_package(cpprestsdk REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(oatpp REQUIRED)
find_package(oatpp-openssl REQUIRED)
find_package(ODBC REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)

find_path(SQLITE_MODERN_CPP_INCLUDE_DIRS "sqlite_modern_cpp.h")

find_path(EPANET2_INCLUDE_DIR "epanet2_2.h")
find_library(EPANET2_LIBRARY epanet2)

if (BUILD_WITH_MSX)
    find_path(EPANET_MSX_INCLUDE_DIR "epanetmsx.h")
    find_library(EPANET_MSX_LIBRARY epanetmsx)
endif()

include_directories (${Boost_INCLUDE_DIRS})
IF (APPLE)
    include_directories(/usr/local/opt/openssl/include/)
ENDIF()

SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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

target_include_directories(
    epanetrtx PRIVATE
    ../../src
    ${SQLITE_MODERN_CPP_INCLUDE_DIRS}
    ${EPANET2_INCLUDE_DIR}
)

IF (BUILD_WITH_MSX)
    target_include_directories(epanetrtx PRIVATE ${EPANET_MSX_INCLUDE_DIR})
ENDIF()

IF (MSVC)
    target_link_libraries(epanetrtx Boost::disable_autolinking)
ENDIF()

target_link_libraries(
    epanetrtx
    ${EPANET2_LIBRARY}
    Boost::headers
    Boost::filesystem
    Boost::iostreams
    Boost::thread
    cpprestsdk::cpprest
    CURL::libcurl
    nlohmann_json::nlohmann_json
    oatpp::oatpp
    oatpp::oatpp-openssl
    ODBC::ODBC
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
)

IF (BUILD_WITH_MSX)
    target_link_libraries(epanetrtx ${EPANET_MSX_LIBRARY})
ENDIF()

install(DIRECTORY   ../../src/
        DESTINATION include/rtx
        FILES_MATCHING PATTERN "*.h")
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
