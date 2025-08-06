// Custom ESPHome CSI Component
#pragma once

#include "esphome.h"
#include "esphome/core/component.h"

#ifdef USE_ESP32
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_err.h>

namespace esphome {
namespace csi_sensor {

class CSISensor : public Component, public CustomMQTTDevice {
 protected:
  static CSISensor *instance_;
  uint32_t packet_count_ = 0;
  bool csi_enabled_ = false;
  
 public:
  void setup() override {
    instance_ = this;
    ESP_LOGI("csi", "Setting up CSI collection...");
    
    // Configure WiFi for CSI
    wifi_csi_config_t csi_config = {
      .lltf_en = true,
      .htltf_en = true, 
      .stbc_htltf2_en = true,
      .ltf_merge_en = true,
      .channel_filter_en = false,
      .manu_scale = false,
      .shift = 0
    };
    
    esp_err_t ret = esp_wifi_set_csi_config(&csi_config);
    if (ret == ESP_OK) {
      ESP_LOGI("csi", "CSI config set successfully");
    }
    
    ret = esp_wifi_set_csi_rx_cb(&csi_rx_callback, this);
    if (ret == ESP_OK) {
      ESP_LOGI("csi", "CSI callback registered");
    }
    
    ret = esp_wifi_set_csi(true);
    if (ret == ESP_OK) {
      csi_enabled_ = true;
      ESP_LOGI("csi", "CSI enabled");
    }
  }
  
  void loop() override {
    // Periodically publish stats
    static uint32_t last_publish = 0;
    if (millis() - last_publish > 5000) {
      publish_json("whofi/csi/" + get_mac_address() + "/stats", [=](JsonObject root) {
        root["packets"] = packet_count_;
        root["enabled"] = csi_enabled_;
        root["uptime"] = millis() / 1000;
      });
      last_publish = millis();
    }
  }
  
  static void csi_rx_callback(void *ctx, wifi_csi_info_t *info) {
    if (!instance_ || !instance_->csi_enabled_) return;
    
    instance_->packet_count_++;
    
    // Build JSON with CSI data
    instance_->publish_json("whofi/csi/" + instance_->get_mac_address() + "/data", [=](JsonObject root) {
      root["timestamp"] = millis();
      root["mac"] = WiFi.macAddress();
      root["rssi"] = info->rx_ctrl.rssi;
      root["rate"] = info->rx_ctrl.rate;
      root["sig_mode"] = info->rx_ctrl.sig_mode;
      root["mcs"] = info->rx_ctrl.mcs;
      root["cwb"] = info->rx_ctrl.cwb;
      root["channel"] = info->rx_ctrl.channel;
      root["secondary_channel"] = info->rx_ctrl.secondary_channel;
      root["nr"] = info->rx_ctrl.nr;
      root["nc"] = info->rx_ctrl.nc;
      root["timestamp"] = info->rx_ctrl.timestamp;
      root["noise_floor"] = info->rx_ctrl.noise_floor;
      root["ant"] = info->rx_ctrl.ant;
      root["sig_len"] = info->rx_ctrl.sig_len;
      root["rx_state"] = info->rx_ctrl.rx_state;
      
      // Add CSI data (first 64 bytes as example)
      JsonArray csi_data = root.createNestedArray("csi_data");
      int len = MIN(64, info->len);
      for (int i = 0; i < len; i++) {
        csi_data.add(info->buf[i]);
      }
      
      root["csi_len"] = info->len;
      root["first_word_invalid"] = info->first_word_invalid;
    });
  }
};

CSISensor *CSISensor::instance_ = nullptr;

} // namespace csi_sensor
} // namespace esphome
#endif