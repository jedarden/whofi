# ESP32 WiFi CSI Technical Overview for WHOFI Project

## Executive Summary

Channel State Information (CSI) on ESP32 provides detailed wireless channel characteristics including signal amplitude, phase, and delay information. This enables sophisticated device fingerprinting and wireless sensing applications for the WHOFI project.

## ESP32 CSI Fundamentals

### What is CSI?
Channel State Information (CSI) describes the characteristics of a wireless communication channel, containing:
- **Signal Amplitude**: Strength of the received signal per subcarrier
- **Phase Information**: Phase shift of each subcarrier
- **Channel Frequency Response**: Detailed response across all OFDM subcarriers
- **Temporal Characteristics**: How these parameters change over time

### ESP32 CSI Architecture

The ESP32 WiFi subsystem provides CSI data through three Channel Frequency Response (CFR) types:
1. **LLTF (Legacy Long Training Field)**: Basic 802.11a/g compatibility
2. **HT-LTF (High Throughput LTF)**: 802.11n MIMO support
3. **STBC-HT-LTF (Space-Time Block Code HT-LTF)**: Advanced MIMO encoding

## Hardware Platform Capabilities

### Supported ESP32 Series
- **ESP32**: Original dual-core, 240MHz, full CSI support
- **ESP32-S2**: Single-core, WiFi-focused variant
- **ESP32-C3**: RISC-V based, compact design
- **ESP32-S3**: Dual-core with AI acceleration
- **ESP32-C6**: Latest generation with enhanced processing

### Processing Capabilities
- **CPU**: Dual-core 240MHz (varies by model)
- **AI Instructions**: Built-in neural network acceleration
- **Memory**: Sufficient for real-time CSI processing
- **Wireless**: 802.11b/g/n support with full CSI extraction

## CSI Data Structure and Format

### Raw CSI Data Format
Each subcarrier's channel frequency response is encoded as:
```
[Imaginary Part (1 byte)][Real Part (1 byte)]
```

### Data Structure (wifi_csi_info_t)
```c
typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;     // Reception control info
    uint8_t mac[6];                 // Source MAC address  
    uint8_t dmac[6];                // Destination MAC address
    bool first_word_invalid;        // Hardware limitation flag
    int8_t *buf;                    // CSI data buffer
    uint16_t len;                   // Buffer length
    int8_t noise_floor;             // RF noise floor
} wifi_csi_info_t;
```

### CSI Buffer Layout
- **LLTF**: 64 subcarriers × 2 bytes = 128 bytes
- **HT-LTF**: Variable based on bandwidth and spatial streams
- **STBC-HT-LTF**: Enhanced spatial diversity data

### Data Extraction Methods

#### Method 1: Active Station Mode
```c
// Connect to AP and actively request CSI data
esp_wifi_set_mode(WIFI_MODE_STA);
esp_wifi_set_csi_config(&csi_config);
esp_wifi_set_csi_rx_cb(&csi_recv_cb, NULL);
```

#### Method 2: Access Point Mode  
```c
// Create AP and collect CSI from connecting devices
esp_wifi_set_mode(WIFI_MODE_AP);
esp_wifi_set_csi_config(&csi_config);
```

#### Method 3: Promiscuous/Monitor Mode
```c
// Passive listening for maximum data collection
esp_wifi_set_promiscuous(true);
esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
```

## Signal Processing for Device Fingerprinting

### CSI Feature Extraction
1. **Amplitude Processing**
   ```
   Amplitude = sqrt(Real² + Imaginary²)
   ```

2. **Phase Calculation**
   ```  
   Phase = atan2(Imaginary, Real)
   ```

3. **Statistical Features**
   - Mean amplitude across subcarriers
   - Variance and standard deviation
   - Correlation between subcarriers
   - Temporal stability metrics

### Fingerprinting Techniques

#### 1. Amplitude-Based Fingerprinting
- Extract amplitude patterns unique to device hardware
- Account for antenna characteristics and RF chain variations
- Apply noise filtering and normalization

#### 2. Phase-Based Analysis
- Utilize phase patterns for fine-grained device identification
- Compensate for timing synchronization differences
- Apply phase unwrapping algorithms

#### 3. Multi-dimensional Feature Vectors
- Combine amplitude, phase, and temporal characteristics
- Apply dimensionality reduction (PCA, t-SNE)
- Use machine learning for classification

### Preprocessing Pipeline
```
Raw CSI → Noise Filtering → Normalization → Feature Extraction → Classification
```

## Implementation Approaches for WHOFI

### Real-time Processing
- **Sampling Rate**: Up to 1000 packets/second
- **Buffer Management**: Circular buffers for continuous operation
- **Memory Usage**: ~2-4KB per CSI frame
- **Processing Latency**: <10ms for basic fingerprinting

### Machine Learning Integration
- **On-device TinyML**: Basic classification directly on ESP32
- **Edge Processing**: More complex algorithms on companion processor
- **Cloud Integration**: Full analysis pipeline for training

### Data Storage and Transmission
- **SD Card Logging**: High-capacity local storage
- **Serial Output**: Real-time streaming to host
- **WiFi Transmission**: Network-based data collection
- **Bluetooth**: Low-power auxiliary channel

## Hardware Requirements and Limitations

### Minimum Requirements
- ESP32 with WiFi capability
- 4MB Flash memory (recommended 16MB+)
- External antenna (optional but recommended)
- Stable power supply (3.3V, >500mA)

### Performance Limitations
- **First Word Invalid**: Hardware bug affects first 4 bytes
- **Callback Constraints**: CSI callback runs in WiFi task context
- **Memory Limitations**: Large CSI buffers require careful management
- **Processing Power**: Complex algorithms may need offloading

### Optimal Hardware Configuration
- ESP32-S3 or ESP32-C6 for enhanced processing
- External high-gain antenna for improved signal quality
- SD card for data logging
- Real-time clock for timestamp synchronization

## Development Resources

### Official Tools
- **ESP-IDF**: Official development framework with CSI APIs
- **esp-csi**: Espressif's reference implementation
- **ESP32 CSI Toolkit**: Comprehensive research tool

### Community Resources
- **ESP32-CSI-Tool**: Steven Hernandez's research toolkit
- **CSIKit**: Python processing framework
- **Wi-ESP**: Device-free WiFi sensing implementation

### API Reference
```c
// Core CSI functions
esp_err_t esp_wifi_set_csi_config(const wifi_csi_config_t *config);
esp_err_t esp_wifi_set_csi_rx_cb(wifi_csi_cb_t cb, void *ctx);
esp_err_t esp_wifi_set_csi(bool en);
```

## Next Steps for WHOFI Implementation

1. **Hardware Selection**: Choose optimal ESP32 variant
2. **Development Environment**: Set up ESP-IDF with CSI support
3. **Proof of Concept**: Implement basic CSI collection
4. **Feature Engineering**: Develop device fingerprinting algorithms
5. **Integration Testing**: Validate with WHOFI architecture
6. **Performance Optimization**: Optimize for real-time operation

## Conclusion

ESP32 provides a cost-effective, powerful platform for WiFi CSI-based device fingerprinting. The combination of hardware CSI extraction capabilities, flexible software framework, and rich community resources makes it an ideal choice for the WHOFI project's wireless sensing requirements.