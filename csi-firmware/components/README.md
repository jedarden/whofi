# CSI Firmware Components

This directory contains the component implementations for the CSI Positioning System firmware. Each component is designed as a modular, reusable ESP-IDF component with comprehensive functionality and error handling.

## Components Overview

### 1. MQTT Client (`mqtt_client/`)

A comprehensive MQTT client wrapper that provides:

#### Core Features
- **SSL/TLS Support**: Secure MQTT connections with automatic certificate handling
- **Automatic Reconnection**: Robust reconnection logic with exponential backoff
- **CSI Data Publishing**: Direct integration with CSI collector for seamless data transmission
- **Remote Control**: Command subscription and processing for remote device management
- **Statistics Tracking**: Comprehensive connection and performance metrics

#### Key Files
- `src/mqtt_client_wrapper.c` - Main MQTT client implementation
- `src/mqtt_publisher.c` - Publishing utilities for device status, metrics, and alerts
- `src/mqtt_subscriber.c` - Subscription handling and remote command processing
- `include/mqtt_client_wrapper.h` - Public API definitions
- `test/test_mqtt_client.c` - Comprehensive unit tests

#### API Highlights
```c
// Initialize and start MQTT client
esp_err_t mqtt_client_init(const mqtt_config_t *config);
esp_err_t mqtt_client_start(void);

// Publish CSI data with automatic JSON serialization
esp_err_t mqtt_client_publish_csi_data(const csi_data_t *csi_data);

// Subscribe to device control topics
esp_err_t mqtt_subscriber_subscribe_device_topics(const char *device_id);

// Publish system status and alerts
esp_err_t mqtt_publish_device_status(const char *device_id, const char *version, 
                                    uint32_t uptime, int8_t wifi_rssi, uint32_t free_heap);
esp_err_t mqtt_publish_alert(const char *device_id, const char *level, 
                           const char *component, const char *message);
```

#### Configuration
```c
typedef struct {
    bool enabled;           ///< MQTT client enabled
    char broker_url[128];   ///< MQTT broker URL
    uint16_t port;          ///< MQTT broker port
    char username[32];      ///< MQTT username
    char password[64];      ///< MQTT password
    char client_id[32];     ///< MQTT client ID
    char topic_prefix[64];  ///< Topic prefix for published data
    bool ssl_enabled;       ///< SSL/TLS enabled
    uint16_t keepalive;     ///< Keepalive interval
    uint8_t qos;            ///< Quality of Service level
    bool retain;            ///< Retain messages flag
} mqtt_config_t;
```

### 2. NTP Sync (`ntp_sync/`)

Advanced network time synchronization with:

#### Core Features
- **Multiple NTP Servers**: Automatic failover between primary, secondary, and tertiary servers
- **Drift Compensation**: Advanced algorithms to maintain accurate time between syncs
- **Timezone Support**: Comprehensive timezone handling with named timezone support
- **Quality Monitoring**: Real-time sync quality assessment and reporting
- **Callback System**: Event notifications for sync status changes

#### Key Files
- `src/ntp_sync.c` - Main NTP synchronization implementation
- `src/ntp_client.c` - Utility functions and timezone handling
- `include/ntp_sync.h` - Public API definitions
- `test/test_ntp_sync.c` - Comprehensive unit tests

#### API Highlights
```c
// Initialize and start NTP synchronization
esp_err_t ntp_sync_init(const ntp_config_t *config);
esp_err_t ntp_sync_start(void);

// Get synchronized time with drift compensation
esp_err_t ntp_sync_get_time(struct timeval *tv);

// Force immediate synchronization
esp_err_t ntp_sync_force_sync(void);

// Timezone utilities
esp_err_t ntp_client_get_timezone_offset(const char *timezone, int16_t *offset_minutes);
esp_err_t ntp_client_format_time(uint64_t timestamp_us, int16_t timezone_offset, 
                                char *buffer, size_t buffer_size);

// Server performance monitoring
esp_err_t ntp_client_get_server_stats(const char *server, ntp_server_stats_t *stats);
esp_err_t ntp_client_get_sync_quality(ntp_sync_quality_t *quality);
```

#### Configuration
```c
typedef struct {
    bool enabled;           ///< NTP sync enabled
    char server1[64];       ///< Primary NTP server
    char server2[64];       ///< Secondary NTP server
    char server3[64];       ///< Tertiary NTP server
    int16_t timezone_offset; ///< Timezone offset in minutes
    uint16_t sync_interval; ///< Sync interval in minutes
    uint16_t timeout;       ///< Sync timeout in seconds
} ntp_config_t;
```

#### Supported Timezones
- UTC, GMT (Universal/Greenwich Mean Time)
- EST, EDT, CST, CDT, MST, MDT, PST, PDT (US timezones)
- CET, CEST (Central European Time)
- JST (Japan Standard Time)
- AEST, AEDT (Australian Eastern Time)

### 3. CSI Collector (`csi_collector/`)

Handles Channel State Information collection and processing (existing component).

### 4. Web Server (`web_server/`)

Provides web-based configuration interface (existing component).

### 5. OTA Updater (`ota_updater/`)

Handles over-the-air firmware updates (existing component).

## Integration Architecture

### Main Application Flow
1. **System Initialization**: Basic ESP32 setup and WiFi connection
2. **NTP Synchronization**: Establish accurate time reference
3. **MQTT Connection**: Connect to broker and subscribe to control topics
4. **CSI Collection**: Start collecting channel state information
5. **Main Loop**: Process data, publish to MQTT, monitor system health

### Component Interactions
```
┌─────────────┐    ┌──────────────┐    ┌─────────────┐
│ CSI         │───▶│ NTP Sync     │───▶│ MQTT Client │
│ Collector   │    │ (Timestamps) │    │ (Publish)   │
└─────────────┘    └──────────────┘    └─────────────┘
       │                   │                    │
       │                   │                    ▼
       │                   │            ┌─────────────┐
       │                   │            │ Remote      │
       │                   │            │ Commands    │
       │                   │            └─────────────┘
       │                   │                    │
       ▼                   ▼                    ▼
┌─────────────────────────────────────────────────────┐
│              Main Application                        │
│  • System monitoring                                │
│  • Error handling                                  │
│  • Configuration management                        │
│  • Health checks and restart logic                │
└─────────────────────────────────────────────────────┘
```

### Error Handling Strategy
- **Graceful Degradation**: Components continue operating even if others fail
- **Automatic Recovery**: Built-in retry logic and reconnection mechanisms
- **Health Monitoring**: Continuous monitoring of system resources and connectivity
- **Alert System**: MQTT-based alerting for critical conditions
- **Emergency Restart**: Automatic restart on critical failures

### Thread Safety
- All components use FreeRTOS synchronization primitives (mutexes, semaphores)
- Thread-safe APIs for cross-component communication
- Proper resource cleanup on shutdown

## Building and Testing

### Build Requirements
- ESP-IDF v4.4 or later
- CMake build system
- Component dependencies defined in CMakeLists.txt

### Running Tests
```bash
# Build and run MQTT client tests
cd components/mqtt_client/test
idf.py build flash monitor

# Build and run NTP sync tests
cd components/ntp_sync/test
idf.py build flash monitor
```

### Configuration Options
Components are configured through the main application's `app_config_t` structure, which includes:
- MQTT broker settings and credentials
- NTP server list and sync parameters
- CSI collection parameters
- System monitoring thresholds

## Performance Characteristics

### Memory Usage
- MQTT Client: ~8KB RAM, ~32KB Flash
- NTP Sync: ~4KB RAM, ~24KB Flash
- Combined with drift compensation: ~2KB additional RAM

### Network Traffic
- CSI Data: Variable, typically 500-2000 bytes per sample
- System Metrics: ~200 bytes every 5 minutes
- NTP Sync: ~48 bytes per sync request
- MQTT Keepalive: Configurable, typically every 60 seconds

### Timing Accuracy
- NTP Synchronization: ±10ms typical accuracy
- Drift Compensation: ±1ms over 24 hours
- CSI Timestamp Precision: 1 microsecond resolution

## Security Considerations

### MQTT Security
- SSL/TLS encryption supported
- Username/password authentication
- Certificate validation (configurable)
- Automatic security upgrades on reconnection

### NTP Security
- Multiple server validation
- Sanity checks on time updates
- Protection against time replay attacks
- Configurable maximum time adjustment limits

### General Security
- No sensitive data in logs
- Secure configuration storage in NVS
- Input validation on all APIs
- Memory protection and bounds checking

## Future Enhancements

### Planned Features
1. **MQTT Message Encryption**: End-to-end payload encryption
2. **Advanced Time Sync**: GPS time synchronization fallback
3. **Load Balancing**: Intelligent NTP server selection
4. **Compression**: CSI data compression for bandwidth optimization
5. **Edge Computing**: Local CSI processing before transmission

### Performance Optimizations
1. **Batch Publishing**: Group multiple CSI samples for efficient transmission
2. **Adaptive Sampling**: Dynamic CSI collection rate based on activity
3. **Smart Reconnection**: Network condition-aware reconnection strategies
4. **Memory Pooling**: Pre-allocated memory pools for high-frequency operations