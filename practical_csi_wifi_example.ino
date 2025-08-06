/*
 * Practical Example: ESP32 Smart Sensor + CSI Positioning
 * Shows WiFi communication and CSI collection working together
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "esp_wifi.h"

// Network Configuration
const char* ssid = "YourWiFi";
const char* password = "YourPassword";
const char* mqtt_server = "192.168.1.100";

// Objects
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

// CSI Data
volatile int csi_packet_count = 0;
volatile float current_rssi = 0;
volatile bool presence_detected = false;

// Sensor Data (example)
float temperature = 22.5;
float humidity = 45.0;

// CSI Callback - Runs for EVERY WiFi packet
void csi_callback(void *ctx, wifi_csi_info_t *info) {
    csi_packet_count++;
    current_rssi = info->rx_ctrl.rssi;
    
    // Simple presence detection from CSI
    static float rssi_baseline = -45;
    if (current_rssi < rssi_baseline - 5) {
        presence_detected = true;
    }
    
    // Note: This runs in WiFi driver context
    // Don't do heavy processing here!
}

// Setup WiFi + CSI
void setup() {
    Serial.begin(115200);
    
    // 1. Connect to WiFi normally
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // 2. Setup MQTT for IoT communication
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(mqtt_callback);
    
    // 3. Setup Web Server
    server.on("/", handleRoot);
    server.on("/api/status", handleAPI);
    server.begin();
    
    // 4. Enable CSI collection AFTER WiFi is connected
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
    esp_wifi_set_csi_rx_cb(&csi_callback, NULL);
    esp_wifi_set_csi(true);
    
    Serial.println("CSI collection enabled!");
    Serial.println("System ready - WiFi + MQTT + Web + CSI all running!");
}

// MQTT Callback
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // Handle incoming MQTT commands
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.print("MQTT message received: ");
    Serial.println(message);
    
    // Example: Control something based on MQTT
    if (String(topic) == "home/control/led") {
        digitalWrite(LED_BUILTIN, message == "ON" ? HIGH : LOW);
    }
}

// Web Server Handlers
void handleRoot() {
    String html = "<html><body>";
    html += "<h1>ESP32 Smart Sensor + CSI</h1>";
    html += "<p>Temperature: " + String(temperature) + "°C</p>";
    html += "<p>Humidity: " + String(humidity) + "%</p>";
    html += "<p>WiFi RSSI: " + String(current_rssi) + " dBm</p>";
    html += "<p>CSI Packets: " + String(csi_packet_count) + "</p>";
    html += "<p>Presence: " + String(presence_detected ? "Detected" : "None") + "</p>";
    html += "<p><a href='/api/status'>JSON API</a></p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleAPI() {
    StaticJsonDocument<256> doc;
    
    // Add sensor data
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    
    // Add WiFi status
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["rssi"] = WiFi.RSSI();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    
    // Add CSI data
    doc["csi"]["packets"] = csi_packet_count;
    doc["csi"]["current_rssi"] = current_rssi;
    doc["csi"]["presence"] = presence_detected;
    
    String json;
    serializeJson(doc, json);
    
    server.send(200, "application/json", json);
}

void loop() {
    // Handle web server
    server.handleClient();
    
    // Handle MQTT
    if (!mqtt.connected()) {
        reconnectMQTT();
    }
    mqtt.loop();
    
    // Send sensor data every 10 seconds
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 10000) {
        lastSend = millis();
        
        // Create JSON payload
        StaticJsonDocument<256> doc;
        doc["temperature"] = temperature + random(-10, 10) / 10.0;
        doc["humidity"] = humidity + random(-20, 20) / 10.0;
        doc["rssi"] = current_rssi;
        doc["presence"] = presence_detected;
        doc["csi_packets"] = csi_packet_count;
        
        String payload;
        serializeJson(doc, payload);
        
        // Publish to MQTT
        mqtt.publish("home/sensor/data", payload.c_str());
        
        Serial.println("Data sent via MQTT: " + payload);
        
        // Reset presence detection
        presence_detected = false;
    }
    
    // Generate some WiFi traffic to ensure CSI flow
    static unsigned long lastPing = 0;
    if (millis() - lastPing > 5000) {
        lastPing = millis();
        
        // Ping gateway to generate packets
        WiFi.ping(WiFi.gatewayIP());
        
        // This creates WiFi traffic that generates CSI data
        Serial.println("Ping sent to generate CSI");
    }
    
    // Display status
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 2000) {
        lastStatus = millis();
        
        Serial.printf("Status - WiFi: %s, MQTT: %s, CSI: %d packets, Web requests: handled\n",
                     WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                     mqtt.connected() ? "Connected" : "Disconnected",
                     csi_packet_count);
    }
}

void reconnectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        if (mqtt.connect("ESP32Client")) {
            Serial.println("connected");
            
            // Subscribe to control topics
            mqtt.subscribe("home/control/+");
            
            // Announce we're online
            mqtt.publish("home/sensor/status", "online");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqtt.state());
            Serial.println(" retry in 5 seconds");
            delay(5000);
        }
    }
}

/*
 * This example demonstrates:
 * 1. Normal WiFi connectivity
 * 2. MQTT for IoT communication  
 * 3. Web server for local access
 * 4. CSI collection happening simultaneously
 * 
 * The ESP32 is doing everything a normal IoT device does,
 * PLUS collecting CSI data for positioning/presence detection!
 * 
 * Access the web interface at: http://[ESP32-IP]/
 * 
 * CSI is collected from:
 * - Router beacons (10Hz)
 * - MQTT traffic
 * - HTTP requests
 * - Ping packets
 * - All other WiFi packets
 */