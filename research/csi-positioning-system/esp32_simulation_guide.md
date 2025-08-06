# ESP32 Simulation Options and Capabilities

## Available ESP32 Simulators

### 1. **Wokwi** (Most Popular - Browser-Based)
- **URL**: https://wokwi.com/
- **Cost**: Free tier + paid features
- **Capabilities**: 
  - Full Arduino IDE support
  - ESP-IDF support
  - WiFi simulation (limited)
  - GPIO, I2C, SPI, UART
  - Many virtual components
- **Limitations**:
  - No CSI support
  - Limited WiFi (basic connectivity only)
  - No Bluetooth
  - No real RF simulation

### 2. **QEMU for ESP32**
- **Type**: Full system emulator
- **Cost**: Free/Open Source
- **Capabilities**:
  - Complete ESP32 emulation
  - GDB debugging support
  - Network simulation
  - Flash/RAM emulation
- **Limitations**:
  - No WiFi RF layer
  - No CSI/Bluetooth
  - Complex setup

### 3. **Proteus VSM**
- **Type**: Commercial circuit simulator
- **Cost**: $500-2000+
- **Capabilities**:
  - Mixed circuit simulation
  - Basic ESP32 support
  - Peripheral simulation
- **Limitations**:
  - Limited ESP32 features
  - No WiFi/Bluetooth
  - Expensive

### 4. **ESP32 Software Simulator** (Native)
- **Type**: Software-only simulation
- **Cost**: Free
- **Capabilities**:
  - Logic testing
  - API simulation
  - Unit testing
- **Limitations**:
  - No hardware timing
  - No peripherals
  - Code refactoring needed

## Wokwi ESP32 Simulation Example

### Basic WiFi RSSI Simulation
```cpp
// This works in Wokwi simulator!
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop() {
  Serial.println("Scanning WiFi networks...");
  int n = WiFi.scanNetworks();
  
  // Wokwi simulates fake networks
  for (int i = 0; i < n; ++i) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm)");
  }
  
  delay(5000);
}
```

### Wokwi Project Configuration (diagram.json)
```json
{
  "version": 1,
  "author": "WhoFi Research",
  "editor": "wokwi",
  "parts": [
    {
      "type": "wokwi-esp32-devkit-v1",
      "id": "esp",
      "top": 0,
      "left": 0,
      "attrs": {}
    }
  ],
  "connections": [],
  "serialMonitor": {
    "display": "terminal",
    "newline": "lf"
  }
}
```

## QEMU ESP32 Setup

### Installation
```bash
# Install dependencies
sudo apt-get install git wget flex bison gperf python3 python3-pip
sudo apt-get install ninja-build ccache libffi-dev libssl-dev dfu-util

# Clone ESP32 QEMU
git clone https://github.com/espressif/qemu.git
cd qemu
./configure --target-list=xtensa-softmmu --enable-debug
make -j8

# Download ESP32 ROM
wget https://github.com/espressif/qemu/releases/download/esp32-rom-v2/esp32-rom.bin
```

### Running ESP32 in QEMU
```bash
# Basic ESP32 emulation
./qemu-system-xtensa \
  -nographic \
  -machine esp32 \
  -drive file=flash_image.bin,if=mtd,format=raw \
  -serial stdio
```

### QEMU Network Simulation
```bash
# Create virtual network
./qemu-system-xtensa \
  -nographic \
  -machine esp32 \
  -netdev user,id=net0,hostfwd=tcp::8080-:80 \
  -device esp32_wifi,netdev=net0 \
  -drive file=esp32_app.bin,if=mtd,format=raw
```

## Software-Only Simulation

### Mock ESP32 WiFi for Testing
```cpp
// esp32_wifi_mock.h
#ifndef RUNNING_ON_ESP32

class MockWiFi {
  private:
    int mock_rssi = -45;
    std::vector<String> mock_networks;
    
  public:
    void begin() { }
    void mode(int mode) { }
    
    int scanNetworks() {
      // Simulate network discovery
      mock_networks = {"TestAP1", "TestAP2", "TestAP3"};
      return mock_networks.size();
    }
    
    String SSID(int index) {
      return mock_networks[index];
    }
    
    int RSSI(int index) {
      // Simulate varying RSSI
      return mock_rssi - (index * 5) + random(-3, 3);
    }
};

MockWiFi WiFi;  // Replace real WiFi object

#else
  #include <WiFi.h>  // Use real WiFi on ESP32
#endif
```

### Python ESP32 Simulator
```python
#!/usr/bin/env python3
"""
Simple ESP32 WiFi behavior simulator for algorithm testing
"""

import random
import numpy as np
import time

class ESP32Simulator:
    def __init__(self):
        self.rssi_baseline = -45
        self.networks = [
            {"ssid": "AP1", "bssid": "AA:BB:CC:DD:EE:01", "channel": 1},
            {"ssid": "AP2", "bssid": "AA:BB:CC:DD:EE:02", "channel": 6},
            {"ssid": "AP3", "bssid": "AA:BB:CC:DD:EE:03", "channel": 11}
        ]
        self.person_present = False
        self.person_position = [0, 0]
        
    def WiFi_RSSI(self, ap_index=0):
        """Simulate RSSI affected by person presence"""
        base = self.rssi_baseline
        
        if self.person_present:
            # Person causes 3-10 dB attenuation
            distance = np.linalg.norm(self.person_position)
            attenuation = min(10, 3 + distance)
            base -= attenuation
            
        # Add realistic noise
        noise = random.gauss(0, 2)
        return int(base + noise)
    
    def WiFi_scanNetworks(self):
        """Simulate network scan"""
        time.sleep(0.1)  # Simulate scan time
        return len(self.networks)
    
    def simulate_csi_data(self):
        """Simulate CSI data (not available on real ESP32 Arduino)"""
        subcarriers = 52
        csi_amplitude = np.ones(subcarriers) * 10
        csi_phase = np.zeros(subcarriers)
        
        if self.person_present:
            # Person affects different subcarriers differently
            for i in range(subcarriers):
                freq = 2.412e9 + i * 312.5e3  # Subcarrier frequency
                wavelength = 3e8 / freq
                
                # Simulate multipath
                path_diff = random.uniform(0, wavelength)
                phase_shift = 2 * np.pi * path_diff / wavelength
                
                csi_phase[i] = phase_shift
                csi_amplitude[i] *= (0.7 + 0.3 * random.random())
        
        return csi_amplitude, csi_phase
    
    def add_person(self, x, y):
        """Simulate person entering space"""
        self.person_present = True
        self.person_position = [x, y]
        
    def remove_person(self):
        """Simulate person leaving"""
        self.person_present = False
        self.person_position = [0, 0]

# Usage example
def test_presence_detection():
    esp32 = ESP32Simulator()
    
    print("Testing RSSI-based presence detection")
    print("Empty room RSSI:", esp32.WiFi_RSSI())
    
    # Person enters
    esp32.add_person(2, 3)
    print("Person present RSSI:", esp32.WiFi_RSSI())
    
    # Get CSI data (simulated)
    amp, phase = esp32.simulate_csi_data()
    print(f"CSI Amplitude mean: {np.mean(amp):.2f}")
    print(f"CSI Phase std: {np.std(phase):.2f}")

if __name__ == "__main__":
    test_presence_detection()
```

## Docker-Based ESP32 Development Environment

### Dockerfile for ESP32 Simulation
```dockerfile
FROM ubuntu:20.04

# Install ESP-IDF dependencies
RUN apt-get update && apt-get install -y \
    git wget flex bison gperf python3 python3-pip \
    python3-setuptools cmake ninja-build ccache \
    libffi-dev libssl-dev dfu-util libusb-1.0-0

# Install ESP-IDF
RUN git clone --recursive https://github.com/espressif/esp-idf.git /esp-idf
WORKDIR /esp-idf
RUN ./install.sh esp32

# Install QEMU
RUN git clone https://github.com/espressif/qemu.git /qemu
WORKDIR /qemu
RUN ./configure --target-list=xtensa-softmmu && make -j8

# Setup environment
ENV IDF_PATH=/esp-idf
ENV PATH=$PATH:/qemu/xtensa-softmmu

WORKDIR /workspace
```

## Simulation Limitations

### What CAN be Simulated:
✅ **Digital I/O** - GPIO, interrupts
✅ **Communication** - UART, I2C, SPI
✅ **Timers** - Basic timing functions
✅ **Memory** - RAM/Flash operations
✅ **Basic WiFi** - Connection, RSSI (limited)
✅ **Program Logic** - All code execution

### What CANNOT be Simulated:
❌ **CSI Data** - No RF channel state
❌ **Real RF** - No actual radio signals
❌ **Bluetooth** - Limited/no support
❌ **Analog** - ADC readings approximate
❌ **Power** - Sleep modes approximate
❌ **Temperature** - Sensor readings fake

## Testing Strategy for WhoFi

### 1. **Unit Testing (No Hardware)**
```cpp
// Test positioning algorithms
void test_trilateration() {
    // Mock RSSI values
    int rssi_values[] = {-45, -50, -48};
    Point ap_positions[] = {{0,0}, {5,0}, {0,5}};
    
    Point result = trilaterate(rssi_values, ap_positions);
    assert(result.x > 1.5 && result.x < 3.5);
    assert(result.y > 1.5 && result.y < 3.5);
}
```

### 2. **Wokwi for Basic WiFi**
- Test WiFi connection logic
- Verify RSSI processing
- Debug state machines
- Test MQTT communication

### 3. **QEMU for System Testing**
- Test OTA updates
- Verify boot sequences
- Debug crashes
- Memory leak detection

### 4. **Hardware-in-Loop**
```python
# Hybrid simulation with real ESP32
class HybridSimulator:
    def __init__(self, serial_port):
        self.serial = serial.Serial(serial_port, 115200)
        self.virtual_environment = VirtualRoom()
        
    def inject_virtual_person(self, x, y):
        # Tell ESP32 to simulate RSSI drop
        self.serial.write(f"SIM_RSSI:{x},{y}\n".encode())
        
    def read_position_estimate(self):
        # Get ESP32's position calculation
        line = self.serial.readline().decode()
        if line.startswith("POS:"):
            return parse_position(line)
```

## Recommended Simulation Approach

### For WhoFi CSI Development:

1. **Algorithm Development**: Python/MATLAB simulation
2. **Basic ESP32 Code**: Wokwi online simulator
3. **System Integration**: QEMU with mock data
4. **Final Testing**: Real hardware required

### Quick Start with Wokwi:
1. Go to https://wokwi.com/
2. Create new ESP32 project
3. Paste Arduino code
4. Add virtual components
5. Run simulation instantly

### Best Practices:
- Simulate algorithms separately from hardware
- Use dependency injection for testability
- Create mock objects for WiFi/CSI
- Test on real hardware regularly
- Document simulator limitations

## Conclusion

ESP32 simulation is **excellent for logic and basic WiFi** but **cannot simulate CSI or RF physics**. For WhoFi development:
- Use simulators for rapid prototyping
- Test algorithms with synthetic data
- Validate on real hardware for RF features
- Combine simulation with hardware-in-loop testing