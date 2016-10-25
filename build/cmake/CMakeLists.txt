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
                                                                      

cmake_minimum_required (VERSION 2.6)

project (EPANET-RTX)
SET (CMAKE_C_FLAGS "-std=c99")
SET (CMAKE_CXX_FLAGS "-Wall -std=c++11 -fblocks")
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
	../../src
	/usr/local/include 
	/usr/local/include/iODBC
	/usr/include
	/usr/include/iodbc
	../../../EPANET/src
	)
	
add_library(epanetrtx SHARED  
	../../src/AggregatorTimeSeries.cpp
	../../src/BaseStatsTimeSeries.cpp
	../../src/BufferPointRecord.cpp
	../../src/Clock.cpp
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
	../../src/GeoMetrics.cpp
	../../src/InfluxDbPointRecord.cpp
	../../src/InpTextPattern.cpp
	../../src/IntegratorTimeSeries.cpp
	../../src/InversionTimeSeries.cpp
	../../src/Junction.cpp
	../../src/LagTimeSeries.cpp
	../../src/TimeSeriesLowess.cpp
	../../src/Link.cpp
	../../src/LogicTimeSeries.cpp
	../../src/MathOpsTimeSeries.cpp
	../../src/MetaTimeSeries.cpp
	../../src/Model.cpp
	../../src/ModelPerformance.cpp
	../../src/ModelPerformanceController.cpp
	../../src/MovingAverage.cpp
	../../src/MultiplierTimeSeries.cpp
	../../src/Node.cpp
	../../src/OdbcDirectPointRecord.cpp
	../../src/OdbcPointRecord.cpp
	../../src/OffsetTimeSeries.cpp
	../../src/OutlierExclusionTimeSeries.cpp
	../../src/Pipe.cpp
	../../src/Point.cpp
	../../src/PointRecord.cpp
	../../src/PointRecordTime.cpp
	../../src/Pump.cpp
	../../src/Reservoir.cpp
	../../src/SineTimeSeries.cpp
	../../src/SqlitePointRecord.cpp
	../../src/StatsTimeSeries.cpp
	../../src/Tank.cpp
	../../src/ThresholdTimeSeries.cpp
	../../src/TimeRange.cpp
	../../src/TimeSeries.cpp
	../../src/TimeSeriesFilter.cpp
	../../src/TimeSeriesFilterSecondary.cpp
	../../src/TimeSeriesFilterSinglePoint.cpp
	../../src/TimeSeriesSynthetic.cpp
	../../src/Units.cpp
	../../src/ValidRangeTimeSeries.cpp
	../../src/Valve.cpp
	)

target_compile_definitions(epanetrtx PRIVATE MAXFLOAT=3.40282347e+38F)
target_link_libraries(
	epanetrtx 
	epanet
	geohash
	curl 
	boost_system 
	boost_filesystem 
	boost_date_time 
	boost_regex 
	boost_iostreams
	iodbc 
	sqlite3 
	m 
	z
	BlocksRuntime
	ssl
	crypto
	)

install(DIRECTORY ../../src/ DESTINATION include/rtx FILES_MATCHING PATTERN "*.h")
install(TARGETS epanetrtx DESTINATION lib)
