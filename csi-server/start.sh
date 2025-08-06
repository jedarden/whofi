#!/bin/bash

# CSI Server Stack Startup Script
# This script initializes the complete Docker server environment

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
COMPOSE_FILE="docker-compose.yml"
PROJECT_NAME="csi-server"

echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}    CSI Positioning Server Startup       ${NC}"
echo -e "${BLUE}===========================================${NC}"

# Function to print status
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    print_error "Docker is not running. Please start Docker first."
    exit 1
fi

# Check if docker-compose is available
if ! command -v docker-compose &> /dev/null; then
    print_error "docker-compose is not installed. Please install docker-compose first."
    exit 1
fi

# Create necessary directories
print_status "Creating necessary directories..."
mkdir -p config/mosquitto
mkdir -p config/chrony
mkdir -p config/nginx/ssl
mkdir -p data/{mosquitto,influxdb,redis}

# Generate self-signed SSL certificates if they don't exist
if [ ! -f "config/nginx/ssl/cert.pem" ] || [ ! -f "config/nginx/ssl/key.pem" ]; then
    print_status "Generating self-signed SSL certificates..."
    openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
        -keyout config/nginx/ssl/key.pem \
        -out config/nginx/ssl/cert.pem \
        -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"
fi

# Create MQTT password file if it doesn't exist
if [ ! -f "config/mosquitto/passwd" ]; then
    print_status "Creating MQTT password file..."
    # This creates a basic password file - you should regenerate with real passwords
    echo "csi_user:\$7\$101\$Kx1234567890\$abcdefghijklmnopqrstuvwxyz" > config/mosquitto/passwd
    echo "admin:\$7\$101\$Ad1234567890\$abcdefghijklmnopqrstuvwxyz" >> config/mosquitto/passwd
    print_warning "Default MQTT passwords created. Please change them in production!"
fi

# Check if .env file exists
if [ ! -f "backend/.env" ]; then
    print_warning "No .env file found in backend/. Using environment defaults."
    print_status "Creating .env file from example..."
    cp backend/.env.example backend/.env
fi

# Stop existing containers
print_status "Stopping existing containers..."
docker-compose -p $PROJECT_NAME down --remove-orphans

# Pull latest images
print_status "Pulling latest Docker images..."
docker-compose -p $PROJECT_NAME pull

# Build custom images
print_status "Building custom Docker images..."
docker-compose -p $PROJECT_NAME build

# Start services
print_status "Starting CSI server services..."
docker-compose -p $PROJECT_NAME up -d

# Wait for services to be ready
print_status "Waiting for services to be ready..."
sleep 10

# Check service health
print_status "Checking service health..."

services=("csi-mosquitto" "csi-influxdb" "csi-redis" "csi-chrony" "csi-backend" "csi-frontend")
healthy_count=0

for service in "${services[@]}"; do
    if docker ps --format "table {{.Names}}\t{{.Status}}" | grep -q "$service.*Up"; then
        print_status "✓ $service is running"
        ((healthy_count++))
    else
        print_error "✗ $service is not running"
    fi
done

echo
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}         Service Status Summary           ${NC}"
echo -e "${BLUE}===========================================${NC}"
echo -e "Services running: ${GREEN}$healthy_count/${#services[@]}${NC}"

if [ $healthy_count -eq ${#services[@]} ]; then
    print_status "All services started successfully!"
    echo
    echo -e "${GREEN}Access the CSI system at:${NC}"
    echo -e "  Web Interface: ${BLUE}https://localhost${NC}"
    echo -e "  API Endpoint:  ${BLUE}https://localhost/api${NC}"
    echo -e "  WebSocket:     ${BLUE}wss://localhost/ws${NC}"
    echo -e "  MQTT Broker:   ${BLUE}localhost:1883${NC}"
    echo -e "  InfluxDB:      ${BLUE}http://localhost:8086${NC}"
    echo
    echo -e "${YELLOW}Default Credentials:${NC}"
    echo -e "  InfluxDB: admin / csipassword123"
    echo -e "  MQTT: csi_user / csi_password"
    echo
    print_warning "Please change default passwords before production use!"
else
    print_error "Some services failed to start. Check logs with:"
    echo "  docker-compose -p $PROJECT_NAME logs"
fi

# Show logs for failed services
for service in "${services[@]}"; do
    if ! docker ps --format "table {{.Names}}\t{{.Status}}" | grep -q "$service.*Up"; then
        print_error "Logs for $service:"
        docker-compose -p $PROJECT_NAME logs --tail=20 $service
    fi
done

echo
echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}     Management Commands                  ${NC}"
echo -e "${BLUE}===========================================${NC}"
echo "  View logs:     docker-compose -p $PROJECT_NAME logs -f"
echo "  Stop services: docker-compose -p $PROJECT_NAME down"
echo "  Restart:       ./start.sh"
echo "  Update:        docker-compose -p $PROJECT_NAME pull && ./start.sh"
echo

exit 0