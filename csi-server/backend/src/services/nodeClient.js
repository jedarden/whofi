/**
 * @file nodeClient.js
 * @brief HTTP client for communicating with ESP32 nodes
 */

import axios from 'axios';
import { EventEmitter } from 'events';
import logger from '../utils/logger.js';

class NodeClient extends EventEmitter {
    constructor() {
        super();
        this.nodes = new Map();
        this.apiKeys = new Map(); // Store API keys per node
        this.tokens = new Map();  // Store auth tokens per node
    }

    /**
     * Register a node with its API key
     */
    registerNode(nodeId, ipAddress, apiKey) {
        this.nodes.set(nodeId, {
            id: nodeId,
            ip: ipAddress,
            baseUrl: `http://${ipAddress}`,
            lastSeen: Date.now(),
            online: false
        });

        if (apiKey) {
            this.apiKeys.set(nodeId, apiKey);
        }

        logger.info(`Registered node ${nodeId} at ${ipAddress}`);
    }

    /**
     * Get authentication headers for a node
     */
    async getAuthHeaders(nodeId) {
        // Try token first
        const token = this.tokens.get(nodeId);
        if (token && token.expiresAt > Date.now()) {
            return { 'Authorization': `Bearer ${token.token}` };
        }

        // Use API key
        const apiKey = this.apiKeys.get(nodeId);
        if (apiKey) {
            return { 'X-API-Key': apiKey };
        }

        // Try to authenticate and get token
        if (apiKey) {
            const newToken = await this.authenticate(nodeId);
            if (newToken) {
                return { 'Authorization': `Bearer ${newToken}` };
            }
        }

        return {};
    }

    /**
     * Authenticate with a node and get token
     */
    async authenticate(nodeId) {
        const node = this.nodes.get(nodeId);
        const apiKey = this.apiKeys.get(nodeId);

        if (!node || !apiKey) {
            throw new Error(`Node ${nodeId} not found or no API key`);
        }

        try {
            const response = await axios.post(`${node.baseUrl}/api/auth`, {
                api_key: apiKey
            }, {
                timeout: 5000
            });

            const { token, expires_in } = response.data;
            
            this.tokens.set(nodeId, {
                token,
                expiresAt: Date.now() + (expires_in * 1000)
            });

            logger.info(`Authenticated with node ${nodeId}`);
            return token;
        } catch (error) {
            logger.error(`Failed to authenticate with node ${nodeId}:`, error.message);
            throw error;
        }
    }

    /**
     * Get node configuration
     */
    async getConfig(nodeId) {
        const node = this.nodes.get(nodeId);
        if (!node) {
            throw new Error(`Node ${nodeId} not found`);
        }

        try {
            const headers = await this.getAuthHeaders(nodeId);
            const response = await axios.get(`${node.baseUrl}/api/config`, {
                headers,
                timeout: 5000
            });

            node.online = true;
            node.lastSeen = Date.now();
            
            return response.data;
        } catch (error) {
            node.online = false;
            logger.error(`Failed to get config from node ${nodeId}:`, error.message);
            throw error;
        }
    }

    /**
     * Update node configuration
     */
    async updateConfig(nodeId, config) {
        const node = this.nodes.get(nodeId);
        if (!node) {
            throw new Error(`Node ${nodeId} not found`);
        }

        try {
            const headers = await this.getAuthHeaders(nodeId);
            const response = await axios.post(`${node.baseUrl}/api/config`, config, {
                headers,
                timeout: 10000
            });

            logger.info(`Updated config for node ${nodeId}`);
            this.emit('config-updated', { nodeId, config, response: response.data });
            
            return response.data;
        } catch (error) {
            logger.error(`Failed to update config for node ${nodeId}:`, error.message);
            throw error;
        }
    }

    /**
     * Send command to node
     */
    async sendCommand(nodeId, command, params = {}) {
        const node = this.nodes.get(nodeId);
        if (!node) {
            throw new Error(`Node ${nodeId} not found`);
        }

        try {
            const headers = await this.getAuthHeaders(nodeId);
            const response = await axios.post(`${node.baseUrl}/api/command`, {
                command,
                ...params
            }, {
                headers,
                timeout: 10000
            });

            logger.info(`Sent command '${command}' to node ${nodeId}`);
            return response.data;
        } catch (error) {
            logger.error(`Failed to send command to node ${nodeId}:`, error.message);
            throw error;
        }
    }

    /**
     * Trigger OTA update on node
     */
    async triggerOtaUpdate(nodeId, updateUrl, version) {
        const node = this.nodes.get(nodeId);
        if (!node) {
            throw new Error(`Node ${nodeId} not found`);
        }

        try {
            const headers = await this.getAuthHeaders(nodeId);
            const response = await axios.post(`${node.baseUrl}/api/ota/update`, {
                url: updateUrl,
                version
            }, {
                headers,
                timeout: 10000
            });

            logger.info(`Triggered OTA update for node ${nodeId}`);
            this.emit('ota-started', { nodeId, updateUrl, version });
            
            // Start monitoring OTA progress
            this.monitorOtaProgress(nodeId);
            
            return response.data;
        } catch (error) {
            logger.error(`Failed to trigger OTA update for node ${nodeId}:`, error.message);
            throw error;
        }
    }

    /**
     * Monitor OTA update progress
     */
    async monitorOtaProgress(nodeId) {
        const checkProgress = async () => {
            try {
                const headers = await this.getAuthHeaders(nodeId);
                const response = await axios.get(`${node.baseUrl}/api/ota/status`, {
                    headers,
                    timeout: 5000
                });

                const status = response.data;
                this.emit('ota-progress', { nodeId, ...status });

                if (status.state === 'idle' || status.state === 'error') {
                    clearInterval(progressInterval);
                    this.emit('ota-complete', { nodeId, ...status });
                }
            } catch (error) {
                logger.error(`Failed to check OTA status for node ${nodeId}:`, error.message);
            }
        };

        const progressInterval = setInterval(checkProgress, 5000);
        checkProgress(); // Initial check
    }

    /**
     * Batch update multiple nodes
     */
    async batchUpdateConfig(nodeIds, config) {
        const results = await Promise.allSettled(
            nodeIds.map(nodeId => this.updateConfig(nodeId, config))
        );

        const successful = [];
        const failed = [];

        results.forEach((result, index) => {
            if (result.status === 'fulfilled') {
                successful.push(nodeIds[index]);
            } else {
                failed.push({
                    nodeId: nodeIds[index],
                    error: result.reason.message
                });
            }
        });

        logger.info(`Batch update: ${successful.length} successful, ${failed.length} failed`);
        return { successful, failed };
    }

    /**
     * Update CSI configuration on all nodes
     */
    async updateAllCsiConfig(csiConfig) {
        const nodeIds = Array.from(this.nodes.keys());
        return this.batchUpdateConfig(nodeIds, { csi: csiConfig });
    }

    /**
     * Update node position
     */
    async updateNodePosition(nodeId, position) {
        return this.updateConfig(nodeId, {
            node: { position }
        });
    }

    /**
     * Calibrate specific node
     */
    async calibrateNode(nodeId) {
        return this.sendCommand(nodeId, 'calibrate');
    }

    /**
     * Start CSI collection on all nodes
     */
    async startAllCsiCollection() {
        const nodeIds = Array.from(this.nodes.keys());
        const results = await Promise.allSettled(
            nodeIds.map(nodeId => this.sendCommand(nodeId, 'start_csi'))
        );

        return results.map((result, index) => ({
            nodeId: nodeIds[index],
            success: result.status === 'fulfilled',
            error: result.status === 'rejected' ? result.reason.message : null
        }));
    }

    /**
     * Health check all nodes
     */
    async healthCheckAll() {
        const nodeIds = Array.from(this.nodes.keys());
        const results = await Promise.allSettled(
            nodeIds.map(nodeId => this.getConfig(nodeId))
        );

        const healthStatus = {};
        results.forEach((result, index) => {
            const nodeId = nodeIds[index];
            healthStatus[nodeId] = {
                online: result.status === 'fulfilled',
                lastSeen: this.nodes.get(nodeId).lastSeen,
                config: result.status === 'fulfilled' ? result.value : null,
                error: result.status === 'rejected' ? result.reason.message : null
            };
        });

        return healthStatus;
    }

    /**
     * Get all node statuses
     */
    getAllNodeStatuses() {
        const statuses = {};
        this.nodes.forEach((node, nodeId) => {
            statuses[nodeId] = {
                ...node,
                hasApiKey: this.apiKeys.has(nodeId),
                authenticated: this.tokens.has(nodeId) && 
                              this.tokens.get(nodeId).expiresAt > Date.now()
            };
        });
        return statuses;
    }
}

// Singleton instance
export default new NodeClient();