
project(${CMAKE_PROJECT_NAME}_gui
    VERSION "${CMAKE_PROJECT_VERSION}"
    LANGUAGES CXX
)

set(projectIncDir ${CMAKE_SOURCE_DIR}/inc)
set(projectScrDir ${CMAKE_SOURCE_DIR}/src/Gui)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

add_executable(${PROJECT_NAME}
    ${projectScrDir}/Main.cc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${backend}
    ${GTKMM_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".exe"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${projectIncDir}
    ${CMAKE_BINARY_DIR}/gen
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${GTKMM_INCLUDE_DIRS}
)

# file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resource)

configure_file(
    InstStyle.css
    ${CMAKE_BINARY_DIR}/gen/resources/InstStyle.css
    COPYONLY
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    CSS_FILE_PATH="${CMAKE_BINARY_DIR}/gen/resources/InstStyle.css"
)

target_sources(${PROJECT_NAME} PRIVATE
    ${projectIncDir}/Gui/InstCommST.hh
    ${projectIncDir}/Gui/IHoverWidget.hh
    ${projectScrDir}/IHoverWidget.cc

    ${projectIncDir}/Gui/AsmMnemonicWidget.hh
    ${projectScrDir}/AsmMnemonicWidget.cc

    ${projectIncDir}/Gui/BinaryFieldWidget.hh
    ${projectScrDir}/BinaryFieldWidget.cc

    ${projectIncDir}/Gui/InstFormatUI.hh
    ${projectScrDir}/InstFormatUI.cc

    ${projectIncDir}/Gui/RISCVInstructionWindow.hh
    ${projectScrDir}/RISCVInstructionWindow.cc
)
