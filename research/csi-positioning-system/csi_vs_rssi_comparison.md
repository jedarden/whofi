# CSI vs RSSI: Complete Technical Comparison

## Fundamental Difference

### RSSI (Received Signal Strength Indicator)
- **Single scalar value**: One number representing total signal power
- **Aggregated across all frequencies**: -45 dBm (example)
- **Time domain measurement**: Power level at receiver
- **Like measuring total brightness of a rainbow**

### CSI (Channel State Information)
- **Complex matrix**: Amplitude AND phase for each subcarrier
- **Frequency-specific data**: 52-256 values per packet
- **Frequency domain measurement**: How each frequency is affected
- **Like measuring each color of the rainbow separately**

## Visual Analogy

```
RSSI: "The room is 75% bright"
CSI:  "Red: 80%, Orange: 70%, Yellow: 85%, Green: 60%, Blue: 75%, Violet: 90%"
```

## Technical Specifications

### RSSI Data Structure
```c
// RSSI: Single value
int8_t rssi = -45;  // dBm, that's it!
```

### CSI Data Structure
```c
// CSI: Complex matrix
typedef struct {
    int8_t real[256];      // Real component for each subcarrier
    int8_t imaginary[256]; // Imaginary component
    uint8_t num_subcarriers; // 52, 56, 114, or 256
} csi_data_t;

// Each subcarrier has magnitude AND phase
// Magnitude = sqrt(real² + imaginary²)
// Phase = atan2(imaginary, real)
```

## Information Content Comparison

| Aspect | RSSI | CSI |
|--------|------|-----|
| **Data points per packet** | 1 | 52-256 |
| **Information type** | Power only | Power + Phase |
| **Frequency resolution** | None | Per subcarrier (312.5 kHz) |
| **Multipath detection** | No | Yes |
| **Direction finding** | No | Yes (with arrays) |
| **Motion sensitivity** | Low | High |
| **Data size** | 1 byte | 100-500 bytes |

## What Each Can Detect

### RSSI Capabilities
```python
# RSSI can detect:
- Presence (someone entered room): 3-5 dB change
- Rough distance: Every 6 dB = 2x distance
- Large movements: Walking causes variations
- Signal quality: Good/bad connection

# RSSI cannot detect:
- Fine motion (breathing, gestures)
- Direction of arrival
- Multiple people accurately
- Through-wall imaging
```

### CSI Capabilities
```python
# CSI can detect:
- Micro-movements: Breathing (mm-level chest motion)
- Gesture recognition: Hand movements
- People counting: Each person affects subcarriers differently
- Direction finding: Phase differences indicate angle
- Material properties: Different materials affect frequencies differently
- Doppler shift: Speed of movement

# CSI advantages:
- 30-50x more data per packet
- Frequency-selective fading visible
- Phase provides distance/angle info
- AI/ML can extract complex patterns
```

## Real-World Example: Person Walking

### RSSI View
```
Time  RSSI
0s    -45 dBm  [baseline]
1s    -46 dBm  [person enters]
2s    -48 dBm  [person walking]
3s    -47 dBm  [still walking]
4s    -49 dBm  [person center of room]
5s    -46 dBm  [person leaving]
```

### CSI View (Simplified - 4 subcarriers shown)
```
Time  Subcarrier 1    Subcarrier 2    Subcarrier 3    Subcarrier 4
0s    10∠0°          12∠0°           11∠0°           10∠0°     [baseline]
1s    9∠15°          13∠-20°         8∠45°           11∠10°    [person enters]
2s    7∠30°          14∠-40°         6∠90°           12∠25°    [person walking]
3s    5∠45°          15∠-60°         4∠135°          13∠40°    [still walking]
4s    4∠60°          13∠-80°         5∠180°          11∠55°    [person center]
5s    8∠30°          12∠-30°         9∠90°           10∠20°    [person leaving]

Note: Format is Magnitude∠Phase
```

## Practical Detection Differences

### Breathing Detection
```python
# RSSI Approach (Very Difficult)
rssi_samples = [-45, -45.1, -45, -44.9, -45, -45.1]  # 0.1 dB variations
# Almost impossible to distinguish from noise

# CSI Approach (Feasible)
# Subcarrier phases shift with chest movement
phase_shifts = [
    [0°, 2°, 4°, 2°, 0°, -2°],  # Subcarrier 1
    [0°, 5°, 10°, 5°, 0°, -5°], # Subcarrier 2 (more sensitive)
    [0°, 1°, 2°, 1°, 0°, -1°],  # Subcarrier 3
]
# Clear breathing pattern visible in phase data
```

### Multiple People Detection
```python
# RSSI: Just sees total signal drop
one_person = -48 dBm
two_people = -52 dBm  # Harder to distinguish from one large person

# CSI: Each person affects different subcarriers
one_person_csi = {
    'subcarriers_affected': [1, 5, 10, 15],  # Pattern A
    'phase_signature': 'smooth'
}
two_people_csi = {
    'subcarriers_affected': [1, 3, 5, 7, 10, 12, 15, 18],  # Patterns A + B
    'phase_signature': 'complex_interference'
}
```

## Processing Requirements

### RSSI Processing
```python
def process_rssi(rssi_value):
    # Simple threshold or averaging
    if rssi_value < baseline - 3:
        return "person_detected"
    return "no_person"
# CPU: ~0.001ms per sample
# Memory: ~100 bytes
```

### CSI Processing
```python
def process_csi(csi_matrix):
    # Complex signal processing
    amplitudes = compute_amplitudes(csi_matrix)  # FFT
    phases = compute_phases(csi_matrix)          # atan2
    features = extract_features(amplitudes, phases)  # Statistical
    result = ml_model.predict(features)          # Neural network
    return result
# CPU: ~10-100ms per sample
# Memory: ~10KB-1MB
```

## Accuracy Comparison

| Application | RSSI Accuracy | CSI Accuracy | Improvement |
|-------------|---------------|--------------|-------------|
| Presence Detection | 85% | 98% | 1.15x |
| Location (room-level) | 75% | 95% | 1.3x |
| Location (meter-level) | 3-5m | 0.5-2m | 3-5x |
| People Counting | ±1 person | ±0.2 people | 5x |
| Gesture Recognition | Not possible | 85-95% | ∞ |
| Breathing Detection | ~10% | 80-90% | 8-9x |
| Through-wall Detection | 20% | 60-70% | 3x |

## Implementation Complexity

### RSSI Implementation
```c
// Complete RSSI detection in 10 lines
void detect_person() {
    static int baseline = -45;
    int current_rssi = WiFi.RSSI();
    
    if (baseline - current_rssi > 3) {
        Serial.println("Person detected!");
    }
}
```

### CSI Implementation
```c
// CSI requires hundreds of lines
void process_csi_callback(wifi_csi_info_t *info) {
    // Extract CSI data
    parse_csi_data(info);
    
    // Compute amplitudes and phases
    compute_magnitude_phase();
    
    // Feature extraction
    extract_statistical_features();
    extract_frequency_features();
    extract_temporal_features();
    
    // Machine learning inference
    run_neural_network();
    
    // Post-processing
    apply_kalman_filter();
    update_tracking_state();
}
```

## When to Use Each

### Use RSSI When:
- Simple presence/absence detection
- Low power consumption required
- Limited processing capability
- Cost is primary concern
- 3-5m accuracy is sufficient

### Use CSI When:
- Sub-meter accuracy needed
- Gesture/activity recognition
- Multiple people tracking
- Research applications
- Breathing/vital signs monitoring
- Through-wall detection

## The Key Insight

**RSSI is like a black-and-white photo, CSI is like a high-resolution color video**

RSSI tells you "how much" total signal you received.
CSI tells you "how each frequency component was affected" by the environment.

This frequency-specific information is why CSI can detect subtle changes like breathing (different frequencies affected differently by chest movement) while RSSI cannot.