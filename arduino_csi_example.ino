/*
 * ESP32 CSI Data Collection with Arduino
 * This example shows how to collect CSI data using Arduino IDE
 */

#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"

// WiFi credentials
const char* ssid = "YourWiFi";
const char* password = "YourPassword";

// CSI data structure
typedef struct {
    wifi_csi_info_t csi_info;
    uint8_t buf[512];  // Buffer for CSI data
} csi_data_t;

// Global variables
static csi_data_t csi_data;
static bool csi_enabled = false;

// CSI callback function
void receive_csi_cb(void *ctx, wifi_csi_info_t *data) {
    // Get CSI data
    wifi_csi_info_t *info = &csi_data.csi_info;
    memcpy(info, data, sizeof(wifi_csi_info_t));
    memcpy(csi_data.buf, data->buf, data->len);
    
    // Print CSI information
    Serial.print("CSI_DATA,");
    Serial.print(info->rx_ctrl.rssi); Serial.print(",");
    Serial.print(info->rx_ctrl.rate); Serial.print(",");
    Serial.print(info->rx_ctrl.sig_mode); Serial.print(",");
    Serial.print(info->rx_ctrl.mcs); Serial.print(",");
    Serial.print(info->rx_ctrl.cwb); Serial.print(",");
    Serial.print(info->rx_ctrl.channel); Serial.print(",");
    Serial.print(info->rx_ctrl.secondary_channel); Serial.print(",");
    Serial.print(info->len); Serial.print(",");
    
    // Print first 64 bytes of CSI data
    for (int i = 0; i < min(64, info->len); i++) {
        Serial.print(csi_data.buf[i], HEX);
        if (i < 63) Serial.print(":");
    }
    Serial.println();
}

void setup() {
    Serial.begin(921600);
    delay(10);
    
    Serial.println("ESP32 CSI Tool - Arduino Version");
    
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    
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
    
    // Enable CSI
    esp_err_t ret = esp_wifi_set_csi_config(&csi_config);
    if (ret == ESP_OK) {
        Serial.println("CSI config set successfully");
    } else {
        Serial.printf("CSI config failed: %d\n", ret);
    }
    
    // Register CSI callback
    ret = esp_wifi_set_csi_rx_cb(&receive_csi_cb, NULL);
    if (ret == ESP_OK) {
        Serial.println("CSI callback registered");
    } else {
        Serial.printf("CSI callback registration failed: %d\n", ret);
    }
    
    // Enable CSI
    ret = esp_wifi_set_csi(true);
    if (ret == ESP_OK) {
        csi_enabled = true;
        Serial.println("CSI enabled successfully");
    } else {
        Serial.printf("CSI enable failed: %d\n", ret);
    }
}

void loop() {
    // Main loop - CSI data is handled in callback
    static unsigned long last_status = 0;
    
    // Print status every 5 seconds
    if (millis() - last_status > 5000) {
        last_status = millis();
        Serial.print("Status: WiFi=");
        Serial.print(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
        Serial.print(", CSI=");
        Serial.println(csi_enabled ? "Enabled" : "Disabled");
    }
    
    delay(10);
}