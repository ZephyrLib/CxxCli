
cmake_minimum_required (VERSION 3.0.2)

set(CXXCLI_DO_TESTS OFF CACHE STRING "CXXCLI_DO_TESTS" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_BINARY_DIR}/CxxCli.build")
