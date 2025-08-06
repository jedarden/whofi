/**
 * Main Dashboard Component
 * Overview of the CSI positioning system
 */

import React, { useState, useEffect } from 'react';
import {
  Grid,
  Card,
  CardContent,
  Typography,
  Box,
  CircularProgress,
  Chip,
  LinearProgress
} from '@mui/material';
import {
  LocationOn as LocationIcon,
  Sensors as SensorsIcon,
  Speed as SpeedIcon,
  Timeline as TimelineIcon,
  Signal as SignalIcon,
  NetworkWifi as NetworkIcon
} from '@mui/icons-material';
import { Line } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend,
} from 'chart.js';

import { useCurrentPosition, useNodes, useStats } from '../hooks/useApi';
import { useWebSocket } from '../context/WebSocketContext';

// Register Chart.js components
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
);

const MetricCard = ({ title, value, unit, icon, color = 'primary', loading = false }) => (
  <Card>
    <CardContent>
      <Box display="flex" alignItems="center" justifyContent="space-between">
        <Box>
          <Typography color="textSecondary" gutterBottom variant="body2">
            {title}
          </Typography>
          {loading ? (
            <CircularProgress size={24} />
          ) : (
            <Typography variant="h4" color={color}>
              {value}
              {unit && <Typography component="span" variant="h6" color="textSecondary"> {unit}</Typography>}
            </Typography>
          )}
        </Box>
        <Box color={`${color}.main`}>
          {icon}
        </Box>
      </Box>
    </CardContent>
  </Card>
);

const StatusCard = ({ title, children }) => (
  <Card>
    <CardContent>
      <Typography variant="h6" gutterBottom>
        {title}
      </Typography>
      {children}
    </CardContent>
  </Card>
);

const Dashboard = () => {
  const { data: currentPosition, isLoading: positionLoading } = useCurrentPosition();
  const { data: nodesData, isLoading: nodesLoading } = useNodes();
  const { data: stats, isLoading: statsLoading } = useStats();
  const { isConnected, lastMessage } = useWebSocket();
  
  const [positionHistory, setPositionHistory] = useState([]);
  const [signalStrengths, setSignalStrengths] = useState({});

  // Update position history from WebSocket
  useEffect(() => {
    if (lastMessage?.type === 'position_update') {
      const position = lastMessage.position;
      setPositionHistory(prev => {
        const newHistory = [...prev, {
          timestamp: new Date(position.timestamp),
          x: position.x,
          y: position.y,
          confidence: position.confidence
        }];
        // Keep only last 50 points
        return newHistory.slice(-50);
      });
    }
  }, [lastMessage]);

  // Update signal strengths from WebSocket
  useEffect(() => {
    if (lastMessage?.type === 'csi_data') {
      const { nodeId, data } = lastMessage;
      setSignalStrengths(prev => ({
        ...prev,
        [nodeId]: {
          rssi: data.rssi,
          timestamp: new Date(data.timestamp)
        }
      }));
    }
  }, [lastMessage]);

  const nodes = nodesData?.nodes || [];
  const activeNodes = nodes.filter(node => 
    Date.now() - new Date(node.lastSeen).getTime() < 30000
  );

  // Chart configuration for position history
  const positionChartData = {
    labels: positionHistory.map(p => p.timestamp.toLocaleTimeString()),
    datasets: [
      {
        label: 'X Position',
        data: positionHistory.map(p => p.x),
        borderColor: 'rgb(75, 192, 192)',
        backgroundColor: 'rgba(75, 192, 192, 0.2)',
        tension: 0.4,
      },
      {
        label: 'Y Position',
        data: positionHistory.map(p => p.y),
        borderColor: 'rgb(255, 99, 132)',
        backgroundColor: 'rgba(255, 99, 132, 0.2)',
        tension: 0.4,
      },
    ],
  };

  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: 'top',
      },
      title: {
        display: true,
        text: 'Position Tracking (Real-time)',
      },
    },
    scales: {
      y: {
        beginAtZero: false,
      },
    },
    animation: {
      duration: 0, // Disable animation for real-time updates
    },
  };

  return (
    <Box>
      <Typography variant="h4" gutterBottom>
        System Dashboard
      </Typography>
      
      <Grid container spacing={3}>
        {/* Key Metrics */}
        <Grid item xs={12} sm={6} md={3}>
          <MetricCard
            title="Current Position"
            value={currentPosition ? `(${currentPosition.x?.toFixed(1)}, ${currentPosition.y?.toFixed(1)})` : 'N/A'}
            icon={<LocationIcon fontSize="large" />}
            loading={positionLoading}
            color="primary"
          />
        </Grid>

        <Grid item xs={12} sm={6} md={3}>
          <MetricCard
            title="Active Nodes"
            value={`${activeNodes.length}/${nodes.length}`}
            icon={<SensorsIcon fontSize="large" />}
            loading={nodesLoading}
            color="success"
          />
        </Grid>

        <Grid item xs={12} sm={6} md={3}>
          <MetricCard
            title="Confidence"
            value={currentPosition?.confidence ? `${(currentPosition.confidence * 100).toFixed(0)}%` : 'N/A'}
            icon={<SpeedIcon fontSize="large" />}
            loading={positionLoading}
            color="warning"
          />
        </Grid>

        <Grid item xs={12} sm={6} md={3}>
          <MetricCard
            title="Positions Calculated"
            value={stats?.positionsCalculated || 0}
            icon={<TimelineIcon fontSize="large" />}
            loading={statsLoading}
            color="info"
          />
        </Grid>

        {/* Position History Chart */}
        <Grid item xs={12} md={8}>
          <Card>
            <CardContent>
              <Box height={300}>
                {positionHistory.length > 0 ? (
                  <Line data={positionChartData} options={chartOptions} />
                ) : (
                  <Box 
                    display="flex" 
                    alignItems="center" 
                    justifyContent="center" 
                    height="100%"
                    flexDirection="column"
                  >
                    <Typography color="textSecondary" gutterBottom>
                      No position data available
                    </Typography>
                    <Typography variant="body2" color="textSecondary">
                      Waiting for CSI data from nodes...
                    </Typography>
                  </Box>
                )}
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* System Status */}
        <Grid item xs={12} md={4}>
          <StatusCard title="System Status">
            <Box mb={2}>
              <Box display="flex" alignItems="center" justifyContent="space-between" mb={1}>
                <Typography variant="body2">WebSocket Connection</Typography>
                <Chip 
                  label={isConnected ? 'Connected' : 'Disconnected'}
                  color={isConnected ? 'success' : 'error'}
                  size="small"
                />
              </Box>
              
              <Box display="flex" alignItems="center" justifyContent="space-between" mb={1}>
                <Typography variant="body2">Data Processing</Typography>
                <Chip 
                  label={stats?.packetsReceived > 0 ? 'Active' : 'Idle'}
                  color={stats?.packetsReceived > 0 ? 'success' : 'default'}
                  size="small"
                />
              </Box>

              <Box display="flex" alignItems="center" justifyContent="space-between" mb={1}>
                <Typography variant="body2">Positioning System</Typography>
                <Chip 
                  label={activeNodes.length >= 3 ? 'Ready' : 'Insufficient Nodes'}
                  color={activeNodes.length >= 3 ? 'success' : 'warning'}
                  size="small"
                />
              </Box>
            </Box>

            {stats && (
              <Box>
                <Typography variant="body2" gutterBottom>
                  System Uptime: {Math.floor((Date.now() - stats.uptime) / 1000)}s
                </Typography>
                <Typography variant="body2" gutterBottom>
                  Packets Received: {stats.packetsReceived.toLocaleString()}
                </Typography>
                <Typography variant="body2">
                  WebSocket Clients: {stats.websocket?.clients || 0}
                </Typography>
              </Box>
            )}
          </StatusCard>
        </Grid>

        {/* Node Signal Strengths */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Node Signal Strengths
              </Typography>
              {Object.entries(signalStrengths).length === 0 ? (
                <Typography color="textSecondary">
                  No signal data available
                </Typography>
              ) : (
                Object.entries(signalStrengths).map(([nodeId, signal]) => (
                  <Box key={nodeId} mb={2}>
                    <Box display="flex" alignItems="center" justifyContent="between" mb={1}>
                      <Typography variant="body2" sx={{ minWidth: 100 }}>
                        {nodeId}
                      </Typography>
                      <Box flex={1} mx={2}>
                        <LinearProgress
                          variant="determinate"
                          value={Math.min(100, Math.max(0, (signal.rssi + 100) * 2))}
                          color={signal.rssi > -50 ? 'success' : signal.rssi > -70 ? 'warning' : 'error'}
                        />
                      </Box>
                      <Typography variant="body2" color="textSecondary">
                        {signal.rssi} dBm
                      </Typography>
                    </Box>
                  </Box>
                ))
              )}
            </CardContent>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
};

export default Dashboard;