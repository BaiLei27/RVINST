# RVINST tests — CTest suites

set(cli_target ${CMAKE_PROJECT_NAME}_cli)
set(cli_cases "${CMAKE_CURRENT_SOURCE_DIR}/inst_sets.txt")

# Register one test executable + ctest entry + convenience build target.
#   NAME          stem: executable + default source ${NAME}.cc
#   SOURCES       .cc files (default: ${NAME}.cc)
#   TIMEOUT       ctest timeout seconds (default: 60)
#   LINK_BACKEND  link ${backend} and add inc/
#   DEPENDS       extra build deps
#   TEST_ARGS     extra args after the exe in add_test COMMAND
#
# Derived from NAME:
#   ctest NAME       = ${NAME}_suite
#   custom target    = ${NAME}_test
function(rvinst_add_ctest)
    set(options LINK_BACKEND)
    set(oneValueArgs NAME TIMEOUT)
    set(multiValueArgs SOURCES DEPENDS TEST_ARGS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAME)
        message(FATAL_ERROR "rvinst_add_ctest: NAME is required")
    endif()
    if(NOT ARG_SOURCES)
        set(ARG_SOURCES ${ARG_NAME}.cc)
    endif()
    if(NOT ARG_TIMEOUT)
        set(ARG_TIMEOUT 60)
    endif()

    set(_test_name "${ARG_NAME}_suite")
    set(_custom_target "${ARG_NAME}_test")

    add_executable(${ARG_NAME} ${ARG_SOURCES})
    set_target_properties(${ARG_NAME} PROPERTIES SUFFIX ".exe")

    if(ARG_LINK_BACKEND)
        target_link_libraries(${ARG_NAME} PRIVATE ${backend})
        target_include_directories(${ARG_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/inc)
    endif()

    if(ARG_DEPENDS)
        add_dependencies(${ARG_NAME} ${ARG_DEPENDS})
    endif()

    add_test(NAME ${_test_name} COMMAND ${ARG_NAME} ${ARG_TEST_ARGS})
    set_tests_properties(${_test_name} PROPERTIES
        WORKING_DIRECTORY $<TARGET_FILE_DIR:${cli_target}>
        TIMEOUT ${ARG_TIMEOUT}
    )

    add_custom_target(${_custom_target}
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ${_test_name}
        DEPENDS ${ARG_NAME} ${ARG_DEPENDS}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "ctest: ${_test_name}"
    )
endfunction()

# CLI instruction vector smoke test (drives RVINST_cli).
# → exe: insts, suite: insts_suite, target: insts_test
rvinst_add_ctest(
    NAME insts
    DEPENDS ${cli_target}
    TEST_ARGS
        $<TARGET_FILE:${cli_target}>
        ${cli_cases}
)

# Util/FileIo unit tests (link backend).
# → exe: file_io, suite: file_io_suite, target: file_io_test
rvinst_add_ctest(
    NAME file_io
    LINK_BACKEND
)

# Util/InputHistoryMgr unit tests (link backend).
# → exe: input_history, suite: input_history_suite, target: input_history_test
rvinst_add_ctest(
    NAME input_history
    TIMEOUT 30
    LINK_BACKEND
)
