/**
 * @file test_web_server.c
 * @brief Unit tests for Web server component
 * 
 * Contains comprehensive unit tests for the web configuration server,
 * including authentication, configuration management, and statistics.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unity.h>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "web_server.h"

static const char *TAG = "WEB_TEST";

// Test configuration
static web_server_config_t test_config = {
    .enabled = true,
    .port = 8080,
    .auth_enabled = true,
    .username = "admin",
    .password = "password123",
    .max_sessions = 5,
    .session_timeout = 30
};

void setUp(void)
{
    // Clean up before each test
    web_server_stop();
}

void tearDown(void)
{
    // Clean up after each test
    web_server_stop();
}

void test_web_server_start_valid_config(void)
{
    ESP_LOGI(TAG, "Testing web server start with valid config");
    
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Check if server is running
    bool running = web_server_is_running();
    TEST_ASSERT_TRUE(running);
}

void test_web_server_start_invalid_config(void)
{
    ESP_LOGI(TAG, "Testing web server start with invalid config");
    
    esp_err_t err = web_server_start(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_web_server_start_invalid_port(void)
{
    ESP_LOGI(TAG, "Testing web server start with invalid port");
    
    web_server_config_t invalid_config = test_config;
    invalid_config.port = 0;
    
    esp_err_t err = web_server_start(&invalid_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_web_server_double_start(void)
{
    ESP_LOGI(TAG, "Testing web server double start");
    
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Second start should return OK (already running)
    err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(web_server_is_running());
}

void test_web_server_stop_not_running(void)
{
    ESP_LOGI(TAG, "Testing web server stop when not running");
    
    esp_err_t err = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err); // Should return OK even if not running
    TEST_ASSERT_FALSE(web_server_is_running());
}

void test_web_server_lifecycle(void)
{
    ESP_LOGI(TAG, "Testing web server lifecycle");
    
    // Start server
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Give some time for server to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Stop server
    err = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_FALSE(web_server_is_running());
}

void test_web_server_statistics(void)
{
    ESP_LOGI(TAG, "Testing web server statistics");
    
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    web_server_stats_t stats;
    err = web_server_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Initial stats should be zero or minimal
    TEST_ASSERT_EQUAL(0, stats.total_requests);
    TEST_ASSERT_EQUAL(0, stats.active_sessions);
    TEST_ASSERT_EQUAL(0, stats.failed_auth);
    TEST_ASSERT_EQUAL(0, stats.bytes_sent);
    TEST_ASSERT_EQUAL(0, stats.bytes_received);
    
    // Reset stats
    err = web_server_reset_stats();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Verify reset
    err = web_server_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(0, stats.total_requests);
}

void test_web_server_statistics_null_pointer(void)
{
    ESP_LOGI(TAG, "Testing web server statistics with NULL pointer");
    
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    err = web_server_get_stats(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_web_server_config_update(void)
{
    ESP_LOGI(TAG, "Testing web server configuration update");
    
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    web_server_config_t new_config = test_config;
    new_config.port = 8081;
    new_config.max_sessions = 10;
    strncpy(new_config.username, "newadmin", sizeof(new_config.username) - 1);
    
    err = web_server_update_config(&new_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_web_server_config_update_null_pointer(void)
{
    ESP_LOGI(TAG, "Testing web server config update with NULL pointer");
    
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    err = web_server_update_config(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_web_server_config_update_not_running(void)
{
    ESP_LOGI(TAG, "Testing web server config update when not running");
    
    web_server_config_t new_config = test_config;
    new_config.port = 8081;
    
    esp_err_t err = web_server_update_config(&new_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

void test_web_server_auth_config(void)
{
    ESP_LOGI(TAG, "Testing web server with authentication disabled");
    
    web_server_config_t no_auth_config = test_config;
    no_auth_config.auth_enabled = false;
    
    esp_err_t err = web_server_start(&no_auth_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    // Give some time for server to initialize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    err = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_web_server_max_sessions_config(void)
{
    ESP_LOGI(TAG, "Testing web server with different max sessions");
    
    web_server_config_t config_low_sessions = test_config;
    config_low_sessions.max_sessions = 1;
    
    esp_err_t err = web_server_start(&config_low_sessions);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    web_server_config_t config_high_sessions = test_config;
    config_high_sessions.max_sessions = 20;
    
    err = web_server_update_config(&config_high_sessions);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_web_server_session_timeout_config(void)
{
    ESP_LOGI(TAG, "Testing web server with different session timeouts");
    
    web_server_config_t config_short_timeout = test_config;
    config_short_timeout.session_timeout = 5;  // 5 minutes
    
    esp_err_t err = web_server_start(&config_short_timeout);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_TRUE(web_server_is_running());
    
    web_server_config_t config_long_timeout = test_config;
    config_long_timeout.session_timeout = 120;  // 2 hours
    
    err = web_server_update_config(&config_long_timeout);
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_web_server_invalid_credentials(void)
{
    ESP_LOGI(TAG, "Testing web server with invalid credentials");
    
    web_server_config_t invalid_config = test_config;
    invalid_config.username[0] = '\0';  // Empty username
    
    esp_err_t err = web_server_start(&invalid_config);
    // Should still start but with default/no authentication
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_web_server_multiple_operations(void)
{
    ESP_LOGI(TAG, "Testing multiple web server operations");
    
    // Start server
    esp_err_t err = web_server_start(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Get initial statistics
    web_server_stats_t initial_stats;
    err = web_server_get_stats(&initial_stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Update configuration
    web_server_config_t new_config = test_config;
    new_config.session_timeout = 60;
    
    err = web_server_update_config(&new_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Get stats after configuration update
    web_server_stats_t after_stats;
    err = web_server_get_stats(&after_stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Reset statistics
    err = web_server_reset_stats();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Verify reset
    web_server_stats_t reset_stats;
    err = web_server_get_stats(&reset_stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_EQUAL(0, reset_stats.total_requests);
    
    // Stop server
    err = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    TEST_ASSERT_FALSE(web_server_is_running());
}

void test_web_server_stress_test(void)
{
    ESP_LOGI(TAG, "Testing web server stress conditions");
    
    // Test rapid start/stop cycles
    for (int i = 0; i < 5; i++) {
        esp_err_t err = web_server_start(&test_config);
        TEST_ASSERT_EQUAL(ESP_OK, err);
        
        // Short delay
        vTaskDelay(pdMS_TO_TICKS(50));
        
        err = web_server_stop();
        TEST_ASSERT_EQUAL(ESP_OK, err);
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Run all web server tests
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting Web Server unit tests");
    
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_web_server_start_valid_config);
    RUN_TEST(test_web_server_start_invalid_config);
    RUN_TEST(test_web_server_start_invalid_port);
    RUN_TEST(test_web_server_double_start);
    RUN_TEST(test_web_server_stop_not_running);
    RUN_TEST(test_web_server_lifecycle);
    
    // Statistics tests
    RUN_TEST(test_web_server_statistics);
    RUN_TEST(test_web_server_statistics_null_pointer);
    
    // Configuration tests
    RUN_TEST(test_web_server_config_update);
    RUN_TEST(test_web_server_config_update_null_pointer);
    RUN_TEST(test_web_server_config_update_not_running);
    
    // Authentication and session tests
    RUN_TEST(test_web_server_auth_config);
    RUN_TEST(test_web_server_max_sessions_config);
    RUN_TEST(test_web_server_session_timeout_config);
    RUN_TEST(test_web_server_invalid_credentials);
    
    // Complex operation tests
    RUN_TEST(test_web_server_multiple_operations);
    RUN_TEST(test_web_server_stress_test);
    
    UNITY_END();
    
    ESP_LOGI(TAG, "Web Server unit tests completed");
}