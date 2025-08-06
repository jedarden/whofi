/**
 * @file csi_filter.c
 * @brief CSI data filtering implementation
 */

#include "csi_filter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <esp_log.h>
#include <freertos/semphr.h>

static const char *TAG = "CSI_FILTER";

/**
 * @brief CSI filter context structure
 */
typedef struct csi_filter_ctx {
    csi_filter_config_t config;
    SemaphoreHandle_t mutex;
    uint32_t total_processed;
    uint32_t total_passed;
    uint32_t total_filtered;
    float *amplitude_history;
    float *phase_history;
    uint16_t history_size;
    uint16_t history_index;
    bool initialized;
} csi_filter_ctx_t;

esp_err_t csi_filter_init(csi_filter_handle_t *handle, const csi_filter_config_t *config)
{
    if (!handle || !config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->threshold < 0.0f || config->threshold > 1.0f) {
        ESP_LOGE(TAG, "Invalid threshold value: %.2f", config->threshold);
        return ESP_ERR_INVALID_ARG;
    }

    csi_filter_ctx_t *ctx = malloc(sizeof(csi_filter_ctx_t));
    if (!ctx) {
        ESP_LOGE(TAG, "Failed to allocate filter context");
        return ESP_ERR_NO_MEM;
    }

    memset(ctx, 0, sizeof(csi_filter_ctx_t));
    memcpy(&ctx->config, config, sizeof(csi_filter_config_t));
    
    ctx->mutex = xSemaphoreCreateMutex();
    if (!ctx->mutex) {
        ESP_LOGE(TAG, "Failed to create filter mutex");
        free(ctx);
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize history buffers for temporal filtering
    ctx->history_size = 10;
    if (config->enable_amplitude_filter) {
        ctx->amplitude_history = malloc(ctx->history_size * CSI_MAX_SUBCARRIERS * sizeof(float));
        if (!ctx->amplitude_history) {
            ESP_LOGE(TAG, "Failed to allocate amplitude history buffer");
            vSemaphoreDelete(ctx->mutex);
            free(ctx);
            return ESP_ERR_NO_MEM;
        }
        memset(ctx->amplitude_history, 0, ctx->history_size * CSI_MAX_SUBCARRIERS * sizeof(float));
    }
    
    if (config->enable_phase_filter) {
        ctx->phase_history = malloc(ctx->history_size * CSI_MAX_SUBCARRIERS * sizeof(float));
        if (!ctx->phase_history) {
            ESP_LOGE(TAG, "Failed to allocate phase history buffer");
            if (ctx->amplitude_history) {
                free(ctx->amplitude_history);
            }
            vSemaphoreDelete(ctx->mutex);
            free(ctx);
            return ESP_ERR_NO_MEM;
        }
        memset(ctx->phase_history, 0, ctx->history_size * CSI_MAX_SUBCARRIERS * sizeof(float));
    }
    
    ctx->history_index = 0;
    ctx->initialized = true;

    ESP_LOGI(TAG, "Filter initialized with threshold %.2f", config->threshold);
    *handle = ctx;
    return ESP_OK;
}

esp_err_t csi_filter_process(csi_filter_handle_t handle, csi_data_t *data)
{
    if (!handle || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    csi_filter_ctx_t *ctx = (csi_filter_ctx_t *)handle;
    if (!ctx->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    ctx->total_processed++;
    
    bool pass_filter = true;
    
    // Apply RSSI threshold filter
    int rssi_threshold = (int)(ctx->config.threshold * -100.0f);
    if (data->rssi < rssi_threshold) {
        ESP_LOGD(TAG, "RSSI filter: %d < %d", data->rssi, rssi_threshold);
        pass_filter = false;
    }
    
    // Apply amplitude-based filtering if enabled and data is available
    if (pass_filter && ctx->config.enable_amplitude_filter && data->amplitude && ctx->amplitude_history) {
        float avg_amplitude = 0.0f;
        for (int i = 0; i < data->subcarrier_count; i++) {
            avg_amplitude += data->amplitude[i];
        }
        avg_amplitude /= data->subcarrier_count;
        
        // Compare with historical average
        float historical_avg = 0.0f;
        int valid_history_count = 0;
        
        for (int h = 0; h < ctx->history_size; h++) {
            float hist_sum = 0.0f;
            for (int i = 0; i < data->subcarrier_count && i < CSI_MAX_SUBCARRIERS; i++) {
                hist_sum += ctx->amplitude_history[h * CSI_MAX_SUBCARRIERS + i];
            }
            if (hist_sum > 0.0f) {
                historical_avg += hist_sum / data->subcarrier_count;
                valid_history_count++;
            }
        }
        
        if (valid_history_count > 0) {
            historical_avg /= valid_history_count;
            
            // Check if current amplitude deviates significantly from history
            float deviation = fabsf(avg_amplitude - historical_avg) / (historical_avg + 0.001f);
            if (deviation > ctx->config.threshold) {
                ESP_LOGD(TAG, "Amplitude filter: deviation %.3f > %.3f", deviation, ctx->config.threshold);
                // Don't filter out, but mark as significant change
                data->valid = true;
            }
        }
        
        // Update history
        for (int i = 0; i < data->subcarrier_count && i < CSI_MAX_SUBCARRIERS; i++) {
            ctx->amplitude_history[ctx->history_index * CSI_MAX_SUBCARRIERS + i] = data->amplitude[i];
        }
    }
    
    // Apply phase-based filtering if enabled and data is available
    if (pass_filter && ctx->config.enable_phase_filter && data->phase && ctx->phase_history) {
        // Calculate phase variance
        float phase_variance = 0.0f;
        float phase_mean = 0.0f;
        
        for (int i = 0; i < data->subcarrier_count; i++) {
            phase_mean += data->phase[i];
        }
        phase_mean /= data->subcarrier_count;
        
        for (int i = 0; i < data->subcarrier_count; i++) {
            float diff = data->phase[i] - phase_mean;
            phase_variance += diff * diff;
        }
        phase_variance /= data->subcarrier_count;
        
        // Filter based on phase stability
        if (phase_variance < ctx->config.threshold * 0.1f) {
            ESP_LOGD(TAG, "Phase filter: low variance %.6f", phase_variance);
        }
        
        // Update phase history
        for (int i = 0; i < data->subcarrier_count && i < CSI_MAX_SUBCARRIERS; i++) {
            ctx->phase_history[ctx->history_index * CSI_MAX_SUBCARRIERS + i] = data->phase[i];
        }
    }
    
    // Update history index
    ctx->history_index = (ctx->history_index + 1) % ctx->history_size;
    
    if (pass_filter) {
        ctx->total_passed++;
    } else {
        ctx->total_filtered++;
    }
    
    xSemaphoreGive(ctx->mutex);
    
    return pass_filter ? ESP_OK : ESP_ERR_NOT_FOUND;
}

esp_err_t csi_filter_deinit(csi_filter_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    csi_filter_ctx_t *ctx = (csi_filter_ctx_t *)handle;
    
    if (ctx->initialized) {
        ESP_LOGI(TAG, "Filter stats - Processed: %lu, Passed: %lu, Filtered: %lu", 
                 ctx->total_processed, ctx->total_passed, ctx->total_filtered);
        
        if (ctx->amplitude_history) {
            free(ctx->amplitude_history);
        }
        
        if (ctx->phase_history) {
            free(ctx->phase_history);
        }
        
        if (ctx->mutex) {
            vSemaphoreDelete(ctx->mutex);
        }
    }
    
    free(ctx);
    return ESP_OK;
}

esp_err_t csi_filter_get_stats(csi_filter_handle_t handle, uint32_t *total_processed, 
                              uint32_t *total_passed, uint32_t *total_filtered)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    csi_filter_ctx_t *ctx = (csi_filter_ctx_t *)handle;
    if (!ctx->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    
    if (total_processed) {
        *total_processed = ctx->total_processed;
    }
    if (total_passed) {
        *total_passed = ctx->total_passed;
    }
    if (total_filtered) {
        *total_filtered = ctx->total_filtered;
    }
    
    xSemaphoreGive(ctx->mutex);
    return ESP_OK;
}

esp_err_t csi_filter_update_config(csi_filter_handle_t handle, const csi_filter_config_t *config)
{
    if (!handle || !config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (config->threshold < 0.0f || config->threshold > 1.0f) {
        ESP_LOGE(TAG, "Invalid threshold value: %.2f", config->threshold);
        return ESP_ERR_INVALID_ARG;
    }

    csi_filter_ctx_t *ctx = (csi_filter_ctx_t *)handle;
    if (!ctx->initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(ctx->mutex, portMAX_DELAY);
    memcpy(&ctx->config, config, sizeof(csi_filter_config_t));
    xSemaphoreGive(ctx->mutex);
    
    ESP_LOGI(TAG, "Filter configuration updated");
    return ESP_OK;
}