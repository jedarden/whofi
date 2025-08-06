/**
 * Advanced CSI Signal Strength Visualization
 * Real-time CSI amplitude and phase analysis with spectral visualization
 */

import React, { useState, useEffect, useRef, useMemo } from 'react';
import {
  Box,
  Card,
  CardContent,
  Typography,
  Grid,
  Switch,
  FormControlLabel,
  Button,
  Chip,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  Slider,
  Paper,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Accordion,
  AccordionSummary,
  AccordionDetails
} from '@mui/material';
import {
  ExpandMore as ExpandMoreIcon,
  Timeline as TimelineIcon,
  ShowChart as ShowChartIcon,
  Download as DownloadIcon,
  Refresh as RefreshIcon
} from '@mui/icons-material';
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
  TimeScale,
} from 'chart.js';
import 'chartjs-adapter-moment';
import { useWebSocket } from '../context/WebSocketContext';
import { useNodes } from '../hooks/useApi';

// Register Chart.js components
ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  BarElement,
  Title,
  Tooltip,
  Legend,
  TimeScale
);

// CSI data processing utilities
const processCSIData = (rawData) => {
  if (!rawData || !rawData.csi_matrix) return null;
  
  const matrix = rawData.csi_matrix;
  const subcarriers = matrix.length;
  
  // Calculate amplitude and phase for each subcarrier
  const amplitudes = matrix.map(complex => 
    Math.sqrt(complex.real * complex.real + complex.imag * complex.imag)
  );
  
  const phases = matrix.map(complex => 
    Math.atan2(complex.imag, complex.real)
  );
  
  return {
    timestamp: new Date(rawData.timestamp),
    nodeId: rawData.node_id,
    rssi: rawData.rssi,
    subcarriers,
    amplitudes,
    phases,
    avgAmplitude: amplitudes.reduce((sum, amp) => sum + amp, 0) / amplitudes.length,
    maxAmplitude: Math.max(...amplitudes),
    minAmplitude: Math.min(...amplitudes),
    channelInfo: rawData.channel_info || {}
  };
};

// Spectral analysis component
const SpectralAnalysis = ({ csiData, showPhase, selectedNode }) => {
  const filteredData = selectedNode 
    ? csiData.filter(data => data.nodeId === selectedNode)
    : csiData;
  
  const latestData = filteredData[filteredData.length - 1];
  
  if (!latestData) return null;
  
  const chartData = {
    labels: Array.from({ length: latestData.subcarriers }, (_, i) => `SC${i}`),
    datasets: [
      {
        label: 'Amplitude (dB)',
        data: latestData.amplitudes.map(amp => 20 * Math.log10(amp)),
        borderColor: '#1976d2',
        backgroundColor: 'rgba(25, 118, 210, 0.1)',
        tension: 0.4,
      },
      ...(showPhase ? [{
        label: 'Phase (radians)',
        data: latestData.phases,
        borderColor: '#f57c00',
        backgroundColor: 'rgba(245, 124, 0, 0.1)',
        tension: 0.4,
        yAxisID: 'y1',
      }] : [])
    ],
  };
  
  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: 'top',
      },
      title: {
        display: true,
        text: `CSI Spectral Analysis - ${latestData.nodeId}`,
      },
    },
    scales: {
      y: {
        type: 'linear',
        display: true,
        position: 'left',
        title: {
          display: true,
          text: 'Amplitude (dB)'
        }
      },
      ...(showPhase ? {
        y1: {
          type: 'linear',
          display: true,
          position: 'right',
          title: {
            display: true,
            text: 'Phase (radians)'
          },
          grid: {
            drawOnChartArea: false,
          },
        }
      } : {})
    },
  };
  
  return (
    <Box height={400}>
      <Line data={chartData} options={options} />
    </Box>
  );
};

// Time series visualization
const TimeSeriesChart = ({ csiData, metric, timeWindow, selectedNodes }) => {
  const processedData = useMemo(() => {
    const nodeData = {};
    
    csiData.forEach(data => {
      if (selectedNodes.length === 0 || selectedNodes.includes(data.nodeId)) {
        if (!nodeData[data.nodeId]) {
          nodeData[data.nodeId] = [];
        }
        
        let value;
        switch (metric) {
          case 'rssi':
            value = data.rssi;
            break;
          case 'avgAmplitude':
            value = 20 * Math.log10(data.avgAmplitude);
            break;
          case 'maxAmplitude':
            value = 20 * Math.log10(data.maxAmplitude);
            break;
          case 'amplitudeVariance':
            const mean = data.avgAmplitude;
            const variance = data.amplitudes.reduce((sum, amp) => sum + Math.pow(amp - mean, 2), 0) / data.amplitudes.length;
            value = variance;
            break;
          default:
            value = data.rssi;
        }
        
        nodeData[data.nodeId].push({
          x: data.timestamp,
          y: value
        });
      }
    });
    
    // Keep only data within time window
    const cutoffTime = Date.now() - (timeWindow * 1000);
    Object.keys(nodeData).forEach(nodeId => {
      nodeData[nodeId] = nodeData[nodeId].filter(point => point.x.getTime() > cutoffTime);
    });
    
    return nodeData;
  }, [csiData, metric, timeWindow, selectedNodes]);
  
  const colors = ['#1976d2', '#f57c00', '#4caf50', '#e91e63', '#9c27b0', '#00bcd4'];
  
  const chartData = {
    datasets: Object.keys(processedData).map((nodeId, index) => ({
      label: nodeId,
      data: processedData[nodeId],
      borderColor: colors[index % colors.length],
      backgroundColor: colors[index % colors.length] + '20',
      tension: 0.4,
      pointRadius: 2,
      pointHoverRadius: 6,
    }))
  };
  
  const options = {
    responsive: true,
    maintainAspectRatio: false,
    plugins: {
      legend: {
        position: 'top',
      },
      title: {
        display: true,
        text: `${metric.toUpperCase()} Time Series`,
      },
    },
    scales: {
      x: {
        type: 'time',
        time: {
          displayFormats: {
            millisecond: 'HH:mm:ss.SSS',
            second: 'HH:mm:ss',
            minute: 'HH:mm'
          }
        },
        title: {
          display: true,
          text: 'Time'
        }
      },
      y: {
        title: {
          display: true,
          text: metric === 'rssi' ? 'RSSI (dBm)' : 
                metric.includes('Amplitude') ? 'Amplitude (dB)' :
                'Value'
        }
      },
    },
  };
  
  return (
    <Box height={400}>
      <Scatter data={chartData} options={options} />
    </Box>
  );
};

// Signal quality metrics
const SignalQualityMetrics = ({ csiData, selectedNode }) => {
  const nodeData = selectedNode 
    ? csiData.filter(data => data.nodeId === selectedNode)
    : csiData;
  
  if (nodeData.length === 0) return null;
  
  const latest = nodeData[nodeData.length - 1];
  const recent = nodeData.slice(-10); // Last 10 samples
  
  const metrics = {
    rssi: {
      current: latest.rssi,
      avg: recent.reduce((sum, d) => sum + d.rssi, 0) / recent.length,
      min: Math.min(...recent.map(d => d.rssi)),
      max: Math.max(...recent.map(d => d.rssi))
    },
    amplitude: {
      current: 20 * Math.log10(latest.avgAmplitude),
      avg: recent.reduce((sum, d) => sum + 20 * Math.log10(d.avgAmplitude), 0) / recent.length,
      min: Math.min(...recent.map(d => 20 * Math.log10(d.avgAmplitude))),
      max: Math.max(...recent.map(d => 20 * Math.log10(d.avgAmplitude)))
    },
    snr: {
      current: latest.channelInfo.snr || 0,
      avg: recent.reduce((sum, d) => sum + (d.channelInfo.snr || 0), 0) / recent.length
    }
  };
  
  return (
    <TableContainer component={Paper}>
      <Table size="small">
        <TableHead>
          <TableRow>
            <TableCell>Metric</TableCell>
            <TableCell align="right">Current</TableCell>
            <TableCell align="right">Average</TableCell>
            <TableCell align="right">Min/Max</TableCell>
            <TableCell align="right">Quality</TableCell>
          </TableRow>
        </TableHead>
        <TableBody>
          <TableRow>
            <TableCell>RSSI (dBm)</TableCell>
            <TableCell align="right">{metrics.rssi.current.toFixed(1)}</TableCell>
            <TableCell align="right">{metrics.rssi.avg.toFixed(1)}</TableCell>
            <TableCell align="right">{metrics.rssi.min.toFixed(1)} / {metrics.rssi.max.toFixed(1)}</TableCell>
            <TableCell align="right">
              <Chip 
                label={metrics.rssi.current > -50 ? 'Excellent' : 
                       metrics.rssi.current > -70 ? 'Good' : 
                       metrics.rssi.current > -80 ? 'Fair' : 'Poor'}
                color={metrics.rssi.current > -50 ? 'success' : 
                       metrics.rssi.current > -70 ? 'primary' : 
                       metrics.rssi.current > -80 ? 'warning' : 'error'}
                size="small"
              />
            </TableCell>
          </TableRow>
          
          <TableRow>
            <TableCell>CSI Amplitude (dB)</TableCell>
            <TableCell align="right">{metrics.amplitude.current.toFixed(1)}</TableCell>
            <TableCell align="right">{metrics.amplitude.avg.toFixed(1)}</TableCell>
            <TableCell align="right">{metrics.amplitude.min.toFixed(1)} / {metrics.amplitude.max.toFixed(1)}</TableCell>
            <TableCell align="right">
              <Chip 
                label={Math.abs(metrics.amplitude.current - metrics.amplitude.avg) < 5 ? 'Stable' : 'Variable'}
                color={Math.abs(metrics.amplitude.current - metrics.amplitude.avg) < 5 ? 'success' : 'warning'}
                size="small"
              />
            </TableCell>
          </TableRow>
          
          <TableRow>
            <TableCell>SNR (dB)</TableCell>
            <TableCell align="right">{metrics.snr.current.toFixed(1)}</TableCell>
            <TableCell align="right">{metrics.snr.avg.toFixed(1)}</TableCell>
            <TableCell align="right">-</TableCell>
            <TableCell align="right">
              <Chip 
                label={metrics.snr.current > 20 ? 'Excellent' : 
                       metrics.snr.current > 10 ? 'Good' : 
                       metrics.snr.current > 0 ? 'Fair' : 'Poor'}
                color={metrics.snr.current > 20 ? 'success' : 
                       metrics.snr.current > 10 ? 'primary' : 
                       metrics.snr.current > 0 ? 'warning' : 'error'}
                size="small"
              />
            </TableCell>
          </TableRow>
        </TableBody>
      </Table>
    </TableContainer>
  );
};

const SignalAnalyzer = () => {
  const { lastMessage } = useWebSocket();
  const { data: nodesData } = useNodes();
  const [csiData, setCsiData] = useState([]);
  const [selectedNodes, setSelectedNodes] = useState([]);
  const [selectedNode, setSelectedNode] = useState('');
  const [showPhase, setShowPhase] = useState(false);
  const [timeWindow, setTimeWindow] = useState(300); // 5 minutes
  const [metric, setMetric] = useState('rssi');
  const [autoRefresh, setAutoRefresh] = useState(true);
  const [expandedPanels, setExpandedPanels] = useState({
    spectral: true,
    timeSeries: true,
    quality: true
  });
  
  // Process incoming CSI data
  useEffect(() => {
    if (lastMessage?.type === 'csi_data') {
      const processed = processCSIData(lastMessage.data);
      if (processed) {
        setCsiData(prev => {
          const newData = [...prev, processed];
          // Keep only data within time window
          const cutoffTime = Date.now() - (timeWindow * 1000);
          return newData.filter(data => data.timestamp.getTime() > cutoffTime);
        });
      }
    }
  }, [lastMessage, timeWindow]);
  
  // Available nodes
  const availableNodes = useMemo(() => {
    const nodeSet = new Set();
    csiData.forEach(data => nodeSet.add(data.nodeId));
    return Array.from(nodeSet);
  }, [csiData]);
  
  // Set default selected node
  useEffect(() => {
    if (availableNodes.length > 0 && !selectedNode) {
      setSelectedNode(availableNodes[0]);
    }
  }, [availableNodes, selectedNode]);
  
  const handlePanelChange = (panel) => (event, isExpanded) => {
    setExpandedPanels(prev => ({
      ...prev,
      [panel]: isExpanded
    }));
  };
  
  const clearData = () => {
    setCsiData([]);
  };
  
  const exportData = () => {
    const exportObj = {
      timestamp: new Date().toISOString(),
      timeWindow,
      nodeCount: availableNodes.length,
      sampleCount: csiData.length,
      data: csiData.map(data => ({
        timestamp: data.timestamp.toISOString(),
        nodeId: data.nodeId,
        rssi: data.rssi,
        avgAmplitude: data.avgAmplitude,
        maxAmplitude: data.maxAmplitude,
        minAmplitude: data.minAmplitude,
        amplitudes: data.amplitudes,
        phases: data.phases,
        channelInfo: data.channelInfo
      }))
    };
    
    const blob = new Blob([JSON.stringify(exportObj, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `csi_analysis_${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
  };
  
  return (
    <Box>
      <Typography variant="h4" gutterBottom>
        CSI Signal Analysis
      </Typography>
      
      <Grid container spacing={3}>
        {/* Controls */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Grid container spacing={2} alignItems="center">
                <Grid item>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={autoRefresh}
                        onChange={(e) => setAutoRefresh(e.target.checked)}
                      />
                    }
                    label="Auto Refresh"
                  />
                </Grid>
                
                <Grid item>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={showPhase}
                        onChange={(e) => setShowPhase(e.target.checked)}
                      />
                    }
                    label="Show Phase"
                  />
                </Grid>
                
                <Grid item>
                  <FormControl size="small" sx={{ minWidth: 120 }}>
                    <InputLabel>Selected Node</InputLabel>
                    <Select
                      value={selectedNode}
                      label="Selected Node"
                      onChange={(e) => setSelectedNode(e.target.value)}
                    >
                      {availableNodes.map(nodeId => (
                        <MenuItem key={nodeId} value={nodeId}>
                          {nodeId}
                        </MenuItem>
                      ))}
                    </Select>
                  </FormControl>
                </Grid>
                
                <Grid item>
                  <FormControl size="small" sx={{ minWidth: 150 }}>
                    <InputLabel>Time Series Metric</InputLabel>
                    <Select
                      value={metric}
                      label="Time Series Metric"
                      onChange={(e) => setMetric(e.target.value)}
                    >
                      <MenuItem value="rssi">RSSI</MenuItem>
                      <MenuItem value="avgAmplitude">Avg Amplitude</MenuItem>
                      <MenuItem value="maxAmplitude">Max Amplitude</MenuItem>
                      <MenuItem value="amplitudeVariance">Amplitude Variance</MenuItem>
                    </Select>
                  </FormControl>
                </Grid>
                
                <Grid item xs={12} sm={3}>
                  <Typography gutterBottom>Time Window (seconds)</Typography>
                  <Slider
                    value={timeWindow}
                    onChange={(e, newValue) => setTimeWindow(newValue)}
                    min={30}
                    max={1800}
                    step={30}
                    valueLabelDisplay="auto"
                  />
                </Grid>
                
                <Grid item>
                  <Button
                    startIcon={<RefreshIcon />}
                    onClick={clearData}
                    variant="outlined"
                  >
                    Clear
                  </Button>
                </Grid>
                
                <Grid item>
                  <Button
                    startIcon={<DownloadIcon />}
                    onClick={exportData}
                    variant="outlined"
                    disabled={csiData.length === 0}
                  >
                    Export
                  </Button>
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>
        
        {/* Status Summary */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Grid container spacing={3}>
                <Grid item xs={12} sm={3}>
                  <Typography variant="body2" color="textSecondary">
                    Active Nodes
                  </Typography>
                  <Typography variant="h6">
                    {availableNodes.length}
                  </Typography>
                </Grid>
                
                <Grid item xs={12} sm={3}>
                  <Typography variant="body2" color="textSecondary">
                    Total Samples
                  </Typography>
                  <Typography variant="h6">
                    {csiData.length.toLocaleString()}
                  </Typography>
                </Grid>
                
                <Grid item xs={12} sm={3}>
                  <Typography variant="body2" color="textSecondary">
                    Sample Rate
                  </Typography>
                  <Typography variant="h6">
                    {csiData.length > 0 ? 
                      ((csiData.length / (timeWindow / 60)).toFixed(1) + '/min') : 
                      '0/min'}
                  </Typography>
                </Grid>
                
                <Grid item xs={12} sm={3}>
                  <Typography variant="body2" color="textSecondary">
                    Data Quality
                  </Typography>
                  <Chip
                    label={csiData.length > 100 ? 'Good' : csiData.length > 10 ? 'Fair' : 'Limited'}
                    color={csiData.length > 100 ? 'success' : csiData.length > 10 ? 'warning' : 'error'}
                  />
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>
        
        {/* Spectral Analysis */}
        <Grid item xs={12}>
          <Accordion 
            expanded={expandedPanels.spectral} 
            onChange={handlePanelChange('spectral')}
          >
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">
                <ShowChartIcon sx={{ mr: 1, verticalAlign: 'middle' }} />
                Spectral Analysis
              </Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Card>
                <SpectralAnalysis 
                  csiData={csiData}
                  showPhase={showPhase}
                  selectedNode={selectedNode}
                />
              </Card>
            </AccordionDetails>
          </Accordion>
        </Grid>
        
        {/* Time Series */}
        <Grid item xs={12}>
          <Accordion 
            expanded={expandedPanels.timeSeries} 
            onChange={handlePanelChange('timeSeries')}
          >
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">
                <TimelineIcon sx={{ mr: 1, verticalAlign: 'middle' }} />
                Time Series Analysis
              </Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Card>
                <TimeSeriesChart 
                  csiData={csiData}
                  metric={metric}
                  timeWindow={timeWindow}
                  selectedNodes={selectedNodes}
                />
              </Card>
            </AccordionDetails>
          </Accordion>
        </Grid>
        
        {/* Signal Quality Metrics */}
        <Grid item xs={12}>
          <Accordion 
            expanded={expandedPanels.quality} 
            onChange={handlePanelChange('quality')}
          >
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">
                Signal Quality Metrics
              </Typography>
            </AccordionSummary>
            <AccordionDetails>
              <SignalQualityMetrics 
                csiData={csiData}
                selectedNode={selectedNode}
              />
            </AccordionDetails>
          </Accordion>
        </Grid>
      </Grid>
    </Box>
  );
};

export default SignalAnalyzer;