#!/bin/bash

# CSI Firmware Build Script
# This script provides convenient build commands for development

set -e  # Exit on any error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check ESP-IDF environment
check_environment() {
    print_status "Checking ESP-IDF environment..."
    
    if [ -z "$IDF_PATH" ]; then
        print_error "ESP-IDF environment not set. Please run: . $HOME/esp/esp-idf/export.sh"
        exit 1
    fi
    
    if ! command -v idf.py &> /dev/null; then
        print_error "idf.py command not found. Please check ESP-IDF installation."
        exit 1
    fi
    
    print_success "ESP-IDF environment OK"
}

# Function to build firmware
build_firmware() {
    print_status "Building CSI firmware..."
    cd "$PROJECT_DIR"
    
    if idf.py build; then
        print_success "Firmware build completed successfully"
    else
        print_error "Firmware build failed"
        exit 1
    fi
}

# Function to build tests
build_tests() {
    print_status "Building unit tests..."
    cd "$PROJECT_DIR/test"
    
    if idf.py build; then
        print_success "Test build completed successfully"
    else
        print_error "Test build failed"
        exit 1
    fi
}

# Function to run static analysis
run_analysis() {
    print_status "Running static analysis..."
    cd "$PROJECT_DIR"
    
    # Check for clang-tidy
    if command -v clang-tidy &> /dev/null; then
        print_status "Running clang-tidy analysis..."
        find . -name "*.c" -not -path "./build/*" | xargs clang-tidy
        print_success "Static analysis completed"
    else
        print_warning "clang-tidy not found, skipping static analysis"
    fi
}

# Function to clean build artifacts
clean_build() {
    print_status "Cleaning build artifacts..."
    cd "$PROJECT_DIR"
    
    idf.py clean
    rm -rf build/
    
    cd "$PROJECT_DIR/test"
    idf.py clean
    rm -rf build/
    
    print_success "Build artifacts cleaned"
}

# Function to flash firmware
flash_firmware() {
    print_status "Flashing firmware to device..."
    cd "$PROJECT_DIR"
    
    if idf.py flash; then
        print_success "Firmware flashed successfully"
    else
        print_error "Firmware flash failed"
        exit 1
    fi
}

# Function to monitor serial output
monitor_device() {
    print_status "Starting serial monitor..."
    cd "$PROJECT_DIR"
    idf.py monitor
}

# Function to show help
show_help() {
    echo "CSI Firmware Build Script"
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  build      Build the firmware"
    echo "  test       Build and run unit tests"
    echo "  analyze    Run static code analysis"
    echo "  clean      Clean build artifacts"
    echo "  flash      Flash firmware to device"
    echo "  monitor    Start serial monitor"
    echo "  all        Build firmware and tests"
    echo "  help       Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 test"
    echo "  $0 flash monitor"
}

# Main script logic
main() {
    check_environment
    
    case "${1:-help}" in
        "build")
            build_firmware
            ;;
        "test")
            build_tests
            cd "$PROJECT_DIR/test"
            print_status "Running unit tests..."
            # Note: Actual test execution requires hardware or emulator
            print_warning "Unit tests require hardware or emulator to execute"
            ;;
        "analyze")
            run_analysis
            ;;
        "clean")
            clean_build
            ;;
        "flash")
            build_firmware
            flash_firmware
            ;;
        "monitor")
            monitor_device
            ;;
        "all")
            build_firmware
            build_tests
            ;;
        "help")
            show_help
            ;;
        *)
            print_error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

# Run main function with all arguments
main "$@"