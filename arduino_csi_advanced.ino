/*
 * Advanced ESP32 CSI with Arduino
 * Includes filtering, buffering, and MQTT transmission
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

// Configuration
const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* mqtt_server = "192.168.1.2";
const char* mqtt_topic = "whofi/csi/data";

// Objects
WiFiClient espClient;
PubSubClient mqtt(espClient);

// CSI data buffer
#define CSI_BUFFER_SIZE 10
struct CSIPacket {
    int8_t rssi;
    uint8_t rate;
    uint8_t channel;
    uint8_t mcs;
    uint32_t timestamp;
    uint8_t data[128];  // Partial CSI data
    uint16_t len;
};

CSIPacket csi_buffer[CSI_BUFFER_SIZE];
volatile uint8_t buffer_head = 0;
volatile uint8_t buffer_tail = 0;
volatile uint32_t packet_count = 0;

// Promiscuous mode for enhanced monitoring
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type == WIFI_PKT_MGMT) {
        const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
        const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
        const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
        
        // Filter for probe requests (optional)
        if (hdr->frame_ctrl == 0x0040) {
            Serial.printf("Probe Request: RSSI=%d, Channel=%d\n", 
                         ppkt->rx_ctrl.rssi, ppkt->rx_ctrl.channel);
        }
    }
}

// CSI callback with buffering
void csi_rx_cb(void *ctx, wifi_csi_info_t *info) {
    uint8_t next_head = (buffer_head + 1) % CSI_BUFFER_SIZE;
    
    if (next_head != buffer_tail) {
        CSIPacket *packet = &csi_buffer[buffer_head];
        
        // Store CSI data
        packet->rssi = info->rx_ctrl.rssi;
        packet->rate = info->rx_ctrl.rate;
        packet->channel = info->rx_ctrl.channel;
        packet->mcs = info->rx_ctrl.mcs;
        packet->timestamp = millis();
        packet->len = min(128, info->len);
        memcpy(packet->data, info->buf, packet->len);
        
        buffer_head = next_head;
        packet_count++;
    }
}

void setup_csi() {
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
    
    esp_wifi_set_csi_config(&csi_config);
    esp_wifi_set_csi_rx_cb(&csi_rx_cb, NULL);
    esp_wifi_set_csi(true);
    
    // Optional: Enable promiscuous mode
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
}

void send_csi_mqtt() {
    if (buffer_tail == buffer_head) return;  // Buffer empty
    
    // Create JSON document
    StaticJsonDocument<512> doc;
    CSIPacket *packet = &csi_buffer[buffer_tail];
    
    doc["rssi"] = packet->rssi;
    doc["channel"] = packet->channel;
    doc["rate"] = packet->rate;
    doc["mcs"] = packet->mcs;
    doc["timestamp"] = packet->timestamp;
    doc["mac"] = WiFi.macAddress();
    
    // Add partial CSI data (first 32 bytes)
    JsonArray csi_array = doc.createNestedArray("csi_data");
    for (int i = 0; i < min(32, packet->len); i++) {
        csi_array.add(packet->data[i]);
    }
    
    // Serialize and send
    char buffer[512];
    size_t len = serializeJson(doc, buffer);
    
    if (mqtt.connected()) {
        mqtt.publish(mqtt_topic, buffer, len);
    }
    
    // Update buffer tail
    buffer_tail = (buffer_tail + 1) % CSI_BUFFER_SIZE;
}

void setup() {
    Serial.begin(921600);
    
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    
    // Setup MQTT
    mqtt.setServer(mqtt_server, 1883);
    
    // Setup CSI
    setup_csi();
    
    Serial.println("CSI collection started");
}

void loop() {
    // Reconnect MQTT if needed
    if (!mqtt.connected()) {
        if (mqtt.connect("ESP32_CSI_Node")) {
            Serial.println("MQTT connected");
        }
    }
    mqtt.loop();
    
    // Send buffered CSI data
    send_csi_mqtt();
    
    // Print statistics
    static unsigned long last_stats = 0;
    if (millis() - last_stats > 5000) {
        last_stats = millis();
        Serial.printf("Packets: %d, Buffer: %d/%d\n", 
                     packet_count, 
                     (buffer_head - buffer_tail + CSI_BUFFER_SIZE) % CSI_BUFFER_SIZE,
                     CSI_BUFFER_SIZE);
    }
    
    delay(1);
}