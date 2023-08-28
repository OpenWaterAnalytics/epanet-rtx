cmake_minimum_required (VERSION 3.16)
project(rtx-link)
add_library(auto_runner STATIC AutoRunner.cpp)
target_include_directories(auto_runner PRIVATE ../../../src)
add_executable(link-server
    LinkService.cpp
    LinkJsonSerialization.cpp
    main.cpp
)
target_include_directories(link-server PRIVATE ../../../src)
target_link_libraries(
    link-server PUBLIC
    auto_runner
    epanetrtx
)
install(TARGETS link-server DESTINATION bin)
