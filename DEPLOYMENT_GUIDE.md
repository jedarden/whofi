# CSI Positioning System - Complete Deployment Guide

## Overview

The CSI (Channel State Information) Positioning System is a production-ready indoor positioning solution using WiFi signals. This guide provides complete deployment instructions for both development and production environments.

## Architecture Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   ESP32 Nodes   │───▶│   CSI Server    │───▶│    Frontend     │
│  (CSI Capture)  │    │  (Processing)   │    │  (Dashboard)    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │              ┌─────────────────┐              │
         └──────────────│   MQTT Broker   │──────────────┘
                        └─────────────────┘
```

## System Requirements

### Hardware Requirements

#### ESP32 Nodes (Minimum 4 nodes recommended)
- **ESP32 DevKit**: ESP32-WROOM-32 or ESP32-S3
- **RAM**: 520KB SRAM minimum
- **Flash**: 4MB minimum (8MB recommended)
- **WiFi**: 802.11n with CSI support
- **Power**: 5V via USB or 3.3-5V DC input
- **Antennas**: Built-in PCB antenna or external via U.FL connector

#### Server Hardware
- **CPU**: Intel Core i5 or AMD Ryzen 5 (4+ cores)
- **RAM**: 8GB minimum (16GB recommended)
- **Storage**: 100GB SSD available space
- **Network**: Gigabit Ethernet
- **OS**: Linux (Ubuntu 20.04+ recommended), macOS, or Windows 10+

### Software Requirements

#### ESP32 Firmware Dependencies
- ESP-IDF v4.4 or later
- FreeRTOS (included with ESP-IDF)
- ESP32 CSI driver
- MQTT client library
- WiFi provisioning library

#### Server Dependencies
- Docker 20.10+
- Docker Compose v2.0+
- Node.js 16+ (for development)
- Python 3.8+ (for backend processing)

## Quick Start Deployment

### 1. Clone the Repository

```bash
git clone https://github.com/your-org/csi-positioning-system.git
cd csi-positioning-system
```

### 2. Environment Configuration

Create environment file:
```bash
cp .env.example .env
```

Edit `.env` with your configuration:
```env
# MQTT Configuration
MQTT_HOST=localhost
MQTT_PORT=1883
MQTT_USERNAME=csi_user
MQTT_PASSWORD=your_secure_password

# Database Configuration
POSTGRES_DB=csi_positioning
POSTGRES_USER=csi_user
POSTGRES_PASSWORD=your_db_password

# Server Configuration
CSI_SERVER_PORT=3000
FRONTEND_PORT=80
TIMEZONE=UTC

# Security
JWT_SECRET=your_jwt_secret_here
API_KEY=your_api_key_here

# OTA Configuration
OTA_SERVER_URL=https://your-domain.com/ota
OTA_CERTIFICATE_PATH=/etc/ssl/certs/ota_cert.pem
```

### 3. Deploy with Docker Compose

```bash
# Production deployment
docker-compose -f docker-compose.prod.yml up -d

# Development deployment
docker-compose up -d
```

### 4. Verify Deployment

Check all services are running:
```bash
docker-compose ps
```

Expected output:
```
NAME                    IMAGE                       STATUS
csi-server             csi-positioning/server      Up
csi-frontend           csi-positioning/frontend    Up
csi-mqtt               eclipse-mosquitto:2.0       Up
csi-database           postgres:14                  Up
csi-nginx              nginx:alpine                 Up
```

## ESP32 Node Setup

### 1. Firmware Compilation

#### Prerequisites
```bash
# Install ESP-IDF
cd ~/
git clone -b v4.4 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh

# Source the environment
. $HOME/esp-idf/export.sh
```

#### Configure and Build
```bash
cd whofi/csi-firmware

# Configure the project
idf.py menuconfig

# Build the firmware
idf.py build

# Flash to ESP32
idf.py -p /dev/ttyUSB0 flash monitor
```

### 2. Node Configuration

#### WiFi Configuration
Each node needs unique configuration in `main/app_config.h`:

```c
// Node-specific configuration
#define NODE_ID "esp32_001"  // Unique identifier
#define WIFI_SSID "YourNetworkSSID"
#define WIFI_PASS "YourNetworkPassword"

// MQTT Configuration
#define MQTT_BROKER_HOST "192.168.1.100"
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME "csi_user"
#define MQTT_PASSWORD "your_secure_password"

// CSI Configuration
#define CSI_SAMPLE_RATE 100  // Samples per second
#define CSI_SUBCARRIERS 64   // Number of subcarriers to capture
```

#### Physical Installation
1. **Positioning**: Place nodes at room corners or walls
2. **Height**: Mount 2-3 meters high for optimal coverage
3. **Spacing**: Minimum 3 meters apart, maximum 15 meters
4. **Power**: Use reliable power supplies or PoE adapters
5. **Orientation**: Keep antennas oriented consistently

### 3. Node Calibration

#### Automatic Calibration
```bash
# Access calibration interface
http://your-server:3000/calibration

# Follow the step-by-step calibration wizard:
# 1. Node Discovery - Verify all nodes are detected
# 2. Physical Position Setup - Mark node locations on map
# 3. Signal Baseline Collection - Capture reference signals
# 4. Reference Point Calibration - Collect data at known positions
# 5. Validation & Testing - Verify accuracy
# 6. Save Configuration - Store calibration data
```

#### Manual Calibration (Advanced)
```bash
# Connect to node via serial
idf.py -p /dev/ttyUSB0 monitor

# Enter calibration mode
calibration start

# Set node position (x, y coordinates in meters)
position set 0.0 0.0

# Collect baseline measurements
baseline collect 300

# Save calibration data
calibration save
```

## Production Deployment Configuration

### 1. Docker Compose Production Setup

Create `docker-compose.prod.yml`:
```yaml
version: '3.8'

services:
  csi-server:
    image: csi-positioning/server:latest
    restart: unless-stopped
    environment:
      - NODE_ENV=production
      - DATABASE_URL=postgresql://csi_user:${POSTGRES_PASSWORD}@database:5432/csi_positioning
      - MQTT_URL=mqtt://mqtt-broker:1883
      - REDIS_URL=redis://redis:6379
    depends_on:
      - database
      - mqtt-broker
      - redis
    networks:
      - csi-network
    volumes:
      - ./logs:/app/logs
      - ./config:/app/config

  csi-frontend:
    image: csi-positioning/frontend:latest
    restart: unless-stopped
    environment:
      - REACT_APP_API_URL=https://your-domain.com/api
      - REACT_APP_WS_URL=wss://your-domain.com/ws
    networks:
      - csi-network

  mqtt-broker:
    image: eclipse-mosquitto:2.0
    restart: unless-stopped
    ports:
      - "1883:1883"
      - "9001:9001"
    volumes:
      - ./mosquitto/config:/mosquitto/config
      - ./mosquitto/data:/mosquitto/data
      - ./mosquitto/log:/mosquitto/log
    networks:
      - csi-network

  database:
    image: postgres:14
    restart: unless-stopped
    environment:
      - POSTGRES_DB=csi_positioning
      - POSTGRES_USER=csi_user
      - POSTGRES_PASSWORD=${POSTGRES_PASSWORD}
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./backups:/backups
    networks:
      - csi-network

  redis:
    image: redis:7-alpine
    restart: unless-stopped
    command: redis-server --appendonly yes
    volumes:
      - redis_data:/data
    networks:
      - csi-network

  nginx:
    image: nginx:alpine
    restart: unless-stopped
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx/conf.d:/etc/nginx/conf.d
      - ./nginx/ssl:/etc/nginx/ssl
      - nginx_logs:/var/log/nginx
    depends_on:
      - csi-server
      - csi-frontend
    networks:
      - csi-network

volumes:
  postgres_data:
  redis_data:
  nginx_logs:

networks:
  csi-network:
    driver: bridge
```

### 2. Nginx Configuration

Create `nginx/conf.d/csi-positioning.conf`:
```nginx
upstream csi_backend {
    server csi-server:3000;
}

upstream csi_frontend {
    server csi-frontend:80;
}

# Redirect HTTP to HTTPS
server {
    listen 80;
    server_name your-domain.com;
    return 301 https://$server_name$request_uri;
}

# HTTPS Configuration
server {
    listen 443 ssl http2;
    server_name your-domain.com;

    # SSL Configuration
    ssl_certificate /etc/nginx/ssl/cert.pem;
    ssl_certificate_key /etc/nginx/ssl/key.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-RSA-AES256-GCM-SHA512:DHE-RSA-AES256-GCM-SHA512:ECDHE-RSA-AES256-GCM-SHA384;
    ssl_prefer_server_ciphers off;
    ssl_session_cache shared:SSL:10m;

    # Security Headers
    add_header X-Frame-Options DENY;
    add_header X-Content-Type-Options nosniff;
    add_header X-XSS-Protection "1; mode=block";
    add_header Strict-Transport-Security "max-age=63072000; includeSubDomains; preload";

    # Frontend
    location / {
        proxy_pass http://csi_frontend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # API Backend
    location /api/ {
        proxy_pass http://csi_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # WebSocket support
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }

    # OTA Update Endpoint
    location /ota/ {
        proxy_pass http://csi_backend/ota/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # Large file upload support
        client_max_body_size 50M;
        proxy_request_buffering off;
    }

    # Static assets caching
    location ~* \.(js|css|png|jpg|jpeg|gif|ico|svg|woff|woff2|ttf|eot)$ {
        proxy_pass http://csi_frontend;
        expires 1y;
        add_header Cache-Control "public, immutable";
    }
}
```

### 3. SSL Certificate Setup

#### Using Let's Encrypt (Recommended)
```bash
# Install Certbot
sudo apt install certbot python3-certbot-nginx

# Obtain certificate
sudo certbot --nginx -d your-domain.com

# Auto-renewal (add to crontab)
0 12 * * * /usr/bin/certbot renew --quiet
```

#### Using Self-Signed Certificate (Development)
```bash
# Generate self-signed certificate
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout nginx/ssl/key.pem \
    -out nginx/ssl/cert.pem \
    -subj "/C=US/ST=State/L=City/O=Organization/CN=your-domain.com"
```

## Monitoring and Maintenance

### 1. Health Monitoring

#### System Health Checks
```bash
# Check service status
docker-compose ps

# View logs
docker-compose logs -f csi-server
docker-compose logs -f csi-frontend

# Monitor resource usage
docker stats

# Check node connectivity
curl http://your-domain.com/api/health
```

#### Automated Monitoring Setup
Create `monitoring/docker-compose.yml`:
```yaml
version: '3.8'

services:
  prometheus:
    image: prom/prometheus:latest
    restart: unless-stopped
    ports:
      - "9090:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus_data:/prometheus

  grafana:
    image: grafana/grafana:latest
    restart: unless-stopped
    ports:
      - "3001:3000"
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=admin
    volumes:
      - grafana_data:/var/lib/grafana
      - ./grafana/dashboards:/etc/grafana/provisioning/dashboards

volumes:
  prometheus_data:
  grafana_data:
```

### 2. Backup and Recovery

#### Database Backup
```bash
# Create backup script
cat > backup_database.sh << 'EOF'
#!/bin/bash
BACKUP_DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="/backups"
DB_CONTAINER="csi-positioning_database_1"

# Create database backup
docker exec $DB_CONTAINER pg_dump -U csi_user csi_positioning > \
    $BACKUP_DIR/csi_db_backup_$BACKUP_DATE.sql

# Compress backup
gzip $BACKUP_DIR/csi_db_backup_$BACKUP_DATE.sql

# Remove backups older than 30 days
find $BACKUP_DIR -name "csi_db_backup_*.sql.gz" -mtime +30 -delete

echo "Database backup completed: csi_db_backup_$BACKUP_DATE.sql.gz"
EOF

chmod +x backup_database.sh

# Schedule daily backups (crontab)
0 2 * * * /path/to/backup_database.sh
```

#### Configuration Backup
```bash
# Backup configuration and calibration data
tar -czf config_backup_$(date +%Y%m%d).tar.gz \
    .env \
    nginx/conf.d/ \
    mosquitto/config/ \
    calibration_data/ \
    certificates/
```

#### Recovery Procedure
```bash
# Stop services
docker-compose down

# Restore database
gunzip -c csi_db_backup_YYYYMMDD_HHMMSS.sql.gz | \
    docker exec -i database psql -U csi_user csi_positioning

# Restore configuration
tar -xzf config_backup_YYYYMMDD.tar.gz

# Restart services
docker-compose up -d
```

### 3. Performance Optimization

#### Database Optimization
```sql
-- Connect to database
docker exec -it database psql -U csi_user csi_positioning

-- Create indexes for better performance
CREATE INDEX idx_positions_timestamp ON positions(timestamp);
CREATE INDEX idx_csi_data_node_id ON csi_data(node_id);
CREATE INDEX idx_csi_data_timestamp ON csi_data(timestamp);

-- Analyze query performance
EXPLAIN ANALYZE SELECT * FROM positions WHERE timestamp > NOW() - INTERVAL '1 hour';
```

#### Node Performance Tuning
```c
// In main/app_config.h
#define CSI_SAMPLE_RATE 50       // Reduce if CPU overloaded
#define CSI_BUFFER_SIZE 1024     // Increase for better throughput
#define MQTT_KEEPALIVE 30        // Adjust based on network conditions
#define WIFI_MAX_RETRY 10        // Increase for unreliable networks
```

## Security Hardening

### 1. Network Security

#### Firewall Configuration
```bash
# UFW firewall setup
sudo ufw default deny incoming
sudo ufw default allow outgoing

# Allow SSH
sudo ufw allow ssh

# Allow HTTP/HTTPS
sudo ufw allow 80
sudo ufw allow 443

# Allow MQTT (internal network only)
sudo ufw allow from 192.168.1.0/24 to any port 1883

sudo ufw enable
```

#### MQTT Security
Edit `mosquitto/config/mosquitto.conf`:
```conf
# Authentication
allow_anonymous false
password_file /mosquitto/config/passwd

# Access Control Lists
acl_file /mosquitto/config/acl.conf

# TLS Configuration
port 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/server.crt
keyfile /mosquitto/config/server.key
tls_version tlsv1.2

# Logging
log_type error
log_type warning  
log_type notice
log_type information
log_dest file /mosquitto/log/mosquitto.log
```

### 2. Application Security

#### Environment Variables Protection
```bash
# Set proper file permissions
chmod 600 .env

# Use Docker secrets in production
docker secret create mqtt_password mqtt_password.txt
```

#### API Rate Limiting
Add to server configuration:
```javascript
// Rate limiting middleware
const rateLimit = require('express-rate-limit');

const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100, // limit each IP to 100 requests per windowMs
  message: 'Too many requests from this IP'
});

app.use('/api/', limiter);
```

## Troubleshooting Guide

### Common Issues

#### ESP32 Node Issues

**Node Not Connecting to WiFi**
```bash
# Check serial output
idf.py -p /dev/ttyUSB0 monitor

# Verify WiFi credentials
# Reset WiFi configuration
nvs_flash_erase
```

**CSI Data Not Received**
```bash
# Check MQTT connection
mosquitto_sub -h localhost -t "csi/+/data"

# Verify node MQTT configuration
# Check firewall rules
```

**Poor Positioning Accuracy**
- Verify node placement (minimum 3 nodes required)
- Run calibration procedure
- Check for interference sources
- Ensure stable power supply

#### Server Issues

**Database Connection Failed**
```bash
# Check database container
docker-compose logs database

# Test connection
docker exec -it database psql -U csi_user csi_positioning

# Reset database if needed
docker-compose down -v
docker-compose up -d
```

**High Memory Usage**
```bash
# Monitor resource usage
docker stats

# Check for memory leaks
docker exec -it csi-server node --max-old-space-size=4096 server.js

# Clear old data
DELETE FROM csi_data WHERE timestamp < NOW() - INTERVAL '7 days';
```

**WebSocket Connection Issues**
- Check nginx WebSocket proxy configuration
- Verify CORS settings
- Test with direct server connection (bypass nginx)

### Log Analysis

#### Enable Debug Logging
```bash
# Server debug logs
DEBUG=csi:* docker-compose up

# ESP32 debug logs
idf.py menuconfig
# Component config → Log output → Default log verbosity → Debug
```

#### Log Locations
- Server logs: `/var/log/csi-server/`
- Nginx logs: `/var/log/nginx/`
- MQTT logs: `/var/log/mosquitto/`
- ESP32 logs: Serial monitor output

## Performance Benchmarks

### Expected Performance Metrics

#### Positioning Accuracy
- **Ideal conditions**: ±0.5 meters
- **Normal conditions**: ±1-2 meters  
- **Poor conditions**: ±3-5 meters

#### System Throughput
- **CSI samples/sec**: 100-1000 per node
- **Position updates/sec**: 1-10 per tracked device
- **Concurrent users**: 50-100 dashboard users

#### Resource Usage
- **ESP32 RAM**: 200-300KB
- **Server RAM**: 2-4GB
- **Database storage**: 1GB per month (4 nodes, 100Hz)

## Support and Updates

### Getting Help

- **Documentation**: Check this deployment guide and API documentation
- **Community**: Join our GitHub Discussions
- **Issues**: Report bugs on GitHub Issues
- **Email**: support@csi-positioning.com

### Update Procedure

#### Firmware Updates
```bash
# Build new firmware
cd csi-firmware
idf.py build

# OTA update (if configured)
curl -X POST http://your-domain.com/api/ota/update \
  -H "Authorization: Bearer YOUR_API_KEY" \
  -F "firmware=@build/csi-firmware.bin"

# Manual update
idf.py -p /dev/ttyUSB0 flash
```

#### Server Updates
```bash
# Pull latest images
docker-compose pull

# Update with zero downtime
docker-compose up -d --no-deps csi-server
docker-compose up -d --no-deps csi-frontend

# Verify update
curl http://your-domain.com/api/health
```

This completes the comprehensive deployment guide. The system is now production-ready with proper security, monitoring, and maintenance procedures.