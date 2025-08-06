/**
 * @file ntp_sync.h
 * @brief NTP time synchronization interface
 * 
 * This component provides network time synchronization functionality
 * for accurate timestamping of CSI data.
 */

#ifndef NTP_SYNC_H
#define NTP_SYNC_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NTP synchronization configuration
 */
typedef struct {
    bool enabled;           ///< NTP sync enabled
    char server1[64];       ///< Primary NTP server
    char server2[64];       ///< Secondary NTP server
    char server3[64];       ///< Tertiary NTP server
    int16_t timezone_offset; ///< Timezone offset in minutes
    uint16_t sync_interval; ///< Sync interval in minutes
    uint16_t timeout;       ///< Sync timeout in seconds
} ntp_config_t;

/**
 * @brief NTP synchronization status
 */
typedef struct {
    bool synchronized;      ///< Time synchronized flag
    uint64_t last_sync;     ///< Last successful sync timestamp
    uint32_t sync_count;    ///< Number of successful syncs
    uint32_t sync_errors;   ///< Number of sync errors
    int32_t time_offset_ms; ///< Current time offset in milliseconds
    char active_server[64]; ///< Currently active NTP server
} ntp_status_t;

/**
 * @brief NTP sync callback function type
 * @param synchronized True if sync was successful
 * @param user_ctx User context pointer
 */
typedef void (*ntp_sync_callback_t)(bool synchronized, void *user_ctx);

/**
 * @brief Initialize NTP synchronization
 * @param config NTP configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_init(const ntp_config_t *config);

/**
 * @brief Start NTP synchronization
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_start(void);

/**
 * @brief Stop NTP synchronization
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_stop(void);

/**
 * @brief Deinitialize NTP synchronization
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_deinit(void);

/**
 * @brief Check if time is synchronized
 * @return true if synchronized, false otherwise
 */
bool ntp_sync_is_synchronized(void);

/**
 * @brief Force immediate synchronization
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_force_sync(void);

/**
 * @brief Get current synchronized time
 * @param tv Pointer to timeval structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_get_time(struct timeval *tv);

/**
 * @brief Get NTP synchronization status
 * @param status Pointer to status structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_get_status(ntp_status_t *status);

/**
 * @brief Register sync callback
 * @param callback Callback function
 * @param user_ctx User context pointer
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_register_callback(ntp_sync_callback_t callback, void *user_ctx);

/**
 * @brief Update NTP configuration
 * @param config New NTP configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_sync_update_config(const ntp_config_t *config);

// ===== NTP CLIENT UTILITIES =====

/**
 * @brief NTP server statistics structure
 */
typedef struct {
    char server_name[64];       ///< Server name
    uint8_t stratum;            ///< Server stratum level
    int8_t precision;           ///< Server precision
    uint8_t poll_interval;      ///< Poll interval
    uint32_t delay_ms;          ///< Round-trip delay in milliseconds
    bool available;             ///< Server availability status
    time_t last_response;       ///< Last successful response timestamp
} ntp_server_stats_t;

/**
 * @brief NTP synchronization quality levels
 */
typedef enum {
    NTP_QUALITY_POOR = 0,       ///< Poor synchronization quality
    NTP_QUALITY_FAIR = 1,       ///< Fair synchronization quality
    NTP_QUALITY_GOOD = 2,       ///< Good synchronization quality
    NTP_QUALITY_EXCELLENT = 3   ///< Excellent synchronization quality
} ntp_quality_level_t;

/**
 * @brief NTP synchronization quality structure
 */
typedef struct {
    bool synchronized;          ///< Time synchronized flag
    ntp_quality_level_t quality; ///< Synchronization quality level
    int32_t offset_ms;          ///< Time offset in milliseconds
    uint64_t time_since_sync_sec; ///< Time since last sync in seconds
} ntp_sync_quality_t;

/**
 * @brief Get timezone offset from timezone name
 * @param timezone Timezone name (e.g., "UTC", "EST", "PST")
 * @param offset_minutes Pointer to store offset in minutes
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if timezone unknown
 */
esp_err_t ntp_client_get_timezone_offset(const char *timezone, int16_t *offset_minutes);

/**
 * @brief List available timezones
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_client_list_timezones(void);

/**
 * @brief Convert timestamp to formatted string with timezone
 * @param timestamp_us Timestamp in microseconds
 * @param timezone_offset Timezone offset in minutes
 * @param buffer Buffer to store formatted string
 * @param buffer_size Buffer size
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_client_format_time(uint64_t timestamp_us, int16_t timezone_offset, 
                                char *buffer, size_t buffer_size);

/**
 * @brief Get current time as formatted string
 * @param buffer Buffer to store formatted string
 * @param buffer_size Buffer size
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_client_get_time_string(char *buffer, size_t buffer_size);

/**
 * @brief Calculate round-trip delay to NTP server
 * @param server NTP server hostname or IP
 * @param delay_ms Pointer to store delay in milliseconds
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_client_measure_server_delay(const char *server, uint32_t *delay_ms);

/**
 * @brief Get NTP server statistics
 * @param server NTP server hostname or IP
 * @param stats Pointer to statistics structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_client_get_server_stats(const char *server, ntp_server_stats_t *stats);

/**
 * @brief Validate NTP server accessibility
 * @param server NTP server hostname or IP
 * @return ESP_OK if server is accessible, error code otherwise
 */
esp_err_t ntp_client_validate_server(const char *server);

/**
 * @brief Get current NTP synchronization quality
 * @param quality Pointer to quality structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ntp_client_get_sync_quality(ntp_sync_quality_t *quality);

#ifdef __cplusplus
}
#endif

#endif // NTP_SYNC_H