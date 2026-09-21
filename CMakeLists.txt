#===----------------------------------------------------------------------===
#
# Part of the ROPsmith Project.
# See LICENSE for license information.
#
#===----------------------------------------------------------------------===
#
# \file CMakeLists.txt
# Orchestration of the compiling process.
#
#===----------------------------------------------------------------------===

# --------------------------------------------------------
# Project configuration
# --------------------------------------------------------
cmake_minimum_required(VERSION 3.16)

# Project name and language
project(ropsmith VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTING "Build the test suite" ON)

# --------------------------------------------------------
# Dependencies
# --------------------------------------------------------
find_package(ZLIB REQUIRED)

# --------------------------------------------------------
# Source files
# --------------------------------------------------------
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/include/*.h" "src/*.hpp")

# --------------------------------------------------------
# Core Library Target
# --------------------------------------------------------
add_library(ropsmith_core STATIC ${CORE_SOURCES} ${HEADERS})

target_include_directories(ropsmith_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ropsmith_core PUBLIC
    ZLIB::ZLIB
)

# --------------------------------------------------------
# Main Executable
# --------------------------------------------------------
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ropsmith_core
)

# --------------------------------------------------------
# Compiler warnings & Sanitizers (Interface Target)
# --------------------------------------------------------
add_library(ropsmith_compiler_flags INTERFACE)

if (MSVC)
    target_compile_options(ropsmith_compiler_flags INTERFACE /W4 /permissive-)
else()
    target_compile_options(ropsmith_compiler_flags INTERFACE
        -Wall
        -Wextra
        -Werror
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wmisleading-indentation
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(ropsmith_compiler_flags INTERFACE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()

    target_compile_options(ropsmith_compiler_flags INTERFACE
        $<$<CONFIG:Debug>:-g -O0 -fsanitize=address,undefined,leak>
        $<$<CONFIG:Release>:-O3>
    )
    target_link_options(ropsmith_compiler_flags INTERFACE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>
    )
endif()

# apply flags to library and binary
target_link_libraries(ropsmith_core PUBLIC ropsmith_compiler_flags)
target_link_libraries(${PROJECT_NAME} PRIVATE ropsmith_compiler_flags)

# --------------------------------------------------------
# Testing with Criterion
# --------------------------------------------------------
if (BUILD_TESTING)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(CRITERION criterion)
    endif()

    if (NOT CRITERION_FOUND)
        find_path(CRITERION_INCLUDE_DIRS criterion/criterion.h)
        find_library(CRITERION_LIBRARIES criterion)
    endif()

    if (CRITERION_LIBRARIES)
        enable_testing()

        file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

        add_executable(test_binary ${TEST_SOURCES})

        target_include_directories(test_binary PRIVATE
            ${CRITERION_INCLUDE_DIRS}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

        target_link_libraries(test_binary PRIVATE
            ropsmith_core
            ${CRITERION_LIBRARIES}
            ropsmith_compiler_flags
        )

        add_test(NAME test_binary COMMAND test_binary --verbose)
    else()
        message(WARNING "Criterion library not found. Skipping test suite compilation.")
    endif()
endif()
