
cmake_minimum_required (VERSION 3.8)

project (ProjectCxxCli)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DO_TESTS "compile tests" ON)

add_library(CxxCli INTERFACE)
target_include_directories(CxxCli INTERFACE "${PROJECT_SOURCE_DIR}/include")

message(STATUS "tests ${DO_TESTS}")
if(${DO_TESTS})
	add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()
