cmake_minimum_required(VERSION 3.16)

project(qtype VERSION 1.0 LANGUAGES CXX)

# ============================================================================
# Build Configuration
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build unit tests with GTest" OFF)
option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

# ============================================================================
# Find Dependencies
# ============================================================================

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    include(GoogleTest)
endif()

# ============================================================================
# Qt Configuration
# ============================================================================

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ============================================================================
# Source Files
# ============================================================================

# typing_engine.h contains both declarations and implementations
# We include it as a header that gets compiled with each source file

set(ENGINE_HEADERS
    typing_engine.h
)

set(APP_SOURCES
    main.cpp
    ${ENGINE_HEADERS}
)

set(TEST_SOURCES
    tests/tests.cpp
    ${ENGINE_HEADERS}
)

# ============================================================================
# Compiler Flags
# ============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
    
    if(ENABLE_SANITIZERS)
        add_compile_options(-fsanitize=address -fsanitize=undefined)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# ============================================================================
# Main Application
# ============================================================================

add_executable(${PROJECT_NAME} ${APP_SOURCES} ${ENGINE_HEADERS})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

# Platform-specific linking
if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework ApplicationServices"
    )
endif()

# Include current directory for typing_engine.h
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# Unit Tests
# ============================================================================

if(BUILD_TESTS)
    add_executable(qtype_tests ${TEST_SOURCES} ${ENGINE_HEADERS})
    
    target_link_libraries(qtype_tests
        PRIVATE
            Qt6::Core
            GTest::gtest
            GTest::gtest_main
    )
    
    target_include_directories(qtype_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    # Discover tests automatically
    gtest_discover_tests(qtype_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PROPERTIES
            LABELS "unit"
    )
    
    # Add custom test target for convenience
    add_custom_target(test_verbose
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure
        DEPENDS qtype_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running tests with verbose output"
    )
    
    # Add custom target to run tests with specific filter
    add_custom_target(test_quick
        COMMAND $<TARGET_FILE:qtype_tests> --gtest_filter=*Test.*
        DEPENDS qtype_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running quick unit tests"
    )
    
    message(STATUS "Tests enabled. Run with: make test or ctest")
    message(STATUS "  - Verbose tests: make test_verbose")
    message(STATUS "  - Quick tests: make test_quick")
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Install desktop file (optional - for Linux)
if(UNIX AND NOT APPLE)
    # You can add a .desktop file here for system integration
    # install(FILES qtype.desktop DESTINATION share/applications)
endif()

# ============================================================================
# Build Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "qtype - Build Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt Version: ${Qt6_VERSION}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "")