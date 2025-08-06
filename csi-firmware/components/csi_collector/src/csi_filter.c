/**
 * @file csi_filter.c
 * @brief CSI data filtering implementation
 */

#include "csi_filter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <esp_log.h>

static const char *TAG = "CSI_FILTER";

/**
 * @brief CSI filter context structure
 */
typedef struct csi_filter_ctx {
    csi_filter_config_t config;
    bool initialized;
} csi_filter_ctx_t;

esp_err_t csi_filter_init(csi_filter_handle_t *handle, const csi_filter_config_t *config)
{
    if (!handle || !config) {
        return ESP_ERR_INVALID_ARG;
    }

    csi_filter_ctx_t *ctx = malloc(sizeof(csi_filter_ctx_t));
    if (!ctx) {
        return ESP_ERR_NO_MEM;
    }

    memcpy(&ctx->config, config, sizeof(csi_filter_config_t));
    ctx->initialized = true;

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

    // Apply RSSI threshold filter
    if (data->rssi < (ctx->config.threshold * -100)) {
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

esp_err_t csi_filter_deinit(csi_filter_handle_t handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    csi_filter_ctx_t *ctx = (csi_filter_ctx_t *)handle;
    free(ctx);
    return ESP_OK;
}