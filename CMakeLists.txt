cmake_minimum_required(VERSION 3.20)

project(flow VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FLOW_COMPILE_TESTS "Compile tests for flow" ON)
option(FLOW_COMPILE_EXAMPLES "Compile examples for flow" ON)

include(GNUInstallDirs)

if(FLOW_COMPILE_EXAMPLES)
    add_subdirectory(examples)
endif()

if(FLOW_COMPILE_TESTS)
    add_subdirectory(tests)
endif()