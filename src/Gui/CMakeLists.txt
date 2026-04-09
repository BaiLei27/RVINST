
project(${CMAKE_PROJECT_NAME}_gui
    VERSION "0.0.1"
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
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${GTKMM_INCLUDE_DIRS}
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resource)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/InstStyle.css
    ${CMAKE_CURRENT_BINARY_DIR}/resource/InstStyle.css
    COPYONLY
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    CSS_FILE_PATH="${CMAKE_CURRENT_BINARY_DIR}/resource/InstStyle.css"
)

target_sources(${PROJECT_NAME} PRIVATE
    ${projectIncDir}/Gui/InstCommST.hh

    ${projectIncDir}/Gui/AsmMnemonicWidget.hh
    ${projectScrDir}/AsmMnemonicWidget.cc

    ${projectIncDir}/Gui/BinaryFieldWidget.hh
    ${projectScrDir}/BinaryFieldWidget.cc

    ${projectIncDir}/Gui/InstFormatUI.hh
    ${projectScrDir}/InstFormatUI.cc

    ${projectIncDir}/Gui/RISCVApplication.hh
    ${projectScrDir}/RISCVApplication.cc

    ${projectIncDir}/Gui/RISCVInstructionWindow.hh
    ${projectScrDir}/RISCVInstructionWindow.cc
)
