/*
 * RSSI-Based Human Detection for ESP32
 * Detects presence and motion using WiFi signal changes
 */

#include <WiFi.h>
#include <CircularBuffer.h>  // Install from Library Manager

// Configuration
const char* ssid = "YourWiFi";
const char* password = "YourPassword";

// Detection parameters
const int RSSI_WINDOW_SIZE = 50;  // 5 seconds at 10 Hz
const float PRESENCE_THRESHOLD = 3.0;  // dB drop for presence
const float MOTION_THRESHOLD = 2.0;    // Variance multiplier for motion
const float BREATHING_THRESHOLD = 0.5; // dB variation for breathing

// Data storage
CircularBuffer<int8_t, RSSI_WINDOW_SIZE> rssi_buffer;
float baseline_rssi = -50.0;  // Will be calibrated
float baseline_variance = 0.5;

// Detection states
bool human_present = false;
bool human_moving = false;
bool possible_breathing = false;

// Statistics
struct Stats {
  float mean;
  float variance;
  float min;
  float max;
  float breathing_score;
} current_stats;

void calibrate_baseline() {
  Serial.println("Calibrating baseline (ensure room is empty)...");
  
  // Collect baseline samples
  for (int i = 0; i < RSSI_WINDOW_SIZE; i++) {
    int8_t rssi = WiFi.RSSI();
    rssi_buffer.push(rssi);
    delay(100);
  }
  
  // Calculate baseline statistics
  float sum = 0, sum_sq = 0;
  for (int i = 0; i < RSSI_WINDOW_SIZE; i++) {
    sum += rssi_buffer[i];
    sum_sq += rssi_buffer[i] * rssi_buffer[i];
  }
  
  baseline_rssi = sum / RSSI_WINDOW_SIZE;
  baseline_variance = (sum_sq / RSSI_WINDOW_SIZE) - (baseline_rssi * baseline_rssi);
  
  Serial.printf("Baseline RSSI: %.1f dBm, Variance: %.2f\n", 
                baseline_rssi, baseline_variance);
}

void calculate_statistics() {
  float sum = 0, sum_sq = 0;
  current_stats.min = 0;
  current_stats.max = -100;
  
  // Basic statistics
  for (int i = 0; i < rssi_buffer.size(); i++) {
    int8_t val = rssi_buffer[i];
    sum += val;
    sum_sq += val * val;
    if (val < current_stats.min) current_stats.min = val;
    if (val > current_stats.max) current_stats.max = val;
  }
  
  current_stats.mean = sum / rssi_buffer.size();
  current_stats.variance = (sum_sq / rssi_buffer.size()) - 
                          (current_stats.mean * current_stats.mean);
  
  // Breathing detection (0.2-0.5 Hz variations)
  // Simple peak detection in RSSI variations
  int peaks = 0;
  for (int i = 1; i < rssi_buffer.size() - 1; i++) {
    if (rssi_buffer[i] > rssi_buffer[i-1] && 
        rssi_buffer[i] > rssi_buffer[i+1]) {
      peaks++;
    }
  }
  
  // Breathing rate estimation (peaks per window)
  float breathing_rate = (peaks * 60.0) / (RSSI_WINDOW_SIZE / 10.0);
  current_stats.breathing_score = 0;
  
  // Normal breathing is 12-20 breaths per minute
  if (breathing_rate > 10 && breathing_rate < 25) {
    current_stats.breathing_score = 1.0 - abs(16 - breathing_rate) / 16.0;
  }
}

void detect_human() {
  // Presence detection: Significant RSSI drop
  float rssi_drop = baseline_rssi - current_stats.mean;
  human_present = rssi_drop > PRESENCE_THRESHOLD;
  
  // Motion detection: Increased variance
  float variance_ratio = current_stats.variance / baseline_variance;
  human_moving = variance_ratio > MOTION_THRESHOLD;
  
  // Breathing detection: Regular small variations
  possible_breathing = current_stats.breathing_score > 0.5 && 
                      !human_moving &&  // Not moving
                      human_present;    // But present
}

void visualize_rssi() {
  // ASCII visualization of RSSI
  int8_t current_rssi = WiFi.RSSI();
  int bar_length = map(current_rssi, -90, -30, 0, 50);
  
  Serial.print("RSSI [");
  for (int i = 0; i < 50; i++) {
    if (i < bar_length) Serial.print("=");
    else Serial.print(" ");
  }
  Serial.printf("] %d dBm ", current_rssi);
  
  // Status indicators
  if (human_present) Serial.print("[PRESENT] ");
  if (human_moving) Serial.print("[MOVING] ");
  if (possible_breathing) Serial.print("[BREATHING?] ");
  
  Serial.println();
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
  
  // Wait for stable signal
  delay(2000);
  
  // Calibrate baseline
  calibrate_baseline();
  
  Serial.println("\nHuman detection started!");
  Serial.println("Try walking in front of the device...");
}

void loop() {
  static unsigned long last_sample = 0;
  static unsigned long last_display = 0;
  
  // Sample RSSI at 10 Hz
  if (millis() - last_sample > 100) {
    last_sample = millis();
    
    int8_t rssi = WiFi.RSSI();
    rssi_buffer.push(rssi);
    
    // Update statistics
    if (rssi_buffer.isFull()) {
      calculate_statistics();
      detect_human();
    }
  }
  
  // Display status at 2 Hz
  if (millis() - last_display > 500) {
    last_display = millis();
    
    visualize_rssi();
    
    // Detailed statistics every 5 seconds
    static int count = 0;
    if (++count >= 10) {
      count = 0;
      Serial.println("\n--- Detailed Statistics ---");
      Serial.printf("Mean RSSI: %.1f dBm (baseline: %.1f)\n", 
                   current_stats.mean, baseline_rssi);
      Serial.printf("Variance: %.2f (baseline: %.2f)\n", 
                   current_stats.variance, baseline_variance);
      Serial.printf("RSSI Range: %.0f to %.0f dBm\n", 
                   current_stats.min, current_stats.max);
      Serial.printf("Breathing Score: %.2f\n", 
                   current_stats.breathing_score);
      Serial.println("------------------------\n");
    }
  }
}