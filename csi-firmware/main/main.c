/**
 * @file main.c
 * @brief Main entry point for CSI Positioning System firmware
 * 
 * This file contains the main application logic that coordinates all system components
 * including CSI data collection, web server, MQTT client, NTP sync, and OTA updates.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "app_config.h"
#include "system_init.h"
#include "csi_collector.h"
#include "web_server.h"
#include "mqtt_client_wrapper.h"
#include "ntp_sync.h"
#include "ota_updater.h"

static const char *TAG = "MAIN";

/**
 * @brief Main application task that coordinates all system components
 * @param pvParameters Task parameters (unused)
 */
static void app_main_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Starting CSI Positioning System v%s", PROJECT_VER);
    
    // Initialize system components
    if (system_init() != ESP_OK) {
        ESP_LOGE(TAG, "System initialization failed");
        vTaskDelete(NULL);
        return;
    }
    
    // Load application configuration
    app_config_t config;
    if (app_config_load(&config) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to load config, using defaults");
        app_config_set_defaults(&config);
    }
    
    // Start web configuration server
    if (web_server_start(&config.web_server) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server");
    }
    
    // Initialize and start CSI collector
    if (csi_collector_init(&config.csi) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CSI collector");
    } else if (csi_collector_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start CSI collector");
    }
    
    // Start MQTT client if configured
    if (config.mqtt.enabled) {
        if (mqtt_client_init(&config.mqtt) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize MQTT client");
        } else if (mqtt_client_start() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start MQTT client");
        }
    }
    
    // Start NTP synchronization
    if (ntp_sync_init(&config.ntp) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NTP sync");
    } else {
        ntp_sync_start();
    }
    
    // Initialize OTA updater
    if (ota_updater_init(&config.ota) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize OTA updater");
    }
    
    ESP_LOGI(TAG, "All systems initialized successfully");
    
    // Main application loop
    while (1) {
        // Check system health and perform maintenance tasks
        if (csi_collector_is_running()) {
            csi_data_t csi_data;
            if (csi_collector_get_data(&csi_data, pdMS_TO_TICKS(100)) == ESP_OK) {
                // Process CSI data
                ESP_LOGD(TAG, "CSI data received: %d bytes", csi_data.len);
                
                // Send to MQTT if connected
                if (mqtt_client_is_connected()) {
                    mqtt_client_publish_csi_data(&csi_data);
                }
            }
        }
        
        // Check for OTA updates periodically
        static uint32_t ota_check_counter = 0;
        if (++ota_check_counter >= (300000 / 1000)) { // Check every 5 minutes
            ota_check_counter = 0;
            if (config.ota.auto_update) {
                ota_updater_check_for_updates();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
    }
    
    vTaskDelete(NULL);
}

/**
 * @brief Application entry point
 */
void app_main(void)
{
    ESP_LOGI(TAG, "CSI Positioning System starting...");
    
    // Create main application task
    xTaskCreate(
        app_main_task,
        "app_main",
        8192,  // Stack size
        NULL,  // Parameters
        5,     // Priority
        NULL   // Task handle
    );
}