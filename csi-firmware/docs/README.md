# CSI Positioning System Firmware

## Overview

This firmware implements a CSI (Channel State Information) positioning system for ESP32 devices. The system collects Wi-Fi CSI data and provides it through various interfaces including web configuration, MQTT publishing, and local storage.

## Architecture

The firmware is built using ESP-IDF with a modular component-based architecture:

### Core Components

1. **CSI Collector** (`components/csi_collector/`)
   - Collects CSI data from Wi-Fi radio
   - Provides filtering and buffering capabilities
   - Supports callback-based and queue-based data access

2. **Web Server** (`components/web_server/`)
   - HTTP server for device configuration
   - Real-time CSI data visualization
   - Authentication and session management

3. **MQTT Client** (`components/mqtt_client/`)
   - Publishes CSI data to MQTT broker
   - Supports SSL/TLS encryption
   - Configurable QoS and topic structure

4. **NTP Sync** (`components/ntp_sync/`)
   - Network time synchronization
   - Accurate timestamping of CSI data
   - Multiple NTP server support

5. **OTA Updater** (`components/ota_updater/`)
   - Over-the-air firmware updates
   - Signature verification
   - Rollback capabilities

## Features

- **Real-time CSI Data Collection**: High-frequency CSI data sampling
- **Web Configuration Interface**: Browser-based setup and monitoring
- **MQTT Integration**: Real-time data streaming to cloud services
- **Time Synchronization**: Precise timestamping via NTP
- **OTA Updates**: Remote firmware management
- **Test-Driven Development**: Comprehensive unit test coverage
- **Modular Design**: Clean component separation

## Building and Testing

### Prerequisites

- ESP-IDF v5.0 or later
- Python 3.8 or later
- Git

### Build Firmware

```bash
cd /workspaces/ardenone-cluster-ws/whofi/csi-firmware
idf.py build
```

### Run Tests

```bash
cd test
idf.py build
idf.py flash monitor
```

### Flash Firmware

```bash
idf.py flash monitor
```

## Configuration

The firmware uses a hierarchical configuration system with defaults defined in `sdkconfig.defaults` and runtime configuration stored in NVS flash.

### Key Configuration Parameters

- CSI sampling rate and buffer size
- Wi-Fi credentials and network settings
- MQTT broker connection parameters
- NTP server addresses
- OTA update server URL

## File System Layout

```
/spiffs/         # Web server static files
/data/           # CSI data storage (FAT filesystem)
```

## Memory Partitions

See `partitions.csv` for detailed partition layout:
- Factory app partition (2MB)
- Two OTA partitions (2MB each)
- NVS storage for configuration
- SPIFFS for web assets
- FAT partition for data storage

## Development

### Adding New Components

1. Create component directory under `components/`
2. Add `CMakeLists.txt` with dependencies
3. Implement header files in `include/`
4. Implement source files in `src/`
5. Add unit tests in `test/`

### Testing Strategy

The project uses Unity test framework with:
- Unit tests for individual functions
- Integration tests for component interactions
- Mock objects for hardware dependencies
- Continuous integration support

### Code Quality

- Static analysis with clang-tidy
- Code formatting with clang-format
- Memory leak detection
- Stack overflow protection

## API Documentation

See individual component header files for detailed API documentation:
- `components/*/include/*.h`

## Troubleshooting

### Common Issues

1. **Build Errors**: Ensure ESP-IDF is properly installed and sourced
2. **Flash Errors**: Check USB connection and device permissions
3. **Wi-Fi Issues**: Verify credentials and network compatibility
4. **Memory Issues**: Monitor heap usage and adjust partition sizes

### Debug Logging

Enable debug logging in menuconfig:
```
Component config → Log output → Default log verbosity → Debug
```

## Contributing

1. Follow ESP-IDF coding standards
2. Add unit tests for new functionality
3. Update documentation for API changes
4. Use meaningful commit messages

## License

See LICENSE file for details.

## Contact

For support and contributions, please create issues in the project repository.