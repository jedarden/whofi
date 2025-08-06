/**
 * @file ota_updater.h
 * @brief Over-The-Air (OTA) update interface
 * 
 * This component provides firmware update functionality over HTTP/HTTPS
 * with integrity verification and rollback capabilities.
 */

#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA update configuration
 */
typedef struct {
    bool enabled;           ///< OTA updates enabled
    char update_url[128];   ///< OTA update server URL
    bool auto_update;       ///< Automatic updates enabled
    uint16_t check_interval; ///< Update check interval in minutes
    bool verify_signature;  ///< Verify update signature
    char cert_pem[2048];    ///< Server certificate (PEM format)
    uint32_t timeout_ms;    ///< Update timeout in milliseconds
} ota_config_t;

/**
 * @brief OTA update status
 */
typedef enum {
    OTA_STATUS_IDLE,        ///< No update in progress
    OTA_STATUS_CHECKING,    ///< Checking for updates
    OTA_STATUS_DOWNLOADING, ///< Downloading update
    OTA_STATUS_VERIFYING,   ///< Verifying update
    OTA_STATUS_INSTALLING,  ///< Installing update
    OTA_STATUS_SUCCESS,     ///< Update completed successfully
    OTA_STATUS_ERROR        ///< Update failed
} ota_status_t;

/**
 * @brief OTA update statistics
 */
typedef struct {
    uint32_t updates_checked;   ///< Number of update checks performed
    uint32_t updates_available; ///< Number of updates found
    uint32_t updates_installed; ///< Number of successful updates
    uint32_t update_failures;   ///< Number of failed updates
    uint64_t last_check_time;   ///< Last update check timestamp
    uint64_t last_update_time;  ///< Last successful update timestamp
    char current_version[32];   ///< Current firmware version
    char available_version[32]; ///< Available update version
} ota_stats_t;

/**
 * @brief OTA progress callback function type
 * @param status Current OTA status
 * @param progress Progress percentage (0-100)
 * @param user_ctx User context pointer
 */
typedef void (*ota_progress_callback_t)(ota_status_t status, uint8_t progress, void *user_ctx);

/**
 * @brief Initialize OTA updater
 * @param config OTA configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_init(const ota_config_t *config);

/**
 * @brief Start OTA updater service
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_start(void);

/**
 * @brief Stop OTA updater service
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_stop(void);

/**
 * @brief Deinitialize OTA updater
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_deinit(void);

/**
 * @brief Check for firmware updates
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_check_for_updates(void);

/**
 * @brief Start firmware update process
 * @param url Update URL (optional, uses config URL if NULL)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_start_update(const char *url);

/**
 * @brief Cancel ongoing update
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_cancel_update(void);

/**
 * @brief Get current OTA status
 * @return Current OTA status
 */
ota_status_t ota_updater_get_status(void);

/**
 * @brief Get OTA statistics
 * @param stats Pointer to statistics structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_get_stats(ota_stats_t *stats);

/**
 * @brief Register progress callback
 * @param callback Callback function
 * @param user_ctx User context pointer
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_register_callback(ota_progress_callback_t callback, void *user_ctx);

/**
 * @brief Update OTA configuration
 * @param config New OTA configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_update_config(const ota_config_t *config);

/**
 * @brief Rollback to previous firmware version
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_rollback(void);

/**
 * @brief Mark current firmware as valid (prevent rollback)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t ota_updater_mark_valid(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_UPDATER_H