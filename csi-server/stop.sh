#!/bin/bash

# CSI Server Stack Stop Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PROJECT_NAME="csi-server"

echo -e "${BLUE}===========================================${NC}"
echo -e "${BLUE}    CSI Positioning Server Shutdown      ${NC}"
echo -e "${BLUE}===========================================${NC}"

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Stop and remove containers
print_status "Stopping CSI server services..."
docker-compose -p $PROJECT_NAME down --remove-orphans

# Remove unused volumes (optional)
read -p "Remove data volumes? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_status "Removing data volumes..."
    docker-compose -p $PROJECT_NAME down -v
    print_status "Data volumes removed."
else
    print_status "Data volumes preserved."
fi

# Clean up unused Docker images (optional)
read -p "Clean up unused Docker images? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    print_status "Cleaning up unused Docker images..."
    docker image prune -f
    print_status "Docker cleanup completed."
fi

print_status "CSI server services stopped successfully!"

echo
echo -e "${BLUE}To restart the services, run: ${GREEN}./start.sh${NC}"