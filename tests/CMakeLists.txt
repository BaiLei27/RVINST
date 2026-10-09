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
