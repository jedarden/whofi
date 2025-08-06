# Remote Configuration Guide for CSI Positioning System

## Overview

The CSI positioning system supports secure remote configuration and management of ESP32 nodes through a REST API. This guide covers the security implementation and usage.

## Security Features

### API Key Authentication

Each ESP32 node generates a unique API key on first boot that is stored in NVS (Non-Volatile Storage). This key is required for all configuration changes and sensitive operations.

**Key Features:**
- 32-character random API key
- Stored securely in NVS flash
- Displayed only once in device logs on first boot
- Can be reset via physical access or factory reset

### Authentication Methods

1. **Direct API Key** (ESPHome-style)
   ```
   X-API-Key: your-32-character-api-key
   ```

2. **Bearer Token** (Time-limited)
   ```
   Authorization: Bearer your-temporary-token
   ```

3. **Basic Auth** (Web UI only)
   ```
   Authorization: Basic base64(username:password)
   ```

## ESP32 Node API Endpoints

### Authentication

**POST /api/auth**
```json
{
  "api_key": "your-32-character-api-key"
}
```

Response:
```json
{
  "token": "temporary-bearer-token",
  "expires_in": 3600
}
```

### Configuration Management

**GET /api/config**
- Requires: API key or valid token
- Returns current node configuration

**POST /api/config**
- Requires: API key or valid token
- Updates node configuration

Example request:
```json
{
  "node": {
    "name": "Living Room Sensor",
    "position": {
      "x": 0,
      "y": 0,
      "z": 2.5
    }
  },
  "csi": {
    "sample_rate": 50,
    "buffer_size": 2048,
    "filter_enabled": true,
    "filter_threshold": 0.7
  },
  "mqtt": {
    "enabled": true,
    "broker_url": "mqtt://192.168.1.100",
    "port": 1883,
    "topic_prefix": "csi/livingroom"
  }
}
```

### Remote Commands

**POST /api/command**
- Requires: API key or valid token

Supported commands:
- `restart` - Restart the device
- `start_csi` - Start CSI data collection
- `stop_csi` - Stop CSI data collection
- `calibrate` - Enter calibration mode
- `factory_reset` - Reset to factory defaults

Example:
```json
{
  "command": "restart"
}
```

### OTA Updates

**GET /api/ota/status**
- Returns current firmware version and update status

**POST /api/ota/update**
- Requires: API key or valid token
- Initiates OTA update

Example:
```json
{
  "url": "https://your-server.com/firmware/csi-v2.0.0.bin",
  "version": "2.0.0"
}
```

## Docker Server Integration

### Node Client Service

The Docker server includes a NodeClient service that manages all ESP32 nodes:

```javascript
import nodeClient from './services/nodeClient.js';

// Register a node with its API key
nodeClient.registerNode('esp32-01', '192.168.1.50', 'api-key-from-device');

// Update configuration
await nodeClient.updateConfig('esp32-01', {
  csi: {
    sample_rate: 100
  }
});

// Batch update all nodes
await nodeClient.updateAllCsiConfig({
  filter_enabled: true,
  filter_threshold: 0.8
});

// Trigger OTA update
await nodeClient.triggerOtaUpdate('esp32-01', 
  'https://server.com/firmware.bin', 
  '2.0.0'
);
```

### Web Interface

The web dashboard includes a Node Management interface:

1. **Node List** - Shows all registered nodes with status
2. **Configuration Editor** - Visual configuration interface
3. **API Key Management** - Securely store API keys
4. **OTA Updates** - Deploy firmware updates
5. **Batch Operations** - Update multiple nodes at once

## Setup Instructions

### 1. Initial Node Setup

When an ESP32 node boots for the first time:

```
I (1234) api_auth: Generated new API key: AbCdEfGhIjKlMnOpQrStUvWxYz012345
I (1235) api_auth: API authentication initialized (required: yes)
```

**Important**: Save this API key immediately! It won't be shown again.

### 2. Register Node in Docker Server

Add the node to the server configuration:

```javascript
// In server startup or via API
nodeClient.registerNode('livingroom-01', '192.168.1.50', 'AbCdEfGhIjKlMnOpQrStUvWxYz012345');
```

### 3. Configure via Web Interface

1. Navigate to Node Management in the web dashboard
2. Click the security icon next to the node
3. Enter the API key
4. Save the configuration

### 4. Test Configuration

```bash
# Test with curl
curl -H "X-API-Key: AbCdEfGhIjKlMnOpQrStUvWxYz012345" \
     http://192.168.1.50/api/config

# Update configuration
curl -X POST \
     -H "X-API-Key: AbCdEfGhIjKlMnOpQrStUvWxYz012345" \
     -H "Content-Type: application/json" \
     -d '{"csi": {"sample_rate": 50}}' \
     http://192.168.1.50/api/config
```

## Security Best Practices

1. **API Key Storage**
   - Never commit API keys to version control
   - Store keys in environment variables or secure vaults
   - Use different keys for each node

2. **Network Security**
   - Place ESP32 nodes on isolated IoT VLAN
   - Use firewall rules to restrict access
   - Enable HTTPS if exposing to internet

3. **OTA Security**
   - Always use HTTPS for firmware URLs
   - Verify firmware signatures
   - Test updates on single node first

4. **Access Control**
   - Regularly rotate API keys
   - Monitor failed authentication attempts
   - Disable authentication only for testing

## Troubleshooting

### Cannot Connect to Node
1. Verify node IP address is correct
2. Check if node is online (ping test)
3. Ensure API key is correct
4. Check firewall rules

### Authentication Failures
1. Verify API key matches device
2. Check for typos or extra spaces
3. Try factory reset if key is lost
4. Check server logs for details

### Configuration Not Applying
1. Some settings require restart
2. Check response for `restart_required` flag
3. Verify configuration format is correct
4. Check node logs via serial console

### OTA Update Failures
1. Ensure firmware URL is accessible from ESP32
2. Verify enough free space for update
3. Check if API key is provided in header
4. Monitor serial output during update

## API Examples

### Python Client Example

```python
import requests

class CSINodeClient:
    def __init__(self, node_ip, api_key):
        self.base_url = f"http://{node_ip}"
        self.headers = {"X-API-Key": api_key}
    
    def get_config(self):
        return requests.get(f"{self.base_url}/api/config", 
                          headers=self.headers).json()
    
    def update_config(self, config):
        return requests.post(f"{self.base_url}/api/config", 
                           headers=self.headers, 
                           json=config).json()
    
    def send_command(self, command):
        return requests.post(f"{self.base_url}/api/command", 
                           headers=self.headers, 
                           json={"command": command}).json()

# Usage
client = CSINodeClient("192.168.1.50", "your-api-key")
config = client.get_config()
print(f"Node name: {config['node']['name']}")
```

### Home Assistant Integration

```yaml
# configuration.yaml
rest_command:
  csi_node_restart:
    url: "http://192.168.1.50/api/command"
    method: POST
    headers:
      X-API-Key: "your-api-key"
    payload: '{"command": "restart"}'
    
  csi_node_update_config:
    url: "http://192.168.1.50/api/config"
    method: POST
    headers:
      X-API-Key: "your-api-key"
    payload: >
      {
        "csi": {
          "sample_rate": {{ sample_rate }},
          "filter_enabled": {{ filter_enabled }}
        }
      }

sensor:
  - platform: rest
    name: "CSI Node Status"
    resource: "http://192.168.1.50/api/status"
    headers:
      X-API-Key: "your-api-key"
    value_template: "{{ value_json.csi.running }}"
    json_attributes:
      - system
      - csi
      - wifi
```

## Summary

The remote configuration system provides:

- **Secure API** - API key authentication protects all sensitive operations
- **Flexible Updates** - Change any setting without physical access
- **Batch Operations** - Update multiple nodes simultaneously
- **OTA Updates** - Deploy firmware updates remotely
- **Integration Ready** - Works with Home Assistant, custom scripts, or web UI

This enables complete remote management of your CSI positioning system while maintaining security and reliability.