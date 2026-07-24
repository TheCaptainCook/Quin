cmake_minimum_required(VERSION 3.22)

project(Quin
    VERSION 0.1.0
    DESCRIPTION "High-performance PS5 emulator & x86-64 translation layer"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set binary output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(BUILD_TESTING "Build unit tests" ON)
option(QUIN_ENABLE_WERROR "Treat compiler warnings as errors" OFF)

# Include third-party dependencies
include(cmake/Dependencies.cmake)

# Find OpenGL for ImGui rendering backend
find_package(OpenGL REQUIRED)

# Compiler Warnings Configuration
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(QUIN_ENABLE_WERROR)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(QUIN_ENABLE_WERROR)
        add_compile_options(-Werror)
    endif()
endif()

# ------------------------------------------------------------------------------
# Target 1: quin-core (Static Library for Emulator Architecture)
# ------------------------------------------------------------------------------
add_library(quin-core STATIC
    src/core/logging.cpp
    src/gui/debug_shell.cpp
)

target_include_directories(quin-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(quin-core PUBLIC
    spdlog::spdlog
    imgui::imgui
    OpenGL::GL
)

if(TARGET SDL2::SDL2-static)
    target_link_libraries(quin-core PUBLIC SDL2::SDL2-static)
else()
    target_link_libraries(quin-core PUBLIC SDL2::SDL2)
endif()

# ------------------------------------------------------------------------------
# Target 2: quin (Main GUI Executable & Debug Shell)
# ------------------------------------------------------------------------------
add_executable(quin
    src/main.cpp
)

target_link_libraries(quin PRIVATE
    quin-core
)

# ------------------------------------------------------------------------------
# Target 3: Unit Test Suite
# ------------------------------------------------------------------------------
if(BUILD_TESTING)
    enable_testing()
    
    add_executable(quin-tests
        tests/unit/test_logging.cpp
    )

    target_link_libraries(quin-tests PRIVATE
        quin-core
        Catch2::Catch2WithMain
    )

    include(Catch)
    catch_discover_tests(quin-tests)
endif()
