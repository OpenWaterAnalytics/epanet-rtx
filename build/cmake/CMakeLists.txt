## cmake .
## make

cmake_minimum_required (VERSION 2.6)
SET(CMAKE_C_FLAGS "-std=c99")
SET(CMAKE_CXX_FLAGS "-std=c++0x -fblocks")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

project (EPANET-RTX)

file(GLOB EPANET_SOURCES ../../../EPANET/src/*.c)
file(GLOB RTX_SOURCES ../../src/*.cpp)

# epanet
include_directories(../../../EPANET/include ../../../EPANET/src)
add_library(epanet STATIC ${EPANET_SOURCES})
SET_TARGET_PROPERTIES(epanet PROPERTIES LINKER_LANGUAGE C)

# the rtx library
include_directories(../../src ../../project ../../../EPANET/include ../../../EPANET/src ../../../epanet-msx/include /usr/local/include /usr/local/include/iODBC /usr/include/python2.7 /usr/include)
add_library(epanet-rtx STATIC  ${RTX_SOURCES})
target_link_libraries(epanet-rtx epanet curl boost_system boost_filesystem boost_date_time boost_regex mysqlcppconn-static mysqlclient iodbc sqlite3)
target_compile_definitions(epanet-rtx PRIVATE MAXFLOAT=3.40282347e+38F)

# the project library
include_directories(../../project)
add_library(epanet-rtx-project STATIC ../../project/ProjectFile.cpp ../../project/SqliteProjectFile.cpp)
target_link_libraries(epanet-rtx-project  epanet-rtx sqlite3)

# the duper library
include_directories(../../examples/Duplicator ../../project)
add_library(timeseries_duplicator STATIC ../../examples/Duplicator/TimeSeriesDuplicator.cpp)

# the standalone duper executable
include_directories(../../examples/Duplicator)
link_directories(/usr/local/lib)
add_executable(rtxduplicator ../../examples/Duplicator/DuperDaemon.cpp)
target_link_libraries(rtxduplicator LINK_PUBLIC timeseries_duplicator epanet-rtx epanet-rtx-project boost_thread-mt pthread BlocksRuntime)
