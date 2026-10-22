# RVINST tests — CTest suite driven by CLI instruction vectors

set(cli_target ${CMAKE_PROJECT_NAME}_cli)
set(cli_cases "${CMAKE_CURRENT_SOURCE_DIR}/inst_sets.txt")

add_executable(tests_inst
    INST.cc
)

set_target_properties(tests_inst PROPERTIES
    SUFFIX ".exe"
)

add_dependencies(tests_inst ${cli_target})

# Ensure the backend DLL is found next to the CLI when ctest runs.
add_test(
    NAME cli_inst_suite
    COMMAND tests_inst
    $<TARGET_FILE:${cli_target}>
    ${cli_cases}
)

set_tests_properties(cli_inst_suite PROPERTIES
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${cli_target}>
    TIMEOUT 60
)

# Convenience: `cmake --build <build> --target test_cli` then runs ctest for this suite.
add_custom_target(test_cli
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R cli_inst_suite
    DEPENDS tests_inst ${cli_target}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "ctest: cli_inst_suite"
)

# Unit tests for the shared Util/FileIo module (links the backend).
add_executable(tests_inputfile
    FileIo_test.cc
)
set_target_properties(tests_inputfile PROPERTIES
    SUFFIX ".exe"
)
target_link_libraries(tests_inputfile PRIVATE
    ${backend}
)
target_include_directories(tests_inputfile PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)
add_test(
    NAME inputfile_suite
    COMMAND tests_inputfile
)
set_tests_properties(inputfile_suite PROPERTIES
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${cli_target}>
    TIMEOUT 60
)
add_custom_target(test_inputfile
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R inputfile_suite
    DEPENDS tests_inputfile
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "ctest: inputfile_suite"
)

add_executable(tests_inputhist
    InputHistoryMgr_test.cc
)
set_target_properties(tests_inputhist PROPERTIES
    SUFFIX ".exe"
)
target_link_libraries(tests_inputhist PRIVATE
    ${backend}
)
target_include_directories(tests_inputhist PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)
add_test(
    NAME inputhist_suite
    COMMAND tests_inputhist
)
set_tests_properties(inputhist_suite PROPERTIES
    WORKING_DIRECTORY $<TARGET_FILE_DIR:${cli_target}>
    TIMEOUT 30
)
add_custom_target(test_inputhist
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R inputhist_suite
    DEPENDS tests_inputhist
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "ctest: inputhist_suite"
)
