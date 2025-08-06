/**
 * @file NodeManagement.js
 * @brief Node management interface with secure API configuration
 */

import React, { useState, useEffect } from 'react';
import {
    Box, Card, CardContent, Typography, Grid, Button,
    TextField, Dialog, DialogTitle, DialogContent, DialogActions,
    IconButton, Chip, Switch, FormControlLabel, Alert,
    Table, TableBody, TableCell, TableContainer, TableHead, TableRow,
    Paper, Tooltip, InputAdornment, Snackbar
} from '@mui/material';
import {
    Edit as EditIcon,
    Refresh as RefreshIcon,
    Security as SecurityIcon,
    CloudUpload as UpdateIcon,
    Settings as SettingsIcon,
    ContentCopy as CopyIcon,
    Visibility as VisibilityIcon,
    VisibilityOff as VisibilityOffIcon,
    CheckCircle as OnlineIcon,
    Cancel as OfflineIcon
} from '@mui/icons-material';
import axios from 'axios';

const NodeManagement = () => {
    const [nodes, setNodes] = useState([]);
    const [selectedNode, setSelectedNode] = useState(null);
    const [configDialog, setConfigDialog] = useState(false);
    const [apiKeyDialog, setApiKeyDialog] = useState(false);
    const [otaDialog, setOtaDialog] = useState(false);
    const [loading, setLoading] = useState(false);
    const [snackbar, setSnackbar] = useState({ open: false, message: '', severity: 'info' });
    const [showApiKeys, setShowApiKeys] = useState({});

    useEffect(() => {
        fetchNodes();
        const interval = setInterval(fetchNodes, 10000); // Refresh every 10s
        return () => clearInterval(interval);
    }, []);

    const fetchNodes = async () => {
        try {
            const response = await axios.get('/api/nodes');
            setNodes(response.data);
        } catch (error) {
            console.error('Failed to fetch nodes:', error);
        }
    };

    const handleNodeConfig = async (nodeId) => {
        setLoading(true);
        try {
            const response = await axios.get(`/api/nodes/${nodeId}/config`);
            setSelectedNode({ id: nodeId, config: response.data });
            setConfigDialog(true);
        } catch (error) {
            showSnackbar('Failed to fetch node configuration', 'error');
        } finally {
            setLoading(false);
        }
    };

    const handleUpdateConfig = async (config) => {
        setLoading(true);
        try {
            await axios.post(`/api/nodes/${selectedNode.id}/config`, config);
            showSnackbar('Configuration updated successfully', 'success');
            setConfigDialog(false);
            fetchNodes();
        } catch (error) {
            showSnackbar('Failed to update configuration', 'error');
        } finally {
            setLoading(false);
        }
    };

    const handleSetApiKey = async (nodeId, apiKey) => {
        try {
            await axios.post(`/api/nodes/${nodeId}/api-key`, { apiKey });
            showSnackbar('API key saved successfully', 'success');
            setApiKeyDialog(false);
        } catch (error) {
            showSnackbar('Failed to save API key', 'error');
        }
    };

    const handleOtaUpdate = async (nodeId, updateUrl, version) => {
        setLoading(true);
        try {
            await axios.post(`/api/nodes/${nodeId}/ota`, { updateUrl, version });
            showSnackbar('OTA update initiated', 'info');
            setOtaDialog(false);
        } catch (error) {
            showSnackbar('Failed to initiate OTA update', 'error');
        } finally {
            setLoading(false);
        }
    };

    const copyToClipboard = (text) => {
        navigator.clipboard.writeText(text);
        showSnackbar('Copied to clipboard', 'info');
    };

    const showSnackbar = (message, severity = 'info') => {
        setSnackbar({ open: true, message, severity });
    };

    return (
        <Box>
            <Typography variant="h4" gutterBottom>
                Node Management
            </Typography>

            <TableContainer component={Paper}>
                <Table>
                    <TableHead>
                        <TableRow>
                            <TableCell>Node ID</TableCell>
                            <TableCell>Name</TableCell>
                            <TableCell>IP Address</TableCell>
                            <TableCell>Status</TableCell>
                            <TableCell>Position</TableCell>
                            <TableCell>API Key</TableCell>
                            <TableCell>Actions</TableCell>
                        </TableRow>
                    </TableHead>
                    <TableBody>
                        {nodes.map((node) => (
                            <TableRow key={node.id}>
                                <TableCell>{node.id}</TableCell>
                                <TableCell>{node.name || 'Unnamed'}</TableCell>
                                <TableCell>{node.ip}</TableCell>
                                <TableCell>
                                    <Chip
                                        icon={node.online ? <OnlineIcon /> : <OfflineIcon />}
                                        label={node.online ? 'Online' : 'Offline'}
                                        color={node.online ? 'success' : 'default'}
                                        size="small"
                                    />
                                </TableCell>
                                <TableCell>
                                    {node.position ? 
                                        `(${node.position.x}, ${node.position.y}, ${node.position.z})` : 
                                        'Not set'
                                    }
                                </TableCell>
                                <TableCell>
                                    {node.hasApiKey ? (
                                        <Box display="flex" alignItems="center">
                                            <Chip 
                                                icon={<SecurityIcon />} 
                                                label="Configured" 
                                                size="small" 
                                                color="primary"
                                            />
                                        </Box>
                                    ) : (
                                        <Chip label="Not set" size="small" />
                                    )}
                                </TableCell>
                                <TableCell>
                                    <Tooltip title="Configure">
                                        <IconButton 
                                            onClick={() => handleNodeConfig(node.id)}
                                            disabled={!node.online}
                                        >
                                            <SettingsIcon />
                                        </IconButton>
                                    </Tooltip>
                                    <Tooltip title="Set API Key">
                                        <IconButton 
                                            onClick={() => {
                                                setSelectedNode(node);
                                                setApiKeyDialog(true);
                                            }}
                                        >
                                            <SecurityIcon />
                                        </IconButton>
                                    </Tooltip>
                                    <Tooltip title="OTA Update">
                                        <IconButton 
                                            onClick={() => {
                                                setSelectedNode(node);
                                                setOtaDialog(true);
                                            }}
                                            disabled={!node.online || !node.hasApiKey}
                                        >
                                            <UpdateIcon />
                                        </IconButton>
                                    </Tooltip>
                                    <Tooltip title="Refresh">
                                        <IconButton onClick={() => handleNodeConfig(node.id)}>
                                            <RefreshIcon />
                                        </IconButton>
                                    </Tooltip>
                                </TableCell>
                            </TableRow>
                        ))}
                    </TableBody>
                </Table>
            </TableContainer>

            {/* Configuration Dialog */}
            <Dialog 
                open={configDialog} 
                onClose={() => setConfigDialog(false)}
                maxWidth="md"
                fullWidth
            >
                <DialogTitle>Node Configuration</DialogTitle>
                <DialogContent>
                    {selectedNode && (
                        <NodeConfigForm 
                            config={selectedNode.config}
                            onSave={handleUpdateConfig}
                            loading={loading}
                        />
                    )}
                </DialogContent>
            </Dialog>

            {/* API Key Dialog */}
            <Dialog 
                open={apiKeyDialog} 
                onClose={() => setApiKeyDialog(false)}
                maxWidth="sm"
                fullWidth
            >
                <DialogTitle>Set API Key</DialogTitle>
                <DialogContent>
                    <ApiKeyForm 
                        node={selectedNode}
                        onSave={(apiKey) => handleSetApiKey(selectedNode.id, apiKey)}
                    />
                </DialogContent>
            </Dialog>

            {/* OTA Update Dialog */}
            <Dialog 
                open={otaDialog} 
                onClose={() => setOtaDialog(false)}
                maxWidth="sm"
                fullWidth
            >
                <DialogTitle>OTA Update</DialogTitle>
                <DialogContent>
                    <OtaUpdateForm 
                        node={selectedNode}
                        onUpdate={(url, version) => handleOtaUpdate(selectedNode.id, url, version)}
                        loading={loading}
                    />
                </DialogContent>
            </Dialog>

            {/* Snackbar for notifications */}
            <Snackbar
                open={snackbar.open}
                autoHideDuration={6000}
                onClose={() => setSnackbar({ ...snackbar, open: false })}
                message={snackbar.message}
            />
        </Box>
    );
};

// Node Configuration Form Component
const NodeConfigForm = ({ config, onSave, loading }) => {
    const [formData, setFormData] = useState({
        node: {
            name: config.node?.name || '',
            position: {
                x: config.node?.position?.x || 0,
                y: config.node?.position?.y || 0,
                z: config.node?.position?.z || 0
            }
        },
        csi: {
            sample_rate: config.csi?.sample_rate || 20,
            buffer_size: config.csi?.buffer_size || 1024,
            filter_enabled: config.csi?.filter_enabled || false,
            filter_threshold: config.csi?.filter_threshold || 0.5
        },
        mqtt: {
            enabled: config.mqtt?.enabled || false,
            broker_url: config.mqtt?.broker_url || '',
            port: config.mqtt?.port || 1883,
            topic_prefix: config.mqtt?.topic_prefix || ''
        }
    });

    const handleSubmit = () => {
        onSave(formData);
    };

    return (
        <Box>
            <Typography variant="h6" gutterBottom>Node Settings</Typography>
            <Grid container spacing={2}>
                <Grid item xs={12}>
                    <TextField
                        fullWidth
                        label="Node Name"
                        value={formData.node.name}
                        onChange={(e) => setFormData({
                            ...formData,
                            node: { ...formData.node, name: e.target.value }
                        })}
                    />
                </Grid>
                <Grid item xs={4}>
                    <TextField
                        fullWidth
                        type="number"
                        label="Position X"
                        value={formData.node.position.x}
                        onChange={(e) => setFormData({
                            ...formData,
                            node: {
                                ...formData.node,
                                position: { ...formData.node.position, x: parseFloat(e.target.value) }
                            }
                        })}
                    />
                </Grid>
                <Grid item xs={4}>
                    <TextField
                        fullWidth
                        type="number"
                        label="Position Y"
                        value={formData.node.position.y}
                        onChange={(e) => setFormData({
                            ...formData,
                            node: {
                                ...formData.node,
                                position: { ...formData.node.position, y: parseFloat(e.target.value) }
                            }
                        })}
                    />
                </Grid>
                <Grid item xs={4}>
                    <TextField
                        fullWidth
                        type="number"
                        label="Position Z"
                        value={formData.node.position.z}
                        onChange={(e) => setFormData({
                            ...formData,
                            node: {
                                ...formData.node,
                                position: { ...formData.node.position, z: parseFloat(e.target.value) }
                            }
                        })}
                    />
                </Grid>
            </Grid>

            <Typography variant="h6" gutterBottom sx={{ mt: 3 }}>CSI Settings</Typography>
            <Grid container spacing={2}>
                <Grid item xs={6}>
                    <TextField
                        fullWidth
                        type="number"
                        label="Sample Rate (Hz)"
                        value={formData.csi.sample_rate}
                        onChange={(e) => setFormData({
                            ...formData,
                            csi: { ...formData.csi, sample_rate: parseInt(e.target.value) }
                        })}
                    />
                </Grid>
                <Grid item xs={6}>
                    <TextField
                        fullWidth
                        type="number"
                        label="Buffer Size"
                        value={formData.csi.buffer_size}
                        onChange={(e) => setFormData({
                            ...formData,
                            csi: { ...formData.csi, buffer_size: parseInt(e.target.value) }
                        })}
                    />
                </Grid>
                <Grid item xs={12}>
                    <FormControlLabel
                        control={
                            <Switch
                                checked={formData.csi.filter_enabled}
                                onChange={(e) => setFormData({
                                    ...formData,
                                    csi: { ...formData.csi, filter_enabled: e.target.checked }
                                })}
                            />
                        }
                        label="Enable Filtering"
                    />
                </Grid>
            </Grid>

            <Box mt={3} display="flex" justifyContent="flex-end">
                <Button 
                    variant="contained" 
                    onClick={handleSubmit}
                    disabled={loading}
                >
                    Save Configuration
                </Button>
            </Box>
        </Box>
    );
};

// API Key Form Component
const ApiKeyForm = ({ node, onSave }) => {
    const [apiKey, setApiKey] = useState('');
    const [showKey, setShowKey] = useState(false);

    return (
        <Box>
            <Alert severity="info" sx={{ mb: 2 }}>
                The API key is used to securely communicate with the ESP32 node.
                Store this key securely as it cannot be retrieved later.
            </Alert>
            
            <TextField
                fullWidth
                label="API Key"
                type={showKey ? 'text' : 'password'}
                value={apiKey}
                onChange={(e) => setApiKey(e.target.value)}
                placeholder="Enter the node's API key"
                InputProps={{
                    endAdornment: (
                        <InputAdornment position="end">
                            <IconButton onClick={() => setShowKey(!showKey)}>
                                {showKey ? <VisibilityOffIcon /> : <VisibilityIcon />}
                            </IconButton>
                        </InputAdornment>
                    )
                }}
            />
            
            <Box mt={3} display="flex" justifyContent="flex-end" gap={2}>
                <Button onClick={() => onSave(apiKey)} variant="contained">
                    Save API Key
                </Button>
            </Box>
        </Box>
    );
};

// OTA Update Form Component
const OtaUpdateForm = ({ node, onUpdate, loading }) => {
    const [updateUrl, setUpdateUrl] = useState('');
    const [version, setVersion] = useState('');

    return (
        <Box>
            <Alert severity="warning" sx={{ mb: 2 }}>
                OTA updates will restart the node. Ensure the firmware URL is correct
                and accessible from the ESP32 device.
            </Alert>
            
            <Grid container spacing={2}>
                <Grid item xs={12}>
                    <TextField
                        fullWidth
                        label="Firmware URL"
                        value={updateUrl}
                        onChange={(e) => setUpdateUrl(e.target.value)}
                        placeholder="https://example.com/firmware.bin"
                    />
                </Grid>
                <Grid item xs={12}>
                    <TextField
                        fullWidth
                        label="Version (optional)"
                        value={version}
                        onChange={(e) => setVersion(e.target.value)}
                        placeholder="1.2.0"
                    />
                </Grid>
            </Grid>
            
            <Box mt={3} display="flex" justifyContent="flex-end" gap={2}>
                <Button 
                    onClick={() => onUpdate(updateUrl, version)} 
                    variant="contained"
                    disabled={loading || !updateUrl}
                >
                    Start Update
                </Button>
            </Box>
        </Box>
    );
};

export default NodeManagement;