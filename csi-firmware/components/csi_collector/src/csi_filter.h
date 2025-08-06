/**
 * @file csi_filter.h
 * @brief CSI data filtering functionality
 */

#ifndef CSI_FILTER_H
#define CSI_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include "csi_collector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CSI filter handle type
 */
typedef struct csi_filter_ctx *csi_filter_handle_t;

/**
 * @brief CSI filter configuration
 */
typedef struct {
    float threshold;                ///< Filter threshold
    bool enable_amplitude_filter;   ///< Enable amplitude filtering
    bool enable_phase_filter;       ///< Enable phase filtering
} csi_filter_config_t;

/**
 * @brief Initialize CSI filter
 * @param handle Pointer to filter handle
 * @param config Filter configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_filter_init(csi_filter_handle_t *handle, const csi_filter_config_t *config);

/**
 * @brief Process CSI data through filter
 * @param handle Filter handle
 * @param data CSI data to filter
 * @return ESP_OK if data passes filter, error code otherwise
 */
esp_err_t csi_filter_process(csi_filter_handle_t handle, csi_data_t *data);

/**
 * @brief Get filter statistics
 * @param handle Filter handle
 * @param total_processed Pointer to store total processed count
 * @param total_passed Pointer to store total passed count
 * @param total_filtered Pointer to store total filtered count
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_filter_get_stats(csi_filter_handle_t handle, uint32_t *total_processed, 
                              uint32_t *total_passed, uint32_t *total_filtered);

/**
 * @brief Update filter configuration
 * @param handle Filter handle
 * @param config New configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_filter_update_config(csi_filter_handle_t handle, const csi_filter_config_t *config);

/**
 * @brief Deinitialize CSI filter
 * @param handle Filter handle
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_filter_deinit(csi_filter_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // CSI_FILTER_H