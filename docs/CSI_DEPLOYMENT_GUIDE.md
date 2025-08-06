# WiFi CSI Positioning System - Deployment Guide

## Device Requirements for Different Scenarios

### Minimum Viable Detection

#### 1. Basic Presence Detection: 1 Device
- **Single ESP32** can detect movement/presence in its vicinity
- Works by analyzing CSI variations when people move through the WiFi signal
- Detection range: ~5-10 meters
- Use case: Room occupancy, motion detection

#### 2. Basic Localization: 3 Devices
- **Minimum 3 ESP32s** for 2D position estimation
- Uses trilateration based on CSI signal characteristics
- Accuracy: 2-3 meters
- Use case: Room-level positioning

#### 3. Accurate Indoor Positioning: 4+ Devices
- **4 ESP32s** for reliable 2D positioning with redundancy
- **6+ ESP32s** for 3D positioning (height detection)
- Accuracy: 0.5-2 meters with proper calibration
- Use case: Asset tracking, navigation

### Deployment Configurations

#### Small Room (20-30 m²)
```
┌─────────────────┐
│ ESP32 ─────── ESP32 │
│  |               |  │
│  |    Target    |  │
│  |               |  │
│ ESP32 ─────── ESP32 │
└─────────────────┘
```
- 4 devices at corners
- 1-2m positioning accuracy

#### Large Area (100+ m²)
```
ESP32 ── ESP32 ── ESP32
  |        |        |
ESP32 ── ESP32 ── ESP32
  |        |        |
ESP32 ── ESP32 ── ESP32
```
- Grid deployment
- 6-9 devices minimum
- Better coverage and accuracy

## 800 sq ft Multi-Room Deployment

For an 800 sq ft multi-room space, you'll need **6-10 ESP32 devices** for effective CSI positioning.

### Recommended Configuration: 8 Devices

#### Typical 800 sq ft Layout Example:
```
┌─────────────┬──────────────┬───────────┐
│   Living    │              │           │
│   Room      │   Kitchen    │  Bedroom  │
│  ESP32(1)   │   ESP32(3)   │  ESP32(5) │
│      •      │      •       │     •     │
│             │              │           │
├──────┬──────┴──────┬───────┤           │
│      │             │       │           │
│ Bath │   Hallway   │ Entry │  ESP32(6) │
│      │   ESP32(4)  │       │     •     │
│      │      •      │ESP32(7)           │
│      │             │   •   ├───────────┤
│      ├─────────────┼───────┤           │
│      │             │       │  Bedroom  │
│      │   Bedroom   │ Bath  │  ESP32(8) │
│      │   ESP32(2)  │       │     •     │
│      │      •      │       │           │
└──────┴─────────────┴───────┴───────────┘
```

### Device Placement Strategy

#### Minimum (6 devices)
- 1 per major room
- Hallway coverage might be limited
- Accuracy: 2-3 meters
- Some dead zones possible

#### Optimal (8 devices)
- 1-2 per room depending on size
- Full hallway coverage
- Accuracy: 1-2 meters
- Minimal dead zones

#### Enhanced (10 devices)
- 2 in larger rooms (living room)
- Extra hallway/transition coverage
- Accuracy: 0.5-1.5 meters
- No dead zones

### Room-by-Room Breakdown

| Room Type | Size (approx) | Devices Needed |
|-----------|---------------|----------------|
| Living Room | 200-250 sq ft | 2 devices |
| Bedroom (Master) | 120-150 sq ft | 1-2 devices |
| Bedroom (Small) | 80-100 sq ft | 1 device |
| Kitchen | 100-120 sq ft | 1 device |
| Hallway | - | 1 device |
| Bathroom | 40-60 sq ft | Optional (1) |

### Expected Performance

With 8 devices in 800 sq ft:
- **Positioning Accuracy**: 1-2 meters
- **Room-level Accuracy**: 95%+ 
- **Response Time**: <500ms
- **Multiple People**: Can track 3-5 people simultaneously
- **Coverage**: Full coverage with no significant dead zones

## What's Viable with Only 3 Devices?

### Zone-Based Positioning
```
Living Area          Kitchen
    ESP32 ─────────── ESP32
      \                /
       \              /
        \            /
         \          /
          \        /
           ESP32
          Bedroom
```
- **Accuracy**: Zone-level (which room/area)
- **Not viable**: Precise X,Y coordinates
- **Use case**: "User is in living room" vs "User is in kitchen"

### Practical Applications with 3 Devices

#### What You CAN Do:
✅ Detect which room someone is in  
✅ Track movement between rooms  
✅ Monitor room occupancy  
✅ Basic home automation triggers  
✅ Security monitoring (intrusion detection)  
✅ Sleep tracking (bedroom presence)  

#### What You CAN'T Do:
❌ Precise indoor navigation  
❌ Accurate position tracking  
❌ Reliable multi-person tracking  
❌ Full home coverage  
❌ Gesture recognition  
❌ Fall detection (needs higher accuracy)  

### Recommended Use Cases for 3 Devices:

1. **Smart Home Automation**
   - Turn lights on/off based on room presence
   - Adjust HVAC when rooms are occupied
   - Trigger scenes based on movement patterns

2. **Security System**
   - Detect unexpected presence
   - Monitor entry points
   - Track movement patterns when away

3. **Behavioral Monitoring**
   - Sleep pattern analysis
   - Daily routine tracking
   - Elderly care (basic monitoring)

## ESPHome vs ESP-IDF for CSI Positioning

**Important**: ESPHome cannot perform CSI positioning - it only supports RSSI-based detection.

### Technology Comparison

| Feature | ESPHome (RSSI) | ESP-IDF (CSI) |
|---------|----------------|---------------|
| Technology | RSSI signal strength only | Full Channel State Information |
| Data points | 1 value per measurement | 52-256 values per measurement |
| Accuracy | 3-5 meters at best | 0.5-2 meters |
| For 800 sq ft | Need 12-20 devices | Need 6-10 devices |

### Why ESPHome Needs More Devices

#### RSSI Limitations
```
RSSI Signal Strength Only:
Strong ────────────────── Weak
 -30dBm                  -90dBm
  │                        │
  └── Single value ────────┘

CSI Frequency Response:
Amplitude + Phase across 52+ subcarriers
│││││││││││││││││││││││││││││││││
└── Rich multipath information ──┘
```

### Recommendation
- ❌ **Don't use ESPHome** for positioning - It lacks CSI support
- ✅ **Use ESP-IDF firmware** - 6-10 devices needed for 800 sq ft
- ✅ **Or use ESPHome** for presence only - 3-4 devices

## Dual-Purpose ESP32: CSI + Sensors

The ESP32 devices can serve double duty for both CSI positioning and sensor data collection!

### Hardware Capabilities
- **Dual-core processor**: Core 0 for WiFi/CSI, Core 1 for sensors
- **Multiple I2C/SPI buses**: Connect multiple sensors
- **Sufficient RAM**: 520KB SRAM handles both tasks
- **WiFi coexistence**: CSI collection doesn't block sensor reading

### Example Integration with BME680

```c
// In your main.c, add sensor task alongside CSI
void sensor_task(void *pvParameters) {
    bme680_init();
    
    while (1) {
        bme680_data_t sensor_data;
        bme680_read(&sensor_data);
        
        // Publish to MQTT for Home Assistant
        mqtt_publish_sensor_data(&sensor_data);
        
        vTaskDelay(pdMS_TO_TICKS(30000)); // Every 30 seconds
    }
}

void app_main(void) {
    // Existing CSI initialization
    csi_collector_init(&csi_config);
    
    // Add sensor task on Core 1
    xTaskCreatePinnedToCore(sensor_task, "sensor", 4096, 
                           NULL, 5, NULL, 1);
}
```

### MQTT Topics for Home Assistant

```
csi-device-01/data          # CSI positioning data
csi-device-01/status        # Device status
csi-device-01/sensor/temp   # Temperature from BME680
csi-device-01/sensor/humid  # Humidity
csi-device-01/sensor/pressure # Pressure
csi-device-01/sensor/gas    # Air quality
```

### Home Assistant Configuration

```yaml
# configuration.yaml
mqtt:
  sensor:
    # CSI Positioning
    - name: "Living Room Position X"
      state_topic: "csi-device-01/data"
      value_template: "{{ value_json.position_x }}"
      
    # BME680 Sensors
    - name: "Living Room Temperature"
      state_topic: "csi-device-01/sensor/temp"
      unit_of_measurement: "°C"
      device_class: temperature
      
    - name: "Living Room Humidity"
      state_topic: "csi-device-01/sensor/humid"
      unit_of_measurement: "%"
      device_class: humidity
      
    - name: "Living Room Air Quality"
      state_topic: "csi-device-01/sensor/gas"
      unit_of_measurement: "kΩ"
```

### Supported Sensors

#### Environmental Sensors
- **BME680/BME280**: Temp, humidity, pressure, air quality
- **SHT31/SHT40**: High-accuracy temp/humidity
- **SGP30/SGP40**: VOC and CO₂ equivalent

#### Motion/Presence
- **PIR sensors**: HC-SR501 for redundant presence
- **mmWave radar**: LD2410 for precise presence
- **Light sensors**: BH1750 for lux monitoring

#### Other Sensors
- **Door/window sensors**: Reed switches
- **Water leak detectors**: Simple GPIO
- **Smoke/gas detectors**: MQ series

### Performance Impact
- CSI collection: ~30% CPU on Core 0
- Sensor reading: ~5% CPU on Core 1
- **Result**: Plenty of headroom for both

### Benefits of Dual-Purpose Nodes

1. **Cost Efficiency**: One device, multiple functions
2. **Simplified Wiring**: Single power/network connection
3. **Correlated Data**: Position + environmental data
4. **Better Automation**: "If person in room AND temp > 25°C, turn on AC"
5. **Unified Management**: Single firmware update for all features

### Example: Enhanced Automation

```yaml
automation:
  - alias: "Smart Climate Control"
    trigger:
      - platform: mqtt
        topic: "csi-device-01/data"
    condition:
      - condition: template
        value_template: "{{ trigger.payload_json.presence == true }}"
      - condition: numeric_state
        entity_id: sensor.living_room_temperature
        above: 25
    action:
      - service: climate.set_temperature
        data:
          temperature: 22
```

## Placement Tips

1. **Corner Placement**: Place devices in room corners for best coverage
2. **Line of Sight**: Ensure devices can "see" into adjacent spaces
3. **Height**: Mount at 6-7 feet for optimal coverage
4. **Avoid Obstacles**: Keep away from large metal objects
5. **Overlap Coverage**: Ensure each location is covered by at least 3 devices

## Cost-Benefit Analysis

- **6 devices**: $150-180 - Basic coverage, good for testing
- **8 devices**: $200-240 - Recommended for reliable positioning
- **10 devices**: $250-300 - Premium accuracy and redundancy

## Key Factors for Device Count

1. **Coverage Area**: Larger areas need more devices
2. **Accuracy Requirements**: More devices = better accuracy
3. **Environment**: Walls and obstacles affect signal propagation
4. **Multi-floor**: Need devices on each floor for 3D positioning

## Upgrade Path

- Start with 3 devices for proof-of-concept
- Test zone detection and automation
- Add more devices incrementally:
  - 4th device: Improves accuracy significantly
  - 5th device: Enables basic positioning
  - 6+ devices: Full positioning capability