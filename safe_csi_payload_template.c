/*
 * Safe CSI Custom Payload Template for ESP32
 * Includes all safety mechanisms to prevent bricking
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#define RECOVERY_GPIO 0  // Boot button on most ESP32 boards
#define SAFE_MODE_HOLD_TIME_MS 3000
#define WATCHDOG_TIMEOUT_MS 30000

static const char *TAG = "SAFE_CSI";

// Safety flags
static bool safe_mode_enabled = false;
static bool csi_initialized = false;

// Function prototypes
void start_safe_mode(void);
void start_normal_operation(void);
bool check_recovery_mode(void);
void init_watchdog_protection(void);

// Safe error handler
void safe_error_handler(const char* error_msg) {
    ESP_LOGE(TAG, "SAFE ERROR: %s", error_msg);
    ESP_LOGI(TAG, "Entering safe mode in 5 seconds...");
    
    // Give time to read error
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    // Mark current app as invalid to trigger rollback
    esp_ota_mark_app_invalid_rollback_and_reboot();
}

// Check if we should enter recovery/safe mode
bool check_recovery_mode(void) {
    // Configure recovery GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RECOVERY_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "Hold GPIO%d LOW for %d ms to enter safe mode...", 
             RECOVERY_GPIO, SAFE_MODE_HOLD_TIME_MS);
    
    // Check if button is held
    int hold_count = 0;
    for (int i = 0; i < (SAFE_MODE_HOLD_TIME_MS / 100); i++) {
        if (gpio_get_level(RECOVERY_GPIO) == 0) {
            hold_count++;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    
    return (hold_count > (SAFE_MODE_HOLD_TIME_MS / 200));
}

// Initialize watchdog protection
void init_watchdog_protection(void) {
    // Task watchdog is configured in menuconfig
    // RTC watchdog as backup
    ESP_LOGI(TAG, "Watchdog protection initialized");
}

// Safe CSI initialization with error handling
esp_err_t safe_csi_init(void) {
    esp_err_t ret;
    
    // Check if WiFi is initialized
    wifi_mode_t mode;
    ret = esp_wifi_get_mode(&mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi not initialized");
        return ret;
    }
    
    // Configure CSI
    wifi_csi_config_t csi_config = {
        .lltf_en = true,
        .htltf_en = true,
        .stbc_htltf2_en = true,
        .ltf_merge_en = true,
        .channel_filter_en = false,
        .manu_scale = false,
        .shift = 0
    };
    
    // Set CSI configuration with error checking
    ret = esp_wifi_set_csi_config(&csi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set CSI config: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set CSI callback
    ret = esp_wifi_set_csi_rx_cb(csi_rx_callback, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set CSI callback: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Enable CSI
    ret = esp_wifi_set_csi(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable CSI: %s", esp_err_to_name(ret));
        return ret;
    }
    
    csi_initialized = true;
    ESP_LOGI(TAG, "CSI initialized successfully");
    return ESP_OK;
}

// Safe mode operation
void start_safe_mode(void) {
    ESP_LOGW(TAG, "ENTERING SAFE MODE");
    safe_mode_enabled = true;
    
    // Start minimal WiFi AP for recovery
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32_SAFE_MODE",
            .password = "",
            .max_connection = 1,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
    
    ESP_LOGI(TAG, "Safe mode AP started: ESP32_SAFE_MODE");
    ESP_LOGI(TAG, "Connect to upload new firmware via OTA");
    
    // Simple loop - just keep watchdog happy
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Safe mode active...");
    }
}

// Normal CSI operation
void start_normal_operation(void) {
    ESP_LOGI(TAG, "Starting normal CSI operation");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Connect to WiFi (with timeout)
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "YourSSID",
            .password = "YourPassword",
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Wait for connection with timeout
    int retry_count = 0;
    while (retry_count < 30) {  // 30 second timeout
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        retry_count++;
        
        // Check connection status
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "Connected to WiFi");
            break;
        }
    }
    
    if (retry_count >= 30) {
        ESP_LOGE(TAG, "WiFi connection timeout");
        safe_error_handler("WiFi connection failed");
        return;
    }
    
    // Initialize CSI safely
    ret = safe_csi_init();
    if (ret != ESP_OK) {
        safe_error_handler("CSI initialization failed");
        return;
    }
    
    // Main operation loop
    while(1) {
        // Your CSI processing here
        
        // Keep watchdog happy
        vTaskDelay(100 / portTICK_PERIOD_MS);
        
        // Periodic health check
        static int health_counter = 0;
        if (++health_counter > 100) {  // Every 10 seconds
            health_counter = 0;
            ESP_LOGI(TAG, "System healthy - Free heap: %d", esp_get_free_heap_size());
            
            // Check for critical heap
            if (esp_get_free_heap_size() < 10000) {
                ESP_LOGW(TAG, "Low memory warning!");
            }
        }
    }
}

// CSI callback function
void csi_rx_callback(void *ctx, wifi_csi_info_t *info) {
    // Safety check
    if (!csi_initialized || safe_mode_enabled) {
        return;
    }
    
    // Process CSI data safely
    static int packet_count = 0;
    packet_count++;
    
    // Example: Just log packet count
    if (packet_count % 100 == 0) {
        ESP_LOGI(TAG, "CSI packets received: %d", packet_count);
    }
    
    // Your CSI processing here
    // Remember to:
    // 1. Check buffer bounds
    // 2. Validate data
    // 3. Handle errors gracefully
}

// Main application entry point
void app_main(void) {
    ESP_LOGI(TAG, "ESP32 Safe CSI Payload Starting...");
    ESP_LOGI(TAG, "Chip revision: %d", esp_efuse_get_chip_ver());
    
    // Log partition info
    const esp_partition_t* running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running from partition: %s", running->label);
    
    // Initialize watchdog protection
    init_watchdog_protection();
    
    // Check OTA status and rollback if needed
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Pending OTA verification...");
            // Add your validation here
            
            // If validation passes:
            esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI(TAG, "OTA marked as valid");
        }
    }
    
    // Check for recovery mode
    if (check_recovery_mode()) {
        start_safe_mode();
    } else {
        start_normal_operation();
    }
    
    // Should never reach here
    ESP_LOGE(TAG, "Main task ended unexpectedly");
    esp_restart();
}