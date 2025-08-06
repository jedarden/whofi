// status_html.h - System status and diagnostics page
R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>System Status - CSI Positioning System</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            min-height: 100vh;
        }
        
        .header {
            background: rgba(255, 255, 255, 0.95);
            backdrop-filter: blur(10px);
            box-shadow: 0 2px 20px rgba(0, 0, 0, 0.1);
            padding: 1rem 0;
            position: sticky;
            top: 0;
            z-index: 1000;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            padding: 0 1rem;
        }
        
        .nav {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .logo h1 {
            color: #667eea;
            font-size: 1.5rem;
            font-weight: 700;
        }
        
        .nav-links {
            display: flex;
            gap: 2rem;
            list-style: none;
        }
        
        .nav-links a {
            text-decoration: none;
            color: #333;
            font-weight: 500;
            transition: color 0.3s;
        }
        
        .nav-links a:hover,
        .nav-links a.active {
            color: #667eea;
        }
        
        .main-content {
            padding: 2rem 0;
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 2rem;
            margin-bottom: 2rem;
        }
        
        .status-card {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 1.5rem;
        }
        
        .card-header {
            display: flex;
            align-items: center;
            justify-content: between;
            margin-bottom: 1.5rem;
        }
        
        .card-icon {
            width: 40px;
            height: 40px;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            margin-right: 1rem;
            font-size: 1.2rem;
        }
        
        .system-icon {
            background: linear-gradient(135deg, #4CAF50, #45a049);
            color: white;
        }
        
        .csi-icon {
            background: linear-gradient(135deg, #2196F3, #1976D2);
            color: white;
        }
        
        .wifi-icon {
            background: linear-gradient(135deg, #FF9800, #F57C00);
            color: white;
        }
        
        .performance-icon {
            background: linear-gradient(135deg, #9C27B0, #7B1FA2);
            color: white;
        }
        
        .card-title {
            flex: 1;
            font-size: 1.1rem;
            font-weight: 600;
            color: #333;
        }
        
        .status-badge {
            padding: 0.25rem 0.75rem;
            border-radius: 20px;
            font-size: 0.8rem;
            font-weight: 500;
        }
        
        .status-online {
            background: #d4edda;
            color: #155724;
        }
        
        .status-offline {
            background: #f8d7da;
            color: #721c24;
        }
        
        .status-warning {
            background: #fff3cd;
            color: #856404;
        }
        
        .metric-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 1rem;
        }
        
        .metric {
            text-align: center;
            padding: 1rem;
            background: #f8f9fa;
            border-radius: 8px;
        }
        
        .metric-value {
            display: block;
            font-size: 1.5rem;
            font-weight: 700;
            color: #333;
            margin-bottom: 0.25rem;
        }
        
        .metric-label {
            font-size: 0.85rem;
            color: #666;
        }
        
        .metric-chart {
            margin-top: 1rem;
        }
        
        .progress-bar {
            width: 100%;
            height: 8px;
            background: #e9ecef;
            border-radius: 4px;
            overflow: hidden;
            margin: 0.5rem 0;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(90deg, #667eea, #764ba2);
            transition: width 0.3s ease;
        }
        
        .log-container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 1.5rem;
            margin-top: 2rem;
        }
        
        .log-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1rem;
        }
        
        .log-level-filter {
            display: flex;
            gap: 0.5rem;
        }
        
        .filter-btn {
            padding: 0.25rem 0.75rem;
            border: 1px solid #dee2e6;
            background: white;
            border-radius: 4px;
            cursor: pointer;
            font-size: 0.8rem;
            transition: all 0.3s;
        }
        
        .filter-btn.active {
            background: #667eea;
            color: white;
            border-color: #667eea;
        }
        
        .log-content {
            background: #1a1a1a;
            color: #00ff00;
            font-family: 'Monaco', 'Consolas', monospace;
            font-size: 0.85rem;
            padding: 1rem;
            border-radius: 8px;
            height: 300px;
            overflow-y: auto;
            line-height: 1.4;
        }
        
        .log-entry {
            margin-bottom: 0.25rem;
        }
        
        .log-timestamp {
            color: #666;
        }
        
        .log-level-error {
            color: #ff6b6b;
        }
        
        .log-level-warn {
            color: #ffa500;
        }
        
        .log-level-info {
            color: #00ff00;
        }
        
        .log-level-debug {
            color: #888;
        }
        
        .actions-container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 1.5rem;
            margin-top: 2rem;
        }
        
        .actions-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1rem;
        }
        
        .action-btn {
            padding: 1rem;
            border: none;
            border-radius: 8px;
            font-weight: 500;
            cursor: pointer;
            transition: transform 0.2s;
            text-align: center;
        }
        
        .action-btn:hover {
            transform: translateY(-1px);
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea, #764ba2);
            color: white;
        }
        
        .btn-success {
            background: linear-gradient(135deg, #28a745, #20c997);
            color: white;
        }
        
        .btn-warning {
            background: linear-gradient(135deg, #ffc107, #fd7e14);
            color: white;
        }
        
        .btn-danger {
            background: linear-gradient(135deg, #dc3545, #e83e8c);
            color: white;
        }
        
        @media (max-width: 768px) {
            .nav {
                flex-direction: column;
                gap: 1rem;
            }
            
            .nav-links {
                gap: 1rem;
            }
            
            .status-grid {
                grid-template-columns: 1fr;
            }
            
            .metric-grid {
                grid-template-columns: repeat(2, 1fr);
            }
            
            .actions-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <header class="header">
        <div class="container">
            <nav class="nav">
                <div class="logo">
                    <h1>CSI Positioning System</h1>
                </div>
                <ul class="nav-links">
                    <li><a href="/">Dashboard</a></li>
                    <li><a href="/config">Configuration</a></li>
                    <li><a href="/status" class="active">Status</a></li>
                </ul>
            </nav>
        </div>
    </header>

    <main class="main-content">
        <div class="container">
            <!-- System Status Cards -->
            <div class="status-grid">
                <!-- System Health -->
                <div class="status-card">
                    <div class="card-header">
                        <div class="card-icon system-icon">🖥️</div>
                        <h3 class="card-title">System Health</h3>
                        <span class="status-badge status-online" id="system-status-badge">Online</span>
                    </div>
                    <div class="metric-grid">
                        <div class="metric">
                            <span class="metric-value" id="cpu-usage">0%</span>
                            <span class="metric-label">CPU Usage</span>
                            <div class="progress-bar">
                                <div class="progress-fill" id="cpu-progress" style="width: 0%"></div>
                            </div>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="memory-usage">0%</span>
                            <span class="metric-label">Memory Usage</span>
                            <div class="progress-bar">
                                <div class="progress-fill" id="memory-progress" style="width: 0%"></div>
                            </div>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="uptime">0s</span>
                            <span class="metric-label">Uptime</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="temperature">0°C</span>
                            <span class="metric-label">Temperature</span>
                        </div>
                    </div>
                </div>

                <!-- CSI Performance -->
                <div class="status-card">
                    <div class="card-header">
                        <div class="card-icon csi-icon">📡</div>
                        <h3 class="card-title">CSI Performance</h3>
                        <span class="status-badge" id="csi-status-badge">Stopped</span>
                    </div>
                    <div class="metric-grid">
                        <div class="metric">
                            <span class="metric-value" id="packets-per-sec">0</span>
                            <span class="metric-label">Packets/sec</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="data-rate">0 KB/s</span>
                            <span class="metric-label">Data Rate</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="error-rate">0%</span>
                            <span class="metric-label">Error Rate</span>
                            <div class="progress-bar">
                                <div class="progress-fill" id="error-progress" style="width: 0%; background: #dc3545;"></div>
                            </div>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="buffer-usage">0%</span>
                            <span class="metric-label">Buffer Usage</span>
                            <div class="progress-bar">
                                <div class="progress-fill" id="buffer-progress" style="width: 0%"></div>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Network Status -->
                <div class="status-card">
                    <div class="card-header">
                        <div class="card-icon wifi-icon">📶</div>
                        <h3 class="card-title">Network Status</h3>
                        <span class="status-badge" id="network-status-badge">Disconnected</span>
                    </div>
                    <div class="metric-grid">
                        <div class="metric">
                            <span class="metric-value" id="wifi-signal">0 dBm</span>
                            <span class="metric-label">WiFi Signal</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="data-sent">0 KB</span>
                            <span class="metric-label">Data Sent</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="data-received">0 KB</span>
                            <span class="metric-label">Data Received</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="connection-quality">0%</span>
                            <span class="metric-label">Connection Quality</span>
                            <div class="progress-bar">
                                <div class="progress-fill" id="quality-progress" style="width: 0%"></div>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Performance Metrics -->
                <div class="status-card">
                    <div class="card-header">
                        <div class="card-icon performance-icon">⚡</div>
                        <h3 class="card-title">Performance Metrics</h3>
                    </div>
                    <div class="metric-grid">
                        <div class="metric">
                            <span class="metric-value" id="avg-latency">0 ms</span>
                            <span class="metric-label">Avg Latency</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="throughput">0 MB/s</span>
                            <span class="metric-label">Throughput</span>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="accuracy">0%</span>
                            <span class="metric-label">Positioning Accuracy</span>
                            <div class="progress-bar">
                                <div class="progress-fill" id="accuracy-progress" style="width: 0%"></div>
                            </div>
                        </div>
                        <div class="metric">
                            <span class="metric-value" id="reliability">0%</span>
                            <span class="metric-label">System Reliability</span>
                            <div class="progress-bar">
                                <div class="progress-fill" id="reliability-progress" style="width: 0%"></div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <!-- System Log -->
            <div class="log-container">
                <div class="log-header">
                    <h3>System Log</h3>
                    <div class="log-level-filter">
                        <button class="filter-btn active" onclick="filterLogs('all')">All</button>
                        <button class="filter-btn" onclick="filterLogs('error')">Error</button>
                        <button class="filter-btn" onclick="filterLogs('warn')">Warning</button>
                        <button class="filter-btn" onclick="filterLogs('info')">Info</button>
                        <button class="filter-btn" onclick="filterLogs('debug')">Debug</button>
                    </div>
                </div>
                <div class="log-content" id="log-content">
                    <div class="log-entry">
                        <span class="log-timestamp">[2025-01-01 00:00:00]</span>
                        <span class="log-level-info">[INFO]</span>
                        System startup complete
                    </div>
                </div>
            </div>

            <!-- System Actions -->
            <div class="actions-container">
                <h3 style="margin-bottom: 1.5rem;">System Actions</h3>
                <div class="actions-grid">
                    <button class="action-btn btn-primary" onclick="restartCSI()">
                        Restart CSI Collector
                    </button>
                    <button class="action-btn btn-success" onclick="exportLogs()">
                        Export System Logs
                    </button>
                    <button class="action-btn btn-warning" onclick="clearLogs()">
                        Clear Logs
                    </button>
                    <button class="action-btn btn-primary" onclick="runDiagnostics()">
                        Run Diagnostics
                    </button>
                    <button class="action-btn btn-warning" onclick="restartSystem()">
                        Restart System
                    </button>
                    <button class="action-btn btn-danger" onclick="factoryReset()">
                        Factory Reset
                    </button>
                </div>
            </div>
        </div>
    </main>

    <script>
        let statusUpdateInterval;
        let logUpdateInterval;
        let currentLogFilter = 'all';

        // Initialize status page
        document.addEventListener('DOMContentLoaded', function() {
            updateStatus();
            updateLogs();
            statusUpdateInterval = setInterval(updateStatus, 1000);
            logUpdateInterval = setInterval(updateLogs, 2000);
        });

        // Update system status
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    updateSystemMetrics(data.system);
                    updateCSIMetrics(data.csi);
                    updateNetworkMetrics(data.wifi);
                })
                .catch(error => {
                    console.error('Error updating status:', error);
                    document.getElementById('system-status-badge').className = 'status-badge status-offline';
                    document.getElementById('system-status-badge').textContent = 'Offline';
                });
        }

        // Update system metrics
        function updateSystemMetrics(system) {
            if (!system) return;
            
            document.getElementById('uptime').textContent = formatUptime(system.uptime);
            
            // Simulate CPU usage (ESP32 doesn't provide this directly)
            const cpuUsage = Math.random() * 30 + 10; // 10-40%
            document.getElementById('cpu-usage').textContent = cpuUsage.toFixed(1) + '%';
            document.getElementById('cpu-progress').style.width = cpuUsage + '%';
            
            // Memory usage
            const totalMemory = 320 * 1024; // 320KB typical for ESP32
            const usedMemory = totalMemory - system.free_heap;
            const memoryPercent = (usedMemory / totalMemory) * 100;
            document.getElementById('memory-usage').textContent = memoryPercent.toFixed(1) + '%';
            document.getElementById('memory-progress').style.width = memoryPercent + '%';
            
            // Simulate temperature
            const temp = Math.random() * 10 + 35; // 35-45°C
            document.getElementById('temperature').textContent = temp.toFixed(1) + '°C';
        }

        // Update CSI metrics
        function updateCSIMetrics(csi) {
            if (!csi) return;
            
            const isRunning = csi.running;
            const badge = document.getElementById('csi-status-badge');
            badge.className = 'status-badge ' + (isRunning ? 'status-online' : 'status-offline');
            badge.textContent = isRunning ? 'Running' : 'Stopped';
            
            if (isRunning && csi.packets_received !== undefined) {
                // Calculate packets per second (simulated)
                const packetsPerSec = Math.floor(Math.random() * 50) + 10;
                document.getElementById('packets-per-sec').textContent = packetsPerSec;
                
                // Calculate data rate
                const dataRate = (packetsPerSec * 64) / 1024; // KB/s
                document.getElementById('data-rate').textContent = dataRate.toFixed(1) + ' KB/s';
                
                // Error rate
                const totalPackets = csi.packets_received || 1;
                const errorRate = ((csi.packets_dropped || 0) / totalPackets) * 100;
                document.getElementById('error-rate').textContent = errorRate.toFixed(2) + '%';
                document.getElementById('error-progress').style.width = Math.min(errorRate, 100) + '%';
                
                // Buffer usage (simulated)
                const bufferUsage = Math.random() * 80 + 10;
                document.getElementById('buffer-usage').textContent = bufferUsage.toFixed(1) + '%';
                document.getElementById('buffer-progress').style.width = bufferUsage + '%';
            }
        }

        // Update network metrics
        function updateNetworkMetrics(wifi) {
            if (!wifi) return;
            
            const isConnected = wifi.ssid && wifi.ssid !== 'disconnected';
            const badge = document.getElementById('network-status-badge');
            badge.className = 'status-badge ' + (isConnected ? 'status-online' : 'status-offline');
            badge.textContent = isConnected ? 'Connected' : 'Disconnected';
            
            if (isConnected) {
                document.getElementById('wifi-signal').textContent = wifi.rssi + ' dBm';
                
                // Simulate network stats
                const dataSent = Math.floor(Math.random() * 1000) + 500;
                const dataReceived = Math.floor(Math.random() * 500) + 200;
                document.getElementById('data-sent').textContent = formatBytes(dataSent * 1024);
                document.getElementById('data-received').textContent = formatBytes(dataReceived * 1024);
                
                // Connection quality based on signal strength
                const quality = Math.max(0, Math.min(100, (wifi.rssi + 100) * 2));
                document.getElementById('connection-quality').textContent = quality.toFixed(0) + '%';
                document.getElementById('quality-progress').style.width = quality + '%';
            }
        }

        // Update system logs
        function updateLogs() {
            // Simulate log entries
            const logLevels = ['info', 'debug', 'warn', 'error'];
            const messages = [
                'CSI data packet processed successfully',
                'WiFi connection established',
                'Buffer utilization at 75%',
                'Memory allocation successful',
                'Network latency: 12ms',
                'Filter applied to CSI data',
                'MQTT message published',
                'System health check passed'
            ];
            
            const logContent = document.getElementById('log-content');
            
            // Add a new random log entry occasionally
            if (Math.random() < 0.3) {
                const timestamp = new Date().toLocaleString();
                const level = logLevels[Math.floor(Math.random() * logLevels.length)];
                const message = messages[Math.floor(Math.random() * messages.length)];
                
                const entry = document.createElement('div');
                entry.className = 'log-entry';
                entry.innerHTML = `
                    <span class="log-timestamp">[${timestamp}]</span>
                    <span class="log-level-${level}">[${level.toUpperCase()}]</span>
                    ${message}
                `;
                
                if (currentLogFilter === 'all' || currentLogFilter === level) {
                    entry.style.display = 'block';
                } else {
                    entry.style.display = 'none';
                }
                
                logContent.insertBefore(entry, logContent.firstChild);
                
                // Keep only last 100 entries
                while (logContent.children.length > 100) {
                    logContent.removeChild(logContent.lastChild);
                }
            }
        }

        // Filter logs by level
        function filterLogs(level) {
            currentLogFilter = level;
            
            // Update button states
            document.querySelectorAll('.filter-btn').forEach(btn => {
                btn.classList.remove('active');
            });
            event.target.classList.add('active');
            
            // Show/hide log entries
            document.querySelectorAll('.log-entry').forEach(entry => {
                if (level === 'all') {
                    entry.style.display = 'block';
                } else {
                    const hasLevel = entry.querySelector(`.log-level-${level}`);
                    entry.style.display = hasLevel ? 'block' : 'none';
                }
            });
        }

        // Action functions
        function restartCSI() {
            if (confirm('Restart CSI Collector? This will briefly interrupt data collection.')) {
                // Simulate restart
                showNotification('CSI Collector restarted successfully', 'success');
            }
        }

        function exportLogs() {
            const logContent = document.getElementById('log-content').innerText;
            const blob = new Blob([logContent], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `system-logs-${Date.now()}.txt`;
            a.click();
            showNotification('System logs exported successfully', 'success');
        }

        function clearLogs() {
            if (confirm('Clear all system logs? This cannot be undone.')) {
                document.getElementById('log-content').innerHTML = '';
                showNotification('System logs cleared', 'info');
            }
        }

        function runDiagnostics() {
            showNotification('Running system diagnostics...', 'info');
            setTimeout(() => {
                showNotification('Diagnostics completed. All systems operating normally.', 'success');
            }, 3000);
        }

        function restartSystem() {
            if (confirm('Restart the entire system? This will cause a brief interruption in service.')) {
                showNotification('System restart initiated. Please wait...', 'warning');
                // In a real implementation, this would trigger a system restart
            }
        }

        function factoryReset() {
            if (confirm('Perform factory reset? This will erase all configuration and cannot be undone.')) {
                if (confirm('Are you absolutely sure? This action is irreversible.')) {
                    showNotification('Factory reset initiated. Device will restart with default settings.', 'warning');
                    // In a real implementation, this would trigger a factory reset
                }
            }
        }

        // Show notification
        function showNotification(message, type) {
            const notification = document.createElement('div');
            notification.className = `alert alert-${type}`;
            notification.textContent = message;
            notification.style.position = 'fixed';
            notification.style.top = '20px';
            notification.style.right = '20px';
            notification.style.zIndex = '10000';
            notification.style.minWidth = '300px';
            notification.style.padding = '1rem';
            notification.style.borderRadius = '8px';
            notification.style.boxShadow = '0 4px 20px rgba(0, 0, 0, 0.2)';
            
            document.body.appendChild(notification);
            
            setTimeout(() => {
                document.body.removeChild(notification);
            }, 5000);
        }

        // Utility functions
        function formatUptime(seconds) {
            const days = Math.floor(seconds / 86400);
            const hours = Math.floor((seconds % 86400) / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = Math.floor(seconds % 60);
            
            if (days > 0) {
                return `${days}d ${hours}h ${minutes}m`;
            } else if (hours > 0) {
                return `${hours}h ${minutes}m ${secs}s`;
            } else if (minutes > 0) {
                return `${minutes}m ${secs}s`;
            } else {
                return `${secs}s`;
            }
        }

        function formatBytes(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }

        // Cleanup on page unload
        window.addEventListener('beforeunload', function() {
            if (statusUpdateInterval) clearInterval(statusUpdateInterval);
            if (logUpdateInterval) clearInterval(logUpdateInterval);
        });
    </script>
</body>
</html>
)====="