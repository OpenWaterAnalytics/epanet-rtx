## cmake .
## make

cmake_minimum_required (VERSION 2.6)

SET (CMAKE_C_COMPILER /usr/bin/gcc)
SET (CMAKE_CXX_COMPILER /usr/bin/g++)
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project (EPANET-RTX)
SET (CMAKE_C_FLAGS "-std=c99")
SET (CMAKE_CXX_FLAGS "-Wall -std=c++11")
add_definitions(-DRTX_NO_MYSQL)


# the rtx library
include_directories(../../src ../../project /usr/local/include /usr/local/include/iODBC /usr/include/iodbc /usr/include/python2.7 /usr/include)
add_library(epanet-rtx STATIC  ../../src/AggregatorTimeSeries.cpp ../../src/BaseStatsTimeSeries.cpp ../../src/BufferPointRecord.cpp ../../src/Clock.cpp ../../src/ConstantTimeSeries.cpp ../../src/CorrelatorTimeSeries.cpp ../../src/Curve.cpp ../../src/CurveFunction.cpp ../../src/DbPointRecord.cpp ../../src/FailoverTimeSeries.cpp ../../src/FirstDerivative.cpp ../../src/GainTimeSeries.cpp ../../src/InfluxDbPointRecord.cpp ../../src/IntegratorTimeSeries.cpp ../../src/InversionTimeSeries.cpp ../../src/LagTimeSeries.cpp ../../src/LogicTimeSeries.cpp ../../src/MathOpsTimeSeries.cpp ../../src/MetaTimeSeries.cpp ../../src/MovingAverage.cpp ../../src/MultiplierTimeSeries.cpp ../../src/OdbcDirectPointRecord.cpp ../../src/OdbcPointRecord.cpp ../../src/OffsetTimeSeries.cpp ../../src/OutlierExclusionTimeSeries.cpp ../../src/Point.cpp ../../src/PointRecord.cpp ../../src/PointRecordTime.cpp ../../src/SineTimeSeries.cpp ../../src/SqlitePointRecord.cpp ../../src/StatsTimeSeries.cpp ../../src/ThresholdTimeSeries.cpp ../../src/TimeRange.cpp ../../src/TimeSeries.cpp ../../src/TimeSeriesFilter.cpp ../../src/TimeSeriesFilterSecondary.cpp ../../src/TimeSeriesFilterSinglePoint.cpp ../../src/TimeSeriesSynthetic.cpp ../../src/Units.cpp ../../src/ValidRangeTimeSeries.cpp)
target_compile_definitions(epanet-rtx PRIVATE MAXFLOAT=3.40282347e+38F)
target_link_libraries(epanet-rtx  curl boost_system boost_filesystem boost_date_time boost_regex iodbc sqlite3 m cpprest)

# the duper library
include_directories(../../examples/Duplicator)
add_library(timeseries_duplicator STATIC ../../examples/Duplicator/TimeSeriesDuplicator.cpp)

# the link service executable
include_directories(../../examples/LINK-service ../../src)
link_directories(/usr/local/lib)
add_executable(link-server ../../examples/LINK-service/LinkService.cpp ../../examples/LINK-service/LinkJsonSerialization.cpp ../../examples/LINK-service/main.cpp)
target_link_libraries(link-server LINK_PUBLIC timeseries_duplicator epanet-rtx boost_thread boost_program_options pthread BlocksRuntime ssl cpprest crypto)
