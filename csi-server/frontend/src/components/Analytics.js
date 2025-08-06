/**
 * Analytics Component
 * System performance and positioning accuracy analytics
 */

import React, { useState } from 'react';
import {
  Box,
  Typography,
  Grid,
  Card,
  CardContent,
  Tab,
  Tabs,
  FormControl,
  InputLabel,
  Select,
  MenuItem
} from '@mui/material';
import { Line, Bar, Scatter } from 'react-chartjs-2';
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  BarElement,
  Title,
  Tooltip,
  Legend,
} from 'chart.js';

import { usePositions, useStats } from '../hooks/useApi';

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  BarElement,
  Title,
  Tooltip,
  Legend
);

const Analytics = () => {
  const [activeTab, setActiveTab] = useState(0);
  const [timeRange, setTimeRange] = useState('1h');
  
  const { data: stats } = useStats();
  const { data: positionsData } = usePositions({ limit: 1000, from: `-${timeRange}` });

  const positions = positionsData?.positions || [];

  const handleTabChange = (event, newValue) => {
    setActiveTab(newValue);
  };

  // Sample data for demonstration
  const accuracyData = {
    labels: ['00:00', '04:00', '08:00', '12:00', '16:00', '20:00'],
    datasets: [{
      label: 'Positioning Accuracy (%)',
      data: [85, 92, 88, 95, 87, 91],
      borderColor: 'rgb(75, 192, 192)',
      backgroundColor: 'rgba(75, 192, 192, 0.2)',
      tension: 0.4,
    }]
  };

  const signalStrengthData = {
    labels: ['Node 1', 'Node 2', 'Node 3', 'Node 4'],
    datasets: [{
      label: 'Average RSSI (dBm)',
      data: [-45, -52, -38, -49],
      backgroundColor: [
        'rgba(255, 99, 132, 0.5)',
        'rgba(54, 162, 235, 0.5)',
        'rgba(255, 205, 86, 0.5)',
        'rgba(75, 192, 192, 0.5)',
      ],
      borderColor: [
        'rgb(255, 99, 132)',
        'rgb(54, 162, 235)',
        'rgb(255, 205, 86)',
        'rgb(75, 192, 192)',
      ],
      borderWidth: 2,
    }]
  };

  const positionScatterData = {
    datasets: [{
      label: 'Position Points',
      data: positions.slice(-100).map(p => ({ x: p.x, y: p.y })),
      backgroundColor: 'rgba(54, 162, 235, 0.6)',
      borderColor: 'rgb(54, 162, 235)',
    }]
  };

  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: { position: 'top' },
    },
  };

  const scatterOptions = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: { position: 'top' },
      title: { display: true, text: 'Position Distribution' },
    },
    scales: {
      x: { title: { display: true, text: 'X Position' } },
      y: { title: { display: true, text: 'Y Position' } },
    },
  };

  return (
    <Box>
      <Box display="flex" alignItems="center" justifyContent="space-between" mb={3}>
        <Typography variant="h4">
          System Analytics
        </Typography>
        <FormControl size="small" sx={{ minWidth: 120 }}>
          <InputLabel>Time Range</InputLabel>
          <Select
            value={timeRange}
            onChange={(e) => setTimeRange(e.target.value)}
            label="Time Range"
          >
            <MenuItem value="1h">1 Hour</MenuItem>
            <MenuItem value="4h">4 Hours</MenuItem>
            <MenuItem value="1d">1 Day</MenuItem>
            <MenuItem value="1w">1 Week</MenuItem>
          </Select>
        </FormControl>
      </Box>

      <Box sx={{ borderBottom: 1, borderColor: 'divider' }}>
        <Tabs value={activeTab} onChange={handleTabChange}>
          <Tab label="Performance" />
          <Tab label="Signal Analysis" />
          <Tab label="Position Analysis" />
          <Tab label="System Health" />
        </Tabs>
      </Box>

      <Box mt={3}>
        {/* Performance Tab */}
        {activeTab === 0 && (
          <Grid container spacing={3}>
            <Grid item xs={12} md={6}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>
                    Positioning Accuracy Over Time
                  </Typography>
                  <Box height={300}>
                    <Line data={accuracyData} options={chartOptions} />
                  </Box>
                </CardContent>
              </Card>
            </Grid>
            
            <Grid item xs={12} md={6}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>
                    System Metrics
                  </Typography>
                  <Grid container spacing={2}>
                    <Grid item xs={6}>
                      <Box textAlign="center">
                        <Typography variant="h4" color="primary">
                          {stats?.positionsCalculated || 0}
                        </Typography>
                        <Typography color="textSecondary">
                          Total Positions
                        </Typography>
                      </Box>
                    </Grid>
                    <Grid item xs={6}>
                      <Box textAlign="center">
                        <Typography variant="h4" color="success.main">
                          94%
                        </Typography>
                        <Typography color="textSecondary">
                          Average Accuracy
                        </Typography>
                      </Box>
                    </Grid>
                    <Grid item xs={6}>
                      <Box textAlign="center">
                        <Typography variant="h4" color="info.main">
                          {stats?.packetsReceived || 0}
                        </Typography>
                        <Typography color="textSecondary">
                          Packets Processed
                        </Typography>
                      </Box>
                    </Grid>
                    <Grid item xs={6}>
                      <Box textAlign="center">
                        <Typography variant="h4" color="warning.main">
                          1.2s
                        </Typography>
                        <Typography color="textSecondary">
                          Avg Response Time
                        </Typography>
                      </Box>
                    </Grid>
                  </Grid>
                </CardContent>
              </Card>
            </Grid>
          </Grid>
        )}

        {/* Signal Analysis Tab */}
        {activeTab === 1 && (
          <Grid container spacing={3}>
            <Grid item xs={12} md={8}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>
                    Node Signal Strength Analysis
                  </Typography>
                  <Box height={300}>
                    <Bar data={signalStrengthData} options={chartOptions} />
                  </Box>
                </CardContent>
              </Card>
            </Grid>
            
            <Grid item xs={12} md={4}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>
                    Signal Quality Metrics
                  </Typography>
                  <Box mt={2}>
                    <Typography variant="body2" gutterBottom>
                      <strong>Best Node:</strong> Node 3 (-38 dBm)
                    </Typography>
                    <Typography variant="body2" gutterBottom>
                      <strong>Weakest Node:</strong> Node 2 (-52 dBm)
                    </Typography>
                    <Typography variant="body2" gutterBottom>
                      <strong>Average RSSI:</strong> -46 dBm
                    </Typography>
                    <Typography variant="body2" gutterBottom>
                      <strong>Signal Stability:</strong> Good
                    </Typography>
                    <Typography variant="body2">
                      <strong>Coverage Area:</strong> 95%
                    </Typography>
                  </Box>
                </CardContent>
              </Card>
            </Grid>
          </Grid>
        )}

        {/* Position Analysis Tab */}
        {activeTab === 2 && (
          <Grid container spacing={3}>
            <Grid item xs={12} md={8}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>
                    Position Distribution
                  </Typography>
                  <Box height={400}>
                    <Scatter data={positionScatterData} options={scatterOptions} />
                  </Box>
                </CardContent>
              </Card>
            </Grid>
            
            <Grid item xs={12} md={4}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>
                    Position Statistics
                  </Typography>
                  <Box mt={2}>
                    {positions.length > 0 ? (
                      <>
                        <Typography variant="body2" gutterBottom>
                          <strong>Total Positions:</strong> {positions.length}
                        </Typography>
                        <Typography variant="body2" gutterBottom>
                          <strong>X Range:</strong> {Math.min(...positions.map(p => p.x)).toFixed(1)} to {Math.max(...positions.map(p => p.x)).toFixed(1)}
                        </Typography>
                        <Typography variant="body2" gutterBottom>
                          <strong>Y Range:</strong> {Math.min(...positions.map(p => p.y)).toFixed(1)} to {Math.max(...positions.map(p => p.y)).toFixed(1)}
                        </Typography>
                        <Typography variant="body2" gutterBottom>
                          <strong>Avg Confidence:</strong> {(positions.reduce((sum, p) => sum + (p.confidence || 0), 0) / positions.length * 100).toFixed(0)}%
                        </Typography>
                      </>
                    ) : (
                      <Typography color="textSecondary">
                        No position data available
                      </Typography>
                    )}
                  </Box>
                </CardContent>
              </Card>
            </Grid>
          </Grid>
        )}

        {/* System Health Tab */}
        {activeTab === 3 && (
          <Grid container spacing={3}>
            <Grid item xs={12}>
              <Card>
                <CardContent>
                  <Typography variant="h6" gutterBottom>
                    System Health Overview
                  </Typography>
                  <Grid container spacing={3}>
                    <Grid item xs={12} md={3}>
                      <Box textAlign="center">
                        <Typography variant="h3" color="success.main">
                          98%
                        </Typography>
                        <Typography color="textSecondary">
                          System Uptime
                        </Typography>
                      </Box>
                    </Grid>
                    <Grid item xs={12} md={3}>
                      <Box textAlign="center">
                        <Typography variant="h3" color="info.main">
                          45%
                        </Typography>
                        <Typography color="textSecondary">
                          CPU Usage
                        </Typography>
                      </Box>
                    </Grid>
                    <Grid item xs={12} md={3}>
                      <Box textAlign="center">
                        <Typography variant="h3" color="warning.main">
                          62%
                        </Typography>
                        <Typography color="textSecondary">
                          Memory Usage
                        </Typography>
                      </Box>
                    </Grid>
                    <Grid item xs={12} md={3}>
                      <Box textAlign="center">
                        <Typography variant="h3" color="success.main">
                          4/4
                        </Typography>
                        <Typography color="textSecondary">
                          Services Running
                        </Typography>
                      </Box>
                    </Grid>
                  </Grid>
                </CardContent>
              </Card>
            </Grid>
          </Grid>
        )}
      </Box>
    </Box>
  );
};

export default Analytics;