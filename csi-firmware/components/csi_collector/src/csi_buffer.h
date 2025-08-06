/**
 * @file csi_buffer.h
 * @brief CSI data buffering functionality
 */

#ifndef CSI_BUFFER_H
#define CSI_BUFFER_H

#include <stdint.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "csi_collector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief CSI buffer handle type
 */
typedef struct csi_buffer_ctx *csi_buffer_handle_t;

/**
 * @brief Initialize CSI buffer
 * @param handle Pointer to buffer handle
 * @param size Buffer size
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_buffer_init(csi_buffer_handle_t *handle, uint16_t size);

/**
 * @brief Put CSI data into buffer
 * @param handle Buffer handle
 * @param data CSI data to store
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_buffer_put_data(csi_buffer_handle_t handle, const csi_data_t *data);

/**
 * @brief Get CSI data from buffer
 * @param handle Buffer handle
 * @param data Pointer to store retrieved data
 * @param timeout_ticks Timeout in FreeRTOS ticks
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_buffer_get_data(csi_buffer_handle_t handle, csi_data_t *data, TickType_t timeout_ticks);

/**
 * @brief Get buffer statistics
 * @param handle Buffer handle
 * @param total_items Pointer to store total items count
 * @param dropped_items Pointer to store dropped items count
 * @param queue_size Pointer to store current queue size
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_buffer_get_stats(csi_buffer_handle_t handle, uint32_t *total_items, 
                              uint32_t *dropped_items, uint32_t *queue_size);

/**
 * @brief Set buffer overwrite mode
 * @param handle Buffer handle
 * @param enable Enable overwrite mode
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_buffer_set_overwrite(csi_buffer_handle_t handle, bool enable);

/**
 * @brief Deinitialize CSI buffer
 * @param handle Buffer handle
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_buffer_deinit(csi_buffer_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // CSI_BUFFER_H