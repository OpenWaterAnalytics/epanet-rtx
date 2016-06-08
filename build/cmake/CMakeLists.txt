## cmake .
## make

cmake_minimum_required (VERSION 2.6)

SET (CMAKE_C_COMPILER /usr/bin/clang)
SET (CMAKE_CXX_COMPILER /usr/bin/clang++)
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project (EPANET-RTX)
SET (CMAKE_C_FLAGS "-std=c99")
SET (CMAKE_CXX_FLAGS "-Wall -std=c++11 -stdlib=libstdc++ -fblocks")
add_definitions(-DRTX_NO_MYSQL)

file(GLOB EPANET_SOURCES ../../../EPANET/src/*.c)

# epanet
include_directories(../../../EPANET/include ../../../EPANET/src)
add_library(epanet STATIC ${EPANET_SOURCES})
SET_TARGET_PROPERTIES(epanet PROPERTIES LINKER_LANGUAGE C)

# the rtx library
include_directories(../../src ../../project ../../../EPANET/include ../../../EPANET/src ../../../epanet-msx/include /usr/local/include /usr/local/include/iODBC /usr/include/python2.7 /usr/include)
add_library(epanet-rtx STATIC  ../../src/AggregatorTimeSeries.cpp ../../src/BaseStatsTimeSeries.cpp ../../src/BufferPointRecord.cpp ../../src/Clock.cpp ../../src/ConstantTimeSeries.cpp ../../src/CorrelatorTimeSeries.cpp ../../src/Curve.cpp ../../src/CurveFunction.cpp ../../src/DbPointRecord.cpp ../../src/Dma.cpp ../../src/Element.cpp ../../src/EpanetModel.cpp ../../src/EpanetModelExporter.cpp ../../src/EpanetMsxModel.cpp ../../src/EpanetSyntheticModel.cpp ../../src/FailoverTimeSeries.cpp ../../src/FirstDerivative.cpp ../../src/GainTimeSeries.cpp ../../src/InfluxDbPointRecord.cpp ../../src/InpTextPattern.cpp ../../src/IntegratorTimeSeries.cpp ../../src/InversionTimeSeries.cpp ../../src/Junction.cpp ../../src/LagTimeSeries.cpp ../../src/Link.cpp ../../src/LogicTimeSeries.cpp ../../src/MathOpsTimeSeries.cpp ../../src/MetaTimeSeries.cpp ../../src/Model.cpp ../../src/ModelPerformance.cpp ../../src/MovingAverage.cpp ../../src/MultiplierTimeSeries.cpp ../../src/Node.cpp ../../src/OdbcDirectPointRecord.cpp ../../src/OdbcPointRecord.cpp ../../src/OffsetTimeSeries.cpp ../../src/OutlierExclusionTimeSeries.cpp ../../src/Pipe.cpp ../../src/Point.cpp ../../src/PointRecord.cpp ../../src/PointRecordTime.cpp ../../src/Pump.cpp ../../src/Reservoir.cpp ../../src/SineTimeSeries.cpp ../../src/SqlitePointRecord.cpp ../../src/StatsTimeSeries.cpp ../../src/Tank.cpp ../../src/ThresholdTimeSeries.cpp ../../src/TimeRange.cpp ../../src/TimeSeries.cpp ../../src/TimeSeriesFilter.cpp ../../src/TimeSeriesFilterSecondary.cpp ../../src/TimeSeriesFilterSinglePoint.cpp ../../src/TimeSeriesSynthetic.cpp ../../src/Units.cpp ../../src/ValidRangeTimeSeries.cpp ../../src/Valve.cpp ../../src/)
target_compile_definitions(epanet-rtx PRIVATE MAXFLOAT=3.40282347e+38F)
target_link_libraries(epanet-rtx epanet curl boost_system boost_filesystem boost_date_time boost_regex iodbc sqlite3 m)

# the duper library
include_directories(../../examples/Duplicator)
add_library(timeseries_duplicator STATIC ../../examples/Duplicator/TimeSeriesDuplicator.cpp)

# the link service executable
include_directories(../../examples/LINK-service)
link_directories(/usr/local/lib)
add_executable(link-server ../../examples/LINK-service/LinkService.cpp ../../examples/LINK-service/LinkJsonSerialization.cpp ../../examples/LINK-service/main.cpp)
target_link_libraries(link-server LINK_PUBLIC timeseries_duplicator epanet-rtx boost_thread boost_program_options pthread BlocksRuntime)
