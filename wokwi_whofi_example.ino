/*
 * WhoFi RSSI Positioning - Wokwi Simulator Compatible
 * Try this at: https://wokwi.com/
 * 
 * This code works in both Wokwi simulator and real ESP32
 */

#include <WiFi.h>
#include <ArduinoJson.h>

// Configuration
const char* ssid = "Wokwi-GUEST";  // Wokwi provides this network
const char* password = "";         // Open network in simulator

// Simulated room with 3 APs
struct AccessPoint {
  String ssid;
  float x;  // Position in meters
  float y;
  int rssi_base;  // Base RSSI at 1 meter
};

AccessPoint aps[] = {
  {"AP_Corner1", 0.0, 0.0, -30},
  {"AP_Corner2", 5.0, 0.0, -30},
  {"AP_Corner3", 2.5, 4.3, -30}  // Equilateral triangle
};

// Position estimation
struct Position {
  float x;
  float y;
  float confidence;
};

Position current_position = {2.5, 2.0, 0.0};

// In Wokwi, we'll simulate position changes
float simulated_x = 2.5;
float simulated_y = 2.0;
float move_speed = 0.1;

// Convert RSSI to distance (simplified path loss model)
float rssi_to_distance(int rssi, int rssi_at_1m = -30) {
  // Path loss: RSSI = RSSI_1m - 10 * n * log10(distance)
  // Rearranged: distance = 10^((RSSI_1m - RSSI) / (10 * n))
  float n = 2.0;  // Path loss exponent (free space)
  return pow(10, (rssi_at_1m - rssi) / (10 * n));
}

// Simulate RSSI based on position (for Wokwi)
int simulate_rssi(float ap_x, float ap_y, float person_x, float person_y, int base_rssi) {
  float distance = sqrt(pow(ap_x - person_x, 2) + pow(ap_y - person_y, 2));
  if (distance < 0.1) distance = 0.1;  // Minimum distance
  
  // Path loss model
  int rssi = base_rssi - 10 * 2.0 * log10(distance);
  
  // Add some noise
  rssi += random(-3, 3);
  
  return constrain(rssi, -90, -20);
}

// Trilateration algorithm
Position trilaterate(float r1, float r2, float r3) {
  Position result;
  
  // AP positions
  float x1 = aps[0].x, y1 = aps[0].y;
  float x2 = aps[1].x, y2 = aps[1].y;
  float x3 = aps[2].x, y3 = aps[2].y;
  
  // Trilateration math
  float A = 2*x2 - 2*x1;
  float B = 2*y2 - 2*y1;
  float C = r1*r1 - r2*r2 - x1*x1 + x2*x2 - y1*y1 + y2*y2;
  float D = 2*x3 - 2*x2;
  float E = 2*y3 - 2*y2;
  float F = r2*r2 - r3*r3 - x2*x2 + x3*x3 - y2*y2 + y3*y3;
  
  result.x = (C*E - F*B) / (E*A - B*D);
  result.y = (C*D - A*F) / (B*D - A*E);
  
  // Calculate confidence based on consistency
  float check_r1 = sqrt(pow(result.x - x1, 2) + pow(result.y - y1, 2));
  float check_r2 = sqrt(pow(result.x - x2, 2) + pow(result.y - y2, 2));
  float check_r3 = sqrt(pow(result.x - x3, 2) + pow(result.y - y3, 2));
  
  float error = abs(check_r1 - r1) + abs(check_r2 - r2) + abs(check_r3 - r3);
  result.confidence = max(0.0, 1.0 - error / 10.0);
  
  return result;
}

// Visualize position in serial monitor
void visualize_position() {
  Serial.println("\n╔════════════════════════════════╗");
  Serial.println("║      Room Position Map         ║");
  Serial.println("╠════════════════════════════════╣");
  
  // Draw 5m x 5m room
  for (int y = 5; y >= 0; y--) {
    Serial.print("║ ");
    for (int x = 0; x <= 5; x++) {
      // Check if current position is here
      if (abs(current_position.x - x) < 0.5 && abs(current_position.y - y) < 0.5) {
        Serial.print("👤 ");
      }
      // Check if AP is here
      else if ((x == 0 && y == 0) || (x == 5 && y == 0) || (x == 2 && y == 4)) {
        Serial.print("📡 ");
      }
      else {
        Serial.print("·  ");
      }
    }
    Serial.println("║");
  }
  
  Serial.println("╚════════════════════════════════╝");
  Serial.printf("Position: (%.1f, %.1f) Confidence: %.0f%%\n", 
                current_position.x, current_position.y, current_position.confidence * 100);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("╔════════════════════════════════╗");
  Serial.println("║   WhoFi RSSI Positioning Demo  ║");
  Serial.println("║   Works in Wokwi Simulator!    ║");
  Serial.println("╚════════════════════════════════╝\n");
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // In Wokwi, connection is instant
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nRunning in offline simulation mode");
  }
  
  Serial.println("\n📡 Access Points:");
  for (int i = 0; i < 3; i++) {
    Serial.printf("%d. %s at (%.1f, %.1f)\n", 
                  i+1, aps[i].ssid.c_str(), aps[i].x, aps[i].y);
  }
  
  Serial.println("\nStarting position tracking...\n");
}

void loop() {
  // Simulate person movement (circular path)
  static float angle = 0;
  angle += 0.1;
  simulated_x = 2.5 + 1.5 * cos(angle);
  simulated_y = 2.0 + 1.5 * sin(angle);
  
  // Get RSSI values (simulated in Wokwi)
  int rssi1, rssi2, rssi3;
  
  #ifdef WOKWI_SIMULATION
    // In Wokwi, we simulate RSSI
    rssi1 = simulate_rssi(aps[0].x, aps[0].y, simulated_x, simulated_y, aps[0].rssi_base);
    rssi2 = simulate_rssi(aps[1].x, aps[1].y, simulated_x, simulated_y, aps[1].rssi_base);
    rssi3 = simulate_rssi(aps[2].x, aps[2].y, simulated_x, simulated_y, aps[2].rssi_base);
  #else
    // On real hardware, scan for networks
    int n = WiFi.scanNetworks();
    rssi1 = -70; rssi2 = -70; rssi3 = -70;  // Defaults
    
    for (int i = 0; i < n; i++) {
      String found_ssid = WiFi.SSID(i);
      if (found_ssid == aps[0].ssid) rssi1 = WiFi.RSSI(i);
      if (found_ssid == aps[1].ssid) rssi2 = WiFi.RSSI(i);
      if (found_ssid == aps[2].ssid) rssi3 = WiFi.RSSI(i);
    }
  #endif
  
  // For Wokwi demo, always simulate
  rssi1 = simulate_rssi(aps[0].x, aps[0].y, simulated_x, simulated_y, aps[0].rssi_base);
  rssi2 = simulate_rssi(aps[1].x, aps[1].y, simulated_x, simulated_y, aps[1].rssi_base);
  rssi3 = simulate_rssi(aps[2].x, aps[2].y, simulated_x, simulated_y, aps[2].rssi_base);
  
  // Convert RSSI to distances
  float d1 = rssi_to_distance(rssi1, aps[0].rssi_base);
  float d2 = rssi_to_distance(rssi2, aps[1].rssi_base);
  float d3 = rssi_to_distance(rssi3, aps[2].rssi_base);
  
  Serial.println("📶 RSSI Measurements:");
  Serial.printf("AP1: %d dBm (%.1f m)\n", rssi1, d1);
  Serial.printf("AP2: %d dBm (%.1f m)\n", rssi2, d2);
  Serial.printf("AP3: %d dBm (%.1f m)\n", rssi3, d3);
  
  // Calculate position
  current_position = trilaterate(d1, d2, d3);
  
  // Apply bounds
  current_position.x = constrain(current_position.x, 0, 5);
  current_position.y = constrain(current_position.y, 0, 5);
  
  // Show results
  visualize_position();
  
  Serial.printf("Actual position: (%.1f, %.1f)\n", simulated_x, simulated_y);
  Serial.printf("Error: %.1f meters\n", 
                sqrt(pow(current_position.x - simulated_x, 2) + 
                     pow(current_position.y - simulated_y, 2)));
  
  delay(2000);  // Update every 2 seconds
}

/* 
 * To use in Wokwi:
 * 1. Go to https://wokwi.com/
 * 2. Create new ESP32 project
 * 3. Copy this code
 * 4. Click "Start Simulation"
 * 
 * The simulation will show:
 * - Real-time position tracking
 * - RSSI values from virtual APs
 * - Position visualization
 * - Error measurements
 */