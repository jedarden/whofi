/**
 * Settings Component
 * System configuration and calibration
 */

import React, { useState } from 'react';
import {
  Box,
  Typography,
  Grid,
  Card,
  CardContent,
  TextField,
  Button,
  Switch,
  FormControlLabel,
  Slider,
  Divider,
  Alert,
  Snackbar
} from '@mui/material';
import {
  Save as SaveIcon,
  RestoreFromTrash as ResetIcon
} from '@mui/icons-material';

import { useConfig, useApiMutation } from '../hooks/useApi';

const Settings = () => {
  const { data: configData } = useConfig();
  const mutation = useApiMutation();
  const [notification, setNotification] = useState(null);
  
  const [config, setConfig] = useState({
    positioning: {
      algorithm: 'trilateration',
      minNodes: 3,
      updateRate: 1000,
      smoothing: true
    },
    calibration: {
      pathLossExponent: 2.0,
      referenceRSSI: -30,
      frequency: 2400
    },
    nodes: {
      esp32_001: { x: 0, y: 0 },
      esp32_002: { x: 10, y: 0 },
      esp32_003: { x: 5, y: 10 },
      esp32_004: { x: 0, y: 10 }
    }
  });

  React.useEffect(() => {
    if (configData) {
      setConfig(configData);
    }
  }, [configData]);

  const handleSave = async () => {
    try {
      await mutation.mutateAsync({
        method: 'PUT',
        url: '/api/config',
        data: config
      });
      setNotification({ type: 'success', message: 'Configuration saved successfully!' });
    } catch (error) {
      setNotification({ type: 'error', message: 'Failed to save configuration' });
    }
  };

  const handleReset = () => {
    setConfig({
      positioning: {
        algorithm: 'trilateration',
        minNodes: 3,
        updateRate: 1000,
        smoothing: true
      },
      calibration: {
        pathLossExponent: 2.0,
        referenceRSSI: -30,
        frequency: 2400
      },
      nodes: {
        esp32_001: { x: 0, y: 0 },
        esp32_002: { x: 10, y: 0 },
        esp32_003: { x: 5, y: 10 },
        esp32_004: { x: 0, y: 10 }
      }
    });
    setNotification({ type: 'info', message: 'Configuration reset to defaults' });
  };

  const updateConfig = (path, value) => {
    const newConfig = { ...config };
    const pathArray = path.split('.');
    let current = newConfig;
    
    for (let i = 0; i < pathArray.length - 1; i++) {
      current = current[pathArray[i]];
    }
    
    current[pathArray[pathArray.length - 1]] = value;
    setConfig(newConfig);
  };

  const updateNodePosition = (nodeId, axis, value) => {
    setConfig(prev => ({
      ...prev,
      nodes: {
        ...prev.nodes,
        [nodeId]: {
          ...prev.nodes[nodeId],
          [axis]: parseFloat(value)
        }
      }
    }));
  };

  return (
    <Box>
      <Box display="flex" alignItems="center" justifyContent="space-between" mb={3}>
        <Typography variant="h4">
          System Settings
        </Typography>
        <Box>
          <Button
            startIcon={<ResetIcon />}
            onClick={handleReset}
            variant="outlined"
            sx={{ mr: 1 }}
          >
            Reset to Defaults
          </Button>
          <Button
            startIcon={<SaveIcon />}
            onClick={handleSave}
            variant="contained"
            disabled={mutation.isLoading}
          >
            Save Changes
          </Button>
        </Box>
      </Box>

      <Grid container spacing={3}>
        {/* Positioning Settings */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Positioning Algorithm
              </Typography>
              
              <TextField
                fullWidth
                label="Algorithm Type"
                value={config.positioning?.algorithm || ''}
                disabled
                margin="normal"
                helperText="Currently only trilateration is supported"
              />
              
              <TextField
                fullWidth
                label="Minimum Nodes Required"
                type="number"
                value={config.positioning?.minNodes || 3}
                onChange={(e) => updateConfig('positioning.minNodes', parseInt(e.target.value))}
                margin="normal"
                inputProps={{ min: 3, max: 10 }}
              />
              
              <TextField
                fullWidth
                label="Update Rate (ms)"
                type="number"
                value={config.positioning?.updateRate || 1000}
                onChange={(e) => updateConfig('positioning.updateRate', parseInt(e.target.value))}
                margin="normal"
                inputProps={{ min: 100, max: 5000, step: 100 }}
              />
              
              <FormControlLabel
                control={
                  <Switch
                    checked={config.positioning?.smoothing || false}
                    onChange={(e) => updateConfig('positioning.smoothing', e.target.checked)}
                  />
                }
                label="Enable Position Smoothing"
                sx={{ mt: 2 }}
              />
            </CardContent>
          </Card>
        </Grid>

        {/* Calibration Settings */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                RF Calibration
              </Typography>
              
              <Box mb={3}>
                <Typography gutterBottom>
                  Path Loss Exponent: {config.calibration?.pathLossExponent || 2.0}
                </Typography>
                <Slider
                  value={config.calibration?.pathLossExponent || 2.0}
                  onChange={(e, value) => updateConfig('calibration.pathLossExponent', value)}
                  min={1.5}
                  max={4.0}
                  step={0.1}
                  marks={[
                    { value: 2.0, label: '2.0 (Free Space)' },
                    { value: 3.0, label: '3.0 (Indoor)' }
                  ]}
                />
              </Box>

              <TextField
                fullWidth
                label="Reference RSSI (dBm)"
                type="number"
                value={config.calibration?.referenceRSSI || -30}
                onChange={(e) => updateConfig('calibration.referenceRSSI', parseInt(e.target.value))}
                margin="normal"
                inputProps={{ min: -100, max: 0 }}
              />
              
              <TextField
                fullWidth
                label="Frequency (MHz)"
                type="number"
                value={config.calibration?.frequency || 2400}
                onChange={(e) => updateConfig('calibration.frequency', parseInt(e.target.value))}
                margin="normal"
                inputProps={{ min: 2400, max: 2500 }}
              />
            </CardContent>
          </Card>
        </Grid>

        {/* Node Positions */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Node Physical Positions
              </Typography>
              <Typography variant="body2" color="textSecondary" gutterBottom>
                Set the physical coordinates of each ESP32 node in your room (in meters)
              </Typography>
              
              <Grid container spacing={2} mt={2}>
                {Object.entries(config.nodes || {}).map(([nodeId, position]) => (
                  <Grid item xs={12} sm={6} md={3} key={nodeId}>
                    <Card variant="outlined">
                      <CardContent>
                        <Typography variant="subtitle1" gutterBottom>
                          {nodeId}
                        </Typography>
                        
                        <TextField
                          fullWidth
                          label="X Position (m)"
                          type="number"
                          value={position.x || 0}
                          onChange={(e) => updateNodePosition(nodeId, 'x', e.target.value)}
                          margin="dense"
                          inputProps={{ step: 0.1 }}
                        />
                        
                        <TextField
                          fullWidth
                          label="Y Position (m)"
                          type="number"
                          value={position.y || 0}
                          onChange={(e) => updateNodePosition(nodeId, 'y', e.target.value)}
                          margin="dense"
                          inputProps={{ step: 0.1 }}
                        />
                      </CardContent>
                    </Card>
                  </Grid>
                ))}
              </Grid>
            </CardContent>
          </Card>
        </Grid>

        {/* Advanced Settings */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Advanced Configuration
              </Typography>
              
              <Alert severity="info" sx={{ mb: 2 }}>
                These settings affect system performance. Only modify if you understand the implications.
              </Alert>
              
              <Grid container spacing={3}>
                <Grid item xs={12} md={4}>
                  <TextField
                    fullWidth
                    label="CSI Buffer Size"
                    type="number"
                    defaultValue={100}
                    margin="normal"
                    helperText="Number of CSI samples to buffer per node"
                  />
                </Grid>
                
                <Grid item xs={12} md={4}>
                  <TextField
                    fullWidth
                    label="Position History Size"
                    type="number"
                    defaultValue={50}
                    margin="normal"
                    helperText="Number of positions to keep in memory"
                  />
                </Grid>
                
                <Grid item xs={12} md={4}>
                  <TextField
                    fullWidth
                    label="WebSocket Update Rate (ms)"
                    type="number"
                    defaultValue={500}
                    margin="normal"
                    helperText="Rate of real-time updates to clients"
                  />
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>
      </Grid>

      {/* Notification Snackbar */}
      <Snackbar
        open={!!notification}
        autoHideDuration={6000}
        onClose={() => setNotification(null)}
        anchorOrigin={{ vertical: 'bottom', horizontal: 'right' }}
      >
        {notification && (
          <Alert
            onClose={() => setNotification(null)}
            severity={notification.type}
            sx={{ width: '100%' }}
          >
            {notification.message}
          </Alert>
        )}
      </Snackbar>
    </Box>
  );
};

export default Settings;