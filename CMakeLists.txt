cmake_minimum_required(VERSION 3.25...3.27)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(RVINST
    VERSION "0.1.3"
    DESCRIPTION "Copyright (c) 2026 Swan BaiLei27"
    HOMEPAGE_URL "https://github.com/BaiLei27/RVINST.git"
    LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

option(enableTests "Build the INST smoke test executable" ON)

# Directory-wide. Gui turns exceptions back on for Glib::Error.
option(noExceptions "Compile C++ without exceptions (-fno-exceptions)" ON)

# One directory for exes and shared libs so Cli/GUI share a single backend DLL
# (Windows/MSYS load from the executable's directory). GUI still copies GTK DLLs there.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(backend ${PROJECT_NAME}_backend)
set(customModulesPath ${CMAKE_CURRENT_LIST_DIR}/cmake/Modules CACHE PATH "custom modules directory")
list(APPEND CMAKE_MODULE_PATH "${customModulesPath}")

find_package(Git QUIET)
set(COMMIT_HASH ${PROJECT_VERSION})

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        OUTPUT_VARIABLE COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/inc/Util/config.hpp.in
    ${CMAKE_BINARY_DIR}/gen/config.hpp
    @ONLY
)

# Before any directory-level compile/link flags, so RVINST options do not leak in.
add_subdirectory(Logger)

include(CheckIPOSupported)
include(CodeCheck)

# check_ipo_supported()
check_ipo_supported(RESULT _ipo_supported)

if(_ipo_supported)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-flto=full)
    else()
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

add_compile_options(
    -fdata-sections
    -ffunction-sections
    -funroll-loops
    -fomit-frame-pointer
    "$<$<CONFIG:Debug>:-O0;-ggdb3;-DDEBUG_>"
    $<$<BOOL:${noExceptions}>:-fno-exceptions>
    $<$<CONFIG:Release,MinSizeRel>:-DNDEBUG_>
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_|amd|AMD)64$")
    add_compile_options(
        -march=native
        -mtune=native
        -mprefer-vector-width=256
    )
endif()

add_link_options(
    -Wl,--gc-sections
    -fno-semantic-interposition
    -lm
    $<$<CONFIG:Release,MinSizeRel>:-s>
)

add_library(${backend} SHARED)

set_target_properties(${backend} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}

    # OUTPUT_NAME "backEnd"
)

include_directories(
    inc
    Logger
)

target_sources(${backend} PRIVATE
    src/ISA/Regs.cc
    src/Util/ErrorHandle.cc
    src/Util/InputParse.cc
    src/Util/InstFormatView.cc
    src/Core/IBaseInstType.cc
    src/ISA/InstFormat.cc
    src/Core/RType.cc
    src/Core/IType.cc
    src/Core/JType.cc
    src/Core/UType.cc
    src/Core/SType.cc
    src/Core/BType.cc
    src/Core/InstTypeFactory.cc
    src/Core/Instruction.cc
    inc/Util/config.hpp.in
)

target_sources(${backend} PUBLIC
    FILE_SET HEADERS
    BASE_DIRS inc
    FILES
        inc/ISA/Regs.hpp
        inc/ISA/InstFormat.hh
        inc/Util/BiLookupTable.hpp
        inc/Util/ErrorHandle.hh
        inc/Util/InputParse.hh
        inc/Util/InstFormatView.hh
        inc/Core/IBaseInstType.hh
        inc/Core/RType.hh
        inc/Core/IType.hh
        inc/Core/JType.hh
        inc/Core/UType.hh
        inc/Core/SType.hh
        inc/Core/BType.hh
        inc/Core/InstTypeFactory.hh
        inc/Core/Instruction.hh
)

add_subdirectory(src/Cli)
add_subdirectory(src/Gui)

if(enableTests)
    include(CTest)
    add_subdirectory(tests)
endif()

SetupCodeChecks(${PROJECT_NAME}_cli ${backend} ${PROJECT_NAME}_gui)
DefineCodeChecksTargets("fmt" "tidy")

if(NOT rootBuilddir OR NOT IS_DIRECTORY "${rootBuilddir}")
    get_filename_component(builddir ${CMAKE_CURRENT_BINARY_DIR} NAME)
else()
    get_filename_component(root "${rootBuilddir}" NAME)
    get_filename_component(dir ${CMAKE_CURRENT_BINARY_DIR} NAME)
    set(builddir "${root}/${dir}")
endif()

if(ClangFormat_FOUND OR ClangTidy_FOUND)
    message(">> Optional: build the '${PROJECT_NAME}' target 'tidy' or 'fmt' ('cmake --build ${builddir} -t help' to know more) before building 'all' target")
endif()
