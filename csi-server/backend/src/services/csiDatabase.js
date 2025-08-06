/**
 * @file csiDatabase.js
 * @brief SQLite database service for CSI data storage and retrieval
 */

import sqlite3 from 'sqlite3';
import { open } from 'sqlite';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

class CSIDatabase {
    constructor() {
        this.db = null;
        this.dbPath = path.join(__dirname, '../../data/csi.db');
    }

    async initialize() {
        // Open database
        this.db = await open({
            filename: this.dbPath,
            driver: sqlite3.Database
        });

        // Enable WAL mode for better concurrent access
        await this.db.exec('PRAGMA journal_mode = WAL');
        await this.db.exec('PRAGMA synchronous = NORMAL');

        // Create tables
        await this.createTables();
        
        // Create indexes
        await this.createIndexes();

        // Set up data retention (optional)
        await this.setupDataRetention();

        console.log('SQLite database initialized at:', this.dbPath);
    }

    async createTables() {
        // CSI data table
        await this.db.exec(`
            CREATE TABLE IF NOT EXISTS csi_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                node_id TEXT NOT NULL,
                timestamp INTEGER NOT NULL,
                mac_address TEXT,
                rssi REAL,
                channel INTEGER,
                rate INTEGER,
                sig_mode INTEGER,
                mcs INTEGER,
                bandwidth INTEGER,
                smoothing INTEGER,
                not_sounding INTEGER,
                aggregation INTEGER,
                stbc INTEGER,
                fec_coding INTEGER,
                sgi INTEGER,
                noise_floor INTEGER,
                ampdu_cnt INTEGER,
                channel_state_info TEXT,
                amplitude_data TEXT,
                phase_data TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        `);

        // Node registry table
        await this.db.exec(`
            CREATE TABLE IF NOT EXISTS nodes (
                id TEXT PRIMARY KEY,
                name TEXT,
                ip_address TEXT,
                position_x REAL DEFAULT 0,
                position_y REAL DEFAULT 0,
                position_z REAL DEFAULT 0,
                api_key_hash TEXT,
                last_seen INTEGER,
                online BOOLEAN DEFAULT 0,
                config TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        `);

        // Calculated positions table
        await this.db.exec(`
            CREATE TABLE IF NOT EXISTS positions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp INTEGER NOT NULL,
                target_mac TEXT,
                x REAL,
                y REAL,
                z REAL,
                confidence REAL,
                algorithm TEXT,
                node_count INTEGER,
                processing_time_ms INTEGER,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        `);

        // Calibration data table
        await this.db.exec(`
            CREATE TABLE IF NOT EXISTS calibration_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                node_id TEXT NOT NULL,
                reference_point_x REAL,
                reference_point_y REAL,
                reference_point_z REAL,
                rssi_avg REAL,
                csi_features TEXT,
                timestamp INTEGER,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (node_id) REFERENCES nodes(id)
            )
        `);
    }

    async createIndexes() {
        // Indexes for fast queries
        await this.db.exec(`
            CREATE INDEX IF NOT EXISTS idx_csi_timestamp ON csi_data(timestamp);
            CREATE INDEX IF NOT EXISTS idx_csi_node_timestamp ON csi_data(node_id, timestamp);
            CREATE INDEX IF NOT EXISTS idx_csi_mac ON csi_data(mac_address);
            CREATE INDEX IF NOT EXISTS idx_positions_timestamp ON positions(timestamp);
            CREATE INDEX IF NOT EXISTS idx_positions_mac ON positions(target_mac);
        `);
    }

    async setupDataRetention() {
        // Delete CSI data older than 7 days (configurable)
        const retentionDays = process.env.CSI_RETENTION_DAYS || 7;
        
        // Run cleanup every hour
        setInterval(async () => {
            const cutoffTime = Date.now() - (retentionDays * 24 * 60 * 60 * 1000);
            await this.db.run(
                'DELETE FROM csi_data WHERE timestamp < ?',
                cutoffTime
            );
            
            // Vacuum occasionally to reclaim space
            if (Math.random() < 0.1) { // 10% chance
                await this.db.exec('VACUUM');
            }
        }, 60 * 60 * 1000);
    }

    // CSI Data Methods

    async insertCSIData(data) {
        const {
            node_id, timestamp, mac_address, rssi, channel,
            rate, sig_mode, mcs, bandwidth, smoothing,
            not_sounding, aggregation, stbc, fec_coding,
            sgi, noise_floor, ampdu_cnt, channel_state_info,
            amplitude_data, phase_data
        } = data;

        const result = await this.db.run(`
            INSERT INTO csi_data (
                node_id, timestamp, mac_address, rssi, channel,
                rate, sig_mode, mcs, bandwidth, smoothing,
                not_sounding, aggregation, stbc, fec_coding,
                sgi, noise_floor, ampdu_cnt, channel_state_info,
                amplitude_data, phase_data
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        `, [
            node_id, timestamp, mac_address, rssi, channel,
            rate, sig_mode, mcs, bandwidth, smoothing,
            not_sounding, aggregation, stbc, fec_coding,
            sgi, noise_floor, ampdu_cnt,
            JSON.stringify(channel_state_info),
            JSON.stringify(amplitude_data),
            JSON.stringify(phase_data)
        ]);

        return result.lastID;
    }

    async getRecentCSIData(nodeId, limit = 100) {
        return await this.db.all(`
            SELECT * FROM csi_data 
            WHERE node_id = ? 
            ORDER BY timestamp DESC 
            LIMIT ?
        `, [nodeId, limit]);
    }

    async getCSIDataByTimeRange(startTime, endTime, nodeId = null) {
        let query = `
            SELECT * FROM csi_data 
            WHERE timestamp BETWEEN ? AND ?
        `;
        const params = [startTime, endTime];

        if (nodeId) {
            query += ' AND node_id = ?';
            params.push(nodeId);
        }

        query += ' ORDER BY timestamp ASC';
        return await this.db.all(query, params);
    }

    async getCSIDataForPositioning(targetMac, timeWindow = 1000) {
        const now = Date.now();
        const startTime = now - timeWindow;

        return await this.db.all(`
            SELECT 
                node_id,
                AVG(rssi) as avg_rssi,
                COUNT(*) as sample_count,
                MAX(timestamp) as latest_timestamp,
                amplitude_data,
                phase_data
            FROM csi_data
            WHERE mac_address = ? 
                AND timestamp BETWEEN ? AND ?
            GROUP BY node_id
            HAVING sample_count > 3
        `, [targetMac, startTime, now]);
    }

    // Node Management Methods

    async registerNode(nodeId, nodeInfo) {
        const { name, ip_address, position, api_key_hash } = nodeInfo;
        
        await this.db.run(`
            INSERT OR REPLACE INTO nodes (
                id, name, ip_address, 
                position_x, position_y, position_z,
                api_key_hash, last_seen, online, updated_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
        `, [
            nodeId, name, ip_address,
            position?.x || 0, position?.y || 0, position?.z || 0,
            api_key_hash, Date.now(), 1
        ]);
    }

    async updateNodeStatus(nodeId, online) {
        await this.db.run(`
            UPDATE nodes 
            SET online = ?, last_seen = ?, updated_at = CURRENT_TIMESTAMP
            WHERE id = ?
        `, [online ? 1 : 0, Date.now(), nodeId]);
    }

    async getActiveNodes() {
        return await this.db.all(`
            SELECT * FROM nodes 
            WHERE online = 1 
            ORDER BY id
        `);
    }

    async getNodeInfo(nodeId) {
        return await this.db.get(
            'SELECT * FROM nodes WHERE id = ?',
            nodeId
        );
    }

    // Position Storage Methods

    async saveCalculatedPosition(positionData) {
        const {
            timestamp, target_mac, x, y, z,
            confidence, algorithm, node_count, processing_time_ms
        } = positionData;

        const result = await this.db.run(`
            INSERT INTO positions (
                timestamp, target_mac, x, y, z,
                confidence, algorithm, node_count, processing_time_ms
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        `, [
            timestamp, target_mac, x, y, z,
            confidence, algorithm, node_count, processing_time_ms
        ]);

        return result.lastID;
    }

    async getLatestPosition(targetMac) {
        return await this.db.get(`
            SELECT * FROM positions 
            WHERE target_mac = ? 
            ORDER BY timestamp DESC 
            LIMIT 1
        `, targetMac);
    }

    async getPositionHistory(targetMac, limit = 100) {
        return await this.db.all(`
            SELECT * FROM positions 
            WHERE target_mac = ? 
            ORDER BY timestamp DESC 
            LIMIT ?
        `, [targetMac, limit]);
    }

    // Calibration Methods

    async saveCalibrationPoint(calibrationData) {
        const {
            node_id, reference_point, rssi_avg, csi_features
        } = calibrationData;

        await this.db.run(`
            INSERT INTO calibration_data (
                node_id, reference_point_x, reference_point_y, reference_point_z,
                rssi_avg, csi_features, timestamp
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
        `, [
            node_id,
            reference_point.x, reference_point.y, reference_point.z,
            rssi_avg, JSON.stringify(csi_features), Date.now()
        ]);
    }

    async getCalibrationData(nodeId = null) {
        if (nodeId) {
            return await this.db.all(
                'SELECT * FROM calibration_data WHERE node_id = ? ORDER BY created_at DESC',
                nodeId
            );
        }
        return await this.db.all('SELECT * FROM calibration_data ORDER BY created_at DESC');
    }

    // Analytics Methods

    async getSystemStats() {
        const stats = {};

        // Total CSI records
        const csiCount = await this.db.get('SELECT COUNT(*) as count FROM csi_data');
        stats.totalCSIRecords = csiCount.count;

        // Active nodes
        const nodeCount = await this.db.get('SELECT COUNT(*) as count FROM nodes WHERE online = 1');
        stats.activeNodes = nodeCount.count;

        // Recent positions
        const posCount = await this.db.get('SELECT COUNT(*) as count FROM positions WHERE timestamp > ?', 
            Date.now() - 3600000); // Last hour
        stats.recentPositions = posCount.count;

        // Database size
        const dbInfo = await this.db.get("SELECT page_count * page_size as size FROM pragma_page_count(), pragma_page_size()");
        stats.databaseSize = dbInfo.size;

        return stats;
    }

    // Cleanup and maintenance

    async close() {
        if (this.db) {
            await this.db.close();
        }
    }

    async vacuum() {
        await this.db.exec('VACUUM');
    }

    async checkpoint() {
        await this.db.exec('PRAGMA wal_checkpoint(TRUNCATE)');
    }
}

// Export singleton instance
export default new CSIDatabase();