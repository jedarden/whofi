# Test Directory CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Set the test project
set(TEST_PROJECT_NAME "csi_firmware_tests")

# Include ESP-IDF build system for tests
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set test components
set(COMPONENTS 
    "unity" 
    "csi_collector" 
    "web_server" 
    "mqtt_client" 
    "ntp_sync" 
    "ota_updater"
    CACHE STRING "List of components to include in the test build" FORCE
)

project(${TEST_PROJECT_NAME})

# Add test-specific compile definitions
target_compile_definitions(${CMAKE_PROJECT_NAME}.elf PRIVATE
    UNITY_INCLUDE_CONFIG_H
    CONFIG_UNITY_ENABLE_DOUBLE=1
    CONFIG_UNITY_ENABLE_FLOAT=1
    CONFIG_UNITY_ENABLE_INC_FLOAT=1
)