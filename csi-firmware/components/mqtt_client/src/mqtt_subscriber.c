/**
 * @file mqtt_subscriber.c
 * @brief MQTT subscriber utility functions
 * 
 * Contains utility functions for handling subscribed messages and
 * processing remote control commands via MQTT.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <esp_log.h>
#include <esp_err.h>
#include <cJSON.h>

#include "mqtt_client_wrapper.h"

static const char *TAG = "MQTT_SUB";

// Function pointer types for command handlers
typedef esp_err_t (*config_update_handler_t)(const cJSON *config);
typedef esp_err_t (*command_handler_t)(const cJSON *params);
typedef esp_err_t (*ota_handler_t)(const char *url, const char *version);

// Handler function pointers
static config_update_handler_t s_config_handler = NULL;
static command_handler_t s_command_handler = NULL;
static ota_handler_t s_ota_handler = NULL;

// Internal function declarations
static esp_err_t handle_config_update(const char *data, int data_len);
static esp_err_t handle_command(const char *data, int data_len);
static esp_err_t handle_ota_request(const char *data, int data_len);

/**
 * @brief Default message callback for MQTT subscriber
 */
void mqtt_subscriber_default_callback(const char *topic, const char *data, int data_len, void *user_ctx)
{
    if (!topic || !data || data_len <= 0) {
        ESP_LOGW(TAG, "Invalid message received");
        return;
    }

    ESP_LOGI(TAG, "Received message on topic: %s (length: %d)", topic, data_len);
    ESP_LOGD(TAG, "Message data: %.*s", data_len, data);

    // Determine message type based on topic
    if (strstr(topic, "/config") != NULL) {
        handle_config_update(data, data_len);
    } else if (strstr(topic, "/command") != NULL || strstr(topic, "/cmd") != NULL) {
        handle_command(data, data_len);
    } else if (strstr(topic, "/ota") != NULL || strstr(topic, "/update") != NULL) {
        handle_ota_request(data, data_len);
    } else {
        ESP_LOGW(TAG, "Unknown topic type: %s", topic);
    }
}

/**
 * @brief Register configuration update handler
 */
esp_err_t mqtt_subscriber_register_config_handler(config_update_handler_t handler)
{
    if (!handler) {
        return ESP_ERR_INVALID_ARG;
    }

    s_config_handler = handler;
    ESP_LOGI(TAG, "Configuration update handler registered");
    return ESP_OK;
}

/**
 * @brief Register command handler
 */
esp_err_t mqtt_subscriber_register_command_handler(command_handler_t handler)
{
    if (!handler) {
        return ESP_ERR_INVALID_ARG;
    }

    s_command_handler = handler;
    ESP_LOGI(TAG, "Command handler registered");
    return ESP_OK;
}

/**
 * @brief Register OTA update handler
 */
esp_err_t mqtt_subscriber_register_ota_handler(ota_handler_t handler)
{
    if (!handler) {
        return ESP_ERR_INVALID_ARG;
    }

    s_ota_handler = handler;
    ESP_LOGI(TAG, "OTA update handler registered");
    return ESP_OK;
}

/**
 * @brief Subscribe to standard device control topics
 */
esp_err_t mqtt_subscriber_subscribe_device_topics(const char *device_id)
{
    if (!device_id) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    char topic[128];

    // Subscribe to configuration updates
    snprintf(topic, sizeof(topic), "devices/%s/config", device_id);
    err = mqtt_client_subscribe(topic, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to config topic: %s", esp_err_to_name(err));
        return err;
    }

    // Subscribe to commands
    snprintf(topic, sizeof(topic), "devices/%s/command", device_id);
    err = mqtt_client_subscribe(topic, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to command topic: %s", esp_err_to_name(err));
        return err;
    }

    // Subscribe to OTA updates
    snprintf(topic, sizeof(topic), "devices/%s/ota", device_id);
    err = mqtt_client_subscribe(topic, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to OTA topic: %s", esp_err_to_name(err));
        return err;
    }

    // Subscribe to broadcast commands
    err = mqtt_client_subscribe("broadcast/command", 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to subscribe to broadcast commands: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Successfully subscribed to device control topics");
    return ESP_OK;
}

/**
 * @brief Unsubscribe from device control topics
 */
esp_err_t mqtt_subscriber_unsubscribe_device_topics(const char *device_id)
{
    if (!device_id) {
        return ESP_ERR_INVALID_ARG;
    }

    char topic[128];

    // Unsubscribe from configuration updates
    snprintf(topic, sizeof(topic), "devices/%s/config", device_id);
    mqtt_client_unsubscribe(topic);

    // Unsubscribe from commands
    snprintf(topic, sizeof(topic), "devices/%s/command", device_id);
    mqtt_client_unsubscribe(topic);

    // Unsubscribe from OTA updates
    snprintf(topic, sizeof(topic), "devices/%s/ota", device_id);
    mqtt_client_unsubscribe(topic);

    // Unsubscribe from broadcast commands
    mqtt_client_unsubscribe("broadcast/command");

    ESP_LOGI(TAG, "Unsubscribed from device control topics");
    return ESP_OK;
}

// ===== INTERNAL FUNCTIONS =====

/**
 * @brief Handle configuration update message
 */
static esp_err_t handle_config_update(const char *data, int data_len)
{
    ESP_LOGI(TAG, "Processing configuration update");

    cJSON *json = cJSON_ParseWithLength(data, data_len);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse configuration JSON");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = ESP_OK;
    
    if (s_config_handler) {
        err = s_config_handler(json);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration update applied successfully");
        } else {
            ESP_LOGE(TAG, "Failed to apply configuration update: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "No configuration handler registered");
        err = ESP_ERR_NOT_SUPPORTED;
    }

    cJSON_Delete(json);
    return err;
}

/**
 * @brief Handle command message
 */
static esp_err_t handle_command(const char *data, int data_len)
{
    ESP_LOGI(TAG, "Processing remote command");

    cJSON *json = cJSON_ParseWithLength(data, data_len);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse command JSON");
        return ESP_ERR_INVALID_ARG;
    }

    // Extract command type
    cJSON *cmd_item = cJSON_GetObjectItem(json, "command");
    if (!cmd_item || !cJSON_IsString(cmd_item)) {
        ESP_LOGE(TAG, "Command field missing or invalid");
        cJSON_Delete(json);
        return ESP_ERR_INVALID_ARG;
    }

    const char *command = cmd_item->valuestring;
    ESP_LOGI(TAG, "Executing command: %s", command);

    esp_err_t err = ESP_OK;

    // Handle built-in commands
    if (strcmp(command, "restart") == 0) {
        ESP_LOGW(TAG, "Restart command received");
        vTaskDelay(pdMS_TO_TICKS(1000)); // Allow time to send response
        esp_restart();
    } else if (strcmp(command, "factory_reset") == 0) {
        ESP_LOGW(TAG, "Factory reset command received");
        // TODO: Implement factory reset logic
        err = ESP_ERR_NOT_SUPPORTED;
    } else if (strcmp(command, "get_status") == 0) {
        ESP_LOGI(TAG, "Status request command received");
        // TODO: Publish current device status
    } else if (strcmp(command, "get_stats") == 0) {
        ESP_LOGI(TAG, "Statistics request command received");
        // TODO: Publish current device statistics
    } else {
        // Try custom command handler
        if (s_command_handler) {
            cJSON *params = cJSON_GetObjectItem(json, "params");
            err = s_command_handler(params ? params : json);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Custom command executed successfully");
            } else {
                ESP_LOGE(TAG, "Custom command failed: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGW(TAG, "Unknown command and no custom handler: %s", command);
            err = ESP_ERR_NOT_SUPPORTED;
        }
    }

    cJSON_Delete(json);
    return err;
}

/**
 * @brief Handle OTA update request
 */
static esp_err_t handle_ota_request(const char *data, int data_len)
{
    ESP_LOGI(TAG, "Processing OTA update request");

    cJSON *json = cJSON_ParseWithLength(data, data_len);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse OTA request JSON");
        return ESP_ERR_INVALID_ARG;
    }

    // Extract OTA parameters
    cJSON *url_item = cJSON_GetObjectItem(json, "url");
    cJSON *version_item = cJSON_GetObjectItem(json, "version");

    if (!url_item || !cJSON_IsString(url_item)) {
        ESP_LOGE(TAG, "OTA URL missing or invalid");
        cJSON_Delete(json);
        return ESP_ERR_INVALID_ARG;
    }

    const char *url = url_item->valuestring;
    const char *version = (version_item && cJSON_IsString(version_item)) ? 
                         version_item->valuestring : "unknown";

    ESP_LOGI(TAG, "OTA update request: URL=%s, Version=%s", url, version);

    esp_err_t err = ESP_OK;

    if (s_ota_handler) {
        err = s_ota_handler(url, version);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OTA update initiated successfully");
        } else {
            ESP_LOGE(TAG, "OTA update failed to start: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "No OTA handler registered");
        err = ESP_ERR_NOT_SUPPORTED;
    }

    cJSON_Delete(json);
    return err;
}