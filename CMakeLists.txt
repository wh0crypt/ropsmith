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

# --------------------------------------------------------
# Dependencies
# --------------------------------------------------------
find_package(ZLIB REQUIRED)

# --------------------------------------------------------
# Source files
# --------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/include/*.h")

# --------------------------------------------------------
# Executables
# --------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# --------------------------------------------------------
# Include directories
# --------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --------------------------------------------------------
# Libraries (Link ZLIB)
# --------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    ZLIB::ZLIB
)

# --------------------------------------------------------
# Compiler warnings
# --------------------------------------------------------
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    # Common warnings across GCC and Clang
    target_compile_options(${PROJECT_NAME} PRIVATE
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

    # GCC-only warnings (prevents Clang failures)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()

    # Sanitizers for Debug builds only
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0 -fsanitize=address,undefined,leak>
        $<$<CONFIG:Release>:-O3>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined,leak>
    )
endif()
