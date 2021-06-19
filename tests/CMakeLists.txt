
cmake_minimum_required (VERSION 3.1)

project (ProjectCxxCliTests)

file(GLOB test_files "${PROJECT_SOURCE_DIR}/*.cxx")

set(i 0)
foreach(file ${test_files})
    set(test_exe cxxclilib_test_${i}_exe)
    add_executable(${test_exe} ${file})
    add_dependencies(${test_exe} CxxCliLib)
    target_link_libraries(${test_exe} CxxCliLib)
    add_test(NAME cxxclilib_test_${i} COMMAND ${test_exe})
    message(STATUS "Found CxxCli test #${i}")
    MATH(EXPR i "${i}+1")
endforeach()

enable_testing()
