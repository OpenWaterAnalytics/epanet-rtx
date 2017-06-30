cmake_minimum_required (VERSION 2.6)


project (EPANET-RTX)
SET (CMAKE_C_FLAGS "-std=c99")
SET (CMAKE_CXX_FLAGS "-Wall -std=c++11")
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


# the rtx library
include_directories(
  ../../../../src
  /usr/local/include
  /usr/local/include/iODBC
  /usr/include/iodbc
  /usr/include/python2.7
  /usr/include
)

add_library(epanet-rtx STATIC
  ../../../../src/AggregatorTimeSeries.cpp
  ../../../../src/BaseStatsTimeSeries.cpp
  ../../../../src/BufferPointRecord.cpp
  ../../../../src/Clock.cpp
  ../../../../src/ConstantTimeSeries.cpp
  ../../../../src/CorrelatorTimeSeries.cpp
  ../../../../src/ConcreteDbRecords.cpp
  ../../../../src/Curve.cpp
  ../../../../src/CurveFunction.cpp
  ../../../../src/DbPointRecord.cpp
  ../../../../src/InfluxAdapter.cpp
  ../../../../src/PiAdapter.cpp
  ../../../../src/SqliteAdapter.cpp
  ../../../../src/OdbcAdapter.cpp
  ../../../../src/FailoverTimeSeries.cpp
  ../../../../src/FirstDerivative.cpp
  ../../../../src/GainTimeSeries.cpp
  ../../../../src/IdentifierUnitsList.cpp
  ../../../../src/IntegratorTimeSeries.cpp
  ../../../../src/InversionTimeSeries.cpp
  ../../../../src/LagTimeSeries.cpp
  ../../../../src/LogicTimeSeries.cpp
  ../../../../src/MathOpsTimeSeries.cpp
  ../../../../src/MetaTimeSeries.cpp
  ../../../../src/MovingAverage.cpp
  ../../../../src/MultiplierTimeSeries.cpp
  ../../../../src/OffsetTimeSeries.cpp
  ../../../../src/OutlierExclusionTimeSeries.cpp
  ../../../../src/Point.cpp
  ../../../../src/PointCollection.cpp
  ../../../../src/PointRecord.cpp
  ../../../../src/PointRecordTime.cpp
  ../../../../src/SineTimeSeries.cpp
  ../../../../src/StatsTimeSeries.cpp
  ../../../../src/ThresholdTimeSeries.cpp
  ../../../../src/TimeRange.cpp
  ../../../../src/TimeSeries.cpp
  ../../../../src/TimeSeriesFilter.cpp
  ../../../../src/TimeSeriesFilterSecondary.cpp
  ../../../../src/TimeSeriesFilterSinglePoint.cpp
  ../../../../src/TimeSeriesSynthetic.cpp
  ../../../../src/Units.cpp
  ../../../../src/ValidRangeTimeSeries.cpp
  ../../../../src/MetricInfo.cpp
)

target_compile_definitions(epanet-rtx PRIVATE MAXFLOAT=3.40282347e+38F PI_SSL_VALIDATE=false)
target_link_libraries(epanet-rtx
  curl
  boost_system
  boost_filesystem
  boost_date_time
  boost_regex
  iodbc
  sqlite3
  m
  cpprest
  z
)

# the duper library
include_directories(../../../Duplicator)
add_library(timeseries_duplicator STATIC ../../../Duplicator/TimeSeriesDuplicator.cpp)

# the link service executable
include_directories(../ ../../../../src)
link_directories(/usr/local/lib)

add_executable(link-server ../LinkService.cpp ../LinkJsonSerialization.cpp ../main.cpp)
target_link_libraries(link-server LINK_PUBLIC timeseries_duplicator epanet-rtx boost_iostreams boost_thread boost_program_options boost_chrono pthread ssl cpprest crypto ${EXTRA_LIBS})

install(TARGETS link-server DESTINATION bin)

