#!/usr/bin/env python3
"""
WhoFi CSI Processing Server
Receives raw CSI data from ESPHome nodes and calculates positions
"""

import json
import asyncio
import logging
import numpy as np
from datetime import datetime
from collections import defaultdict, deque
import paho.mqtt.client as mqtt
from scipy.spatial import distance
from sklearn.cluster import DBSCAN
import yaml

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class CSIProcessor:
    def __init__(self, config_file='config.yaml'):
        with open(config_file, 'r') as f:
            self.config = yaml.safe_load(f)
        
        self.mqtt_client = mqtt.Client()
        self.nodes = {}  # Store node positions and data
        self.csi_buffer = defaultdict(lambda: deque(maxlen=100))
        self.positions = {}  # Calculated positions
        
        # Node positions (configure these based on your setup)
        self.node_positions = self.config['node_positions']
        
        # MQTT callbacks
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        
    def on_connect(self, client, userdata, flags, rc):
        logger.info(f"Connected to MQTT broker with result code {rc}")
        # Subscribe to CSI data from all nodes
        client.subscribe("whofi/csi/+/data")
        client.subscribe("whofi/csi/+/stats")
        
    def on_message(self, client, userdata, msg):
        try:
            topic_parts = msg.topic.split('/')
            if len(topic_parts) >= 4:
                node_mac = topic_parts[2]
                data_type = topic_parts[3]
                
                payload = json.loads(msg.payload.decode())
                
                if data_type == 'data':
                    self.process_csi_data(node_mac, payload)
                elif data_type == 'stats':
                    self.update_node_stats(node_mac, payload)
                    
        except Exception as e:
            logger.error(f"Error processing message: {e}")
    
    def process_csi_data(self, node_mac, data):
        """Process incoming CSI data"""
        # Store CSI data
        self.csi_buffer[node_mac].append({
            'timestamp': data['timestamp'],
            'rssi': data['rssi'],
            'csi_data': data.get('csi_data', []),
            'channel': data['channel'],
            'rate': data['rate']
        })
        
        # Calculate position if we have enough data
        if len(self.csi_buffer) >= 3:  # Need at least 3 nodes
            self.calculate_position()
    
    def calculate_position(self):
        """Calculate position using CSI data from multiple nodes"""
        try:
            # Simple RSSI-based trilateration as starting point
            # In production, you'd use actual CSI amplitude/phase data
            
            distances = {}
            for node_mac, buffer in self.csi_buffer.items():
                if node_mac in self.node_positions and len(buffer) > 0:
                    # Average recent RSSI values
                    recent_rssi = [d['rssi'] for d in list(buffer)[-10:]]
                    avg_rssi = np.mean(recent_rssi)
                    
                    # Convert RSSI to distance (simplified path loss model)
                    # d = 10^((P_tx - RSSI - 10*n*log10(f) + 30*n - 32.44) / (10*n))
                    # Simplified: d = 10^((A - RSSI) / (10 * n))
                    A = -30  # Reference RSSI at 1 meter
                    n = 2.0  # Path loss exponent
                    distance = 10 ** ((A - avg_rssi) / (10 * n))
                    distances[node_mac] = distance
            
            if len(distances) >= 3:
                # Trilateration
                position = self.trilaterate(distances)
                if position is not None:
                    self.publish_position(position)
                    
        except Exception as e:
            logger.error(f"Error calculating position: {e}")
    
    def trilaterate(self, distances):
        """Simple trilateration algorithm"""
        try:
            # Get first 3 nodes with distances
            nodes = list(distances.keys())[:3]
            
            # Node positions
            p1 = np.array(self.node_positions[nodes[0]])
            p2 = np.array(self.node_positions[nodes[1]])
            p3 = np.array(self.node_positions[nodes[2]])
            
            # Distances
            r1 = distances[nodes[0]]
            r2 = distances[nodes[1]]
            r3 = distances[nodes[2]]
            
            # Trilateration math
            A = 2*p2[0] - 2*p1[0]
            B = 2*p2[1] - 2*p1[1]
            C = r1**2 - r2**2 - p1[0]**2 + p2[0]**2 - p1[1]**2 + p2[1]**2
            D = 2*p3[0] - 2*p2[0]
            E = 2*p3[1] - 2*p2[1]
            F = r2**2 - r3**2 - p2[0]**2 + p3[0]**2 - p2[1]**2 + p3[1]**2
            
            x = (C*E - F*B) / (E*A - B*D)
            y = (C*D - A*F) / (B*D - A*E)
            
            return [x, y]
            
        except Exception as e:
            logger.error(f"Trilateration error: {e}")
            return None
    
    def publish_position(self, position):
        """Publish calculated position to Home Assistant"""
        payload = {
            'x': round(position[0], 2),
            'y': round(position[1], 2),
            'timestamp': datetime.now().isoformat(),
            'confidence': 0.8,  # Add confidence scoring based on CSI quality
            'source': 'csi_trilateration'
        }
        
        # Publish to MQTT for Home Assistant
        self.mqtt_client.publish(
            'homeassistant/sensor/whofi_position/state',
            json.dumps(payload)
        )
        
        # Also publish as device_tracker for Home Assistant
        tracker_payload = {
            'latitude': position[1] / 100.0,  # Convert to lat/long
            'longitude': position[0] / 100.0,
            'gps_accuracy': 2.0,
            'source_type': 'router'
        }
        
        self.mqtt_client.publish(
            'homeassistant/device_tracker/whofi_tracker/attributes',
            json.dumps(tracker_payload)
        )
        
        logger.info(f"Published position: {position}")
    
    def update_node_stats(self, node_mac, stats):
        """Update node statistics"""
        self.nodes[node_mac] = {
            'packets': stats.get('packets', 0),
            'enabled': stats.get('enabled', False),
            'uptime': stats.get('uptime', 0),
            'last_seen': datetime.now()
        }
    
    def run(self):
        """Main run loop"""
        # Connect to MQTT
        self.mqtt_client.username_pw_set(
            self.config['mqtt']['username'],
            self.config['mqtt']['password']
        )
        self.mqtt_client.connect(
            self.config['mqtt']['broker'],
            self.config['mqtt']['port'],
            60
        )
        
        # Start MQTT loop
        self.mqtt_client.loop_forever()

if __name__ == "__main__":
    processor = CSIProcessor()
    processor.run()