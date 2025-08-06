/**
 * @file api_handlers.c
 * @brief REST API handlers for remote configuration and control
 */

#include <string.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <cJSON.h>
#include "web_server.h"
#include "app_config.h"
#include "csi_collector.h"
#include "ota_updater.h"
#include "mqtt_client_wrapper.h"

static const char *TAG = "api_handlers";

/**
 * @brief GET /api/config - Get current configuration
 */
esp_err_t api_get_config_handler(httpd_req_t *req)
{
    app_config_t *cfg = app_config_get();
    csi_collector_config_t csi_cfg;
    csi_collector_get_config(&csi_cfg);
    
    cJSON *root = cJSON_CreateObject();
    
    // Node information
    cJSON *node = cJSON_CreateObject();
    cJSON_AddStringToObject(node, "device_id", cfg->device_id);
    cJSON_AddStringToObject(node, "name", cfg->device_name);
    cJSON_AddStringToObject(node, "version", cfg->version);
    
    cJSON *position = cJSON_CreateObject();
    cJSON_AddNumberToObject(position, "x", cfg->node_position_x);
    cJSON_AddNumberToObject(position, "y", cfg->node_position_y);
    cJSON_AddNumberToObject(position, "z", cfg->node_position_z);
    cJSON_AddItemToObject(node, "position", position);
    cJSON_AddItemToObject(root, "node", node);
    
    // CSI configuration
    cJSON *csi = cJSON_CreateObject();
    cJSON_AddNumberToObject(csi, "sample_rate", csi_cfg.sample_rate);
    cJSON_AddNumberToObject(csi, "buffer_size", csi_cfg.buffer_size);
    cJSON_AddBoolToObject(csi, "filter_enabled", csi_cfg.filter_enabled);
    cJSON_AddNumberToObject(csi, "filter_threshold", csi_cfg.filter_threshold);
    cJSON_AddBoolToObject(csi, "enable_rssi", csi_cfg.enable_rssi);
    cJSON_AddBoolToObject(csi, "enable_phase", csi_cfg.enable_phase);
    cJSON_AddBoolToObject(csi, "enable_amplitude", csi_cfg.enable_amplitude);
    cJSON_AddItemToObject(root, "csi", csi);
    
    // WiFi configuration (no password)
    cJSON *wifi = cJSON_CreateObject();
    cJSON_AddStringToObject(wifi, "ssid", cfg->wifi_ssid);
    cJSON_AddBoolToObject(wifi, "connected", true); // TODO: Get actual status
    cJSON_AddItemToObject(root, "wifi", wifi);
    
    // MQTT configuration
    cJSON *mqtt = cJSON_CreateObject();
    cJSON_AddStringToObject(mqtt, "broker_url", cfg->mqtt.broker_url);
    cJSON_AddNumberToObject(mqtt, "port", cfg->mqtt.port);
    cJSON_AddStringToObject(mqtt, "topic_prefix", cfg->mqtt.topic_prefix);
    cJSON_AddBoolToObject(mqtt, "enabled", cfg->mqtt.enabled);
    cJSON_AddItemToObject(root, "mqtt", mqtt);
    
    // Send response
    char *response = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    free(response);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief POST /api/config - Update configuration
 */
esp_err_t api_post_config_handler(httpd_req_t *req)
{
    char content[1024];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    app_config_t *cfg = app_config_get();
    bool restart_required = false;
    bool csi_restart_required = false;
    
    // Update node configuration
    cJSON *node = cJSON_GetObjectItem(json, "node");
    if (node) {
        cJSON *name = cJSON_GetObjectItem(node, "name");
        if (name && cJSON_IsString(name)) {
            strncpy(cfg->device_name, name->valuestring, sizeof(cfg->device_name) - 1);
        }
        
        cJSON *position = cJSON_GetObjectItem(node, "position");
        if (position) {
            cJSON *x = cJSON_GetObjectItem(position, "x");
            cJSON *y = cJSON_GetObjectItem(position, "y");
            cJSON *z = cJSON_GetObjectItem(position, "z");
            
            if (x && cJSON_IsNumber(x)) cfg->node_position_x = x->valuedouble;
            if (y && cJSON_IsNumber(y)) cfg->node_position_y = y->valuedouble;
            if (z && cJSON_IsNumber(z)) cfg->node_position_z = z->valuedouble;
        }
    }
    
    // Update CSI configuration
    cJSON *csi = cJSON_GetObjectItem(json, "csi");
    if (csi) {
        csi_collector_config_t csi_cfg;
        csi_collector_get_config(&csi_cfg);
        
        cJSON *item = cJSON_GetObjectItem(csi, "sample_rate");
        if (item && cJSON_IsNumber(item)) {
            csi_cfg.sample_rate = item->valueint;
            csi_restart_required = true;
        }
        
        item = cJSON_GetObjectItem(csi, "buffer_size");
        if (item && cJSON_IsNumber(item)) {
            csi_cfg.buffer_size = item->valueint;
            csi_restart_required = true;
        }
        
        item = cJSON_GetObjectItem(csi, "filter_enabled");
        if (item && cJSON_IsBool(item)) {
            csi_cfg.filter_enabled = cJSON_IsTrue(item);
        }
        
        item = cJSON_GetObjectItem(csi, "filter_threshold");
        if (item && cJSON_IsNumber(item)) {
            csi_cfg.filter_threshold = item->valuedouble;
        }
        
        if (csi_restart_required) {
            csi_collector_stop();
            csi_collector_update_config(&csi_cfg);
            csi_collector_start();
        } else {
            csi_collector_update_config(&csi_cfg);
        }
    }
    
    // Update WiFi configuration
    cJSON *wifi = cJSON_GetObjectItem(json, "wifi");
    if (wifi) {
        cJSON *ssid = cJSON_GetObjectItem(wifi, "ssid");
        cJSON *password = cJSON_GetObjectItem(wifi, "password");
        
        if (ssid && cJSON_IsString(ssid)) {
            strncpy(cfg->wifi_ssid, ssid->valuestring, sizeof(cfg->wifi_ssid) - 1);
            restart_required = true;
        }
        if (password && cJSON_IsString(password)) {
            strncpy(cfg->wifi_password, password->valuestring, sizeof(cfg->wifi_password) - 1);
            restart_required = true;
        }
    }
    
    // Update MQTT configuration
    cJSON *mqtt = cJSON_GetObjectItem(json, "mqtt");
    if (mqtt) {
        cJSON *broker = cJSON_GetObjectItem(mqtt, "broker_url");
        if (broker && cJSON_IsString(broker)) {
            strncpy(cfg->mqtt.broker_url, broker->valuestring, sizeof(cfg->mqtt.broker_url) - 1);
        }
        
        cJSON *port = cJSON_GetObjectItem(mqtt, "port");
        if (port && cJSON_IsNumber(port)) {
            cfg->mqtt.port = port->valueint;
        }
        
        cJSON *enabled = cJSON_GetObjectItem(mqtt, "enabled");
        if (enabled && cJSON_IsBool(enabled)) {
            cfg->mqtt.enabled = cJSON_IsTrue(enabled);
        }
        
        // Restart MQTT if needed
        if (cfg->mqtt.enabled) {
            mqtt_client_stop();
            vTaskDelay(pdMS_TO_TICKS(500));
            mqtt_client_init(&cfg->mqtt);
            mqtt_client_start();
        }
    }
    
    // Save configuration
    app_config_save();
    
    // Send response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddBoolToObject(response, "restart_required", restart_required);
    
    char *resp_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    free(resp_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    // Schedule restart if needed
    if (restart_required) {
        ESP_LOGW(TAG, "Configuration requires restart, restarting in 3 seconds");
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }
    
    return ESP_OK;
}

/**
 * @brief POST /api/command - Execute remote command
 */
esp_err_t api_command_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *cmd = cJSON_GetObjectItem(json, "command");
    if (!cmd || !cJSON_IsString(cmd)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing command");
        return ESP_FAIL;
    }
    
    const char *command = cmd->valuestring;
    esp_err_t err = ESP_OK;
    char *message = "Command executed";
    
    if (strcmp(command, "restart") == 0) {
        message = "Restarting in 2 seconds";
        // Schedule restart after response
        xTaskCreate([](void *param) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
            vTaskDelete(NULL);
        }, "restart_task", 2048, NULL, 5, NULL);
    }
    else if (strcmp(command, "start_csi") == 0) {
        err = csi_collector_start();
        message = "CSI collection started";
    }
    else if (strcmp(command, "stop_csi") == 0) {
        err = csi_collector_stop();
        message = "CSI collection stopped";
    }
    else if (strcmp(command, "calibrate") == 0) {
        // TODO: Implement calibration mode
        message = "Calibration mode started";
    }
    else if (strcmp(command, "factory_reset") == 0) {
        nvs_flash_erase();
        message = "Factory reset complete, restarting";
        xTaskCreate([](void *param) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
            vTaskDelete(NULL);
        }, "reset_task", 2048, NULL, 5, NULL);
    }
    else {
        err = ESP_ERR_NOT_SUPPORTED;
        message = "Unknown command";
    }
    
    // Send response
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", err == ESP_OK);
    cJSON_AddStringToObject(response, "message", message);
    cJSON_AddStringToObject(response, "command", command);
    
    char *resp_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    free(resp_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

/**
 * @brief GET /api/ota/status - Get OTA update status
 */
esp_err_t api_ota_status_handler(httpd_req_t *req)
{
    ota_status_t status;
    ota_updater_get_status(&status);
    
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "state", 
        status.state == OTA_STATE_IDLE ? "idle" :
        status.state == OTA_STATE_CHECKING ? "checking" :
        status.state == OTA_STATE_DOWNLOADING ? "downloading" :
        status.state == OTA_STATE_VERIFYING ? "verifying" :
        status.state == OTA_STATE_UPDATING ? "updating" : "unknown");
    
    cJSON_AddStringToObject(root, "current_version", status.current_version);
    cJSON_AddStringToObject(root, "available_version", status.available_version);
    cJSON_AddNumberToObject(root, "progress", status.progress);
    cJSON_AddStringToObject(root, "last_error", status.last_error);
    cJSON_AddNumberToObject(root, "last_check", status.last_check_timestamp);
    
    char *response = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    
    free(response);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief POST /api/ota/update - Trigger OTA update
 */
esp_err_t api_ota_update_handler(httpd_req_t *req)
{
    char content[512];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *url = cJSON_GetObjectItem(json, "url");
    cJSON *version = cJSON_GetObjectItem(json, "version");
    
    if (!url || !cJSON_IsString(url)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing update URL");
        return ESP_FAIL;
    }
    
    // Start OTA update
    esp_err_t err = ota_updater_start_update(
        url->valuestring,
        version && cJSON_IsString(version) ? version->valuestring : NULL
    );
    
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", err == ESP_OK);
    cJSON_AddStringToObject(response, "message", 
        err == ESP_OK ? "Update started" : "Failed to start update");
    
    char *resp_str = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    free(resp_str);
    cJSON_Delete(response);
    cJSON_Delete(json);
    
    return ESP_OK;
}

/**
 * @brief Register all API handlers
 */
esp_err_t register_api_handlers(httpd_handle_t server)
{
    httpd_uri_t uri;
    
    // GET /api/config
    uri.uri = "/api/config";
    uri.method = HTTP_GET;
    uri.handler = api_get_config_handler;
    uri.user_ctx = NULL;
    httpd_register_uri_handler(server, &uri);
    
    // POST /api/config
    uri.uri = "/api/config";
    uri.method = HTTP_POST;
    uri.handler = api_post_config_handler;
    httpd_register_uri_handler(server, &uri);
    
    // POST /api/command
    uri.uri = "/api/command";
    uri.method = HTTP_POST;
    uri.handler = api_command_handler;
    httpd_register_uri_handler(server, &uri);
    
    // GET /api/ota/status
    uri.uri = "/api/ota/status";
    uri.method = HTTP_GET;
    uri.handler = api_ota_status_handler;
    httpd_register_uri_handler(server, &uri);
    
    // POST /api/ota/update
    uri.uri = "/api/ota/update";
    uri.method = HTTP_POST;
    uri.handler = api_ota_update_handler;
    httpd_register_uri_handler(server, &uri);
    
    ESP_LOGI(TAG, "API handlers registered");
    return ESP_OK;
}