/**
 * @file test_main.c
 * @brief Main test runner for CSI firmware
 */

#include <stdio.h>
#include <unity.h>
#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "TEST_MAIN";

/**
 * @brief Test runner task
 */
static void test_runner_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting CSI Firmware Test Suite");
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // Allow system to stabilize
    
    // Initialize Unity test framework
    UNITY_BEGIN();
    
    ESP_LOGI(TAG, "Running all component tests...");
    
    // Run all tests here
    // Tests are registered in individual component test files
    
    UNITY_END();
    
    ESP_LOGI(TAG, "Test suite completed");
    
    // Clean shutdown
    vTaskDelete(NULL);
}

/**
 * @brief Main test application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "CSI Firmware Test Suite starting...");
    
    // Create test runner task
    xTaskCreate(
        test_runner_task,
        "test_runner",
        8192,  // Stack size
        NULL,  // Parameters
        5,     // Priority
        NULL   // Task handle
    );
}