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
target_include_directories(fastws INTERFACE include ${Boost_INCLUDE_DIRS})

if (${PROJECT_IS_TOP_LEVEL})
    file( GLOB DRIVER_SOURCES examples/*.cpp )
    foreach( sourcefile ${DRIVER_SOURCES} )
        get_filename_component( name ${sourcefile} NAME_WE )
        add_executable( ${name} ${sourcefile} )
        target_link_libraries( ${name} fastws )
    endforeach( sourcefile ${DRIVER_SOURCES} )
    if (BUILD_BENCHMARK)
        add_executable(fastws_benchmark benchmark/fastws_benchmark.cpp)
        target_link_libraries( fastws_benchmark fastws )
        add_executable(websocketpp_benchmark benchmark/websocketpp_benchmark.cpp)
        target_include_directories( websocketpp_benchmark PUBLIC ext/websocketpp)
        target_include_directories( websocketpp_benchmark PUBLIC ${Boost_INCLUDE_DIRS})
        target_link_libraries( websocketpp_benchmark OpenSSL::SSL)
    endif()
endif()