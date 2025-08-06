/**
 * Nodes Management Component
 * Monitor and configure ESP32 CSI nodes
 */

import React, { useState } from 'react';
import {
  Box,
  Card,
  CardContent,
  Typography,
  Grid,
  Chip,
  Button,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Paper,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  TextField,
  IconButton,
  Tooltip
} from '@mui/material';
import {
  Refresh as RefreshIcon,
  Settings as SettingsIcon,
  SignalWifi4Bar as SignalIcon,
  SignalWifiOff as NoSignalIcon,
  Memory as MemoryIcon,
  Schedule as ScheduleIcon
} from '@mui/icons-material';

import { useNodes, useApiMutation } from '../hooks/useApi';
import { useWebSocket } from '../context/WebSocketContext';

const NodeCard = ({ node, onConfigure, onRefresh }) => {
  const isOnline = node.lastSeen && (Date.now() - new Date(node.lastSeen).getTime() < 30000);
  const lastSeenText = node.lastSeen 
    ? new Date(node.lastSeen).toLocaleString()
    : 'Never';

  return (
    <Card>
      <CardContent>
        <Box display="flex" alignItems="center" justifyContent="between" mb={2}>
          <Typography variant="h6">
            {node.id}
          </Typography>
          <Chip
            label={isOnline ? 'Online' : 'Offline'}
            color={isOnline ? 'success' : 'error'}
            size="small"
          />
        </Box>

        <Box mb={2}>
          <Typography variant="body2" color="textSecondary" gutterBottom>
            Last Seen: {lastSeenText}
          </Typography>
          
          {node.lastRSSI && (
            <Box display="flex" alignItems="center" gap={1} mb={1}>
              {node.lastRSSI > -50 ? <SignalIcon color="success" /> : <NoSignalIcon color="error" />}
              <Typography variant="body2">
                Signal: {node.lastRSSI} dBm
              </Typography>
            </Box>
          )}

          {node.packetCount && (
            <Typography variant="body2" color="textSecondary">
              Packets: {node.packetCount.toLocaleString()}
            </Typography>
          )}

          {node.memoryFree && (
            <Box display="flex" alignItems="center" gap={1} mb={1}>
              <MemoryIcon fontSize="small" />
              <Typography variant="body2">
                Memory: {Math.round(node.memoryFree / 1024)} KB free
              </Typography>
            </Box>
          )}

          {node.uptime && (
            <Box display="flex" alignItems="center" gap={1}>
              <ScheduleIcon fontSize="small" />
              <Typography variant="body2">
                Uptime: {Math.floor(node.uptime / 1000)}s
              </Typography>
            </Box>
          )}
        </Box>

        <Box display="flex" gap={1}>
          <Button
            startIcon={<SettingsIcon />}
            onClick={() => onConfigure(node)}
            variant="outlined"
            size="small"
          >
            Configure
          </Button>
          <Tooltip title="Refresh Node Status">
            <IconButton onClick={() => onRefresh(node.id)} size="small">
              <RefreshIcon />
            </IconButton>
          </Tooltip>
        </Box>
      </CardContent>
    </Card>
  );
};

const ConfigDialog = ({ open, node, onClose, onSave }) => {
  const [config, setConfig] = useState({
    samplingRate: node?.config?.samplingRate || 100,
    channel: node?.config?.channel || 6,
    enabled: node?.config?.enabled ?? true,
    txPower: node?.config?.txPower || 20,
    filterAlpha: node?.config?.filterAlpha || 0.1
  });

  const handleSave = () => {
    onSave(node.id, config);
    onClose();
  };

  if (!node) return null;

  return (
    <Dialog open={open} onClose={onClose} maxWidth="sm" fullWidth>
      <DialogTitle>Configure Node: {node.id}</DialogTitle>
      <DialogContent>
        <Grid container spacing={2} sx={{ mt: 1 }}>
          <Grid item xs={6}>
            <TextField
              fullWidth
              label="Sampling Rate (Hz)"
              type="number"
              value={config.samplingRate}
              onChange={(e) => setConfig({...config, samplingRate: parseInt(e.target.value)})}
              inputProps={{ min: 1, max: 1000 }}
            />
          </Grid>
          <Grid item xs={6}>
            <TextField
              fullWidth
              label="Channel"
              type="number"
              value={config.channel}
              onChange={(e) => setConfig({...config, channel: parseInt(e.target.value)})}
              inputProps={{ min: 1, max: 13 }}
            />
          </Grid>
          <Grid item xs={6}>
            <TextField
              fullWidth
              label="TX Power (dBm)"
              type="number"
              value={config.txPower}
              onChange={(e) => setConfig({...config, txPower: parseInt(e.target.value)})}
              inputProps={{ min: 0, max: 20 }}
            />
          </Grid>
          <Grid item xs={6}>
            <TextField
              fullWidth
              label="Filter Alpha"
              type="number"
              value={config.filterAlpha}
              onChange={(e) => setConfig({...config, filterAlpha: parseFloat(e.target.value)})}
              inputProps={{ min: 0, max: 1, step: 0.01 }}
            />
          </Grid>
        </Grid>
      </DialogContent>
      <DialogActions>
        <Button onClick={onClose}>Cancel</Button>
        <Button onClick={handleSave} variant="contained">Save Configuration</Button>
      </DialogActions>
    </Dialog>
  );
};

const NodesView = () => {
  const { data: nodesData, refetch } = useNodes();
  const mutation = useApiMutation();
  const { lastMessage } = useWebSocket();
  const [configDialog, setConfigDialog] = useState({ open: false, node: null });

  const nodes = nodesData?.nodes || [];

  const handleConfigure = (node) => {
    setConfigDialog({ open: true, node });
  };

  const handleSaveConfig = async (nodeId, config) => {
    try {
      await mutation.mutateAsync({
        method: 'POST',
        url: `/api/nodes/${nodeId}/config`,
        data: config
      });
      refetch(); // Refresh node data
    } catch (error) {
      console.error('Failed to save configuration:', error);
    }
  };

  const handleRefreshNode = (nodeId) => {
    // This would typically send a command to the node to report its status
    console.log('Refreshing node:', nodeId);
    refetch();
  };

  const handleRefreshAll = () => {
    refetch();
  };

  // Get active nodes count
  const activeNodes = nodes.filter(node => 
    node.lastSeen && (Date.now() - new Date(node.lastSeen).getTime() < 30000)
  );

  return (
    <Box>
      <Box display="flex" alignItems="center" justifyContent="between" mb={3}>
        <Typography variant="h4">
          Node Management
        </Typography>
        <Button
          startIcon={<RefreshIcon />}
          onClick={handleRefreshAll}
          variant="outlined"
        >
          Refresh All
        </Button>
      </Box>

      {/* Summary Cards */}
      <Grid container spacing={2} mb={3}>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent sx={{ textAlign: 'center' }}>
              <Typography color="textSecondary" gutterBottom>
                Total Nodes
              </Typography>
              <Typography variant="h4" color="primary">
                {nodes.length}
              </Typography>
            </CardContent>
          </Card>
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent sx={{ textAlign: 'center' }}>
              <Typography color="textSecondary" gutterBottom>
                Online Nodes
              </Typography>
              <Typography variant="h4" color="success.main">
                {activeNodes.length}
              </Typography>
            </CardContent>
          </Card>
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent sx={{ textAlign: 'center' }}>
              <Typography color="textSecondary" gutterBottom>
                Offline Nodes
              </Typography>
              <Typography variant="h4" color="error.main">
                {nodes.length - activeNodes.length}
              </Typography>
            </CardContent>
          </Card>
        </Grid>
        <Grid item xs={12} sm={6} md={3}>
          <Card>
            <CardContent sx={{ textAlign: 'center' }}>
              <Typography color="textSecondary" gutterBottom>
                Positioning Ready
              </Typography>
              <Typography variant="h4" color={activeNodes.length >= 3 ? "success.main" : "warning.main"}>
                {activeNodes.length >= 3 ? 'Yes' : 'No'}
              </Typography>
            </CardContent>
          </Card>
        </Grid>
      </Grid>

      {/* Node Cards Grid */}
      <Grid container spacing={3} mb={3}>
        {nodes.map((node) => (
          <Grid item xs={12} sm={6} md={4} key={node.id}>
            <NodeCard
              node={node}
              onConfigure={handleConfigure}
              onRefresh={handleRefreshNode}
            />
          </Grid>
        ))}
        {nodes.length === 0 && (
          <Grid item xs={12}>
            <Card>
              <CardContent sx={{ textAlign: 'center', py: 4 }}>
                <Typography color="textSecondary" gutterBottom>
                  No nodes detected
                </Typography>
                <Typography variant="body2" color="textSecondary">
                  Make sure your ESP32 nodes are powered on and connected to the network
                </Typography>
              </CardContent>
            </Card>
          </Grid>
        )}
      </Grid>

      {/* Detailed Node Table */}
      <Card>
        <CardContent>
          <Typography variant="h6" gutterBottom>
            Detailed Node Status
          </Typography>
          
          <TableContainer component={Paper}>
            <Table>
              <TableHead>
                <TableRow>
                  <TableCell>Node ID</TableCell>
                  <TableCell>Status</TableCell>
                  <TableCell>Last Seen</TableCell>
                  <TableCell>RSSI</TableCell>
                  <TableCell>Packets</TableCell>
                  <TableCell>Memory</TableCell>
                  <TableCell>Uptime</TableCell>
                  <TableCell>Actions</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {nodes.map((node) => {
                  const isOnline = node.lastSeen && (Date.now() - new Date(node.lastSeen).getTime() < 30000);
                  return (
                    <TableRow key={node.id}>
                      <TableCell>{node.id}</TableCell>
                      <TableCell>
                        <Chip
                          label={isOnline ? 'Online' : 'Offline'}
                          color={isOnline ? 'success' : 'error'}
                          size="small"
                        />
                      </TableCell>
                      <TableCell>
                        {node.lastSeen ? new Date(node.lastSeen).toLocaleString() : 'Never'}
                      </TableCell>
                      <TableCell>
                        {node.lastRSSI ? `${node.lastRSSI} dBm` : 'N/A'}
                      </TableCell>
                      <TableCell>
                        {node.packetCount ? node.packetCount.toLocaleString() : 'N/A'}
                      </TableCell>
                      <TableCell>
                        {node.memoryFree ? `${Math.round(node.memoryFree / 1024)} KB` : 'N/A'}
                      </TableCell>
                      <TableCell>
                        {node.uptime ? `${Math.floor(node.uptime / 1000)}s` : 'N/A'}
                      </TableCell>
                      <TableCell>
                        <IconButton
                          onClick={() => handleConfigure(node)}
                          size="small"
                        >
                          <SettingsIcon />
                        </IconButton>
                      </TableCell>
                    </TableRow>
                  );
                })}
              </TableBody>
            </Table>
          </TableContainer>
        </CardContent>
      </Card>

      {/* Configuration Dialog */}
      <ConfigDialog
        open={configDialog.open}
        node={configDialog.node}
        onClose={() => setConfigDialog({ open: false, node: null })}
        onSave={handleSaveConfig}
      />
    </Box>
  );
};

export default NodesView;