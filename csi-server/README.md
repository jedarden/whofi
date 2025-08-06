# CSI Positioning Server

A comprehensive Docker-based server stack for WiFi CSI-based indoor positioning using ESP32 devices.

## 🏗️ Architecture

The CSI server stack consists of the following components:

- **MQTT Broker (Mosquitto)**: Handles real-time CSI data from ESP32 nodes
- **Time-series Database (InfluxDB)**: Stores historical CSI and position data
- **NTP Server (Chrony)**: Provides precise time synchronization
- **Backend API (Node.js)**: Processes CSI data and calculates positions
- **Frontend (React)**: Web dashboard for monitoring and configuration
- **Redis Cache**: High-speed data caching and session management
- **Nginx**: Web server and reverse proxy with SSL termination

## 🚀 Quick Start

### Prerequisites

- Docker and Docker Compose installed
- At least 4GB RAM available
- ESP32 devices with CSI firmware (see `/csi-firmware/` directory)

### Installation

1. **Security Setup (REQUIRED):**
   ```bash
   # Copy environment template
   cp .env.example .env
   
   # Generate secure passwords
   openssl rand -base64 32  # Use for each password in .env
   
   # Edit .env with your secure values
   nano .env
   ```

2. **Start the server stack:**
   ```bash
   ./start.sh
   ```

2. **Access the web interface:**
   - Open https://localhost in your browser
   - Accept the self-signed certificate warning

3. **Configure your ESP32 nodes:**
   - Set MQTT broker to your server IP: `your-server-ip:1883`
   - Use credentials from your `.env` file

### Default Access Points

| Service | URL | Credentials |
|---------|-----|-------------|
| Web Dashboard | https://localhost | - |
| API Endpoint | https://localhost/api | - |
| InfluxDB | http://localhost:8086 | Set in .env file |
| MQTT Broker | localhost:1883 | Set in .env file |

## 📊 Features

### Real-time Position Tracking
- **Trilateration Algorithm**: Calculate positions using RSSI from multiple nodes
- **WebSocket Updates**: Real-time position streaming to web clients
- **Interactive Map**: Visual position tracking with confidence indicators
- **Position History**: Trail visualization and historical data

### Node Management
- **Auto-discovery**: Automatically detect ESP32 nodes
- **Health Monitoring**: Monitor node status, signal strength, and uptime
- **Remote Configuration**: Update node settings via MQTT
- **Signal Analysis**: RSSI monitoring and signal quality assessment

### Data Analytics
- **Performance Metrics**: System accuracy and response time tracking
- **Signal Analysis**: RF environment monitoring and optimization
- **Position Statistics**: Movement patterns and accuracy analysis
- **System Health**: Resource usage and service monitoring

### Advanced Features
- **Time Synchronization**: High-precision timing via NTP server
- **Data Persistence**: Long-term storage in InfluxDB
- **Caching**: Redis-based performance optimization
- **SSL/TLS**: Secure web interface and API access
- **RESTful API**: Complete programmatic access to system functions

## 🔧 Configuration

### Environment Variables

Create a `.env` file in the `backend/` directory:

```env
# Server Configuration
NODE_ENV=production
PORT=3000
WS_PORT=8080

# MQTT Configuration
MQTT_BROKER_HOST=mosquitto
MQTT_USERNAME=csi_user
MQTT_PASSWORD=your_secure_password

# InfluxDB Configuration
INFLUXDB_URL=http://influxdb:8086
INFLUXDB_TOKEN=your_influxdb_token
INFLUXDB_ORG=csi-org
INFLUXDB_BUCKET=csi_data

# Redis Configuration
REDIS_HOST=redis
REDIS_PASSWORD=your_redis_password
```

### Node Positioning

Configure ESP32 node physical positions in the web interface:

1. Go to **Settings** → **Node Physical Positions**
2. Set X,Y coordinates for each node in meters
3. Ensure at least 3 nodes for trilateration

### RF Calibration

Optimize positioning accuracy:

- **Path Loss Exponent**: Adjust based on environment (2.0 for free space, 3.0+ for indoor)
- **Reference RSSI**: Calibrate based on known distance measurements
- **Frequency**: Match your WiFi channel frequency

## 📡 ESP32 Integration

### MQTT Topics

The system uses the following MQTT topic structure:

```
csi/{node_id}/data      # CSI measurements
csi/{node_id}/stats     # Node statistics
csi/{node_id}/config    # Configuration updates
csi/{node_id}/heartbeat # Health status
```

### Data Format

CSI data messages should follow this JSON format:

```json
{
  "timestamp": 1640995200000,
  "rssi": -45,
  "channel": 6,
  "rate": 54,
  "csi_data": [/* CSI amplitude array */]
}
```

### Sample ESP32 Code

```cpp
// Send CSI data via MQTT
void publishCSIData() {
  StaticJsonDocument<1024> doc;
  doc["timestamp"] = millis();
  doc["rssi"] = WiFi.RSSI();
  doc["channel"] = 6;
  doc["rate"] = 54;
  
  JsonArray csiArray = doc.createNestedArray("csi_data");
  for (int i = 0; i < csi_len; i++) {
    csiArray.add(csi_amplitude[i]);
  }
  
  String payload;
  serializeJson(doc, payload);
  
  String topic = "csi/" + String(WiFi.macAddress()) + "/data";
  mqtt.publish(topic.c_str(), payload.c_str());
}
```

## 🔍 API Reference

### REST Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/health` | System health status |
| GET | `/api/nodes` | List all nodes |
| GET | `/api/positions` | Position history |
| GET | `/api/positions/current` | Latest position |
| GET | `/api/stats` | System statistics |
| GET | `/api/config` | System configuration |
| PUT | `/api/config` | Update configuration |
| POST | `/api/nodes/{id}/config` | Configure specific node |

### WebSocket Events

| Event Type | Description |
|------------|-------------|
| `position_update` | New position calculated |
| `csi_data` | Raw CSI data received |
| `node_status` | Node status change |
| `system_alert` | System notifications |

## 🐛 Troubleshooting

### Common Issues

1. **Services won't start:**
   ```bash
   # Check logs
   docker-compose logs
   
   # Restart services
   ./stop.sh && ./start.sh
   ```

2. **No position data:**
   - Ensure at least 3 ESP32 nodes are online
   - Check MQTT connectivity
   - Verify node positions in settings

3. **Poor accuracy:**
   - Calibrate RF parameters in settings
   - Check for signal interference
   - Verify node positioning

4. **WebSocket connection issues:**
   - Check browser console for errors
   - Verify SSL certificate acceptance
   - Try http://localhost if HTTPS fails

### Performance Optimization

- **Resource Allocation**: Ensure adequate RAM and CPU
- **Database Tuning**: Configure InfluxDB retention policies
- **Network**: Use wired connections for ESP32 nodes when possible
- **Caching**: Monitor Redis memory usage

## 🔒 Security

### Production Deployment

1. **Change Default Passwords:**
   ```bash
   # Update MQTT passwords
   mosquitto_passwd -c config/mosquitto/passwd username
   
   # Update InfluxDB credentials
   # Use InfluxDB web interface
   ```

2. **SSL Certificates:**
   ```bash
   # Replace self-signed certificates with real ones
   cp your-cert.pem config/nginx/ssl/cert.pem
   cp your-key.pem config/nginx/ssl/key.pem
   ```

3. **Firewall Configuration:**
   - Expose only necessary ports (80, 443, 1883)
   - Consider VPN access for management

## 📈 Monitoring

### System Metrics

- **Position Accuracy**: Track via Analytics dashboard
- **Node Health**: Monitor in Nodes view
- **System Performance**: Check API `/api/stats` endpoint
- **Database Growth**: Monitor InfluxDB disk usage

### Alerting

The system provides WebSocket-based real-time alerts for:
- Node disconnections
- Positioning failures
- System errors
- Performance degradation

## 🛠️ Development

### Building from Source

```bash
# Backend
cd backend/
npm install
npm run dev

# Frontend
cd frontend/
npm install
npm start
```

### Custom Modifications

- **Position Algorithms**: Modify `backend/server.js` trilateration logic
- **UI Components**: Add React components in `frontend/src/components/`
- **MQTT Topics**: Extend message handling in backend MQTT client
- **Database Schema**: Add InfluxDB measurements as needed

## 📚 Additional Resources

- [ESP32 CSI Documentation](../esp32_csi/docs/)
- [Hardware Requirements](../esp32_csi/specs/Hardware_Requirements.md)
- [CSI Data Format](../esp32_csi/specs/CSI_Data_Format_Specification.md)
- [Performance Analysis](../performance/)

## 🤝 Support

For issues and questions:
1. Check the troubleshooting section
2. Review system logs: `docker-compose logs`
3. Check ESP32 firmware logs
4. Verify network connectivity and MQTT topics

## 📄 License

This project is part of the WhoFi positioning system. See the main LICENSE file for details.