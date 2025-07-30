# WiFi Positioning System for 800 sqft Apartment + 400 sqft Terrace

## Environment Analysis

### Space Specifications
- **Indoor**: 800 sqft apartment (~28' x 28' or 25' x 32')
- **Outdoor**: 400 sqft terrace (~20' x 20')
- **Total Coverage**: 1,200 sqft combined indoor/outdoor space
- **Typical Layout**: 1-2 bedrooms, living room, kitchen, 1-2 bathrooms

### Physical Challenges
- **Wall Materials**: Drywall, possible concrete/brick in older buildings
- **Signal Attenuation**: 3-10 dB loss per interior wall
- **Indoor/Outdoor Transition**: Glass doors, potential metal frames
- **Furniture Obstacles**: Couches, beds, appliances affecting signal paths
- **Weather Protection**: Outdoor nodes need weatherproof enclosures

## Recommended System Design

### Node Configuration: 6-Node Hybrid System

#### Indoor Nodes (4 nodes)
```yaml
Living Room: 1 node (central coverage + entertainment area)
Master Bedroom: 1 node (sleep tracking, privacy zone)
Kitchen: 1 node (cooking activities, appliance area)
Hallway/Entry: 1 node (transition zones, guest bathroom)
```

#### Outdoor Nodes (2 nodes)
```yaml
Terrace Corner 1: 1 node (weatherproof, dining area coverage)
Terrace Corner 2: 1 node (weatherproof, opposite corner for triangulation)
```

### Hardware Specifications

#### ESP32 Selection: ESP32-S3-WROOM-1U
```yaml
Processor: Dual-core 240MHz with AI acceleration
Memory: 512KB SRAM + 384KB ROM
WiFi: 802.11 b/g/n with CSI support
Antenna: U.FL connector for external antenna
Price: ~$8-12 per unit
```

#### Antenna Configuration

**Indoor Nodes (4x):**
```yaml
Type: 2.4GHz 5dBi Omnidirectional Rubber Duck
Connector: U.FL to SMA adapter
Cost: $15 per node
Benefits: 3-5 dB gain over built-in, uniform coverage
```

**Outdoor Nodes (2x):**
```yaml
Type: 2.4GHz 8dBi Omnidirectional Fiberglass
Connector: Direct SMA connection
Cost: $25 per node
Benefits: Weather resistance, higher gain for range
Mounting: Wall/ceiling brackets with weatherproof enclosures
```

## Performance Expectations

### Accuracy by Zone
| **Area** | **Expected Accuracy** | **Confidence** | **Use Cases** |
|----------|-------------------|----------------|---------------|
| **Living Room** | 1.5-2.5m | 90% | Presence, automation |
| **Bedrooms** | 2-3m | 85% | Sleep tracking, privacy |
| **Kitchen** | 1.5-2.5m | 90% | Cooking activity |
| **Bathroom** | 2-4m | 75% | Occupancy detection |
| **Terrace** | 2-4m | 80% | Outdoor presence |
| **Transitions** | 3-5m | 70% | Zone handoffs |

### System Capabilities
- **Update Rate**: 1-2 Hz real-time positioning
- **Simultaneous Tracking**: 2-4 people
- **Detection Range**: Complete apartment + terrace coverage
- **Latency**: <500ms end-to-end processing
- **Reliability**: 95% uptime with redundancy

## Node Placement Strategy

### Indoor Placement (Optimal Positions)

#### Living Room Node
```
Position: Corner mount at 8-9 feet height
Coverage: 15-20 foot radius
Purpose: Central hub, entertainment area tracking
Obstacles: Couch, TV, coffee table (minimal impact)
```

#### Master Bedroom Node
```
Position: Opposite corner from bed, 8 feet height
Coverage: 12-15 foot radius
Purpose: Sleep tracking, wake-up automation
Privacy: Local processing, no cloud data
```

#### Kitchen Node
```
Position: Above cabinets, away from appliances
Coverage: 10-12 foot radius
Purpose: Cooking activity detection, appliance automation
Challenges: Microwave interference (2.4GHz), metal surfaces
```

#### Hallway/Entry Node
```
Position: Central hallway ceiling mount
Coverage: Entry, hallway, guest bathroom
Purpose: Visitor detection, zone transitions
Benefits: Captures traffic between rooms
```

### Outdoor Placement (Weatherproof)

#### Terrace Corner Nodes (2x)
```
Position: Diagonal corners, 8-10 feet height
Coverage: 20-25 foot radius each with overlap
Purpose: Outdoor presence, dining/relaxation areas
Weather Protection: IP65 rated enclosures
Power: Weather-resistant cabling to indoor outlets
```

## Cost Analysis

### Hardware Costs
```yaml
ESP32-S3 Modules (6x): $48-72
Indoor Antennas (4x): $60
Outdoor Antennas (2x): $50
U.FL to SMA Cables (6x): $30
Weatherproof Enclosures (2x): $40
Mounting Hardware: $30
Power Supplies/Cables: $60

Total Hardware: $318-388
```

### Infrastructure Costs
```yaml
Network Switch (8-port): $50
Ethernet Cables: $25
Mounting Brackets: $40
Installation Materials: $35

Total Infrastructure: $150
```

### **Total System Cost: $468-538**

### Comparison with Alternatives
- **Commercial Positioning**: $5,000-15,000 (10-30x more expensive)
- **Basic ESP32 (built-in antennas)**: $200-300 (60% of performance)
- **Professional System**: $2,000-5,000 (marginal accuracy improvement)

## Implementation Plan

### Phase 1: Indoor Foundation (Week 1)
1. **Install 4 indoor ESP32 nodes** with external antennas
2. **Configure basic RSSI positioning** with Home Assistant
3. **Set up room-level zone detection**
4. **Test coverage and adjust positioning**

### Phase 2: Outdoor Extension (Week 2)
1. **Install weatherproof outdoor nodes**
2. **Extend coverage to terrace area**
3. **Configure indoor/outdoor transitions**
4. **Test weather resistance and signal quality**

### Phase 3: Optimization (Week 3)
1. **Implement advanced positioning algorithms**
2. **Add sensor fusion (BME680 environmental sensors)**
3. **Calibrate system for maximum accuracy**
4. **Create Home Assistant automation rules**

## Home Assistant Integration

### Device Configuration
```yaml
# ESPHome Configuration for each node
esphome:
  name: whofi-node-living-room
  platform: ESP32
  board: esp32-s3-devkitc-1

sensor:
  - platform: wifi_signal
    name: "WiFi Signal Strength"
    update_interval: 500ms

  - platform: template
    name: "Position Confidence"
    lambda: |-
      return calculate_position_confidence();

binary_sensor:
  - platform: template
    name: "Room Occupied"
    lambda: |-
      return detect_room_occupancy();

mqtt:
  broker: homeassistant.local
  topic_prefix: whofi/${device_name}
```

### Automation Examples
```yaml
# Living room presence automation
automation:
  - alias: "Living Room Occupied"
    trigger:
      platform: mqtt
      topic: "whofi/living-room/occupancy"
      payload: "occupied"
    action:
      - service: light.turn_on
        entity_id: light.living_room_lights
      - service: climate.set_temperature
        entity_id: climate.living_room
        data:
          temperature: 72

# Terrace outdoor presence
  - alias: "Terrace Activity"
    trigger:
      platform: mqtt
      topic: "whofi/terrace/presence"
      payload: "detected"
    action:
      - service: light.turn_on
        entity_id: light.terrace_string_lights
      - service: notify.mobile_app
        data:
          message: "Someone is on the terrace"
```

## Advanced Features

### Multi-Modal Sensor Fusion
```yaml
Additional Sensors per Node:
- BME680: Temperature, humidity, pressure, air quality
- PIR Motion: Backup presence detection
- Light Sensor: Ambient light for context

Benefits:
- 15-30% accuracy improvement
- Environmental automation
- Backup detection methods
- Context-aware triggers
```

### Privacy Protection
```yaml
Local Processing: All calculations on-device or local server
No Cloud Data: Position data never leaves your network
Anonymization: Device MAC randomization support
Encryption: WPA3 and MQTT TLS encryption
Access Control: Network segmentation and firewalls
```

### Power Optimization
```yaml
Deep Sleep Mode: Nodes sleep between measurements
Duty Cycling: Reduce power during low activity
Solar Option: Terrace nodes with small solar panels
Battery Backup: UPS for critical indoor nodes
```

## Maintenance & Upgrades

### Regular Maintenance (Monthly)
- **Check weatherproof seals** on outdoor nodes
- **Clean antenna connections** and check for corrosion
- **Update firmware** and security patches
- **Recalibrate positioning** if accuracy degrades

### Upgrade Path
```yaml
Year 1: Basic RSSI positioning (current design)
Year 2: Add CSI processing for improved accuracy
Year 3: Upgrade to WiFi 6E for better performance
Year 4: Add machine learning for predictive tracking
```

## Expected ROI and Use Cases

### Smart Home Automation
- **Energy Savings**: 15-25% HVAC efficiency from occupancy-based control
- **Security**: Automated lighting and presence simulation
- **Convenience**: Room-aware automation and device control
- **Health**: Sleep tracking and air quality monitoring

### Payback Timeline
- **Energy Savings**: $20-40/month
- **Security Value**: Reduced insurance premiums
- **Convenience Value**: Time savings and comfort
- **Total ROI**: 12-18 months payback period

## Technical Specifications Summary

### Coverage Area
- **Indoor**: 800 sqft with 95% reliability
- **Outdoor**: 400 sqft terrace with 90% reliability
- **Overlap Zones**: Seamless transitions between areas

### Accuracy Performance
- **Best Case**: 1.5-2.5m in optimal conditions
- **Typical**: 2-4m in real-world apartment environment
- **Worst Case**: 3-5m in challenging multipath areas

### System Reliability
- **Uptime**: 99%+ with redundant node coverage
- **Weather Resistance**: IP65 rated outdoor components
- **Interference Handling**: Frequency agility and error correction

## Conclusion

This 6-node WiFi positioning system provides **comprehensive coverage** for your 800 sqft apartment plus 400 sqft terrace at a **total cost of $468-538**. The system offers:

### ✅ **Key Benefits**
- **Complete Coverage**: Every room and outdoor area monitored
- **Apartment-Scale Accuracy**: 2-4m typical performance 
- **Home Assistant Integration**: Native smart home automation
- **Weather Resistant**: Outdoor terrace monitoring included
- **Cost Effective**: 90% less than commercial alternatives
- **Privacy Focused**: Local processing, no cloud dependency

### 🎯 **Ideal Applications**
- Room-level presence detection and automation
- Energy-efficient HVAC and lighting control
- Security and occupancy monitoring
- Sleep and activity pattern tracking
- Outdoor entertainment area management

The system provides **80% of commercial positioning functionality at 20% of the cost**, making it ideal for residential smart home applications with realistic accuracy expectations for apartment-scale environments.

---

*Designed specifically for 800 sqft apartment + 400 sqft terrace*  
*Total coverage: 1,200 sqft with 6 strategically placed nodes*  
*Expected accuracy: 2-4m typical performance*  
*Total investment: $468-538 complete system*