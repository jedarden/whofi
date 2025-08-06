#!/bin/bash

# Simple verification script for CSI firmware tests
# This checks that all test files exist and have proper structure

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}CSI Firmware Test Verification${NC}"
echo -e "${BLUE}==============================${NC}"
echo

# Check components
COMPONENTS=("csi_collector" "mqtt_client" "ntp_sync" "ota_updater" "web_server")
TOTAL_TESTS=0
MISSING_TESTS=0

for component in "${COMPONENTS[@]}"; do
    echo -e "${BLUE}Checking $component:${NC}"
    
    # Check if component directory exists
    if [ ! -d "components/$component" ]; then
        echo -e "  ${RED}✗ Component directory missing${NC}"
        ((MISSING_TESTS++))
        continue
    fi
    
    # Check if test directory exists
    if [ ! -d "components/$component/test" ]; then
        echo -e "  ${YELLOW}⚠ Test directory missing${NC}"
        ((MISSING_TESTS++))
        continue
    fi
    
    # Check for test file
    test_file="components/$component/test/test_${component}.c"
    if [ -f "$test_file" ]; then
        echo -e "  ${GREEN}✓ Test file found${NC}"
        
        # Count test functions
        test_count=$(grep -c "void test_" "$test_file" || true)
        echo -e "  ${GREEN}✓ Found $test_count test functions${NC}"
        ((TOTAL_TESTS += test_count))
        
        # Check for Unity framework usage
        if grep -q "#include <unity.h>" "$test_file"; then
            echo -e "  ${GREEN}✓ Unity framework included${NC}"
        else
            echo -e "  ${YELLOW}⚠ Unity framework not included${NC}"
        fi
        
        # Check for test runner
        if grep -q "UNITY_BEGIN()" "$test_file"; then
            echo -e "  ${GREEN}✓ Test runner found${NC}"
        else
            echo -e "  ${YELLOW}⚠ Test runner not found${NC}"
        fi
    else
        echo -e "  ${RED}✗ Test file missing: test_${component}.c${NC}"
        ((MISSING_TESTS++))
    fi
    
    echo
done

# Check main test runner
echo -e "${BLUE}Checking main test runner:${NC}"
if [ -f "test/main/test_main.c" ]; then
    echo -e "  ${GREEN}✓ Main test runner found${NC}"
else
    echo -e "  ${RED}✗ Main test runner missing${NC}"
fi

echo
echo -e "${BLUE}==============================${NC}"
echo -e "${BLUE}Summary:${NC}"
echo -e "  Total test functions: ${GREEN}$TOTAL_TESTS${NC}"
if [ $MISSING_TESTS -eq 0 ]; then
    echo -e "  All components have tests: ${GREEN}✓${NC}"
else
    echo -e "  Missing tests: ${RED}$MISSING_TESTS${NC}"
fi

# Check for known issues from the summary
echo
echo -e "${BLUE}Checking for known issues:${NC}"

# Check csi_buffer.c
if [ -f "components/csi_collector/src/csi_buffer.c" ]; then
    echo -e "  ${GREEN}✓ csi_buffer.c exists${NC}"
    if grep -q "SemaphoreHandle_t mutex" "components/csi_collector/src/csi_buffer.c"; then
        echo -e "  ${GREEN}✓ Mutex member found in csi_buffer${NC}"
    fi
fi

# Check mqtt_publisher.c
if [ -f "components/mqtt_client/src/mqtt_publisher.c" ]; then
    echo -e "  ${GREEN}✓ mqtt_publisher.c exists${NC}"
    if grep -q "#include <sys/time.h>" "components/mqtt_client/src/mqtt_publisher.c"; then
        echo -e "  ${GREEN}✓ sys/time.h included in mqtt_publisher${NC}"
    fi
fi

# Check mqtt_subscriber.c
if [ -f "components/mqtt_client/src/mqtt_subscriber.c" ]; then
    echo -e "  ${GREEN}✓ mqtt_subscriber.c exists${NC}"
    # Check for duplicate typedefs
    if grep -q "typedef.*config_update_handler_t" "components/mqtt_client/src/mqtt_subscriber.c"; then
        echo -e "  ${RED}✗ Duplicate typedef found in mqtt_subscriber.c${NC}"
    else
        echo -e "  ${GREEN}✓ No duplicate typedef in mqtt_subscriber.c${NC}"
    fi
fi

# Check ota_updater.c
if [ -f "components/ota_updater/src/ota_updater.c" ]; then
    echo -e "  ${GREEN}✓ ota_updater.c exists${NC}"
    required_includes=("sys/time.h" "freertos/FreeRTOS.h" "freertos/task.h")
    for inc in "${required_includes[@]}"; do
        if grep -q "#include <$inc>" "components/ota_updater/src/ota_updater.c"; then
            echo -e "  ${GREEN}✓ $inc included${NC}"
        else
            echo -e "  ${RED}✗ $inc missing${NC}"
        fi
    done
fi

echo
echo -e "${BLUE}Test structure verification complete!${NC}"