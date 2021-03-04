
cmake_minimum_required (VERSION 3.8)

project (ProjectCxxCli)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CXXCLI_DO_TESTS "compile tests" ON)

add_library(CxxCliLib INTERFACE)
target_include_directories(CxxCliLib INTERFACE "${PROJECT_SOURCE_DIR}/include")

if(${CXXCLI_DO_TESTS})
    message(STATUS "CxxCli tests enabled")
    add_subdirectory("${PROJECT_SOURCE_DIR}/tests")
endif()
