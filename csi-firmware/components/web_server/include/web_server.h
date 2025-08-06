/**
 * @file web_server.h
 * @brief Web configuration server interface
 * 
 * This component provides a web-based interface for configuring and monitoring
 * the CSI positioning system.
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Web server configuration structure
 */
typedef struct {
    bool enabled;           ///< Web server enabled
    uint16_t port;          ///< Server port
    bool auth_enabled;      ///< Authentication enabled
    char username[32];      ///< Admin username
    char password[64];      ///< Admin password
    uint8_t max_sessions;   ///< Maximum concurrent sessions
    uint16_t session_timeout; ///< Session timeout in minutes
} web_server_config_t;

/**
 * @brief Web server statistics
 */
typedef struct {
    uint32_t total_requests;    ///< Total HTTP requests received
    uint32_t active_sessions;   ///< Currently active sessions
    uint32_t failed_auth;       ///< Failed authentication attempts
    uint64_t bytes_sent;        ///< Total bytes sent
    uint64_t bytes_received;    ///< Total bytes received
    uint64_t uptime;            ///< Server uptime in seconds
} web_server_stats_t;

/**
 * @brief Start the web server
 * @param config Web server configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t web_server_start(const web_server_config_t *config);

/**
 * @brief Stop the web server
 * @return ESP_OK on success, error code on failure
 */
esp_err_t web_server_stop(void);

/**
 * @brief Check if web server is running
 * @return true if running, false otherwise
 */
bool web_server_is_running(void);

/**
 * @brief Get web server statistics
 * @param stats Pointer to statistics structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t web_server_get_stats(web_server_stats_t *stats);

/**
 * @brief Reset web server statistics
 * @return ESP_OK on success, error code on failure
 */
esp_err_t web_server_reset_stats(void);

/**
 * @brief Update web server configuration
 * @param config New configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t web_server_update_config(const web_server_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // WEB_SERVER_H