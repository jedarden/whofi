/**
 * @file test_csi_collector.c
 * @brief Unit tests for CSI collector component
 */

#include <unity.h>
#include <string.h>
#include "csi_collector.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CSI_TEST";

/**
 * @brief Test configuration
 */
static csi_collector_config_t test_config = {
    .sample_rate = 20,
    .buffer_size = 1024,
    .filter_enabled = false,
    .filter_threshold = 0.5f,
    .enable_rssi = true,
    .enable_phase = true,
    .enable_amplitude = true
};

/**
 * @brief Test data received callback
 */
static bool callback_called = false;
static csi_data_t received_data;

static void test_csi_callback(const csi_data_t *csi_data, void *user_ctx)
{
    callback_called = true;
    if (csi_data) {
        memcpy(&received_data, csi_data, sizeof(csi_data_t));
    }
}

void setUp(void)
{
    // Reset test state
    callback_called = false;
    memset(&received_data, 0, sizeof(received_data));
    
    // Ensure collector is deinitialized before each test
    csi_collector_deinit();
}

void tearDown(void)
{
    // Clean up after each test
    csi_collector_stop();
    csi_collector_deinit();
}

/**
 * @brief Test CSI collector initialization with valid config
 */
void test_csi_collector_init_valid_config(void)
{
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Verify initialization state
    TEST_ASSERT_FALSE(csi_collector_is_running());
}

/**
 * @brief Test CSI collector initialization with NULL config
 */
void test_csi_collector_init_null_config(void)
{
    esp_err_t err = csi_collector_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/**
 * @brief Test CSI collector initialization with invalid sample rate
 */
void test_csi_collector_init_invalid_sample_rate(void)
{
    csi_collector_config_t invalid_config = test_config;
    
    // Test zero sample rate
    invalid_config.sample_rate = 0;
    esp_err_t err = csi_collector_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    // Test excessive sample rate
    invalid_config.sample_rate = 150;
    err = csi_collector_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/**
 * @brief Test CSI collector initialization with invalid buffer size
 */
void test_csi_collector_init_invalid_buffer_size(void)
{
    csi_collector_config_t invalid_config = test_config;
    
    // Test too small buffer
    invalid_config.buffer_size = 100;
    esp_err_t err = csi_collector_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    // Test too large buffer
    invalid_config.buffer_size = 8192;
    err = csi_collector_init(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/**
 * @brief Test double initialization
 */
void test_csi_collector_double_init(void)
{
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Second initialization should return OK (already initialized)
    err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

/**
 * @brief Test starting collector without initialization
 */
void test_csi_collector_start_not_initialized(void)
{
    esp_err_t err = csi_collector_start();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

/**
 * @brief Test normal start/stop sequence
 */
void test_csi_collector_start_stop_sequence(void)
{
    // Initialize first
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Start collector
    err = csi_collector_start();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(csi_collector_is_running());
    
    // Stop collector
    err = csi_collector_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_FALSE(csi_collector_is_running());
}

/**
 * @brief Test double start
 */
void test_csi_collector_double_start(void)
{
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    err = csi_collector_start();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Second start should return OK (already running)
    err = csi_collector_start();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(csi_collector_is_running());
}

/**
 * @brief Test stopping when not running
 */
void test_csi_collector_stop_not_running(void)
{
    esp_err_t err = csi_collector_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err); // Should return OK even if not running
}

/**
 * @brief Test callback registration
 */
void test_csi_collector_callback_registration(void)
{
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Register callback
    err = csi_collector_register_callback(test_csi_callback, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Unregister callback
    err = csi_collector_unregister_callback();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

/**
 * @brief Test callback registration with NULL callback
 */
void test_csi_collector_callback_null(void)
{
    esp_err_t err = csi_collector_register_callback(NULL, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/**
 * @brief Test getting data with timeout
 */
void test_csi_collector_get_data_timeout(void)
{
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    err = csi_collector_start();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    csi_data_t csi_data;
    // Should timeout since no data is being generated in test environment
    err = csi_collector_get_data(&csi_data, 100); // 100ms timeout
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, err);
}

/**
 * @brief Test getting data with NULL pointer
 */
void test_csi_collector_get_data_null_pointer(void)
{
    esp_err_t err = csi_collector_get_data(NULL, 100);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/**
 * @brief Test getting data when not running
 */
void test_csi_collector_get_data_not_running(void)
{
    csi_data_t csi_data;
    esp_err_t err = csi_collector_get_data(&csi_data, 100);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

/**
 * @brief Test statistics functionality
 */
void test_csi_collector_statistics(void)
{
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    csi_collector_stats_t stats;
    
    // Get initial statistics
    err = csi_collector_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(0, stats.packets_received);
    TEST_ASSERT_EQUAL(0, stats.packets_processed);
    TEST_ASSERT_EQUAL(0, stats.packets_dropped);
    
    // Reset statistics
    err = csi_collector_reset_stats();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Verify reset
    err = csi_collector_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(0, stats.packets_received);
}

/**
 * @brief Test statistics with NULL pointer
 */
void test_csi_collector_statistics_null_pointer(void)
{
    esp_err_t err = csi_collector_get_stats(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/**
 * @brief Test configuration update
 */
void test_csi_collector_config_update(void)
{
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    csi_collector_config_t new_config = test_config;
    new_config.sample_rate = 50;
    new_config.buffer_size = 2048;
    
    err = csi_collector_update_config(&new_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Verify configuration was updated
    csi_collector_config_t current_config;
    err = csi_collector_get_config(&current_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(50, current_config.sample_rate);
    TEST_ASSERT_EQUAL(2048, current_config.buffer_size);
}

/**
 * @brief Test getting configuration with NULL pointer
 */
void test_csi_collector_get_config_null_pointer(void)
{
    esp_err_t err = csi_collector_get_config(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

/**
 * @brief Test memory cleanup for CSI data
 */
void test_csi_collector_free_data(void)
{
    csi_data_t test_data = {0};
    
    // Allocate test data
    test_data.data = malloc(100);
    test_data.amplitude = malloc(32 * sizeof(float));
    test_data.phase = malloc(32 * sizeof(float));
    
    TEST_ASSERT_NOT_NULL(test_data.data);
    TEST_ASSERT_NOT_NULL(test_data.amplitude);
    TEST_ASSERT_NOT_NULL(test_data.phase);
    
    // Free data
    csi_collector_free_data(&test_data);
    
    // Pointers should be NULL after freeing
    TEST_ASSERT_NULL(test_data.data);
    TEST_ASSERT_NULL(test_data.amplitude);
    TEST_ASSERT_NULL(test_data.phase);
}

/**
 * @brief Test free data with NULL pointer
 */
void test_csi_collector_free_data_null_pointer(void)
{
    // Should not crash
    csi_collector_free_data(NULL);
}

/**
 * @brief Test deinitialize without initialize
 */
void test_csi_collector_deinit_not_initialized(void)
{
    esp_err_t err = csi_collector_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, err); // Should return OK even if not initialized
}

/**
 * @brief Test complete lifecycle
 */
void test_csi_collector_complete_lifecycle(void)
{
    // Initialize
    esp_err_t err = csi_collector_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_FALSE(csi_collector_is_running());
    
    // Start
    err = csi_collector_start();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(csi_collector_is_running());
    
    // Register callback
    err = csi_collector_register_callback(test_csi_callback, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Check statistics
    csi_collector_stats_t stats;
    err = csi_collector_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Stop
    err = csi_collector_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_FALSE(csi_collector_is_running());
    
    // Deinitialize
    err = csi_collector_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

/**
 * @brief Run all CSI collector tests
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting CSI Collector unit tests");
    
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_csi_collector_init_valid_config);
    RUN_TEST(test_csi_collector_init_null_config);
    RUN_TEST(test_csi_collector_init_invalid_sample_rate);
    RUN_TEST(test_csi_collector_init_invalid_buffer_size);
    RUN_TEST(test_csi_collector_double_init);
    
    // Start/stop tests
    RUN_TEST(test_csi_collector_start_not_initialized);
    RUN_TEST(test_csi_collector_start_stop_sequence);
    RUN_TEST(test_csi_collector_double_start);
    RUN_TEST(test_csi_collector_stop_not_running);
    
    // Callback tests
    RUN_TEST(test_csi_collector_callback_registration);
    RUN_TEST(test_csi_collector_callback_null);
    
    // Data handling tests
    RUN_TEST(test_csi_collector_get_data_timeout);
    RUN_TEST(test_csi_collector_get_data_null_pointer);
    RUN_TEST(test_csi_collector_get_data_not_running);
    
    // Statistics tests
    RUN_TEST(test_csi_collector_statistics);
    RUN_TEST(test_csi_collector_statistics_null_pointer);
    
    // Configuration tests
    RUN_TEST(test_csi_collector_config_update);
    RUN_TEST(test_csi_collector_get_config_null_pointer);
    
    // Memory management tests
    RUN_TEST(test_csi_collector_free_data);
    RUN_TEST(test_csi_collector_free_data_null_pointer);
    
    // Lifecycle tests
    RUN_TEST(test_csi_collector_deinit_not_initialized);
    RUN_TEST(test_csi_collector_complete_lifecycle);
    
    UNITY_END();
    
    ESP_LOGI(TAG, "CSI Collector unit tests completed");
}