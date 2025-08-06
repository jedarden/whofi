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

#ifdef __cplusplus
}
#endif

#endif // NTP_SYNC_H