/**
 * @file autoCalibration.js
 * @brief Automatic node position calibration using inter-node measurements
 */

import csiDatabase from './csiDatabase.js';
import logger from '../utils/logger.js';

class AutoCalibration {
    constructor() {
        this.calibrationData = new Map();
        this.isCalibrating = false;
    }

    /**
     * Start auto-calibration process
     */
    async startCalibration(nodeIds) {
        if (this.isCalibrating) {
            throw new Error('Calibration already in progress');
        }

        this.isCalibrating = true;
        logger.info(`Starting auto-calibration for ${nodeIds.length} nodes`);

        try {
            // Step 1: Collect inter-node measurements
            const distanceMatrix = await this.collectInterNodeDistances(nodeIds);
            
            // Step 2: Apply MDS to get relative positions
            const positions = this.multidimensionalScaling(distanceMatrix);
            
            // Step 3: Align to real-world coordinates (if anchors available)
            const alignedPositions = await this.alignToAnchors(positions, nodeIds);
            
            // Step 4: Save calculated positions
            await this.saveNodePositions(alignedPositions);
            
            // Step 5: Validate accuracy
            const accuracy = await this.validatePositions(alignedPositions, distanceMatrix);
            
            return {
                success: true,
                positions: alignedPositions,
                accuracy,
                timestamp: Date.now()
            };
        } finally {
            this.isCalibrating = false;
        }
    }

    /**
     * Collect distance measurements between all node pairs
     */
    async collectInterNodeDistances(nodeIds) {
        const n = nodeIds.length;
        const distanceMatrix = Array(n).fill(null).map(() => Array(n).fill(0));
        
        // Request each node to measure others
        for (let i = 0; i < n; i++) {
            for (let j = i + 1; j < n; j++) {
                const distance = await this.measureNodeDistance(nodeIds[i], nodeIds[j]);
                distanceMatrix[i][j] = distance;
                distanceMatrix[j][i] = distance; // Symmetric
            }
        }
        
        return distanceMatrix;
    }

    /**
     * Measure distance between two nodes using CSI
     */
    async measureNodeDistance(nodeId1, nodeId2) {
        // Get recent CSI measurements between nodes
        const measurements = await csiDatabase.db.all(`
            SELECT 
                AVG(rssi) as avg_rssi,
                COUNT(*) as count,
                AVG(json_extract(amplitude_data, '$[0]')) as avg_amplitude
            FROM csi_data
            WHERE node_id = ? 
                AND mac_address IN (
                    SELECT mac_address FROM nodes WHERE id = ?
                )
                AND timestamp > ?
        `, [nodeId1, nodeId2, Date.now() - 60000]); // Last minute

        if (measurements[0].count < 10) {
            // Request fresh measurements
            await this.requestInterNodeMeasurement(nodeId1, nodeId2);
            // Wait and retry
            await new Promise(resolve => setTimeout(resolve, 5000));
            return this.measureNodeDistance(nodeId1, nodeId2);
        }

        // Convert RSSI to distance using path loss model
        const rssi = measurements[0].avg_rssi;
        const referenceRSSI = -40; // RSSI at 1 meter
        const pathLossExponent = 2.0; // Free space
        
        const distance = Math.pow(10, (referenceRSSI - rssi) / (10 * pathLossExponent));
        
        return distance;
    }

    /**
     * Request a node to measure another node
     */
    async requestInterNodeMeasurement(sourceNodeId, targetNodeId) {
        // Send command to source node to measure target
        const nodeClient = (await import('./nodeClient.js')).default;
        
        await nodeClient.sendCommand(sourceNodeId, 'measure_node', {
            target_node_id: targetNodeId,
            duration_ms: 3000,
            sample_rate: 50
        });
    }

    /**
     * Classical Multidimensional Scaling (MDS) algorithm
     */
    multidimensionalScaling(distanceMatrix) {
        const n = distanceMatrix.length;
        
        // Step 1: Square the distance matrix
        const D2 = distanceMatrix.map(row => row.map(d => d * d));
        
        // Step 2: Double centering
        const rowMeans = D2.map(row => row.reduce((a, b) => a + b) / n);
        const totalMean = rowMeans.reduce((a, b) => a + b) / n;
        
        const B = Array(n).fill(null).map(() => Array(n).fill(0));
        for (let i = 0; i < n; i++) {
            for (let j = 0; j < n; j++) {
                B[i][j] = -0.5 * (D2[i][j] - rowMeans[i] - rowMeans[j] + totalMean);
            }
        }
        
        // Step 3: Eigenvalue decomposition (simplified using power iteration)
        const positions = this.extractPositions(B, 3); // 3D positions
        
        return positions;
    }

    /**
     * Extract 3D positions from matrix B using simplified decomposition
     */
    extractPositions(B, dimensions = 3) {
        const n = B.length;
        const positions = [];
        
        // Simplified: Use first 3 principal components
        // In production, use proper eigenvalue decomposition
        for (let i = 0; i < n; i++) {
            positions.push({
                x: this.getPrincipalComponent(B, i, 0),
                y: this.getPrincipalComponent(B, i, 1),
                z: this.getPrincipalComponent(B, i, 2)
            });
        }
        
        // Normalize positions
        return this.normalizePositions(positions);
    }

    /**
     * Get principal component (simplified)
     */
    getPrincipalComponent(B, nodeIndex, component) {
        // Simplified extraction - in production use proper linear algebra
        let sum = 0;
        for (let j = 0; j < B.length; j++) {
            sum += B[nodeIndex][j] * Math.sin((j + 1) * (component + 1) * Math.PI / B.length);
        }
        return sum / B.length;
    }

    /**
     * Normalize positions to reasonable scale
     */
    normalizePositions(positions) {
        // Find bounding box
        let minX = Infinity, maxX = -Infinity;
        let minY = Infinity, maxY = -Infinity;
        let minZ = Infinity, maxZ = -Infinity;
        
        positions.forEach(pos => {
            minX = Math.min(minX, pos.x);
            maxX = Math.max(maxX, pos.x);
            minY = Math.min(minY, pos.y);
            maxY = Math.max(maxY, pos.y);
            minZ = Math.min(minZ, pos.z);
            maxZ = Math.max(maxZ, pos.z);
        });
        
        const scaleX = maxX - minX || 1;
        const scaleY = maxY - minY || 1;
        const scaleZ = maxZ - minZ || 1;
        
        // Normalize to 10-meter space
        const targetScale = 10;
        const scale = targetScale / Math.max(scaleX, scaleY, scaleZ);
        
        return positions.map(pos => ({
            x: (pos.x - minX) * scale,
            y: (pos.y - minY) * scale,
            z: (pos.z - minZ) * scale
        }));
    }

    /**
     * Align positions to known anchor points (if available)
     */
    async alignToAnchors(positions, nodeIds) {
        // Check if any nodes have manually set positions (anchors)
        const anchors = [];
        
        for (let i = 0; i < nodeIds.length; i++) {
            const node = await csiDatabase.getNodeInfo(nodeIds[i]);
            if (node && node.is_anchor) {
                anchors.push({
                    index: i,
                    target: { x: node.position_x, y: node.position_y, z: node.position_z },
                    current: positions[i]
                });
            }
        }
        
        if (anchors.length < 3) {
            // Not enough anchors, return as-is
            return nodeIds.map((id, i) => ({
                nodeId: id,
                ...positions[i]
            }));
        }
        
        // Apply transformation to align with anchors
        const transformation = this.calculateTransformation(anchors);
        
        return nodeIds.map((id, i) => ({
            nodeId: id,
            ...this.applyTransformation(positions[i], transformation)
        }));
    }

    /**
     * Calculate transformation matrix from anchors
     */
    calculateTransformation(anchors) {
        // Simplified: Calculate translation and rotation
        // In production, use proper 3D transformation (Kabsch algorithm)
        
        const translation = {
            x: anchors[0].target.x - anchors[0].current.x,
            y: anchors[0].target.y - anchors[0].current.y,
            z: anchors[0].target.z - anchors[0].current.z
        };
        
        // Simplified rotation (around Z axis only)
        let rotation = 0;
        if (anchors.length >= 2) {
            const currentAngle = Math.atan2(
                anchors[1].current.y - anchors[0].current.y,
                anchors[1].current.x - anchors[0].current.x
            );
            const targetAngle = Math.atan2(
                anchors[1].target.y - anchors[0].target.y,
                anchors[1].target.x - anchors[0].target.x
            );
            rotation = targetAngle - currentAngle;
        }
        
        return { translation, rotation };
    }

    /**
     * Apply transformation to position
     */
    applyTransformation(position, transformation) {
        // Apply rotation
        const cos = Math.cos(transformation.rotation);
        const sin = Math.sin(transformation.rotation);
        
        const rotated = {
            x: position.x * cos - position.y * sin,
            y: position.x * sin + position.y * cos,
            z: position.z
        };
        
        // Apply translation
        return {
            x: rotated.x + transformation.translation.x,
            y: rotated.y + transformation.translation.y,
            z: rotated.z + transformation.translation.z
        };
    }

    /**
     * Save calculated node positions
     */
    async saveNodePositions(alignedPositions) {
        for (const pos of alignedPositions) {
            await csiDatabase.db.run(`
                UPDATE nodes 
                SET position_x = ?, position_y = ?, position_z = ?, 
                    updated_at = CURRENT_TIMESTAMP
                WHERE id = ?
            `, [pos.x, pos.y, pos.z, pos.nodeId]);
            
            logger.info(`Updated position for ${pos.nodeId}: (${pos.x.toFixed(2)}, ${pos.y.toFixed(2)}, ${pos.z.toFixed(2)})`);
        }
    }

    /**
     * Validate calculated positions against distance matrix
     */
    async validatePositions(positions, distanceMatrix) {
        let totalError = 0;
        let count = 0;
        
        for (let i = 0; i < positions.length; i++) {
            for (let j = i + 1; j < positions.length; j++) {
                const calculatedDistance = Math.sqrt(
                    Math.pow(positions[i].x - positions[j].x, 2) +
                    Math.pow(positions[i].y - positions[j].y, 2) +
                    Math.pow(positions[i].z - positions[j].z, 2)
                );
                
                const measuredDistance = distanceMatrix[i][j];
                const error = Math.abs(calculatedDistance - measuredDistance);
                
                totalError += error;
                count++;
            }
        }
        
        const avgError = totalError / count;
        const accuracy = Math.max(0, 100 - (avgError * 10)); // Rough accuracy percentage
        
        logger.info(`Calibration accuracy: ${accuracy.toFixed(1)}% (avg error: ${avgError.toFixed(2)}m)`);
        
        return {
            averageError: avgError,
            accuracy: accuracy,
            validated: avgError < 1.0 // Less than 1 meter average error
        };
    }

    /**
     * Continuous calibration refinement
     */
    async refineCalibration() {
        // Get all active nodes
        const nodes = await csiDatabase.getActiveNodes();
        const nodeIds = nodes.map(n => n.id);
        
        if (nodeIds.length < 3) {
            return; // Not enough nodes
        }
        
        // Collect recent inter-node measurements
        const recentMeasurements = await this.getRecentInterNodeMeasurements(nodeIds);
        
        // Apply incremental adjustments
        for (const measurement of recentMeasurements) {
            await this.incrementalPositionAdjustment(measurement);
        }
    }

    /**
     * Get recent inter-node CSI measurements
     */
    async getRecentInterNodeMeasurements(nodeIds) {
        return await csiDatabase.db.all(`
            SELECT 
                c1.node_id as node1,
                n2.id as node2,
                AVG(c1.rssi) as avg_rssi,
                COUNT(*) as count
            FROM csi_data c1
            JOIN nodes n2 ON c1.mac_address = n2.mac_address
            WHERE c1.node_id IN (${nodeIds.map(() => '?').join(',')})
                AND n2.id IN (${nodeIds.map(() => '?').join(',')})
                AND c1.node_id != n2.id
                AND c1.timestamp > ?
            GROUP BY c1.node_id, n2.id
            HAVING count > 5
        `, [...nodeIds, ...nodeIds, Date.now() - 300000]); // Last 5 minutes
    }

    /**
     * Incremental position adjustment based on measurements
     */
    async incrementalPositionAdjustment(measurement) {
        const node1 = await csiDatabase.getNodeInfo(measurement.node1);
        const node2 = await csiDatabase.getNodeInfo(measurement.node2);
        
        if (!node1 || !node2) return;
        
        // Calculate current distance
        const currentDistance = Math.sqrt(
            Math.pow(node1.position_x - node2.position_x, 2) +
            Math.pow(node1.position_y - node2.position_y, 2) +
            Math.pow(node1.position_z - node2.position_z, 2)
        );
        
        // Calculate measured distance from RSSI
        const measuredDistance = this.rssiToDistance(measurement.avg_rssi);
        
        // Apply small adjustment if error is significant
        const error = measuredDistance - currentDistance;
        if (Math.abs(error) > 0.5) { // More than 0.5m error
            const adjustment = error * 0.1; // 10% adjustment
            
            // Move nodes slightly closer/further
            const direction = {
                x: (node2.position_x - node1.position_x) / currentDistance,
                y: (node2.position_y - node1.position_y) / currentDistance,
                z: (node2.position_z - node1.position_z) / currentDistance
            };
            
            // Update node2 position slightly
            await csiDatabase.db.run(`
                UPDATE nodes 
                SET position_x = position_x + ?,
                    position_y = position_y + ?,
                    position_z = position_z + ?
                WHERE id = ?
            `, [
                direction.x * adjustment,
                direction.y * adjustment,
                direction.z * adjustment,
                measurement.node2
            ]);
        }
    }

    /**
     * Convert RSSI to distance
     */
    rssiToDistance(rssi) {
        const referenceRSSI = -40; // RSSI at 1 meter
        const pathLossExponent = 2.0; // Adjust based on environment
        return Math.pow(10, (referenceRSSI - rssi) / (10 * pathLossExponent));
    }
}

// Export singleton
export default new AutoCalibration();