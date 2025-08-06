/**
 * @file system_init.h
 * @brief System initialization functions
 */

#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize system components
 * @return ESP_OK on success, error code on failure
 */
esp_err_t system_init(void);

/**
 * @brief Initialize NVS flash storage
 * @return ESP_OK on success, error code on failure
 */
esp_err_t nvs_init(void);

/**
 * @brief Initialize Wi-Fi subsystem
 * @return ESP_OK on success, error code on failure
 */
esp_err_t wifi_init(void);

/**
 * @brief Initialize event loop
 * @return ESP_OK on success, error code on failure
 */
esp_err_t event_loop_init(void);

/**
 * @brief Initialize file systems (SPIFFS, FAT)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t filesystem_init(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_INIT_H