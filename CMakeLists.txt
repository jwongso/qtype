cmake_minimum_required(VERSION 3.16)

project(qtype VERSION 1.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Qt6 (recommended for Wayland + modern Qt APIs)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Enable automatic MOC/UIC/RCC handling
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Link against Qt libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# macOS: enable secure keyboard events
if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        "-framework ApplicationServices"
    )
endif()

# Install target (optional)
install(TARGETS ${PROJECT_NAME})

