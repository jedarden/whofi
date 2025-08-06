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

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_WRAPPER_H