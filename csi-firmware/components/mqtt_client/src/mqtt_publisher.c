/**
 * @file mqtt_publisher.c
 * @brief MQTT publisher utility functions
 * 
 * Contains utility functions for publishing different types of messages
 * and handling message formatting for the MQTT client component.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <esp_log.h>
#include <esp_system.h>
#include <cJSON.h>

#include "mqtt_client_wrapper.h"

static const char *TAG = "MQTT_PUB";

/**
 * @brief Publish device status information
 */
esp_err_t mqtt_publish_device_status(const char *device_id, const char *version, 
                                    uint32_t uptime, int8_t wifi_rssi, uint32_t free_heap)
{
    if (!device_id || !version) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "Failed to create JSON object for device status");
        return ESP_ERR_NO_MEM;
    }

    // Add device information
    cJSON_AddStringToObject(json, "device_id", device_id);
    cJSON_AddStringToObject(json, "version", version);
    cJSON_AddNumberToObject(json, "uptime", uptime);
    cJSON_AddNumberToObject(json, "wifi_rssi", wifi_rssi);
    cJSON_AddNumberToObject(json, "free_heap", free_heap);
    
    // Add timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = tv.tv_sec * 1000000ULL + tv.tv_usec;
    cJSON_AddNumberToObject(json, "timestamp", timestamp);

    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "Failed to serialize device status JSON");
        return ESP_ERR_NO_MEM;
    }

    char topic[128];
    snprintf(topic, sizeof(topic), "devices/%s/status", device_id);

    esp_err_t err = mqtt_client_publish(topic, json_string, strlen(json_string), 1, true);
    
    free(json_string);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Device status published successfully");
    } else {
        ESP_LOGE(TAG, "Failed to publish device status: %s", esp_err_to_name(err));
    }

    return err;
}

/**
 * @brief Publish system metrics
 */
esp_err_t mqtt_publish_system_metrics(const char *device_id, float cpu_usage, 
                                     uint32_t free_heap, uint32_t min_free_heap,
                                     uint32_t task_count)
{
    if (!device_id) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "Failed to create JSON object for system metrics");
        return ESP_ERR_NO_MEM;
    }

    // Add system metrics
    cJSON_AddNumberToObject(json, "cpu_usage", cpu_usage);
    cJSON_AddNumberToObject(json, "free_heap", free_heap);
    cJSON_AddNumberToObject(json, "min_free_heap", min_free_heap);
    cJSON_AddNumberToObject(json, "task_count", task_count);
    
    // Add timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = tv.tv_sec * 1000000ULL + tv.tv_usec;
    cJSON_AddNumberToObject(json, "timestamp", timestamp);

    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "Failed to serialize system metrics JSON");
        return ESP_ERR_NO_MEM;
    }

    char topic[128];
    snprintf(topic, sizeof(topic), "devices/%s/metrics", device_id);

    esp_err_t err = mqtt_client_publish(topic, json_string, strlen(json_string), 0, false);
    
    free(json_string);

    if (err == ESP_OK) {
        ESP_LOGD(TAG, "System metrics published successfully");
    } else {
        ESP_LOGE(TAG, "Failed to publish system metrics: %s", esp_err_to_name(err));
    }

    return err;
}

/**
 * @brief Publish error/alert message
 */
esp_err_t mqtt_publish_alert(const char *device_id, const char *level, 
                           const char *component, const char *message)
{
    if (!device_id || !level || !component || !message) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "Failed to create JSON object for alert");
        return ESP_ERR_NO_MEM;
    }

    // Add alert information
    cJSON_AddStringToObject(json, "level", level);
    cJSON_AddStringToObject(json, "component", component);
    cJSON_AddStringToObject(json, "message", message);
    
    // Add timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = tv.tv_sec * 1000000ULL + tv.tv_usec;
    cJSON_AddNumberToObject(json, "timestamp", timestamp);

    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "Failed to serialize alert JSON");
        return ESP_ERR_NO_MEM;
    }

    char topic[128];
    snprintf(topic, sizeof(topic), "devices/%s/alerts", device_id);

    esp_err_t err = mqtt_client_publish(topic, json_string, strlen(json_string), 1, false);
    
    free(json_string);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Alert published successfully: %s - %s", level, message);
    } else {
        ESP_LOGE(TAG, "Failed to publish alert: %s", esp_err_to_name(err));
    }

    return err;
}

/**
 * @brief Publish configuration acknowledgment
 */
esp_err_t mqtt_publish_config_ack(const char *device_id, const char *config_id, 
                                 bool success, const char *error_msg)
{
    if (!device_id || !config_id) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "Failed to create JSON object for config ack");
        return ESP_ERR_NO_MEM;
    }

    // Add acknowledgment information
    cJSON_AddStringToObject(json, "config_id", config_id);
    cJSON_AddBoolToObject(json, "success", success);
    
    if (error_msg && !success) {
        cJSON_AddStringToObject(json, "error", error_msg);
    }
    
    // Add timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = tv.tv_sec * 1000000ULL + tv.tv_usec;
    cJSON_AddNumberToObject(json, "timestamp", timestamp);

    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "Failed to serialize config ack JSON");
        return ESP_ERR_NO_MEM;
    }

    char topic[128];
    snprintf(topic, sizeof(topic), "devices/%s/config/ack", device_id);

    esp_err_t err = mqtt_client_publish(topic, json_string, strlen(json_string), 1, false);
    
    free(json_string);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Config ack published: %s - %s", config_id, success ? "SUCCESS" : "FAILED");
    } else {
        ESP_LOGE(TAG, "Failed to publish config ack: %s", esp_err_to_name(err));
    }

    return err;
}

/**
 * @brief Publish last will and testament message
 */
esp_err_t mqtt_publish_last_will(const char *device_id)
{
    if (!device_id) {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *json = cJSON_CreateObject();
    if (!json) {
        ESP_LOGE(TAG, "Failed to create JSON object for last will");
        return ESP_ERR_NO_MEM;
    }

    // Add status information
    cJSON_AddStringToObject(json, "status", "offline");
    cJSON_AddStringToObject(json, "reason", "unexpected_disconnect");
    
    // Add timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t timestamp = tv.tv_sec * 1000000ULL + tv.tv_usec;
    cJSON_AddNumberToObject(json, "timestamp", timestamp);

    char *json_string = cJSON_Print(json);
    cJSON_Delete(json);

    if (!json_string) {
        ESP_LOGE(TAG, "Failed to serialize last will JSON");
        return ESP_ERR_NO_MEM;
    }

    char topic[128];
    snprintf(topic, sizeof(topic), "devices/%s/status", device_id);

    esp_err_t err = mqtt_client_publish(topic, json_string, strlen(json_string), 1, true);
    
    free(json_string);

    return err;
}