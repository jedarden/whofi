/**
 * @file mqtt_client_wrapper.h
 * @brief MQTT client wrapper interface
 * 
 * This component provides MQTT connectivity for publishing CSI data and
 * receiving configuration updates.
 */

#ifndef MQTT_CLIENT_WRAPPER_H
#define MQTT_CLIENT_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <cJSON.h>
#include "csi_collector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MQTT client configuration structure
 */
typedef struct {
    bool enabled;           ///< MQTT client enabled
    char broker_url[128];   ///< MQTT broker URL
    uint16_t port;          ///< MQTT broker port
    char username[32];      ///< MQTT username
    char password[64];      ///< MQTT password
    char client_id[32];     ///< MQTT client ID
    char topic_prefix[64];  ///< Topic prefix for published data
    bool ssl_enabled;       ///< SSL/TLS enabled
    uint16_t keepalive;     ///< Keepalive interval
    uint8_t qos;            ///< Quality of Service level
    bool retain;            ///< Retain messages flag
} mqtt_config_t;

/**
 * @brief MQTT client statistics
 */
typedef struct {
    uint32_t messages_sent;     ///< Total messages sent
    uint32_t messages_received; ///< Total messages received
    uint32_t connection_errors; ///< Connection error count
    uint32_t publish_errors;    ///< Publish error count
    bool connected;             ///< Current connection status
    uint64_t last_activity;     ///< Last activity timestamp
} mqtt_stats_t;

/**
 * @brief MQTT message callback function type
 * @param topic Message topic
 * @param data Message data
 * @param data_len Message data length
 * @param user_ctx User context pointer
 */
typedef void (*mqtt_message_callback_t)(const char *topic, const char *data, int data_len, void *user_ctx);

/**
 * @brief Initialize MQTT client
 * @param config MQTT configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_init(const mqtt_config_t *config);

/**
 * @brief Start MQTT client
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_start(void);

/**
 * @brief Stop MQTT client
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_stop(void);

/**
 * @brief Deinitialize MQTT client
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_deinit(void);

/**
 * @brief Check if MQTT client is connected
 * @return true if connected, false otherwise
 */
bool mqtt_client_is_connected(void);

/**
 * @brief Publish CSI data
 * @param csi_data CSI data to publish
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_publish_csi_data(const csi_data_t *csi_data);

/**
 * @brief Publish generic message
 * @param topic Topic to publish to
 * @param data Message data
 * @param data_len Message data length
 * @param qos Quality of Service level
 * @param retain Retain message flag
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_publish(const char *topic, const char *data, int data_len, int qos, int retain);

/**
 * @brief Subscribe to topic
 * @param topic Topic to subscribe to
 * @param qos Quality of Service level
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_subscribe(const char *topic, int qos);

/**
 * @brief Unsubscribe from topic
 * @param topic Topic to unsubscribe from
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_unsubscribe(const char *topic);

/**
 * @brief Register message callback
 * @param callback Callback function
 * @param user_ctx User context pointer
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_register_callback(mqtt_message_callback_t callback, void *user_ctx);

/**
 * @brief Get MQTT client statistics
 * @param stats Pointer to statistics structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_get_stats(mqtt_stats_t *stats);

/**
 * @brief Reset MQTT client statistics
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_client_reset_stats(void);

// ===== MQTT PUBLISHER UTILITIES =====

/**
 * @brief Publish device status information
 * @param device_id Device identifier
 * @param version Firmware version
 * @param uptime Uptime in seconds
 * @param wifi_rssi Wi-Fi RSSI value
 * @param free_heap Free heap memory in bytes
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_publish_device_status(const char *device_id, const char *version, 
                                    uint32_t uptime, int8_t wifi_rssi, uint32_t free_heap);

/**
 * @brief Publish system metrics
 * @param device_id Device identifier
 * @param cpu_usage CPU usage percentage
 * @param free_heap Free heap memory in bytes
 * @param min_free_heap Minimum free heap memory
 * @param task_count Number of running tasks
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_publish_system_metrics(const char *device_id, float cpu_usage, 
                                     uint32_t free_heap, uint32_t min_free_heap,
                                     uint32_t task_count);

/**
 * @brief Publish error/alert message
 * @param device_id Device identifier
 * @param level Alert level ("ERROR", "WARNING", "INFO")
 * @param component Component name that generated the alert
 * @param message Alert message
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_publish_alert(const char *device_id, const char *level, 
                           const char *component, const char *message);

/**
 * @brief Publish configuration acknowledgment
 * @param device_id Device identifier
 * @param config_id Configuration change ID
 * @param success Whether configuration was applied successfully
 * @param error_msg Error message if unsuccessful
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_publish_config_ack(const char *device_id, const char *config_id, 
                                 bool success, const char *error_msg);

// ===== MQTT SUBSCRIBER UTILITIES =====

/**
 * @brief Configuration update handler function type
 */
typedef esp_err_t (*config_update_handler_t)(const cJSON *config);

/**
 * @brief Command handler function type
 */
typedef esp_err_t (*command_handler_t)(const cJSON *params);

/**
 * @brief OTA handler function type
 */
typedef esp_err_t (*ota_handler_t)(const char *url, const char *version);

/**
 * @brief Default message callback for MQTT subscriber
 */
void mqtt_subscriber_default_callback(const char *topic, const char *data, int data_len, void *user_ctx);

/**
 * @brief Register configuration update handler
 * @param handler Configuration update handler function
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_subscriber_register_config_handler(config_update_handler_t handler);

/**
 * @brief Register command handler
 * @param handler Command handler function
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_subscriber_register_command_handler(command_handler_t handler);

/**
 * @brief Register OTA update handler
 * @param handler OTA update handler function
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_subscriber_register_ota_handler(ota_handler_t handler);

/**
 * @brief Subscribe to standard device control topics
 * @param device_id Device identifier
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_subscriber_subscribe_device_topics(const char *device_id);

/**
 * @brief Unsubscribe from device control topics
 * @param device_id Device identifier
 * @return ESP_OK on success, error code on failure
 */
esp_err_t mqtt_subscriber_unsubscribe_device_topics(const char *device_id);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_WRAPPER_H