/*
 * Simple WiFi Positioning with Arduino
 * Uses RSSI-based positioning (CSI data for enhanced accuracy)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "esp_wifi.h"

// Configuration
const char* ssid = "YourWiFi";
const char* password = "YourPassword";

// Web server for data access
WebServer server(80);

// Position estimation data
struct APData {
    String ssid;
    String bssid;
    int8_t rssi;
    uint8_t channel;
    uint32_t last_seen;
};

#define MAX_APS 20
APData access_points[MAX_APS];
int ap_count = 0;

// Simple distance estimation from RSSI
float rssi_to_distance(int8_t rssi) {
    // Path loss formula: RSSI = -10 * n * log10(d) + A
    // Rearranged: d = 10^((A - RSSI) / (10 * n))
    const float A = -30.0;  // RSSI at 1 meter
    const float n = 2.0;    // Path loss exponent
    
    return pow(10, (A - rssi) / (10 * n));
}

// Scan for WiFi networks and estimate position
void update_position() {
    Serial.println("Scanning WiFi networks...");
    int networks = WiFi.scanNetworks();
    
    ap_count = min(networks, MAX_APS);
    
    for (int i = 0; i < ap_count; i++) {
        access_points[i].ssid = WiFi.SSID(i);
        access_points[i].bssid = WiFi.BSSIDstr(i);
        access_points[i].rssi = WiFi.RSSI(i);
        access_points[i].channel = WiFi.channel(i);
        access_points[i].last_seen = millis();
        
        float distance = rssi_to_distance(access_points[i].rssi);
        
        Serial.printf("AP %d: %s, RSSI: %d dBm, Distance: %.1f m\n",
                     i, access_points[i].ssid.c_str(), 
                     access_points[i].rssi, distance);
    }
}

// Web API endpoint
void handle_api() {
    StaticJsonDocument<2048> doc;
    
    doc["timestamp"] = millis();
    doc["ap_count"] = ap_count;
    
    JsonArray aps = doc.createNestedArray("access_points");
    
    for (int i = 0; i < ap_count; i++) {
        JsonObject ap = aps.createNestedObject();
        ap["ssid"] = access_points[i].ssid;
        ap["bssid"] = access_points[i].bssid;
        ap["rssi"] = access_points[i].rssi;
        ap["channel"] = access_points[i].channel;
        ap["distance"] = rssi_to_distance(access_points[i].rssi);
        ap["age"] = millis() - access_points[i].last_seen;
    }
    
    // Add CSI status
    doc["csi_enabled"] = false;  // Would be true if CSI is working
    
    String response;
    serializeJson(doc, response);
    
    server.send(200, "application/json", response);
}

// HTML interface
void handle_root() {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 WiFi Positioning</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #4CAF50; color: white; }
        .distance { font-weight: bold; color: #2196F3; }
    </style>
</head>
<body>
    <h1>ESP32 WiFi Positioning</h1>
    <div id="data">Loading...</div>
    <script>
        function updateData() {
            fetch('/api')
                .then(response => response.json())
                .then(data => {
                    let html = '<h2>Detected Access Points</h2>';
                    html += '<table>';
                    html += '<tr><th>SSID</th><th>BSSID</th><th>RSSI</th><th>Distance</th></tr>';
                    
                    data.access_points.forEach(ap => {
                        html += '<tr>';
                        html += '<td>' + ap.ssid + '</td>';
                        html += '<td>' + ap.bssid + '</td>';
                        html += '<td>' + ap.rssi + ' dBm</td>';
                        html += '<td class="distance">' + ap.distance.toFixed(1) + ' m</td>';
                        html += '</tr>';
                    });
                    
                    html += '</table>';
                    html += '<p>Last update: ' + new Date().toLocaleTimeString() + '</p>';
                    
                    document.getElementById('data').innerHTML = html;
                });
        }
        
        setInterval(updateData, 5000);
        updateData();
    </script>
</body>
</html>
)";
    
    server.send(200, "text/html", html);
}

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // Setup web server
    server.on("/", handle_root);
    server.on("/api", handle_api);
    server.begin();
    
    // Initial scan
    update_position();
}

void loop() {
    server.handleClient();
    
    // Update position every 10 seconds
    static unsigned long last_scan = 0;
    if (millis() - last_scan > 10000) {
        last_scan = millis();
        update_position();
    }
}