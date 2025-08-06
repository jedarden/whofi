/**
 * @file csi_collector.h
 * @brief CSI (Channel State Information) data collector interface
 * 
 * This component handles the collection and processing of CSI data from the ESP32
 * Wi-Fi radio for positioning and localization applications.
 */

#ifndef CSI_COLLECTOR_H
#define CSI_COLLECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <esp_wifi_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of subcarriers in CSI data
 */
#define CSI_MAX_SUBCARRIERS     64

/**
 * @brief Maximum CSI data length in bytes
 */
#define CSI_MAX_DATA_LEN        1024

/**
 * @brief CSI collector configuration structure
 */
typedef struct {
    uint8_t sample_rate;        ///< Sampling rate in Hz (1-100)
    uint16_t buffer_size;       ///< Buffer size for CSI data (256-4096)
    bool filter_enabled;        ///< Enable CSI data filtering
    float filter_threshold;     ///< Filter threshold (0.0-1.0)
    bool enable_rssi;           ///< Include RSSI data
    bool enable_phase;          ///< Include phase information
    bool enable_amplitude;      ///< Include amplitude information
} csi_collector_config_t;

/**
 * @brief CSI data structure
 */
typedef struct {
    uint64_t timestamp;         ///< Timestamp in microseconds
    uint8_t mac[6];            ///< Source MAC address
    int8_t rssi;               ///< RSSI value
    uint8_t channel;           ///< Wi-Fi channel
    uint8_t secondary_channel; ///< Secondary channel
    uint16_t len;              ///< Length of CSI data
    int8_t *data;              ///< Raw CSI data buffer
    float *amplitude;          ///< Processed amplitude data
    float *phase;              ///< Processed phase data
    uint8_t subcarrier_count;  ///< Number of subcarriers
    bool valid;                ///< Data validity flag
} csi_data_t;

/**
 * @brief CSI collector statistics
 */
typedef struct {
    uint32_t packets_received; ///< Total packets received
    uint32_t packets_processed; ///< Total packets processed
    uint32_t packets_dropped;  ///< Total packets dropped
    uint32_t filter_hits;      ///< Number of filter hits
    uint32_t buffer_overruns;  ///< Buffer overrun count
    float average_rssi;        ///< Average RSSI
    uint64_t last_packet_time; ///< Last packet timestamp
} csi_collector_stats_t;

/**
 * @brief CSI data callback function type
 * @param csi_data Pointer to CSI data structure
 * @param user_ctx User context pointer
 */
typedef void (*csi_data_callback_t)(const csi_data_t *csi_data, void *user_ctx);

/**
 * @brief Initialize the CSI collector
 * @param config Configuration structure
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_init(const csi_collector_config_t *config);

/**
 * @brief Start CSI data collection
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_start(void);

/**
 * @brief Stop CSI data collection
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_stop(void);

/**
 * @brief Deinitialize the CSI collector
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_deinit(void);

/**
 * @brief Check if CSI collector is running
 * @return true if running, false otherwise
 */
bool csi_collector_is_running(void);

/**
 * @brief Get CSI data from the queue
 * @param csi_data Pointer to CSI data structure to fill
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success, ESP_ERR_TIMEOUT on timeout, other error codes on failure
 */
esp_err_t csi_collector_get_data(csi_data_t *csi_data, uint32_t timeout_ms);

/**
 * @brief Register callback for CSI data
 * @param callback Callback function
 * @param user_ctx User context pointer
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_register_callback(csi_data_callback_t callback, void *user_ctx);

/**
 * @brief Unregister CSI data callback
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_unregister_callback(void);

/**
 * @brief Get collector statistics
 * @param stats Pointer to statistics structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_get_stats(csi_collector_stats_t *stats);

/**
 * @brief Reset collector statistics
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_reset_stats(void);

/**
 * @brief Update collector configuration
 * @param config New configuration
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_update_config(const csi_collector_config_t *config);

/**
 * @brief Get current collector configuration
 * @param config Pointer to configuration structure to fill
 * @return ESP_OK on success, error code on failure
 */
esp_err_t csi_collector_get_config(csi_collector_config_t *config);

/**
 * @brief Free CSI data structure memory
 * @param csi_data Pointer to CSI data structure
 */
void csi_collector_free_data(csi_data_t *csi_data);

#ifdef __cplusplus
}
#endif

#endif // CSI_COLLECTOR_H