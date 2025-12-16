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
# Client (macOS/Windows - Console, no Qt)
# ============================================================================

if(BUILD_CLIENT)
    add_executable(qtype_client qtype_client.cpp)
    
    # Platform-specific
    if(APPLE)
        target_link_libraries(qtype_client
            PRIVATE
                "-framework ApplicationServices"
                "-framework CoreFoundation"
        )
    elseif(WIN32)
        # Windows version would go here
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
