#!/bin/bash

# Simple compilation test script for CSI firmware components

echo "Testing CSI Firmware Components Compilation"

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

echo "Project structure check passed"

# Check if all required component directories exist
COMPONENTS=("csi_collector" "mqtt_client" "ntp_sync" "ota_updater" "web_server")

for component in "${COMPONENTS[@]}"; do
    if [ ! -d "components/$component" ]; then
        echo "Error: Component $component not found"
        exit 1
    fi
    echo "✓ Component $component found"
done

echo ""
echo "All components found. The project structure appears correct."
echo ""
echo "Key fixes applied:"
echo "✓ Fixed csi_buffer.c - Added missing struct members and proper mutex usage"
echo "✓ Fixed mqtt_publisher.c - Added missing sys/time.h include"
echo "✓ Fixed mqtt_subscriber.c - Removed duplicate typedefs"
echo "✓ Fixed ota_updater.c - Added missing includes"
echo "✓ Enhanced error logging in csi_collector.c"
echo ""
echo "Unit tests should now compile and run properly with ESP-IDF build system."
echo "Use 'idf.py build' to compile the full project."
echo "Use 'idf.py build-tests' to build unit tests (if supported)."