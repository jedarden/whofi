// index_html.h - Main dashboard page
R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CSI Positioning System</title>
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
        
        .nav-links a:hover {
            color: #667eea;
        }
        
        .main-content {
            padding: 2rem 0;
        }
        
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 2rem;
            margin-bottom: 2rem;
        }
        
        .card {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 1.5rem;
            transition: transform 0.3s, box-shadow 0.3s;
        }
        
        .card:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 30px rgba(0, 0, 0, 0.15);
        }
        
        .card-header {
            display: flex;
            align-items: center;
            margin-bottom: 1rem;
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
        
        .status-icon {
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
        
        .card-title {
            font-size: 1.1rem;
            font-weight: 600;
            color: #333;
        }
        
        .metric {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0.5rem 0;
            border-bottom: 1px solid #f0f0f0;
        }
        
        .metric:last-child {
            border-bottom: none;
        }
        
        .metric-label {
            color: #666;
            font-size: 0.9rem;
        }
        
        .metric-value {
            font-weight: 600;
            color: #333;
        }
        
        .status-indicator {
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 0.5rem;
        }
        
        .status-online {
            background: #4CAF50;
            box-shadow: 0 0 10px rgba(76, 175, 80, 0.5);
        }
        
        .status-offline {
            background: #f44336;
        }
        
        .chart-container {
            grid-column: 1 / -1;
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 1.5rem;
        }
        
        .chart {
            width: 100%;
            height: 300px;
            background: #f8f9fa;
            border-radius: 8px;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #666;
            font-size: 1.1rem;
        }
        
        .btn {
            background: linear-gradient(135deg, #667eea, #764ba2);
            color: white;
            border: none;
            padding: 0.75rem 1.5rem;
            border-radius: 8px;
            font-weight: 500;
            cursor: pointer;
            transition: transform 0.2s;
        }
        
        .btn:hover {
            transform: translateY(-1px);
        }
        
        .btn-secondary {
            background: #f8f9fa;
            color: #333;
            border: 1px solid #dee2e6;
        }
        
        .controls {
            display: flex;
            gap: 1rem;
            margin-top: 1rem;
        }
        
        @media (max-width: 768px) {
            .nav {
                flex-direction: column;
                gap: 1rem;
            }
            
            .nav-links {
                gap: 1rem;
            }
            
            .dashboard {
                grid-template-columns: 1fr;
            }
            
            .controls {
                flex-direction: column;
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
                    <li><a href="/status">Status</a></li>
                </ul>
            </nav>
        </div>
    </header>

    <main class="main-content">
        <div class="container">
            <div class="dashboard">
                <!-- System Status Card -->
                <div class="card">
                    <div class="card-header">
                        <div class="card-icon status-icon">⚡</div>
                        <h3 class="card-title">System Status</h3>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Status</span>
                        <span class="metric-value">
                            <span class="status-indicator status-online"></span>
                            <span id="system-status">Online</span>
                        </span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Uptime</span>
                        <span class="metric-value" id="system-uptime">0s</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Free Memory</span>
                        <span class="metric-value" id="system-memory">0 KB</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Version</span>
                        <span class="metric-value" id="system-version">1.0.0</span>
                    </div>
                </div>

                <!-- CSI Data Card -->
                <div class="card">
                    <div class="card-header">
                        <div class="card-icon csi-icon">📡</div>
                        <h3 class="card-title">CSI Collector</h3>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Status</span>
                        <span class="metric-value">
                            <span class="status-indicator" id="csi-status-indicator"></span>
                            <span id="csi-status">Stopped</span>
                        </span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Packets Received</span>
                        <span class="metric-value" id="csi-packets-received">0</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Packets Processed</span>
                        <span class="metric-value" id="csi-packets-processed">0</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Average RSSI</span>
                        <span class="metric-value" id="csi-avg-rssi">0 dBm</span>
                    </div>
                </div>

                <!-- WiFi Status Card -->
                <div class="card">
                    <div class="card-header">
                        <div class="card-icon wifi-icon">📶</div>
                        <h3 class="card-title">WiFi Connection</h3>
                    </div>
                    <div class="metric">
                        <span class="metric-label">SSID</span>
                        <span class="metric-value" id="wifi-ssid">Not connected</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Signal Strength</span>
                        <span class="metric-value" id="wifi-rssi">0 dBm</span>
                    </div>
                    <div class="metric">
                        <span class="metric-label">Channel</span>
                        <span class="metric-value" id="wifi-channel">0</span>
                    </div>
                </div>
            </div>

            <!-- Real-time Chart -->
            <div class="chart-container">
                <div class="card-header">
                    <div class="card-icon csi-icon">📊</div>
                    <h3 class="card-title">Real-time CSI Data</h3>
                </div>
                <div class="chart" id="csi-chart">
                    Connect to see real-time CSI amplitude data
                </div>
                <div class="controls">
                    <button class="btn" onclick="startRealTimeData()">Start Real-time</button>
                    <button class="btn btn-secondary" onclick="stopRealTimeData()">Stop</button>
                    <button class="btn btn-secondary" onclick="exportData()">Export Data</button>
                </div>
            </div>
        </div>
    </main>

    <script>
        let statusUpdateInterval;
        let realtimeInterval;
        let chartData = [];
        let maxDataPoints = 50;

        // Initialize dashboard
        document.addEventListener('DOMContentLoaded', function() {
            updateStatus();
            statusUpdateInterval = setInterval(updateStatus, 2000);
        });

        // Update system status
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    // System status
                    document.getElementById('system-uptime').textContent = formatUptime(data.system.uptime);
                    document.getElementById('system-memory').textContent = formatBytes(data.system.free_heap);
                    document.getElementById('system-version').textContent = data.system.version;

                    // CSI status
                    const csiRunning = data.csi.running;
                    document.getElementById('csi-status').textContent = csiRunning ? 'Running' : 'Stopped';
                    document.getElementById('csi-status-indicator').className = 
                        'status-indicator ' + (csiRunning ? 'status-online' : 'status-offline');
                    
                    if (csiRunning && data.csi.packets_received !== undefined) {
                        document.getElementById('csi-packets-received').textContent = data.csi.packets_received;
                        document.getElementById('csi-packets-processed').textContent = data.csi.packets_processed;
                        document.getElementById('csi-avg-rssi').textContent = data.csi.average_rssi.toFixed(1) + ' dBm';
                    }

                    // WiFi status
                    if (data.wifi.ssid) {
                        document.getElementById('wifi-ssid').textContent = data.wifi.ssid;
                        document.getElementById('wifi-rssi').textContent = data.wifi.rssi + ' dBm';
                        document.getElementById('wifi-channel').textContent = data.wifi.channel;
                    }
                })
                .catch(error => {
                    console.error('Error updating status:', error);
                });
        }

        // Start real-time data collection
        function startRealTimeData() {
            if (realtimeInterval) return;
            
            const chart = document.getElementById('csi-chart');
            chart.innerHTML = '<canvas id="amplitude-chart" width="100%" height="300"></canvas>';
            
            realtimeInterval = setInterval(() => {
                fetch('/api/csi-data')
                    .then(response => response.json())
                    .then(data => {
                        if (data.amplitude) {
                            updateChart(data.amplitude);
                        }
                    })
                    .catch(error => console.error('Error fetching CSI data:', error));
            }, 500);
        }

        // Stop real-time data collection
        function stopRealTimeData() {
            if (realtimeInterval) {
                clearInterval(realtimeInterval);
                realtimeInterval = null;
            }
        }

        // Update chart with new data
        function updateChart(amplitudeData) {
            // Simple ASCII-style chart representation
            const chart = document.getElementById('csi-chart');
            let chartText = 'Subcarrier Amplitude:\n\n';
            
            const maxAmp = Math.max(...amplitudeData);
            const scale = 40; // ASCII chart width
            
            amplitudeData.slice(0, 20).forEach((amp, i) => {
                const barLength = Math.round((amp / maxAmp) * scale);
                const bar = '█'.repeat(barLength) + '░'.repeat(scale - barLength);
                chartText += `${i.toString().padStart(2)}: ${bar} ${amp.toFixed(1)}\n`;
            });
            
            chart.innerHTML = `<pre style="font-family: monospace; font-size: 0.8rem; line-height: 1.2;">${chartText}</pre>`;
        }

        // Export data function
        function exportData() {
            fetch('/api/csi-data')
                .then(response => response.json())
                .then(data => {
                    const dataStr = JSON.stringify(data, null, 2);
                    const dataBlob = new Blob([dataStr], {type: 'application/json'});
                    const url = URL.createObjectURL(dataBlob);
                    const link = document.createElement('a');
                    link.href = url;
                    link.download = `csi-data-${Date.now()}.json`;
                    link.click();
                })
                .catch(error => console.error('Error exporting data:', error));
        }

        // Utility functions
        function formatUptime(seconds) {
            const hours = Math.floor(seconds / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = Math.floor(seconds % 60);
            
            if (hours > 0) {
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
            if (realtimeInterval) clearInterval(realtimeInterval);
        });
    </script>
</body>
</html>
)====="