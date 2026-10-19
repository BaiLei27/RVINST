
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

# Directory COMPILE_OPTIONS already passed -fno-exceptions. This later flag wins for Gui only.
target_compile_options(${PROJECT_NAME} PRIVATE -fexceptions)

if(WIN32)
    # No console window on Explorer launch. Main.cc must init stdio first
    # (AttachConsole / NUL) so println(stderr) never hits an invalid FILE*.
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${projectIncDir}
    ${CMAKE_BINARY_DIR}/gen
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${GTKMM_INCLUDE_DIRS}
)

# file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resource)

# Single type-scale stylesheet: font size is em-relative and switched at
# runtime via the 'compact'/'standard' class on the .main-container root
# (see RISCVInstructionWindow). No per-platform CSS files are needed.
configure_file(
    CSS/InstStyle.css
    ${CMAKE_BINARY_DIR}/gen/css/InstStyle.css
    COPYONLY
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    CSS_FILE_PATH="${CMAKE_BINARY_DIR}/gen/css/InstStyle.css"
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
