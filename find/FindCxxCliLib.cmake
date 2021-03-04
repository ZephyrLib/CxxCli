
cmake_minimum_required (VERSION 3.0.2)

include(FindPackageHandleStandardArgs)

if(NOT CXXCLI_FOUND)

    find_package(Git REQUIRED)

    set(FIND_CxxCli_IMPORT_DIR "${CMAKE_BINARY_DIR}/CxxCli")
    
    macro(find_list_CxxCliLib)
        find_path(
            list_CxxCliLib
            "CMakeLists.txt"
            HINTS
            "${FIND_CxxCli_IMPORT_DIR}"
        )
    endmacro()

    find_list_CxxCliLib()
    if(EXISTS "${list_CxxCliLib}")
        set(CXXCLI_DO_TESTS OFF CACHE STRING "CXXCLI_DO_TESTS" FORCE)
        add_subdirectory("${FIND_CxxCli_IMPORT_DIR}" "${FIND_CxxCli_IMPORT_DIR}")
    else()
        file(MAKE_DIRECTORY "${FIND_CxxCli_IMPORT_DIR}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" "clone" "https://github.com/LibYutil/CxxCli.git" "${FIND_CxxCli_IMPORT_DIR}"
        )

        find_list_CxxCliLib()
        if(EXISTS "${list_CxxCliLib}")
            set(CXXCLI_DO_TESTS OFF CACHE STRING "CXXCLI_DO_TESTS" FORCE)
            add_subdirectory("${FIND_CxxCli_IMPORT_DIR}" "${FIND_CxxCli_IMPORT_DIR}")
        endif()

        mark_as_advanced(FIND_CxxCli_IMPORT_DIR list_CxxCliLib)

    endif()
    find_package_handle_standard_args(CxxCliLib DEFAULT_MSG list_CxxCliLib)
endif()
