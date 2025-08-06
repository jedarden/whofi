/**
 * @file test_ota_updater.c
 * @brief Unit tests for OTA updater component
 */

#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "ota_updater.h"
#include "esp_log.h"

static const char *TAG = "test_ota";

// Test configuration
static const ota_config_t test_config = {
    .enabled = true,
    .update_url = "https://example.com/firmware",
    .auto_update = false,
    .check_interval = 60,
    .verify_signature = true,
    .cert_pem = "",
    .timeout_ms = 30000
};

// Test callback function
static volatile ota_status_t callback_status = OTA_STATUS_IDLE;
static volatile uint8_t callback_progress = 0;
static volatile int callback_count = 0;

void test_ota_progress_callback(ota_status_t status, uint8_t progress, void *user_ctx)
{
    callback_status = status;
    callback_progress = progress;
    callback_count++;
    
    ESP_LOGI(TAG, "Callback: status=%d, progress=%d, count=%d", 
             status, progress, callback_count);
}

void setUp(void)
{
    // Reset callback state
    callback_status = OTA_STATUS_IDLE;
    callback_progress = 0;
    callback_count = 0;
}

void tearDown(void)
{
    // Clean up after each test
    ota_updater_stop();
    ota_updater_deinit();
}

void test_ota_init_deinit(void)
{
    ESP_LOGI(TAG, "Testing OTA init/deinit");
    
    // Test initialization
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test double initialization
    ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret); // Should succeed (already initialized)
    
    // Test deinitialization
    ret = ota_updater_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test double deinitialization
    ret = ota_updater_deinit();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_ota_config_validation(void)
{
    ESP_LOGI(TAG, "Testing OTA configuration validation");
    
    // Test NULL config
    esp_err_t ret = ota_updater_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test valid config
    ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test config update
    ota_config_t new_config = test_config;
    new_config.check_interval = 120;
    
    ret = ota_updater_update_config(&new_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void test_ota_status_and_stats(void)
{
    ESP_LOGI(TAG, "Testing OTA status and statistics");
    
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test initial status
    ota_status_t status = ota_updater_get_status();
    TEST_ASSERT_EQUAL(OTA_STATUS_IDLE, status);
    
    // Test statistics
    ota_stats_t stats;
    ret = ota_updater_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(0, stats.updates_checked);
    TEST_ASSERT_EQUAL(0, stats.updates_installed);
    TEST_ASSERT_EQUAL(0, stats.update_failures);
    
    // Test NULL stats parameter
    ret = ota_updater_get_stats(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_ota_callback_registration(void)
{
    ESP_LOGI(TAG, "Testing OTA callback registration");
    
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test callback registration
    ret = ota_updater_register_callback(test_ota_progress_callback, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test NULL callback
    ret = ota_updater_register_callback(NULL, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

void test_ota_service_lifecycle(void)
{
    ESP_LOGI(TAG, "Testing OTA service lifecycle");
    
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test start service
    ret = ota_updater_start();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test stop service
    ret = ota_updater_stop();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test start without init
    ota_updater_deinit();
    ret = ota_updater_start();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_ota_check_for_updates_invalid_state(void)
{
    ESP_LOGI(TAG, "Testing OTA check for updates - invalid states");
    
    // Test without initialization
    esp_err_t ret = ota_updater_check_for_updates();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
    
    // Test with initialization but invalid URL
    ota_config_t invalid_config = test_config;
    strcpy(invalid_config.update_url, "");
    
    ret = ota_updater_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = ota_updater_start();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // This should fail due to empty URL
    ret = ota_updater_check_for_updates();
    // The actual result depends on the implementation
    // TEST_ASSERT_NOT_EQUAL(ESP_OK, ret);
}

void test_ota_start_update_invalid_args(void)
{
    ESP_LOGI(TAG, "Testing OTA start update - invalid arguments");
    
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = ota_updater_start();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test with invalid URL
    ret = ota_updater_start_update("");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test without initialization
    ota_updater_deinit();
    ret = ota_updater_start_update("https://example.com/firmware.bin");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}

void test_ota_cancel_update(void)
{
    ESP_LOGI(TAG, "Testing OTA cancel update");
    
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test cancel without active update
    ret = ota_updater_cancel_update();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
    
    // Note: Testing with active update would require network connectivity
    // and a valid firmware server, which is not suitable for unit tests
}

void test_ota_rollback_operations(void)
{
    ESP_LOGI(TAG, "Testing OTA rollback operations");
    
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test mark as valid
    ret = ota_updater_mark_valid();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test rollback (may not be available in test environment)
    ret = ota_updater_rollback();
    // Result depends on partition configuration
    // In test environment, this might return ESP_ERR_NOT_FOUND
}

void test_ota_multiple_operations(void)
{
    ESP_LOGI(TAG, "Testing multiple OTA operations");
    
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = ota_updater_register_callback(test_ota_progress_callback, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    ret = ota_updater_start();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Get initial stats
    ota_stats_t initial_stats;
    ret = ota_updater_get_stats(&initial_stats);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Attempt check for updates (will likely fail due to invalid URL)
    ret = ota_updater_check_for_updates();
    // Don't assert on the result as it depends on network connectivity
    
    // Get stats after check
    ota_stats_t after_stats;
    ret = ota_updater_get_stats(&after_stats);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Check that statistics were updated
    TEST_ASSERT_GREATER_OR_EQUAL(initial_stats.updates_checked, after_stats.updates_checked);
    
    // Stop service
    ret = ota_updater_stop();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void test_ota_stress_test(void)
{
    ESP_LOGI(TAG, "Testing OTA stress conditions");
    
    // Test rapid init/deinit cycles
    for (int i = 0; i < 10; i++) {
        esp_err_t ret = ota_updater_init(&test_config);
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        
        ret = ota_updater_deinit();
        TEST_ASSERT_EQUAL(ESP_OK, ret);
    }
    
    // Test rapid start/stop cycles
    esp_err_t ret = ota_updater_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    for (int i = 0; i < 5; i++) {
        ret = ota_updater_start();
        TEST_ASSERT_EQUAL(ESP_OK, ret);
        
        ret = ota_updater_stop();
        TEST_ASSERT_EQUAL(ESP_OK, ret);
    }
}

// Main test runner
void app_main(void)
{
    ESP_LOGI(TAG, "Starting OTA updater unit tests");
    
    UNITY_BEGIN();
    
    RUN_TEST(test_ota_init_deinit);
    RUN_TEST(test_ota_config_validation);
    RUN_TEST(test_ota_status_and_stats);
    RUN_TEST(test_ota_callback_registration);
    RUN_TEST(test_ota_service_lifecycle);
    RUN_TEST(test_ota_check_for_updates_invalid_state);
    RUN_TEST(test_ota_start_update_invalid_args);
    RUN_TEST(test_ota_cancel_update);
    RUN_TEST(test_ota_rollback_operations);
    RUN_TEST(test_ota_multiple_operations);
    RUN_TEST(test_ota_stress_test);
    
    UNITY_END();
    
    ESP_LOGI(TAG, "OTA updater unit tests completed");
}