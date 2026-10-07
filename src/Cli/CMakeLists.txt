
project(${CMAKE_PROJECT_NAME}_cli
    VERSION "${CMAKE_PROJECT_VERSION}"
    LANGUAGES CXX
)

set(projectIncDir ${CMAKE_SOURCE_DIR}/inc)
set(projectScrDir ${CMAKE_SOURCE_DIR}/src/Cli)

add_executable(${PROJECT_NAME}
    ${projectScrDir}/Main.cc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${backend}
    Logger
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".exe"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${projectIncDir}
    ${CMAKE_BINARY_DIR}/gen
)

target_sources(${PROJECT_NAME} PRIVATE
    ${projectIncDir}/Cli/CliApp.hh
    ${projectScrDir}/CliApp.cc

    ${projectIncDir}/Cli/Commands.hh
    ${projectScrDir}/Commands.cc

    ${projectIncDir}/Cli/InstrEntry.hh
    ${projectScrDir}/InstrEntry.cc

    ${projectIncDir}/Cli/OutputTable.hh
    ${projectScrDir}/OutputTable.cc
)
