/**
 * @file csiReceiver.js
 * @brief Simple CSI data receiver that stores directly to SQLite
 */

import csiDatabase from './csiDatabase.js';
import { EventEmitter } from 'events';
import logger from '../utils/logger.js';

class CSIReceiver extends EventEmitter {
    constructor() {
        super();
        this.processingQueue = [];
        this.isProcessing = false;
    }

    /**
     * Handle incoming CSI data from ESP32 node
     */
    async handleCSIData(nodeId, csiData) {
        try {
            // Parse CSI data if it's a string
            const data = typeof csiData === 'string' ? JSON.parse(csiData) : csiData;
            
            // Add node ID and timestamp if not present
            data.node_id = nodeId;
            data.timestamp = data.timestamp || Date.now();

            // Store raw CSI data
            await csiDatabase.insertCSIData(data);

            // Update node last seen
            await csiDatabase.updateNodeStatus(nodeId, true);

            // Emit for real-time processing
            this.emit('csi-data', {
                nodeId,
                timestamp: data.timestamp,
                mac: data.mac_address,
                rssi: data.rssi,
                amplitude: data.amplitude_data,
                phase: data.phase_data
            });

            // Queue for position calculation
            this.queueForPositioning(data);

            logger.debug(`CSI data received from ${nodeId}: RSSI=${data.rssi}`);
        } catch (error) {
            logger.error(`Failed to process CSI data from ${nodeId}:`, error);
        }
    }

    /**
     * Queue CSI data for position calculation
     */
    queueForPositioning(data) {
        this.processingQueue.push(data);
        
        // Process queue if not already processing
        if (!this.isProcessing) {
            this.processPositioningQueue();
        }
    }

    /**
     * Process positioning queue
     */
    async processPositioningQueue() {
        if (this.processingQueue.length === 0) {
            this.isProcessing = false;
            return;
        }

        this.isProcessing = true;

        try {
            // Group by MAC address for batch processing
            const groupedData = this.groupByMac(this.processingQueue);
            this.processingQueue = [];

            for (const [mac, dataPoints] of Object.entries(groupedData)) {
                await this.calculatePosition(mac, dataPoints);
            }
        } catch (error) {
            logger.error('Position calculation error:', error);
        }

        // Continue processing if more data
        setTimeout(() => this.processPositioningQueue(), 100);
    }

    /**
     * Group CSI data by MAC address
     */
    groupByMac(dataArray) {
        return dataArray.reduce((acc, data) => {
            const mac = data.mac_address;
            if (!acc[mac]) acc[mac] = [];
            acc[mac].push(data);
            return acc;
        }, {});
    }

    /**
     * Calculate position from CSI data
     */
    async calculatePosition(targetMac, dataPoints) {
        try {
            // Get recent CSI data from multiple nodes
            const csiData = await csiDatabase.getCSIDataForPositioning(targetMac, 2000);
            
            if (csiData.length < 3) {
                // Not enough nodes for positioning
                return;
            }

            // Get node positions
            const nodePositions = {};
            for (const data of csiData) {
                const nodeInfo = await csiDatabase.getNodeInfo(data.node_id);
                if (nodeInfo) {
                    nodePositions[data.node_id] = {
                        x: nodeInfo.position_x,
                        y: nodeInfo.position_y,
                        z: nodeInfo.position_z
                    };
                }
            }

            // Simple trilateration based on RSSI
            const position = this.trilateration(csiData, nodePositions);
            
            if (position) {
                // Save calculated position
                await csiDatabase.saveCalculatedPosition({
                    timestamp: Date.now(),
                    target_mac: targetMac,
                    x: position.x,
                    y: position.y,
                    z: position.z || 1.0,
                    confidence: position.confidence,
                    algorithm: 'rssi-trilateration',
                    node_count: csiData.length,
                    processing_time_ms: position.processingTime
                });

                // Emit position update
                this.emit('position-update', {
                    mac: targetMac,
                    position,
                    timestamp: Date.now()
                });
            }
        } catch (error) {
            logger.error(`Failed to calculate position for ${targetMac}:`, error);
        }
    }

    /**
     * Simple trilateration algorithm
     */
    trilateration(csiData, nodePositions) {
        const startTime = Date.now();
        
        try {
            // Convert RSSI to distance estimates
            const measurements = csiData.map(data => {
                const nodePos = nodePositions[data.node_id];
                if (!nodePos) return null;

                // Simple path loss model: RSSI = -10 * n * log10(d) + A
                // Assuming n=2 (free space), A=-30 (1m reference)
                const distance = Math.pow(10, (data.avg_rssi + 30) / -20);

                return {
                    x: nodePos.x,
                    y: nodePos.y,
                    distance,
                    weight: 1 / Math.pow(distance, 2) // Weight by inverse square
                };
            }).filter(m => m !== null);

            if (measurements.length < 3) {
                return null;
            }

            // Weighted centroid as simple approximation
            let sumX = 0, sumY = 0, sumWeight = 0;
            
            for (const m of measurements) {
                sumX += m.x * m.weight;
                sumY += m.y * m.weight;
                sumWeight += m.weight;
            }

            const position = {
                x: sumX / sumWeight,
                y: sumY / sumWeight,
                z: 1.0, // Default height
                confidence: this.calculateConfidence(measurements),
                processingTime: Date.now() - startTime
            };

            return position;
        } catch (error) {
            logger.error('Trilateration error:', error);
            return null;
        }
    }

    /**
     * Calculate position confidence
     */
    calculateConfidence(measurements) {
        // Simple confidence based on:
        // 1. Number of measurements
        // 2. Signal strength consistency
        
        const nodeCount = measurements.length;
        const nodeScore = Math.min(nodeCount / 5, 1.0); // Max at 5 nodes
        
        // Calculate variance in distances
        const distances = measurements.map(m => m.distance);
        const avgDistance = distances.reduce((a, b) => a + b, 0) / distances.length;
        const variance = distances.reduce((sum, d) => sum + Math.pow(d - avgDistance, 2), 0) / distances.length;
        const consistencyScore = 1 / (1 + variance / 10); // Lower variance = higher score
        
        return nodeScore * 0.5 + consistencyScore * 0.5;
    }

    /**
     * Get recent positions for visualization
     */
    async getRecentPositions(limit = 50) {
        const positions = await csiDatabase.db.all(`
            SELECT 
                p.*,
                COUNT(DISTINCT c.node_id) as node_count
            FROM positions p
            LEFT JOIN csi_data c ON c.mac_address = p.target_mac 
                AND c.timestamp BETWEEN p.timestamp - 2000 AND p.timestamp
            GROUP BY p.id
            ORDER BY p.timestamp DESC
            LIMIT ?
        `, limit);

        return positions;
    }

    /**
     * Get system statistics
     */
    async getStats() {
        const stats = await csiDatabase.getSystemStats();
        
        // Add real-time stats
        stats.queueSize = this.processingQueue.length;
        stats.isProcessing = this.isProcessing;
        
        return stats;
    }
}

// Export singleton instance
export default new CSIReceiver();