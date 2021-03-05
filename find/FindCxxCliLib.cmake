
cmake_minimum_required (VERSION 3.0.2)

if(NOT CXXCLILIB_FOUND)

    find_package(Git REQUIRED)

    set(FIND_CxxCli_IMPORT_DIR "${CMAKE_BINARY_DIR}/CxxCli")
    set(FIND_CxxCli_BUILD_DIR "${CMAKE_BINARY_DIR}/CxxCli.build")
    set(FIND_CxxCli_LISTFILE "${FIND_CxxCli_IMPORT_DIR}/CMakeLists.txt")

    if(NOT EXISTS "${FIND_CxxCli_LISTFILE}")
        file(MAKE_DIRECTORY "${FIND_CxxCli_IMPORT_DIR}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" "clone" "https://github.com/LibYutil/CxxCli.git" "${FIND_CxxCli_IMPORT_DIR}"
        )
    endif()
    if(EXISTS "${FIND_CxxCli_LISTFILE}")
        set(CXXCLI_DO_TESTS OFF CACHE STRING "CXXCLI_DO_TESTS" FORCE)
        add_subdirectory("${FIND_CxxCli_IMPORT_DIR}" "${FIND_CxxCli_BUILD_DIR}")
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(CxxCliLib DEFAULT_MSG FIND_CxxCli_LISTFILE)

    mark_as_advanced(
        FIND_CxxCli_IMPORT_DIR
        FIND_CxxCli_BUILD_DIR
        FIND_CxxCli_LISTFILE
    )
endif()
