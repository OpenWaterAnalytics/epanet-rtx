## cmake .
## make

cmake_minimum_required (VERSION 2.6)
SET(CMAKE_CXX_FLAGS "-std=c++0x")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project (EPANET-RTX)

file(GLOB RTX_SOURCES ../../src/*.cpp)

# the library
include_directories(../../src ../../../EPANET/include ../../../EPANET/src ../../../epanet-msx/include /usr/local/include)
add_library(epanet-rtx STATIC  ${RTX_SOURCES})

# the duper library
include_directories(../../examples/Duplicator)
add_library(timeseries_duplicator STATIC ../../examples/Duplicator/TimeSeriesDuplicator.cpp)

# the standalone duper executable
include_directories(../../examples/Duplicator)
add_executable(ts_duplicator ../../examples/Duplicator/DuperDaemon.cpp)
target_link_libraries (ts_duplicator LINK_PUBLIC timeseries_duplicator epanet-rtx)
