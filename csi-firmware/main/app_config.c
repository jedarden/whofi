/**
 * @file app_config.c
 * @brief Application configuration implementation
 */

#include "app_config.h"
#include <string.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

static const char *TAG = "APP_CONFIG";
static const char *NVS_NAMESPACE = "csi_config";

esp_err_t app_config_load(app_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = sizeof(app_config_t);
    err = nvs_get_blob(handle, "config", config, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config from NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Configuration loaded successfully");
    }

    nvs_close(handle);
    return err;
}

esp_err_t app_config_save(const app_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = app_config_validate(config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Configuration validation failed");
        return err;
    }

    nvs_handle_t handle;
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, "config", config, sizeof(app_config_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config to NVS: %s", esp_err_to_name(err));
    } else {
        err = nvs_commit(handle);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Configuration saved successfully");
        }
    }

    nvs_close(handle);
    return err;
}

void app_config_set_defaults(app_config_t *config)
{
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(app_config_t));

    // Device identification
    strcpy(config->device_name, "CSI_Device_001");
    strcpy(config->firmware_version, PROJECT_VER);

    // Wi-Fi defaults
    strcpy(config->wifi.ssid, "");
    strcpy(config->wifi.password, "");
    config->wifi.channel = 0; // Auto
    config->wifi.sta_mode = true;
    config->wifi.ap_mode = false;
    strcpy(config->wifi.ap_ssid, "CSI_Setup");
    strcpy(config->wifi.ap_password, "csi123456");

    // CSI collector defaults
    config->csi.enabled = true;
    config->csi.sample_rate = 20;
    config->csi.buffer_size = 1024;
    config->csi.filter_enabled = false;
    config->csi.filter_threshold = 0.5f;

    // Web server defaults
    config->web_server.enabled = true;
    config->web_server.port = 80;
    config->web_server.auth_enabled = false;
    strcpy(config->web_server.username, "admin");
    strcpy(config->web_server.password, "admin");

    // MQTT defaults
    config->mqtt.enabled = false;
    strcpy(config->mqtt.broker_url, "mqtt://localhost");
    config->mqtt.port = 1883;
    strcpy(config->mqtt.username, "");
    strcpy(config->mqtt.password, "");
    strcpy(config->mqtt.client_id, "csi_device");
    strcpy(config->mqtt.topic_prefix, "csi/data");
    config->mqtt.ssl_enabled = false;
    config->mqtt.keepalive = 60;

    // NTP defaults
    config->ntp.enabled = true;
    strcpy(config->ntp.server1, "pool.ntp.org");
    strcpy(config->ntp.server2, "time.nist.gov");
    strcpy(config->ntp.server3, "time.google.com");
    config->ntp.timezone_offset = 0;
    config->ntp.sync_interval = 60;

    // OTA defaults
    config->ota.enabled = true;
    strcpy(config->ota.update_url, "");
    config->ota.auto_update = false;
    config->ota.check_interval = 60;
    config->ota.verify_signature = true;

    ESP_LOGI(TAG, "Default configuration set");
}

esp_err_t app_config_validate(const app_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate device name
    if (strlen(config->device_name) == 0 || strlen(config->device_name) >= 32) {
        ESP_LOGE(TAG, "Invalid device name");
        return ESP_ERR_INVALID_ARG;
    }

    // Validate Wi-Fi configuration
    if (config->wifi.sta_mode && strlen(config->wifi.ssid) == 0) {
        ESP_LOGE(TAG, "STA mode enabled but SSID is empty");
        return ESP_ERR_INVALID_ARG;
    }

    // Validate CSI configuration
    if (config->csi.enabled) {
        if (config->csi.sample_rate == 0 || config->csi.sample_rate > 100) {
            ESP_LOGE(TAG, "Invalid CSI sample rate: %d", config->csi.sample_rate);
            return ESP_ERR_INVALID_ARG;
        }
        if (config->csi.buffer_size < 256 || config->csi.buffer_size > 4096) {
            ESP_LOGE(TAG, "Invalid CSI buffer size: %d", config->csi.buffer_size);
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Validate web server configuration
    if (config->web_server.enabled && config->web_server.port == 0) {
        ESP_LOGE(TAG, "Web server enabled but port is 0");
        return ESP_ERR_INVALID_ARG;
    }

    // Validate MQTT configuration
    if (config->mqtt.enabled) {
        if (strlen(config->mqtt.broker_url) == 0) {
            ESP_LOGE(TAG, "MQTT enabled but broker URL is empty");
            return ESP_ERR_INVALID_ARG;
        }
        if (config->mqtt.port == 0) {
            ESP_LOGE(TAG, "MQTT enabled but port is 0");
            return ESP_ERR_INVALID_ARG;
        }
    }

    // Validate NTP configuration
    if (config->ntp.enabled && strlen(config->ntp.server1) == 0) {
        ESP_LOGE(TAG, "NTP enabled but no servers configured");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Configuration validation passed");
    return ESP_OK;
}