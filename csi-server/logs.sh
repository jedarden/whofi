#!/bin/bash

# CSI Server Logs Viewer Script

set -e

PROJECT_NAME="csi-server"
BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${BLUE}CSI Server - Log Viewer${NC}"
echo

# Available services
services=("mosquitto" "influxdb" "redis" "chrony" "backend" "frontend")

if [ $# -eq 0 ]; then
    echo -e "${GREEN}Available services:${NC}"
    for i in "${!services[@]}"; do
        echo "  $((i+1))) ${services[$i]}"
    done
    echo "  a) All services"
    echo "  f) Follow all logs"
    echo
    read -p "Select service (1-${#services[@]}, a, or f): " choice
else
    choice=$1
fi

case $choice in
    [1-6])
        service_index=$((choice-1))
        service="csi-${services[$service_index]}"
        echo -e "${GREEN}Showing logs for $service:${NC}"
        docker-compose -p $PROJECT_NAME logs --tail=100 -f $service
        ;;
    "a"|"A")
        echo -e "${GREEN}Showing logs for all services:${NC}"
        docker-compose -p $PROJECT_NAME logs --tail=50
        ;;
    "f"|"F")
        echo -e "${GREEN}Following logs for all services:${NC}"
        docker-compose -p $PROJECT_NAME logs -f
        ;;
    *)
        echo "Invalid selection"
        exit 1
        ;;
esac