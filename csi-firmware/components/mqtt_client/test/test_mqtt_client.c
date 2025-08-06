/**
 * @file test_mqtt_client.c
 * @brief Unit tests for MQTT client component
 * 
 * Contains comprehensive unit tests for the MQTT client wrapper,
 * publisher utilities, and subscriber utilities.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unity.h>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mqtt_client_wrapper.h"

static const char *TAG = "MQTT_TEST";

// Test configuration
static mqtt_config_t test_config = {
    .enabled = true,
    .broker_url = "test.mosquitto.org",
    .port = 1883,
    .username = "",
    .password = "",
    .client_id = "test_esp32_client",
    .topic_prefix = "test/esp32",
    .ssl_enabled = false,
    .keepalive = 60,
    .qos = 1,
    .retain = false
};

// Test callback data
static int callback_count = 0;
static char last_topic[128] = {0};
static char last_message[256] = {0};

// Test callback function
static void test_message_callback(const char *topic, const char *data, int data_len, void *user_ctx)
{
    callback_count++;
    strncpy(last_topic, topic, sizeof(last_topic) - 1);
    
    int copy_len = data_len < sizeof(last_message) - 1 ? data_len : sizeof(last_message) - 1;
    memcpy(last_message, data, copy_len);
    last_message[copy_len] = '\0';
    
    ESP_LOGI(TAG, "Test callback received: topic=%s, data=%s", topic, last_message);
}

void setUp(void)
{
    // Reset test state
    callback_count = 0;
    memset(last_topic, 0, sizeof(last_topic));
    memset(last_message, 0, sizeof(last_message));
}

void tearDown(void)
{
    // Clean up after each test
    mqtt_client_deinit();
}

void test_mqtt_client_init_valid_config(void)
{
    ESP_LOGI(TAG, "Testing MQTT client initialization with valid config");
    
    esp_err_t err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Test that we can't initialize twice
    err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

void test_mqtt_client_init_invalid_config(void)
{
    ESP_LOGI(TAG, "Testing MQTT client initialization with invalid config");
    
    esp_err_t err = mqtt_client_init(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_client_operations_not_initialized(void)
{
    ESP_LOGI(TAG, "Testing MQTT client operations without initialization");
    
    esp_err_t err = mqtt_client_start();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    
    err = mqtt_client_stop();
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    
    bool connected = mqtt_client_is_connected();
    TEST_ASSERT_FALSE(connected);
}

void test_mqtt_client_lifecycle(void)
{
    ESP_LOGI(TAG, "Testing MQTT client lifecycle");
    
    // Initialize
    esp_err_t err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Start
    err = mqtt_client_start();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Give some time for connection attempt
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Stop
    err = mqtt_client_stop();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Deinitialize
    err = mqtt_client_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

void test_mqtt_client_publish_without_connection(void)
{
    ESP_LOGI(TAG, "Testing MQTT publish without connection");
    
    esp_err_t err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Try to publish without connection
    const char *test_data = "test message";
    err = mqtt_client_publish("test/topic", test_data, strlen(test_data), 0, false);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
}

void test_mqtt_client_callback_registration(void)
{
    ESP_LOGI(TAG, "Testing MQTT callback registration");
    
    esp_err_t err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Register callback
    err = mqtt_client_register_callback(test_message_callback, NULL);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Test invalid callback
    err = mqtt_client_register_callback(NULL, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_client_statistics(void)
{
    ESP_LOGI(TAG, "Testing MQTT client statistics");
    
    esp_err_t err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    mqtt_stats_t stats;
    err = mqtt_client_get_stats(&stats);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Initial stats should be zero
    TEST_ASSERT_EQUAL(0, stats.messages_sent);
    TEST_ASSERT_EQUAL(0, stats.messages_received);
    TEST_ASSERT_EQUAL(0, stats.connection_errors);
    TEST_ASSERT_EQUAL(0, stats.publish_errors);
    TEST_ASSERT_FALSE(stats.connected);
    
    // Reset stats
    err = mqtt_client_reset_stats();
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Test invalid stats pointer
    err = mqtt_client_get_stats(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_client_publish_invalid_args(void)
{
    ESP_LOGI(TAG, "Testing MQTT publish with invalid arguments");
    
    esp_err_t err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    const char *test_data = "test message";
    
    // Test invalid topic
    err = mqtt_client_publish(NULL, test_data, strlen(test_data), 0, false);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    // Test invalid data
    err = mqtt_client_publish("test/topic", NULL, strlen(test_data), 0, false);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    // Test invalid data length
    err = mqtt_client_publish("test/topic", test_data, -1, 0, false);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_client_subscribe_operations(void)
{
    ESP_LOGI(TAG, "Testing MQTT subscribe operations");
    
    esp_err_t err = mqtt_client_init(&test_config);
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Test subscribe without connection (should fail)
    err = mqtt_client_subscribe("test/topic", 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    
    // Test unsubscribe without connection (should fail)
    err = mqtt_client_unsubscribe("test/topic");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, err);
    
    // Test invalid arguments
    err = mqtt_client_subscribe(NULL, 1);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    err = mqtt_client_unsubscribe(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_publish_device_status(void)
{
    ESP_LOGI(TAG, "Testing MQTT device status publishing");
    
    esp_err_t err = mqtt_publish_device_status("test_device", "1.0.0", 12345, -45, 100000);
    // Should fail without initialized client
    TEST_ASSERT_NOT_EQUAL(ESP_OK, err);
    
    // Test invalid arguments
    err = mqtt_publish_device_status(NULL, "1.0.0", 12345, -45, 100000);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    err = mqtt_publish_device_status("test_device", NULL, 12345, -45, 100000);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_publish_system_metrics(void)
{
    ESP_LOGI(TAG, "Testing MQTT system metrics publishing");
    
    esp_err_t err = mqtt_publish_system_metrics("test_device", 25.5f, 100000, 80000, 10);
    // Should fail without initialized client
    TEST_ASSERT_NOT_EQUAL(ESP_OK, err);
    
    // Test invalid arguments
    err = mqtt_publish_system_metrics(NULL, 25.5f, 100000, 80000, 10);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_publish_alert(void)
{
    ESP_LOGI(TAG, "Testing MQTT alert publishing");
    
    esp_err_t err = mqtt_publish_alert("test_device", "ERROR", "CSI_COLLECTOR", "Test error message");
    // Should fail without initialized client
    TEST_ASSERT_NOT_EQUAL(ESP_OK, err);
    
    // Test invalid arguments
    err = mqtt_publish_alert(NULL, "ERROR", "CSI_COLLECTOR", "Test error message");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    err = mqtt_publish_alert("test_device", NULL, "CSI_COLLECTOR", "Test error message");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    err = mqtt_publish_alert("test_device", "ERROR", NULL, "Test error message");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    err = mqtt_publish_alert("test_device", "ERROR", "CSI_COLLECTOR", NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_subscriber_device_topics(void)
{
    ESP_LOGI(TAG, "Testing MQTT subscriber device topics");
    
    esp_err_t err = mqtt_subscriber_subscribe_device_topics("test_device");
    // Should fail without initialized client
    TEST_ASSERT_NOT_EQUAL(ESP_OK, err);
    
    err = mqtt_subscriber_unsubscribe_device_topics("test_device");
    // Should succeed even without connection
    TEST_ASSERT_EQUAL(ESP_OK, err);
    
    // Test invalid arguments
    err = mqtt_subscriber_subscribe_device_topics(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    err = mqtt_subscriber_unsubscribe_device_topics(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void test_mqtt_subscriber_handler_registration(void)
{
    ESP_LOGI(TAG, "Testing MQTT subscriber handler registration");
    
    // Test config handler registration
    esp_err_t err = mqtt_subscriber_register_config_handler(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    // Test command handler registration
    err = mqtt_subscriber_register_command_handler(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
    
    // Test OTA handler registration
    err = mqtt_subscriber_register_ota_handler(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, err);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting MQTT Client Unit Tests");
    
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_mqtt_client_init_valid_config);
    RUN_TEST(test_mqtt_client_init_invalid_config);
    RUN_TEST(test_mqtt_client_operations_not_initialized);
    RUN_TEST(test_mqtt_client_lifecycle);
    RUN_TEST(test_mqtt_client_callback_registration);
    RUN_TEST(test_mqtt_client_statistics);
    
    // Publish/Subscribe tests
    RUN_TEST(test_mqtt_client_publish_without_connection);
    RUN_TEST(test_mqtt_client_publish_invalid_args);
    RUN_TEST(test_mqtt_client_subscribe_operations);
    
    // Publisher utility tests
    RUN_TEST(test_mqtt_publish_device_status);
    RUN_TEST(test_mqtt_publish_system_metrics);
    RUN_TEST(test_mqtt_publish_alert);
    
    // Subscriber utility tests
    RUN_TEST(test_mqtt_subscriber_device_topics);
    RUN_TEST(test_mqtt_subscriber_handler_registration);
    
    UNITY_END();
    
    ESP_LOGI(TAG, "MQTT Client Unit Tests Completed");
}