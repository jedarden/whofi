# GitHub Repository Index: WhoFi and ESP32 CSI/WiFi Fingerprinting

## Research Overview
This document provides a comprehensive index of GitHub repositories related to WhoFi research, ESP32 Channel State Information (CSI), and WiFi fingerprinting implementations.

## Key Findings Summary

### WhoFi Academic Research
- **WhoFi** is a research project from La Sapienza University of Rome
- Published paper: "WhoFi: Deep Person Re-Identification via Wi-Fi Channel Signal Encoding"
- Achieves 95.5% accuracy for person re-identification using WiFi CSI
- Uses Transformer-based deep neural networks
- **No public GitHub repository available** - academic research only

### ESP32 CSI and WiFi Fingerprinting Repositories

## 1. Channel State Information (CSI) Tools

### 1.1 ESP32-CSI-Tool (Primary Repository)
- **Repository**: `https://github.com/StevenMHernandez/ESP32-CSI-Tool`
- **Author**: Steven M. Hernandez
- **Description**: Extract Channel State Information from WiFi-enabled ESP32 microcontroller
- **Website**: https://stevenmhernandez.github.io/ESP32-CSI-Tool/

#### Key Features:
- **Active Station Mode** (`./active_sta`) - Connects to AP and sends packet requests (CSI-TX)
- **Active AP Mode** (`./active_ap`) - Creates AP for device connections (CSI-RX)  
- **Passive Mode** (`./passive`) - Passively listens for CSI frames on channel 3
- Automatic data output to serial port and SD card
- Requires ESP-IDF v4.3
- High baud rate support (921600-1552000) for high sampling rates

#### Applications:
- Wi-Fi Sensing
- Device-free Localization
- Human Activity Recognition
- Gesture Detection
- Indoor Positioning

### 1.2 oh-my-physec/esp32-csi-tool
- **Repository**: `https://github.com/oh-my-physec/esp32-csi-tool`
- **Description**: Alternative ESP32 CSI extraction implementation
- **Purpose**: WiFi sensing and device-free localization research

### 1.3 ESP32-CSI-Python-Parser
- **Repository**: `https://github.com/RikeshMMM/ESP32-CSI-Python-Parser`
- **Description**: Python parser for ESP32 Wi-Fi Channel State Information
- **Features**: Based on ESP32 CSI specification for data analysis

### 1.4 Android-ESP32-CSI
- **Repository**: `https://github.com/StevenMHernandez/Android-ESP32-CSI`
- **Description**: Android library for receiving CSI data through Serial from ESP32
- **Features**: Standard Android app integration, no custom firmware required

## 2. WiFi Probe Request Sniffing and Analysis

### 2.1 ESP32 Sniffer (ETS-PoliTO)
- **Repository**: `https://github.com/ETS-PoliTO/esp32-sniffer`
- **Author**: ETS-PoliTO Research Team
- **Description**: ESP32 firmware for sniffing Probe Request packets from smartphones

#### Technical Details:
- **ESP-IDF Version**: v3.2
- **WiFi Mode**: WIFI_MODE_APSTA (simultaneous sniffing and transmission)
- **Data Extracted**:
  - MAC addresses of devices
  - SSID of requested networks
  - Timestamps
  - RSSI (Received Signal Strength Indicator)
  - Sequence Numbers
  - HT Capabilities Info

#### System Architecture:
- **Sniffer Task**: Captures probe requests and saves to file
- **Wi-Fi Task**: Sends data to server every minute
- Uses file locks for thread-safe I/O operations
- Integrates with ETS-Server and GUI-Application

### 2.2 esp32-probe-sniffer
- **Repository**: `https://github.com/d-michele/esp32-probe-sniffer`
- **Description**: WiFi probe request analyzer
- **Features**: Focused probe request analysis implementation

## 3. MAC Address Scanning and Device Fingerprinting

### 3.1 ESP32-MAC-Scanner
- **Repository**: `https://github.com/AndreasFischer1985/ESP32-MAC-Scanner`
- **Description**: Enables ESP32 to scan/detect MAC addresses in monitor mode
- **Use Cases**:
  - Smart home device counting
  - Presence detection based on device MAC addresses
  - Educational and private use applications

### 3.2 ESP32-Promiscuous-osc
- **Repository**: `https://github.com/thevixer/ESP32-Promiscuous-osc`
- **Description**: ESP32 promiscuous MAC address sniffer with OSC messaging
- **Features**: Real-time MAC address monitoring with Open Sound Control integration

## 4. Comprehensive WiFi Security Tools

### 4.1 ESP32 Marauder
- **Repository**: `https://github.com/justcallmekoko/ESP32Marauder`
- **Author**: justcallmekoko
- **Description**: Suite of WiFi/Bluetooth offensive and defensive tools

#### Key Features:
- **Probe Request Sniffing**: Captures and analyzes probe requests
- **Probe Request Flooding**: Generates probe request traffic
- **PCAP File Support**: Saves captured data to SD card
- **WiFi Attack Framework**: Multiple attack vector implementations
- **Bluetooth Capabilities**: BLE scanning and analysis

### 4.2 ESP32 WiFi Penetration Tool
- **Repository**: `https://github.com/risinek/esp32-wifi-penetration-tool`
- **Description**: Universal ESP32 platform for WiFi network attacks
- **Features**: Common WiFi attack functionality and penetration testing

## 5. Indoor Positioning and Tracking Systems

### 5.1 WiFi_Tracker_Project
- **Repository**: `https://github.com/iFederx/WiFi_Tracker_Project`
- **Institution**: Politecnico di Torino
- **Description**: Wi-Fi indoor localization system using ESP32 boards

#### System Architecture:
- **Multiple ESP32 Boards**: Used as fixed sniffing stations
- **Distributed System**: Provides indoor device localization and monitoring
- **Probe Request Analysis**: Collects and analyzes WiFi probe requests
- **Centralized Processing**: Forwards data to centralized server for analysis

#### Applications:
- Indoor positioning systems
- Device tracking within buildings
- Location-based services
- Smart building applications

## 6. Technical Requirements and Dependencies

### ESP-IDF Versions:
- **ESP32-CSI-Tool**: Requires ESP-IDF v4.3
- **ESP32 Sniffer**: Tested with ESP-IDF v3.2
- **General Compatibility**: Most projects support ESP-WROOM-32 modules

### Hardware Requirements:
- ESP32 microcontroller (ESP-WROOM-32 recommended)
- SD card support for data logging (optional but recommended)
- Serial connection for data collection and monitoring

### Development Environment:
- ESP-IDF (Espressif IoT Development Framework)
- Arduino IDE (for some projects)
- Python environment for data analysis
- MATLAB support for advanced CSI analysis

## 7. Research Applications and Use Cases

### Academic Research:
- **Person Re-identification**: Using WiFi signal patterns to identify individuals
- **Activity Recognition**: Detecting human activities through CSI changes
- **Indoor Localization**: WiFi fingerprinting for positioning systems
- **Device-free Sensing**: Monitoring environments without wearing devices

### Commercial Applications:
- **Smart Home Systems**: Occupancy detection and automation
- **Retail Analytics**: Customer movement and behavior analysis
- **Security Systems**: Intrusion detection and monitoring
- **Building Management**: Space utilization and environmental control

### Privacy and Security Research:
- **MAC Address Randomization**: Studying device privacy techniques
- **Probe Request Analysis**: Understanding device behavior patterns
- **WiFi Security Testing**: Penetration testing and vulnerability assessment

## 8. Installation and Setup Guide

### General Prerequisites:
1. Install ESP-IDF development environment
2. Clone desired repository
3. Configure ESP32 connection and settings
4. Build and flash firmware to ESP32
5. Set up data collection and analysis environment

### Common Configuration Steps:
1. **Serial Configuration**: Set baud rate (921600+ recommended for CSI)
2. **WiFi Settings**: Configure SSID, password, and channel settings
3. **Data Output**: Choose between serial port and SD card logging
4. **Channel Selection**: Set appropriate WiFi channel for monitoring

## 9. Data Analysis and Visualization

### Python Tools:
- **Real-time Visualization**: Live CSI amplitude plotting
- **Data Parsing**: CSV-based data processing
- **Timestamp Synchronization**: Computer-based timing for ESP32 data
- **Machine Learning**: Integration with ML frameworks for pattern recognition

### MATLAB Support:
- CSI data analysis and processing
- Advanced signal processing algorithms
- Research-oriented analysis tools

## 10. Limitations and Considerations

### Technical Limitations:
- **ESP32 Clock Synchronization**: No real-world time sync without external input
- **Sampling Rate**: Limited by serial port baud rate
- **MAC Randomization**: Modern devices use randomized MAC addresses
- **Range Limitations**: WiFi signal strength affects detection range

### Privacy and Legal Considerations:
- **Educational Use Only**: Most tools intended for research and education
- **Legal Compliance**: Must comply with local regulations for WiFi monitoring
- **Privacy Concerns**: Be aware of data collection and privacy implications

## 11. Related Research and Academic Papers

### Key Publications:
- **WhoFi Paper**: "Deep Person Re-Identification via Wi-Fi Channel Signal Encoding"
- **CSI Research**: Multiple papers on WiFi sensing and localization
- **Indoor Positioning**: Academic research on WiFi fingerprinting techniques

### Citation Information:
- ESP32 CSI Tool BibTeX citation available
- Academic papers available on ArXiv and IEEE databases

## 12. Community and Support

### Developer Communities:
- **ESP32 Forum**: Official Espressif community support
- **GitHub Issues**: Individual repository issue tracking
- **Academic Collaboration**: University research partnerships

### Documentation Resources:
- **Official ESP-IDF Documentation**: Comprehensive development guides
- **Project Wikis**: Detailed usage instructions and examples
- **Video Tutorials**: YouTube demonstrations and walkthroughs

---

## Repository Clone Status

All repositories have been successfully cloned to `/workspaces/scratchpad/whofi/repos/`:

✅ ESP32-CSI-Tool  
✅ oh-my-physec-csi  
✅ ESP32-CSI-Python-Parser  
✅ Android-ESP32-CSI  
✅ esp32-sniffer  
✅ esp32-probe-sniffer  
✅ ESP32-MAC-Scanner  
✅ ESP32Marauder  
✅ WiFi_Tracker_Project  

This comprehensive index provides researchers and developers with a complete overview of available tools and research in the field of WiFi fingerprinting and ESP32-based sensing systems.