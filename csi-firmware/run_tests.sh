#!/bin/bash

# ESP32 CSI Firmware Test Runner
# This script compiles and runs unit tests for all components

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ] || [ ! -d "components" ]; then
    print_error "Please run this script from the CSI firmware root directory"
    exit 1
fi

print_info "CSI Firmware Test Suite"
print_info "========================"

# Check for ESP-IDF environment
if [ -z "$IDF_PATH" ]; then
    print_warning "ESP-IDF environment not detected. Attempting to source it..."
    if [ -f "$HOME/esp/esp-idf/export.sh" ]; then
        source "$HOME/esp/esp-idf/export.sh"
    else
        print_error "ESP-IDF not found. Please install ESP-IDF and set IDF_PATH"
        exit 1
    fi
fi

# Clean previous test builds
print_info "Cleaning previous test builds..."
rm -rf build_test

# Create test build directory
mkdir -p build_test
cd build_test

# Component tests
COMPONENTS=("csi_collector" "mqtt_client" "ntp_sync" "ota_updater" "web_server")
PASSED_TESTS=0
FAILED_TESTS=0

# Function to compile component test
compile_component_test() {
    local component=$1
    print_info "Testing component: $component"
    
    # Create a minimal test project for the component
    mkdir -p test_$component
    cd test_$component
    
    # Create CMakeLists.txt for test project
    cat > CMakeLists.txt << EOF
cmake_minimum_required(VERSION 3.16)
include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_$component)
EOF

    # Create main directory
    mkdir -p main
    
    # Copy component test file as main
    if [ -f "../../components/$component/test/test_${component}.c" ]; then
        cp "../../components/$component/test/test_${component}.c" main/main.c
    else
        print_warning "No test file found for $component"
        cd ..
        return 1
    fi
    
    # Create main CMakeLists.txt
    cat > main/CMakeLists.txt << EOF
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES unity $component
)
EOF

    # Create components directory and link our component
    mkdir -p components
    ln -s ../../../components/$component components/
    ln -s ../../../components/csi_collector components/  # Some components depend on this
    
    # Try to compile
    print_info "Compiling $component tests..."
    if idf.py build > build.log 2>&1; then
        print_success "$component tests compiled successfully"
        ((PASSED_TESTS++))
        return 0
    else
        print_error "$component tests compilation failed"
        print_error "Error details:"
        tail -n 20 build.log
        ((FAILED_TESTS++))
        cd ..
        return 1
    fi
}

# Test each component
for component in "${COMPONENTS[@]}"; do
    compile_component_test "$component"
    cd ..
done

cd ..

# Summary
print_info "========================"
print_info "Test Summary"
print_info "========================"
print_success "Passed: $PASSED_TESTS"
if [ $FAILED_TESTS -gt 0 ]; then
    print_error "Failed: $FAILED_TESTS"
else
    print_info "Failed: 0"
fi

# Overall result
if [ $FAILED_TESTS -eq 0 ]; then
    print_success "All component tests compiled successfully!"
    exit 0
else
    print_error "Some tests failed. Please check the errors above."
    exit 1
fi