cmake_minimum_required(VERSION 3.25...3.27)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(INST
    VERSION "0.0.2"
    HOMEPAGE_URL "https://github.com/Baiyi27/RVINST.git"
    LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(backend ${PROJECT_NAME}_backend)
set(customModulesPath ${CMAKE_CURRENT_LIST_DIR}/cmake/Modules CACHE PATH "custom modules directory")
list(APPEND CMAKE_MODULE_PATH "${customModulesPath}")

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
    -fno-semantic-interposition
    -fno-PIE
    "$<$<CONFIG:Debug>:-ggdb3;-DDEBUG>"
    $<$<CONFIG:Release,MinSizeRel>:-DNDEBUG>
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
    -lm
    $<$<CONFIG:Release,MinSizeRel>:-s>
)

add_subdirectory(Logger)
add_library(${backend} SHARED)
add_executable(${PROJECT_NAME} tests/INST.cc)

set_target_properties(${backend} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}

    # OUTPUT_NAME "backEnd"
)

include_directories(
    inc
    Logger
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${backend}
    $<$<CONFIG:Debug>:Logger>
)

target_sources(${backend} PRIVATE
    inc/ISA/Regs.hpp
    src/ISA/Regs.cc

    inc/Util/BiLookupTable.hpp

    inc/Core/IBaseInstType.hh
    src/Core/IBaseInstType.cc

    inc/ISA/InstFormat.hh
    src/ISA/InstFormat.cc

    inc/Core/RType.hh
    src/Core/RType.cc

    inc/Core/IType.hh
    src/Core/IType.cc

    inc/Core/JType.hh
    src/Core/JType.cc

    inc/Core/UType.hh
    src/Core/UType.cc

    inc/Core/SType.hh
    src/Core/SType.cc

    inc/Core/BType.hh
    src/Core/BType.cc

    inc/Core/InstTypeFactory.hh
    src/Core/InstTypeFactory.cc

    inc/Core/Instruction.hh
    src/Core/Instruction.cc
)
add_subdirectory(src/Gui)

SetupCodeChecks(${PROJECT_NAME} ${backend} ${PROJECT_NAME}_gui)
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
