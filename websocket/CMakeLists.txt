cmake_minimum_required(VERSION 3.16)

project(qtype_network VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_SERVER "Build server (Qt + WebSocket)" OFF)
option(BUILD_CLIENT "Build client (no Qt, console only)" OFF)

# ============================================================================
# Server (Linux - Qt with WebSockets)
# ============================================================================

if(BUILD_SERVER)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network WebSockets)
    
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    
    add_executable(qtype_server qtype_server.cpp)
    
    target_link_libraries(qtype_server
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Network
            Qt6::WebSockets
    )
    
    message(STATUS "Building qtype_server (Qt WebSocket Server)")
endif()

# ============================================================================
# Client (Cross-Platform - Console, no Qt)
# ============================================================================

if(BUILD_CLIENT)
    add_executable(qtype_client qtype_client.cpp)

    # Platform-specific libraries
    if(APPLE)
        target_link_libraries(qtype_client
            PRIVATE
                "-framework ApplicationServices"
                "-framework CoreFoundation"
        )
        message(STATUS "Building qtype_client for MacOS (CGEvent API)")
    elseif(UNIX AND NOT APPLE)
        # Linux/WSL
        find_package(X11 REQUIRED)
        if(NOT X11_XTest_FOUND)
            message(FATAL_ERROR "XTest extension not found. Install libxtst-dev: sudo apt-get install libxtst-dev")
        endif()
        target_link_libraries(qtype_client
            PRIVATE
                X11::X11
                X11::Xt
                X11::Xext
                X11::Xtst
        )
        message(STATUS "Building qtype_client for Linux/WSL (X11/XTest)")
    elseif(WIN32)
        # Native Windows would use Windows API
        message(WARNING "Native Windows build not fully implemented yet. Use WSL with X11 instead.")
    endif()
    
    message(STATUS "Building qtype_client (Console WebSocket Client)")
endif()

# ============================================================================
# Build Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "qtype Network - Build Configuration")
message(STATUS "========================================")
message(STATUS "Build server: ${BUILD_SERVER}")
message(STATUS "Build client: ${BUILD_CLIENT}")
message(STATUS "========================================")
message(STATUS "")
