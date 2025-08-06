# RSSI Human Detection: Physics and Principles

## Physical Phenomena

### 1. **Signal Attenuation (Primary Effect)**
When a human body is between a WiFi transmitter and receiver:
- Human body is ~60% water
- Water absorbs 2.4 GHz signals strongly
- Typical attenuation: 3-10 dB through human torso
- Creates a "shadow" in the WiFi signal

### 2. **Multipath Reflection**
Human bodies act as reflectors:
- WiFi signals bounce off human body
- Creates additional signal paths
- Causes constructive/destructive interference
- Results in RSSI fluctuations

### 3. **Fresnel Zone Obstruction**
- WiFi signals propagate in elliptical zones
- Human body blocks part of Fresnel zone
- Even partial obstruction affects signal strength
- More sensitive than line-of-sight blocking

## Detection Mechanisms

### Static Presence Detection
```
Empty Room: RSSI = -45 dBm (stable)
Person Present: RSSI = -52 dBm (7 dB drop)
Detection: Threshold-based presence
```

### Motion Detection
```
Person Walking: RSSI variance increases 5-10x
Person Still: RSSI variance minimal
Detection: Variance-based motion sensing
```

### Breathing Detection (Advanced)
```
Breathing Rate: 12-20 breaths/min (0.2-0.33 Hz)
Chest Movement: 1-3 cm
RSSI Variation: ±0.5-1 dB at breathing frequency
Detection: Frequency analysis of RSSI
```

## Scientific Measurements

### Human Body RF Properties at 2.4 GHz:
- Dielectric constant (εr): ~50-60
- Conductivity (σ): ~1.7 S/m
- Penetration depth: ~2-3 cm
- Reflection coefficient: ~0.5-0.7

### Typical RSSI Changes:
| Scenario | RSSI Change | Detection Confidence |
|----------|-------------|---------------------|
| Person walks between nodes | -5 to -15 dB | 95% |
| Person stands nearby | -2 to -5 dB | 70% |
| Person sits still | -1 to -3 dB | 50% |
| Multiple people | -10 to -20 dB | 90% |

## Detection Algorithms

### 1. Simple Threshold Detection
```python
def detect_presence(rssi_baseline, rssi_current, threshold=3):
    """Detect presence based on RSSI drop"""
    if rssi_baseline - rssi_current > threshold:
        return True
    return False
```

### 2. Variance-Based Motion Detection
```python
def detect_motion(rssi_window, threshold=2.0):
    """Detect motion based on RSSI variance"""
    variance = np.var(rssi_window)
    baseline_variance = 0.5  # Typical empty room
    if variance > baseline_variance * threshold:
        return True
    return False
```

### 3. Machine Learning Detection
```python
def ml_detection(rssi_features):
    """ML-based detection using multiple features"""
    features = [
        np.mean(rssi_features),
        np.var(rssi_features),
        np.max(rssi_features) - np.min(rssi_features),
        scipy.stats.kurtosis(rssi_features)
    ]
    return trained_model.predict([features])
```

## Real-World Performance

### Detection Reliability:
- **Walking person**: 90-95% detection rate
- **Standing person**: 70-85% detection rate
- **Sitting person**: 50-70% detection rate
- **Sleeping person**: 30-50% detection rate

### False Positive Sources:
- Large pets (dogs >20kg)
- Moving curtains/doors
- HVAC airflow
- Microwave ovens (2.4 GHz interference)
- Other WiFi devices

### Environmental Factors:
- Wall materials affect propagation
- Furniture creates multipath
- Room size impacts sensitivity
- Temperature/humidity minimal effect

## Limitations

### What RSSI Cannot Detect:
1. **Person identification** - Cannot distinguish individuals
2. **Exact position** - Only presence/absence in zones
3. **Posture/orientation** - Limited information
4. **Through walls** - Significant attenuation
5. **Small movements** - Finger typing, eye movement

### What RSSI Can Detect:
1. **Presence/absence** - Reliable room occupancy
2. **Motion** - Walking, large gestures
3. **Count estimation** - Approximate number of people
4. **Breathing** - With sensitive equipment
5. **Activity level** - Still vs. moving

## Comparison with Other Technologies

| Technology | Detection Principle | Range | Accuracy | Privacy |
|------------|-------------------|-------|----------|---------|
| RSSI | RF attenuation | 10-20m | 3-5m | High |
| PIR | Infrared heat | 5-10m | Zone | High |
| Camera | Visual | Line of sight | cm | Low |
| Radar | Doppler shift | 5-15m | 0.1-1m | High |
| CSI | RF channel state | 10-20m | 0.5-2m | High |

## Practical Implementation

### Minimum Requirements:
- 2+ WiFi devices (nodes)
- RSSI sampling rate: 5-10 Hz
- Baseline calibration: Empty room
- Detection threshold: 3-5 dB

### Enhanced Detection:
- 4+ nodes for better coverage
- Machine learning models
- Sensor fusion (PIR + RSSI)
- Temporal filtering

## Conclusion

RSSI detects living beings through:
1. **Physical obstruction** of radio waves
2. **Reflection and scattering** creating multipath
3. **Absorption** by water content in body
4. **Movement** causing signal variations

It's not "life detection" but rather "object detection" where humans happen to be large, water-filled, moving objects that significantly affect WiFi signals.