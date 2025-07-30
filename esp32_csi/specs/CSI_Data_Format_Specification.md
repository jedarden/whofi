# ESP32 CSI Data Format Specification

## Overview
This document provides detailed specifications for ESP32 Channel State Information (CSI) data formats, extraction methods, and processing requirements for the WHOFI project.

## CSI Data Structure

### Primary Data Structure (wifi_csi_info_t)
```c
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;     // Packet reception control information
    uint8_t mac[6];                 // Source MAC address (transmitter)
    uint8_t dmac[6];                // Destination MAC address (receiver)  
    bool first_word_invalid;        // Hardware limitation indicator
    int8_t *buf;                    // CSI data buffer pointer
    uint16_t len;                   // CSI buffer length in bytes
    int8_t noise_floor;             // RF noise floor measurement
} wifi_csi_info_t;
```

### Reception Control Structure (wifi_pkt_rx_ctrl_t)
```c  
typedef struct {
    signed rssi:8;                  // RSSI of packet (dBm)
    unsigned rate:4;                // PHY rate encoding
    unsigned is_group:1;            // Group addressed frame
    unsigned:1;                     // Reserved
    unsigned sig_mode:2;            // Signal mode (legacy/HT/VHT)
    unsigned legacy_length:12;      // Legacy length field
    unsigned damatch0:1;            // DA match 0
    unsigned damatch1:1;            // DA match 1
    unsigned bssidmatch0:1;         // BSSID match 0
    unsigned bssidmatch1:1;         // BSSID match 1
    unsigned MCS:7;                 // MCS index
    unsigned CWB:1;                 // Channel bandwidth (0=20MHz, 1=40MHz)
    unsigned HT_length:16;          // HT length field
    unsigned Smoothing:1;           // Channel smoothing
    unsigned Not_Sounding:1;        // Not sounding frame
    unsigned:1;                     // Reserved
    unsigned Aggregation:1;         // AMPDU aggregation
    unsigned STBC:2;                // STBC streams
    unsigned FEC_CODING:1;          // FEC coding (0=BCC, 1=LDPC)
    unsigned SGI:1;                 // Short guard interval
    unsigned rxend_state:8;         // RX end state
    unsigned ampdu_cnt:8;           // AMPDU count
    unsigned channel:4;             // Primary channel
    unsigned secondary_channel:2;   // Secondary channel
    unsigned:2;                     // Reserved
    unsigned timestamp:32;          // Timestamp
    unsigned noise_floor:8;         // Noise floor
    unsigned ant:8;                 // Antenna configuration
    unsigned sig_len:12;            // Signal field length
    unsigned:12;                    // Reserved
    unsigned rx_state:8;            // RX state machine
} wifi_pkt_rx_ctrl_t;
```

## CSI Buffer Format

### Subcarrier Data Encoding
Each subcarrier's complex channel response is encoded as two consecutive signed 8-bit integers:
```
Byte N:   Imaginary component (int8_t)
Byte N+1: Real component (int8_t)
```

### Channel Frequency Response Types

#### 1. LLTF (Legacy Long Training Field)
- **Subcarriers**: 64 total (52 data + 4 pilots + 8 unused)
- **Data Length**: 128 bytes (64 × 2 bytes)
- **Frequency Mapping**: -26 to +26 (excluding DC)
- **Compatibility**: 802.11a/g legacy devices

#### 2. HT-LTF (High Throughput LTF)  
- **Subcarriers**: Variable based on bandwidth
- **20MHz**: 56 subcarriers → 112 bytes
- **40MHz**: 114 subcarriers → 228 bytes
- **Spatial Streams**: 1-4 streams supported
- **Compatibility**: 802.11n MIMO systems

#### 3. STBC-HT-LTF (Space-Time Block Code HT-LTF)
- **Purpose**: Space-time diversity encoding
- **Subcarriers**: Same as HT-LTF
- **Encoding**: Additional spatial processing
- **Applications**: Enhanced MIMO performance

## CSI Configuration

### Configuration Structure (wifi_csi_config_t)
```c
typedef struct {
    bool lltf_en;           // Enable LLTF CSI collection
    bool htltf_en;          // Enable HT-LTF CSI collection  
    bool stbc_htltf2_en;    // Enable STBC HT-LTF CSI collection
    bool ltf_merge_en;      // Enable LTF merge
    bool channel_filter_en; // Enable channel filter
    bool manu_scale;        // Manual scaling enable
    uint8_t shift;          // Bit shift for scaling
} wifi_csi_config_t;
```

### Recommended Configuration for WHOFI
```c
wifi_csi_config_t csi_config = {
    .lltf_en = true,           // Enable for maximum compatibility
    .htltf_en = true,          // Enable for MIMO information
    .stbc_htltf2_en = false,   // Disable to reduce processing load
    .ltf_merge_en = false,     // Keep separate for analysis
    .channel_filter_en = true, // Enable filtering
    .manu_scale = false,       // Use automatic scaling
    .shift = 0                 // No additional shifting
};
```

## Data Extraction Patterns

### Pattern 1: Real-time Streaming
```c
void csi_recv_cb(void *ctx, wifi_csi_info_t *data) {
    // Process CSI data in callback
    // WARNING: Keep processing minimal - runs in WiFi task
    
    // Queue data for background processing
    xQueueSend(csi_queue, data, 0);
}
```

### Pattern 2: Batch Processing
```c
void csi_batch_processor(void *pvParameters) {
    wifi_csi_info_t csi_data;
    
    while (1) {
        if (xQueueReceive(csi_queue, &csi_data, portMAX_DELAY)) {
            // Perform intensive processing here
            process_csi_fingerprint(&csi_data);
        }
    }
}
```

### Pattern 3: Selective Collection
```c
void selective_csi_cb(void *ctx, wifi_csi_info_t *data) {
    // Filter by MAC address for specific devices
    if (is_target_device(data->mac)) {
        store_csi_data(data);
    }
}
```

## Data Processing Pipeline

### Stage 1: Raw Data Validation
```c
bool validate_csi_data(wifi_csi_info_t *csi) {
    // Check for hardware limitation
    if (csi->first_word_invalid) {
        // Skip first 4 bytes of CSI buffer
        csi->buf += 4;
        csi->len -= 4;
    }
    
    // Validate buffer length
    return (csi->len > 0 && csi->buf != NULL);
}
```

### Stage 2: Complex Number Extraction
```c
typedef struct {
    float real;
    float imag;
    float amplitude;
    float phase;
} csi_complex_t;

void extract_complex_csi(int8_t *raw_buf, int len, csi_complex_t *output) {
    for (int i = 0; i < len/2; i++) {
        output[i].imag = (float)raw_buf[i*2];       // First byte: imaginary
        output[i].real = (float)raw_buf[i*2 + 1];   // Second byte: real
        
        // Calculate amplitude and phase
        output[i].amplitude = sqrt(output[i].real * output[i].real + 
                                  output[i].imag * output[i].imag);
        output[i].phase = atan2(output[i].imag, output[i].real);
    }
}
```

### Stage 3: Feature Extraction for Fingerprinting
```c
typedef struct {
    float mean_amplitude;
    float amplitude_variance;
    float phase_stability;
    float subcarrier_correlation[64];
    uint8_t device_signature[32];
} csi_fingerprint_t;

void extract_fingerprint_features(csi_complex_t *csi_data, int num_subcarriers, 
                                 csi_fingerprint_t *fingerprint) {
    // Calculate statistical features
    float sum_amp = 0, sum_amp_sq = 0;
    
    for (int i = 0; i < num_subcarriers; i++) {
        sum_amp += csi_data[i].amplitude;
        sum_amp_sq += csi_data[i].amplitude * csi_data[i].amplitude;
    }
    
    fingerprint->mean_amplitude = sum_amp / num_subcarriers;
    fingerprint->amplitude_variance = (sum_amp_sq / num_subcarriers) - 
                                     (fingerprint->mean_amplitude * fingerprint->mean_amplitude);
    
    // Additional feature extraction...
}
```

## Output Formats

### CSV Format (for analysis tools)
```
timestamp,mac_addr,rssi,noise_floor,channel,sig_mode,csi_data
1234567890,aa:bb:cc:dd:ee:ff,-45,-95,6,1,"128,45,67,89,..."
```

### Binary Format (for real-time processing)
```
Header (32 bytes):
- Timestamp (8 bytes)
- MAC Address (6 bytes) 
- RSSI (1 byte)
- Channel (1 byte)
- CSI Length (2 bytes)
- Reserved (14 bytes)

CSI Data (variable length):
- Raw CSI buffer content
```

### JSON Format (for web interfaces)
```json
{
    "timestamp": 1234567890,
    "mac_address": "aa:bb:cc:dd:ee:ff",
    "rssi": -45,
    "noise_floor": -95,
    "channel": 6,
    "signal_mode": 1,
    "csi_data": {
        "length": 128,
        "subcarriers": [
            {"real": 45, "imag": 67, "amplitude": 81.4, "phase": 0.98},
            ...
        ]
    }
}
```

## Storage Considerations

### Memory Requirements
- **Per CSI Frame**: 128-512 bytes (depending on configuration)
- **Processing Buffer**: 4-16KB recommended
- **Historical Storage**: 1MB per hour (typical collection rate)

### SD Card Structure
```
/csi_data/
├── raw/          # Raw CSI dumps
├── processed/    # Processed features
├── fingerprints/ # Device fingerprint database
└── logs/         # System logs
```

## Performance Optimization

### Callback Optimization
```c
// Minimize processing in CSI callback
void IRAM_ATTR csi_recv_cb(void *ctx, wifi_csi_info_t *data) {
    // Copy essential data only
    csi_sample_t sample;
    sample.timestamp = esp_timer_get_time();
    sample.rssi = data->rx_ctrl.rssi;
    memcpy(sample.mac, data->mac, 6);
    
    // Queue for background processing
    xQueueSendFromISR(csi_queue, &sample, NULL);
}
```

### Buffer Management
```c
// Circular buffer for continuous operation
#define CSI_BUFFER_SIZE 1024
static csi_sample_t csi_buffer[CSI_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;
```

## Hardware-Specific Considerations

### ESP32 Original
- First word invalid bug affects all CSI data
- Maximum practical sampling rate: ~500 Hz
- Recommended for proof-of-concept

### ESP32-S3
- Enhanced processing capabilities  
- Better for real-time machine learning
- Recommended for production deployment

### ESP32-C6
- Latest architecture with improved CSI handling
- Better power efficiency
- Future-proof choice for WHOFI

## Integration with WHOFI Architecture

### Data Flow
```
ESP32 CSI → Feature Extraction → Device Classification → WHOFI Database
```

### API Interface
```c
// WHOFI integration functions
whofi_device_id_t whofi_identify_device(csi_fingerprint_t *fingerprint);
bool whofi_update_fingerprint_db(uint8_t *mac_addr, csi_fingerprint_t *fingerprint);
whofi_confidence_t whofi_get_identification_confidence(whofi_device_id_t device_id);
```

This specification provides the foundation for implementing ESP32 CSI-based device fingerprinting within the WHOFI project architecture.