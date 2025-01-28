cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(FASTWEBSOCKETCLIENT)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)

option(BUILD_BENCHMARK "Build benchmarks" ON)

add_library(fastws INTERFACE)

add_subdirectory(ext/websocket-frame-utility)

target_link_libraries(fastws INTERFACE OpenSSL::SSL wsframe)
target_include_directories(fastws INTERFACE include ${Boost_INCLUDE_DIRS} ext/plf_nanotimer)

if (${PROJECT_IS_TOP_LEVEL})
    file( GLOB DRIVER_SOURCES examples/*.cpp )
    foreach( sourcefile ${DRIVER_SOURCES} )
        get_filename_component( name ${sourcefile} NAME_WE )
        add_executable( ${name} ${sourcefile} )
        target_link_libraries( ${name} fastws )
    endforeach( sourcefile ${DRIVER_SOURCES} )
    file( GLOB TEST_SOURCES tests/*.cpp )
    foreach( sourcefile ${TEST_SOURCES} )
        get_filename_component( name ${sourcefile} NAME_WE )
        add_executable( ${name} ${sourcefile} )
        target_link_libraries( ${name} fastws )
    endforeach( sourcefile ${TEST_SOURCES} )
    if (BUILD_BENCHMARK)
        add_executable(fastws_latency benchmark/latency/fastws_latency.cpp)
        target_link_libraries( fastws_latency fastws )
        target_include_directories( fastws_latency PUBLIC benchmark/latency )
        add_executable(websocketpp_latency benchmark/latency/websocketpp_benchmark.cpp)
        #add_executable(echo_server benchmark/echo_server.cpp)
        #add_executable(echo_client benchmark/echo_client.cpp)
        target_include_directories( websocketpp_latency PUBLIC ext/websocketpp benchmark/latency)
        target_include_directories( websocketpp_latency PUBLIC ${Boost_INCLUDE_DIRS})
        target_link_libraries( websocketpp_latency OpenSSL::SSL)
    endif()
endif()