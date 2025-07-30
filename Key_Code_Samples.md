# Key Code Samples from ESP32 WiFi Fingerprinting Repositories

## ESP32 CSI Data Structure (from ESP32-CSI-Tool)

### CSI Data Collection (Active Station Mode)
```c
// From active_sta implementation - CSI collection and processing
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_err.h"

// WiFi CSI callback function
void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *info) {
    if (!info || !info->buf) {
        ESP_LOGW(TAG, "WiFi CSI CB: Invalid data");
        return;
    }
    
    // Extract CSI data
    wifi_csi_info_t csi = *info;
    char* buffer = (char*) info->buf;
    
    // Process CSI amplitude and phase data
    for (int i = 0; i < csi.len; i++) {
        // CSI data processing for fingerprinting
        int8_t imaginary = buffer[i * 2];
        int8_t real = buffer[i * 2 + 1];
        
        // Calculate amplitude for fingerprinting
        float amplitude = sqrt(pow(real, 2) + pow(imaginary, 2));
        
        // Log CSI data in CSV format
        printf("CSI_DATA,%d,%d,%f,%lld\n", 
               csi.rx_ctrl.rssi, i, amplitude, esp_timer_get_time());
    }
}
```

### WiFi CSI Configuration
```c
// CSI configuration for optimal fingerprinting
wifi_csi_config_t csi_config = {
    .lltf_en = 1,           // Enable Long Training Field
    .htltf_en = 1,          // Enable HT-LTF
    .stbc_en = 1,           // Enable Space-Time Block Coding
    .ltf_merge_en = 1,      // Enable LTF merging
    .channel_filter_en = 0, // Disable channel filtering
    .manu_scale = 0,        // Manual scaling disabled
    .shift = 0,             // No bit shifting
};

ESP_ERROR_CHECK(esp_wifi_set_csi_config(&csi_config));
ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(wifi_csi_rx_cb, NULL));
ESP_ERROR_CHECK(esp_wifi_set_csi(1)); // Enable CSI
```

## Probe Request Sniffing (from esp32-sniffer)

### Packet Sniffing and MAC Extraction
```c
// Probe request packet structure and extraction
typedef struct {
    uint8_t mac[6];
    char ssid[33];
    uint32_t timestamp;
    int8_t rssi;
    uint16_t seq_ctrl;
    uint8_t ht_cc;
} probe_request_info_t;

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

    // Check if it's a probe request (subtype 0x04)
    if ((hdr->frame_ctrl & 0xFF) == 0x40) {
        probe_request_info_t info;
        
        // Extract MAC address
        memcpy(info.mac, hdr->addr2, 6);
        
        // Extract RSSI
        info.rssi = ppkt->rx_ctrl.rssi;
        
        // Extract timestamp
        info.timestamp = esp_timer_get_time();
        
        // Extract sequence control
        info.seq_ctrl = hdr->sequence_ctrl;
        
        // Process probe request for device fingerprinting
        process_probe_request(&info);
    }
}
```

### WiFi Monitor Mode Setup
```c
// Configure ESP32 for promiscuous mode packet capture
void setup_wifi_monitor() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Set promiscuous mode callback
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    
    // Set channel for monitoring
    ESP_ERROR_CHECK(esp_wifi_set_channel(SNIFFING_CHANNEL, WIFI_SECOND_CHAN_NONE));
    
    ESP_LOGI(TAG, "WiFi monitor mode initialized on channel %d", SNIFFING_CHANNEL);
}
```

## MAC Address Scanning (from ESP32-MAC-Scanner)

### MAC Address Detection and Fingerprinting
```c
// MAC address scanning and device identification
#include "esp_wifi.h"
#include "esp_wifi_types.h"

void wifi_promiscuous_rx(void *buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;
    
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_ieee80211_packet_t *ieee_pkt = (wifi_ieee80211_packet_t*)pkt->payload;
    wifi_ieee80211_mac_hdr_t *mac_hdr = &ieee_pkt->hdr;
    
    // Extract source MAC address
    uint8_t *src_mac = mac_hdr->addr2;
    
    // Check for known device patterns
    if (is_known_device_mac(src_mac)) {
        device_info_t device;
        extract_device_fingerprint(src_mac, &device, pkt->rx_ctrl.rssi);
        
        // Log device detection
        ESP_LOGI(TAG, "Device detected: %02x:%02x:%02x:%02x:%02x:%02x, RSSI: %d",
                 src_mac[0], src_mac[1], src_mac[2], 
                 src_mac[3], src_mac[4], src_mac[5], 
                 pkt->rx_ctrl.rssi);
    }
}
```

### Device Fingerprinting Based on MAC OUI
```c
// Device identification using MAC address OUI (Organizationally Unique Identifier)
typedef struct {
    uint8_t oui[3];
    const char* manufacturer;
    const char* device_type;
} mac_oui_entry_t;

const mac_oui_entry_t known_ouis[] = {
    {{0xF4, 0xF5, 0xDB}, "Apple", "iPhone/iPad"},
    {{0x3C, 0x15, 0xC2}, "Apple", "MacBook"},
    {{0x28, 0x16, 0xAD}, "Samsung", "Galaxy"},
    {{0xDC, 0x85, 0xDE}, "Samsung", "Galaxy"},
    // Add more OUI entries for device identification
};

bool identify_device_by_mac(uint8_t* mac, char* manufacturer, char* device_type) {
    for (int i = 0; i < sizeof(known_ouis) / sizeof(mac_oui_entry_t); i++) {
        if (memcmp(mac, known_ouis[i].oui, 3) == 0) {
            strcpy(manufacturer, known_ouis[i].manufacturer);
            strcpy(device_type, known_ouis[i].device_type);
            return true;
        }
    }
    return false;
}
```

## Indoor Positioning (from WiFi_Tracker_Project)

### RSSI-Based Trilateration
```c
// Indoor positioning using RSSI trilateration
typedef struct {
    float x, y;           // Position coordinates
    int8_t rssi;          // Signal strength
    uint8_t mac[6];       // Device MAC address
} position_data_t;

typedef struct {
    float x, y;           // Beacon position
    int beacon_id;        // Beacon identifier
} beacon_position_t;

// Calculate distance from RSSI using path loss model
float rssi_to_distance(int8_t rssi, int tx_power) {
    if (rssi == 0) return -1.0;
    
    // Free space path loss model
    float ratio = (float)(tx_power - rssi) / 20.0;
    return pow(10, ratio);
}

// Trilateration algorithm for position estimation
bool trilaterate_position(beacon_position_t* beacons, float* distances, 
                         int num_beacons, float* x, float* y) {
    if (num_beacons < 3) return false;
    
    // Simplified trilateration using least squares
    float A[2][2] = {0};
    float B[2] = {0};
    
    for (int i = 1; i < num_beacons; i++) {
        A[0][0] += 2 * (beacons[i].x - beacons[0].x);
        A[0][1] += 2 * (beacons[i].y - beacons[0].y);
        A[1][0] = A[0][0];
        A[1][1] = A[0][1];
        
        B[0] += pow(distances[0], 2) - pow(distances[i], 2) +
               pow(beacons[i].x, 2) - pow(beacons[0].x, 2) +
               pow(beacons[i].y, 2) - pow(beacons[0].y, 2);
    }
    
    // Solve linear system for position estimate
    float det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if (abs(det) < 0.001) return false;
    
    *x = (B[0] * A[1][1] - B[1] * A[0][1]) / det;
    *y = (A[0][0] * B[1] - A[1][0] * B[0]) / det;
    
    return true;
}
```

## Advanced CSI Processing (Python Integration)

### CSI Data Analysis Pipeline
```python
# Python CSI data processing for fingerprinting
import numpy as np
import pandas as pd
from scipy import signal
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier

class CSIFingerprinter:
    def __init__(self):
        self.scaler = StandardScaler()
        self.classifier = RandomForestClassifier(n_estimators=100)
        
    def process_csi_data(self, csi_file):
        """Process raw CSI data from ESP32"""
        # Read CSV data from ESP32
        data = pd.read_csv(csi_file)
        
        # Extract CSI amplitude features
        amplitudes = data['amplitude'].values
        
        # Apply signal processing
        # Low-pass filter to remove noise
        b, a = signal.butter(4, 0.1, 'low')
        filtered = signal.filtfilt(b, a, amplitudes)
        
        # Extract statistical features for fingerprinting
        features = {
            'mean': np.mean(filtered),
            'std': np.std(filtered),
            'variance': np.var(filtered),
            'skewness': signal.stats.skew(filtered),
            'kurtosis': signal.stats.kurtosis(filtered),
            'peak_freq': self.get_dominant_frequency(filtered),
            'spectral_centroid': self.spectral_centroid(filtered)
        }
        
        return features
    
    def get_dominant_frequency(self, signal_data):
        """Extract dominant frequency from CSI signal"""
        fft = np.fft.fft(signal_data)
        freqs = np.fft.fftfreq(len(signal_data))
        magnitude = np.abs(fft)
        return freqs[np.argmax(magnitude)]
    
    def spectral_centroid(self, signal_data):
        """Calculate spectral centroid for signal characterization"""
        fft = np.fft.fft(signal_data)
        magnitude = np.abs(fft)
        freqs = np.fft.fftfreq(len(signal_data))
        return np.sum(freqs * magnitude) / np.sum(magnitude)
    
    def train_fingerprint_model(self, training_data):
        """Train machine learning model for device fingerprinting"""
        features = []
        labels = []
        
        for sample in training_data:
            feature_vector = list(sample['features'].values())
            features.append(feature_vector)
            labels.append(sample['device_id'])
        
        # Normalize features
        features_scaled = self.scaler.fit_transform(features)
        
        # Train classifier
        self.classifier.fit(features_scaled, labels)
        
    def identify_device(self, csi_features):
        """Identify device based on CSI fingerprint"""
        feature_vector = list(csi_features.values())
        feature_scaled = self.scaler.transform([feature_vector])
        
        prediction = self.classifier.predict(feature_scaled)[0]
        confidence = max(self.classifier.predict_proba(feature_scaled)[0])
        
        return prediction, confidence
```

## Configuration Examples

### ESP-IDF menuconfig Settings for CSI
```bash
# Key configuration parameters for optimal CSI collection
Component config -> Wi-Fi -> WiFi CSI(Channel State Information) [*]
Component config -> FreeRTOS -> Tick rate (Hz) -> 1000
Serial flasher config -> Custom baud rate value -> 921600
ESP32 CSI Tool Config -> CSI send rate -> 100 (packets per second)
ESP32 CSI Tool Config -> CSI output format -> CSV
```

### Arduino IDE Setup for WiFi Monitoring
```cpp
// Arduino-compatible setup for ESP32 WiFi monitoring
#include "WiFi.h"
#include "esp_wifi.h"

void setup() {
    Serial.begin(921600);
    
    // Initialize WiFi in monitor mode
    WiFi.mode(WIFI_MODE_NULL);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_callback);
    esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
    
    Serial.println("WiFi monitoring started");
}

void wifi_sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    // Process intercepted packets for fingerprinting
    if (type == WIFI_PKT_MGMT) {
        // Extract and process management frames
        process_management_frame(buf);
    }
}
```

This collection of code samples demonstrates the core functionality implemented across the various ESP32 WiFi fingerprinting repositories, showing practical implementations for CSI data collection, probe request sniffing, MAC address scanning, and indoor positioning systems.