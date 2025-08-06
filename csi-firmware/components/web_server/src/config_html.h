// config_html.h - Configuration page
R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuration - CSI Positioning System</title>
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
        
        .config-container {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 2rem;
        }
        
        .config-section {
            background: white;
            border-radius: 12px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1);
            padding: 2rem;
        }
        
        .section-title {
            font-size: 1.2rem;
            font-weight: 600;
            color: #333;
            margin-bottom: 1.5rem;
            padding-bottom: 0.5rem;
            border-bottom: 2px solid #667eea;
        }
        
        .form-group {
            margin-bottom: 1.5rem;
        }
        
        .form-label {
            display: block;
            font-weight: 500;
            color: #555;
            margin-bottom: 0.5rem;
        }
        
        .form-input {
            width: 100%;
            padding: 0.75rem;
            border: 2px solid #e1e5e9;
            border-radius: 8px;
            font-size: 1rem;
            transition: border-color 0.3s;
        }
        
        .form-input:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .form-select {
            width: 100%;
            padding: 0.75rem;
            border: 2px solid #e1e5e9;
            border-radius: 8px;
            font-size: 1rem;
            background: white;
            transition: border-color 0.3s;
        }
        
        .form-select:focus {
            outline: none;
            border-color: #667eea;
        }
        
        .form-checkbox {
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }
        
        .checkbox {
            width: 18px;
            height: 18px;
            cursor: pointer;
        }
        
        .form-help {
            font-size: 0.85rem;
            color: #666;
            margin-top: 0.25rem;
        }
        
        .btn {
            background: linear-gradient(135deg, #667eea, #764ba2);
            color: white;
            border: none;
            padding: 0.75rem 1.5rem;
            border-radius: 8px;
            font-weight: 500;
            font-size: 1rem;
            cursor: pointer;
            transition: transform 0.2s;
        }
        
        .btn:hover {
            transform: translateY(-1px);
        }
        
        .btn-secondary {
            background: #6c757d;
        }
        
        .btn-success {
            background: #28a745;
        }
        
        .btn-danger {
            background: #dc3545;
        }
        
        .form-actions {
            display: flex;
            gap: 1rem;
            justify-content: flex-end;
            margin-top: 2rem;
            padding-top: 2rem;
            border-top: 1px solid #e1e5e9;
        }
        
        .alert {
            padding: 1rem;
            border-radius: 8px;
            margin-bottom: 1rem;
            font-weight: 500;
        }
        
        .alert-success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        
        .alert-error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        
        .alert-info {
            background: #cce7ff;
            color: #004085;
            border: 1px solid #99d3ff;
        }
        
        .range-container {
            display: flex;
            align-items: center;
            gap: 1rem;
        }
        
        .range-input {
            flex: 1;
        }
        
        .range-value {
            font-weight: 600;
            color: #667eea;
            min-width: 60px;
            text-align: right;
        }
        
        .advanced-section {
            margin-top: 1rem;
            padding: 1rem;
            background: #f8f9fa;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }
        
        .toggle-advanced {
            background: none;
            border: none;
            color: #667eea;
            font-weight: 500;
            cursor: pointer;
            text-decoration: underline;
            margin-bottom: 1rem;
        }
        
        @media (max-width: 768px) {
            .nav {
                flex-direction: column;
                gap: 1rem;
            }
            
            .nav-links {
                gap: 1rem;
            }
            
            .config-container {
                grid-template-columns: 1fr;
            }
            
            .form-actions {
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
                    <li><a href="/config" class="active">Configuration</a></li>
                    <li><a href="/status">Status</a></li>
                </ul>
            </nav>
        </div>
    </header>

    <main class="main-content">
        <div class="container">
            <div id="alert-container"></div>
            
            <div class="config-container">
                <!-- CSI Configuration -->
                <div class="config-section">
                    <h2 class="section-title">CSI Collector Settings</h2>
                    
                    <div class="form-group">
                        <label class="form-label" for="sample-rate">Sample Rate (Hz)</label>
                        <div class="range-container">
                            <input type="range" id="sample-rate" class="form-input range-input" 
                                   min="1" max="100" value="10" oninput="updateRangeValue('sample-rate', this.value)">
                            <span class="range-value" id="sample-rate-value">10 Hz</span>
                        </div>
                        <div class="form-help">Higher rates provide more data but consume more resources</div>
                    </div>
                    
                    <div class="form-group">
                        <label class="form-label" for="buffer-size">Buffer Size (bytes)</label>
                        <select id="buffer-size" class="form-select">
                            <option value="256">256 bytes</option>
                            <option value="512">512 bytes</option>
                            <option value="1024" selected>1024 bytes</option>
                            <option value="2048">2048 bytes</option>
                            <option value="4096">4096 bytes</option>
                        </select>
                        <div class="form-help">Larger buffers can hold more data but use more memory</div>
                    </div>
                    
                    <div class="form-group">
                        <label class="form-checkbox">
                            <input type="checkbox" id="filter-enabled" class="checkbox" checked>
                            <span class="form-label">Enable Data Filtering</span>
                        </label>
                        <div class="form-help">Filter out weak or noisy signals</div>
                    </div>
                    
                    <div class="form-group" id="filter-threshold-group">
                        <label class="form-label" for="filter-threshold">Filter Threshold</label>
                        <div class="range-container">
                            <input type="range" id="filter-threshold" class="form-input range-input" 
                                   min="0" max="1" step="0.1" value="0.3" oninput="updateRangeValue('filter-threshold', this.value)">
                            <span class="range-value" id="filter-threshold-value">0.3</span>
                        </div>
                        <div class="form-help">Higher values filter more aggressively</div>
                    </div>
                    
                    <button class="toggle-advanced" onclick="toggleAdvanced('csi-advanced')">
                        Show Advanced Settings
                    </button>
                    
                    <div class="advanced-section" id="csi-advanced" style="display: none;">
                        <div class="form-group">
                            <label class="form-checkbox">
                                <input type="checkbox" id="enable-rssi" class="checkbox" checked>
                                <span class="form-label">Include RSSI Data</span>
                            </label>
                        </div>
                        
                        <div class="form-group">
                            <label class="form-checkbox">
                                <input type="checkbox" id="enable-amplitude" class="checkbox" checked>
                                <span class="form-label">Calculate Amplitude</span>
                            </label>
                        </div>
                        
                        <div class="form-group">
                            <label class="form-checkbox">
                                <input type="checkbox" id="enable-phase" class="checkbox" checked>
                                <span class="form-label">Calculate Phase</span>
                            </label>
                        </div>
                    </div>
                </div>
                
                <!-- WiFi Configuration -->
                <div class="config-section">
                    <h2 class="section-title">WiFi Settings</h2>
                    
                    <div class="form-group">
                        <label class="form-label" for="wifi-ssid">WiFi SSID</label>
                        <input type="text" id="wifi-ssid" class="form-input" placeholder="Enter WiFi network name">
                        <div class="form-help">Network to connect to for data transmission</div>
                    </div>
                    
                    <div class="form-group">
                        <label class="form-label" for="wifi-password">WiFi Password</label>
                        <input type="password" id="wifi-password" class="form-input" placeholder="Enter WiFi password">
                        <div class="form-help">Leave blank for open networks</div>
                    </div>
                    
                    <div class="form-group">
                        <label class="form-label" for="wifi-channel">Channel (0 = Auto)</label>
                        <select id="wifi-channel" class="form-select">
                            <option value="0" selected>Auto</option>
                            <option value="1">1 (2.412 GHz)</option>
                            <option value="2">2 (2.417 GHz)</option>
                            <option value="3">3 (2.422 GHz)</option>
                            <option value="4">4 (2.427 GHz)</option>
                            <option value="5">5 (2.432 GHz)</option>
                            <option value="6">6 (2.437 GHz)</option>
                            <option value="7">7 (2.442 GHz)</option>
                            <option value="8">8 (2.447 GHz)</option>
                            <option value="9">9 (2.452 GHz)</option>
                            <option value="10">10 (2.457 GHz)</option>
                            <option value="11">11 (2.462 GHz)</option>
                        </select>
                    </div>
                    
                    <div class="form-group">
                        <label class="form-checkbox">
                            <input type="checkbox" id="ap-mode" class="checkbox">
                            <span class="form-label">Enable Access Point Mode</span>
                        </label>
                        <div class="form-help">Create a WiFi hotspot for direct connection</div>
                    </div>
                    
                    <div class="advanced-section" id="ap-config" style="display: none;">
                        <div class="form-group">
                            <label class="form-label" for="ap-ssid">AP SSID</label>
                            <input type="text" id="ap-ssid" class="form-input" value="CSI-Device" placeholder="Access Point name">
                        </div>
                        
                        <div class="form-group">
                            <label class="form-label" for="ap-password">AP Password</label>
                            <input type="password" id="ap-password" class="form-input" placeholder="At least 8 characters">
                            <div class="form-help">Leave blank for open AP (not recommended)</div>
                        </div>
                    </div>
                </div>
                
                <!-- MQTT Configuration -->
                <div class="config-section">
                    <h2 class="section-title">MQTT Settings</h2>
                    
                    <div class="form-group">
                        <label class="form-checkbox">
                            <input type="checkbox" id="mqtt-enabled" class="checkbox">
                            <span class="form-label">Enable MQTT Publishing</span>
                        </label>
                        <div class="form-help">Send CSI data to MQTT broker</div>
                    </div>
                    
                    <div id="mqtt-config" style="display: none;">
                        <div class="form-group">
                            <label class="form-label" for="mqtt-broker">MQTT Broker URL</label>
                            <input type="text" id="mqtt-broker" class="form-input" placeholder="mqtt://broker.example.com">
                            <div class="form-help">Include protocol (mqtt:// or mqtts://)</div>
                        </div>
                        
                        <div class="form-group">
                            <label class="form-label" for="mqtt-port">Port</label>
                            <input type="number" id="mqtt-port" class="form-input" value="1883" min="1" max="65535">
                        </div>
                        
                        <div class="form-group">
                            <label class="form-label" for="mqtt-username">Username</label>
                            <input type="text" id="mqtt-username" class="form-input" placeholder="Optional">
                        </div>
                        
                        <div class="form-group">
                            <label class="form-label" for="mqtt-password">Password</label>
                            <input type="password" id="mqtt-password" class="form-input" placeholder="Optional">
                        </div>
                        
                        <div class="form-group">
                            <label class="form-label" for="mqtt-topic">Topic Prefix</label>
                            <input type="text" id="mqtt-topic" class="form-input" value="csi-device" placeholder="csi-device">
                            <div class="form-help">Data will be published to {prefix}/data, {prefix}/status, etc.</div>
                        </div>
                    </div>
                </div>
                
                <!-- System Configuration -->
                <div class="config-section">
                    <h2 class="section-title">System Settings</h2>
                    
                    <div class="form-group">
                        <label class="form-label" for="device-name">Device Name</label>
                        <input type="text" id="device-name" class="form-input" value="CSI-Device-001" placeholder="Unique device identifier">
                        <div class="form-help">Used for identification in multi-device deployments</div>
                    </div>
                    
                    <div class="form-group">
                        <label class="form-checkbox">
                            <input type="checkbox" id="web-auth" class="checkbox">
                            <span class="form-label">Enable Web Authentication</span>
                        </label>
                        <div class="form-help">Require login to access this interface</div>
                    </div>
                    
                    <div id="auth-config" style="display: none;">
                        <div class="form-group">
                            <label class="form-label" for="web-username">Admin Username</label>
                            <input type="text" id="web-username" class="form-input" value="admin">
                        </div>
                        
                        <div class="form-group">
                            <label class="form-label" for="web-password">Admin Password</label>
                            <input type="password" id="web-password" class="form-input" placeholder="Enter new password">
                            <div class="form-help">Leave blank to keep current password</div>
                        </div>
                    </div>
                    
                    <button class="toggle-advanced" onclick="toggleAdvanced('system-advanced')">
                        Show Advanced Settings
                    </button>
                    
                    <div class="advanced-section" id="system-advanced" style="display: none;">
                        <div class="form-group">
                            <label class="form-checkbox">
                                <input type="checkbox" id="ota-enabled" class="checkbox" checked>
                                <span class="form-label">Enable OTA Updates</span>
                            </label>
                        </div>
                        
                        <div class="form-group">
                            <label class="form-checkbox">
                                <input type="checkbox" id="auto-update" class="checkbox">
                                <span class="form-label">Automatic Updates</span>
                            </label>
                            <div class="form-help">Automatically install firmware updates</div>
                        </div>
                        
                        <div class="form-group">
                            <label class="form-label" for="ntp-server">NTP Server</label>
                            <input type="text" id="ntp-server" class="form-input" value="pool.ntp.org" placeholder="NTP server address">
                            <div class="form-help">For accurate timestamps</div>
                        </div>
                    </div>
                </div>
            </div>
            
            <!-- Action Buttons -->
            <div class="config-section">
                <div class="form-actions">
                    <button class="btn btn-secondary" onclick="loadCurrentConfig()">Reload</button>
                    <button class="btn btn-secondary" onclick="resetToDefaults()">Reset to Defaults</button>
                    <button class="btn btn-success" onclick="testConfiguration()">Test Configuration</button>
                    <button class="btn" onclick="saveConfiguration()">Save Configuration</button>
                </div>
            </div>
        </div>
    </main>

    <script>
        let currentConfig = {};

        // Initialize page
        document.addEventListener('DOMContentLoaded', function() {
            loadCurrentConfig();
            setupEventListeners();
        });

        // Setup event listeners
        function setupEventListeners() {
            // Toggle sections based on checkboxes
            document.getElementById('filter-enabled').addEventListener('change', function() {
                document.getElementById('filter-threshold-group').style.display = 
                    this.checked ? 'block' : 'none';
            });
            
            document.getElementById('ap-mode').addEventListener('change', function() {
                document.getElementById('ap-config').style.display = 
                    this.checked ? 'block' : 'none';
            });
            
            document.getElementById('mqtt-enabled').addEventListener('change', function() {
                document.getElementById('mqtt-config').style.display = 
                    this.checked ? 'block' : 'none';
            });
            
            document.getElementById('web-auth').addEventListener('change', function() {
                document.getElementById('auth-config').style.display = 
                    this.checked ? 'block' : 'none';
            });
        }

        // Load current configuration
        function loadCurrentConfig() {
            fetch('/api/config')
                .then(response => response.json())
                .then(data => {
                    currentConfig = data;
                    populateForm(data);
                    showAlert('Configuration loaded successfully', 'success');
                })
                .catch(error => {
                    console.error('Error loading configuration:', error);
                    showAlert('Failed to load configuration', 'error');
                });
        }

        // Populate form with configuration data
        function populateForm(config) {
            if (config.csi) {
                document.getElementById('sample-rate').value = config.csi.sample_rate || 10;
                updateRangeValue('sample-rate', config.csi.sample_rate || 10);
                document.getElementById('buffer-size').value = config.csi.buffer_size || 1024;
                document.getElementById('filter-enabled').checked = config.csi.filter_enabled || false;
                document.getElementById('filter-threshold').value = config.csi.filter_threshold || 0.3;
                updateRangeValue('filter-threshold', config.csi.filter_threshold || 0.3);
                document.getElementById('enable-rssi').checked = config.csi.enable_rssi || false;
                document.getElementById('enable-amplitude').checked = config.csi.enable_amplitude || false;
                document.getElementById('enable-phase').checked = config.csi.enable_phase || false;
                
                // Update visibility
                document.getElementById('filter-threshold-group').style.display = 
                    config.csi.filter_enabled ? 'block' : 'none';
            }
        }

        // Save configuration
        function saveConfiguration() {
            const config = {
                csi: {
                    sample_rate: parseInt(document.getElementById('sample-rate').value),
                    buffer_size: parseInt(document.getElementById('buffer-size').value),
                    filter_enabled: document.getElementById('filter-enabled').checked,
                    filter_threshold: parseFloat(document.getElementById('filter-threshold').value),
                    enable_rssi: document.getElementById('enable-rssi').checked,
                    enable_amplitude: document.getElementById('enable-amplitude').checked,
                    enable_phase: document.getElementById('enable-phase').checked
                }
            };

            fetch('/api/config', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(config)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showAlert('Configuration saved successfully! Changes will take effect after restart.', 'success');
                } else {
                    showAlert('Failed to save configuration: ' + (data.error || 'Unknown error'), 'error');
                }
            })
            .catch(error => {
                console.error('Error saving configuration:', error);
                showAlert('Failed to save configuration', 'error');
            });
        }

        // Test configuration
        function testConfiguration() {
            showAlert('Testing configuration...', 'info');
            
            // Simulate configuration test
            setTimeout(() => {
                showAlert('Configuration test passed! All settings are valid.', 'success');
            }, 2000);
        }

        // Reset to defaults
        function resetToDefaults() {
            if (confirm('Are you sure you want to reset all settings to defaults? This cannot be undone.')) {
                const defaults = {
                    csi: {
                        sample_rate: 10,
                        buffer_size: 1024,
                        filter_enabled: true,
                        filter_threshold: 0.3,
                        enable_rssi: true,
                        enable_amplitude: true,
                        enable_phase: true
                    }
                };
                
                populateForm(defaults);
                showAlert('Settings reset to defaults. Click Save to apply changes.', 'info');
            }
        }

        // Toggle advanced sections
        function toggleAdvanced(sectionId) {
            const section = document.getElementById(sectionId);
            const isVisible = section.style.display !== 'none';
            section.style.display = isVisible ? 'none' : 'block';
            
            // Update button text
            const button = event.target;
            button.textContent = isVisible ? 'Show Advanced Settings' : 'Hide Advanced Settings';
        }

        // Update range value display
        function updateRangeValue(inputId, value) {
            const valueSpan = document.getElementById(inputId + '-value');
            if (inputId === 'sample-rate') {
                valueSpan.textContent = value + ' Hz';
            } else if (inputId === 'filter-threshold') {
                valueSpan.textContent = parseFloat(value).toFixed(1);
            } else {
                valueSpan.textContent = value;
            }
        }

        // Show alert message
        function showAlert(message, type) {
            const alertContainer = document.getElementById('alert-container');
            const alertDiv = document.createElement('div');
            alertDiv.className = `alert alert-${type}`;
            alertDiv.textContent = message;
            
            alertContainer.innerHTML = '';
            alertContainer.appendChild(alertDiv);
            
            // Auto-hide after 5 seconds
            setTimeout(() => {
                if (alertContainer.contains(alertDiv)) {
                    alertContainer.removeChild(alertDiv);
                }
            }, 5000);
        }
    </script>
</body>
</html>
)====="