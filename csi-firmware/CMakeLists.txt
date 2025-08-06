# CMakeLists.txt for CSI Positioning System Firmware
cmake_minimum_required(VERSION 3.16)

# Set the project name and version
set(PROJECT_NAME "csi_positioning_firmware")
set(PROJECT_VER "1.0.0")

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Enable testing support
set(COMPONENTS "unity" CACHE STRING "List of components to include in the build" FORCE)

# Project configuration
project(${PROJECT_NAME} VERSION ${PROJECT_VER})

# Add custom components path
set(EXTRA_COMPONENT_DIRS "components")

# Configure build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Add compile definitions
target_compile_definitions(${CMAKE_PROJECT_NAME}.elf PRIVATE
    PROJECT_VER="${PROJECT_VER}"
    CONFIG_CSI_ENABLED=1
)

# Enable additional warnings for better code quality
target_compile_options(${CMAKE_PROJECT_NAME}.elf PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)