/**
 * Advanced Position Visualization with Heatmaps
 * Real-time CSI-based position tracking with heat distribution
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
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
  Slider,
  Paper,
  ButtonGroup,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  Tooltip
} from '@mui/material';
import {
  MyLocation as MyLocationIcon,
  History as HistoryIcon,
  Clear as ClearIcon,
  PlayArrow as PlayIcon,
  Pause as PauseIcon,
  Settings as SettingsIcon,
  Download as DownloadIcon
} from '@mui/icons-material';
import { Canvas, useFrame } from '@react-three/fiber';
import { OrbitControls } from '@react-three/drei';
import * as THREE from 'three';
import { useWebSocket } from '../context/WebSocketContext';

// Heatmap color gradients
const heatmapColors = {
  viridis: [
    [68, 1, 84],
    [59, 82, 139],
    [33, 145, 140],
    [94, 201, 98],
    [253, 231, 37]
  ],
  hot: [
    [0, 0, 0],
    [128, 0, 0],
    [255, 0, 0],
    [255, 128, 0],
    [255, 255, 0],
    [255, 255, 255]
  ],
  cool: [
    [0, 255, 255],
    [128, 128, 255],
    [255, 0, 255]
  ]
};

// 3D Heatmap Component
const HeatmapMesh = ({ data, colorScheme, intensity, showNodes, nodePositions }) => {
  const meshRef = useRef();
  const [heatmapTexture, setHeatmapTexture] = useState(null);
  
  useEffect(() => {
    if (!data || data.length === 0) return;
    
    // Create heatmap texture
    const canvas = document.createElement('canvas');
    const ctx = canvas.getContext('2d');
    canvas.width = 256;
    canvas.height = 256;
    
    // Create gradient
    const gradient = ctx.createLinearGradient(0, 0, 0, canvas.height);
    const colors = heatmapColors[colorScheme] || heatmapColors.viridis;
    
    colors.forEach((color, index) => {
      const position = index / (colors.length - 1);
      gradient.addColorStop(position, `rgb(${color[0]}, ${color[1]}, ${color[2]})`);
    });
    
    // Clear canvas
    ctx.fillStyle = 'rgba(0, 0, 0, 0)';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    // Draw heatmap points
    data.forEach(point => {
      const x = ((point.x + 10) / 20) * canvas.width; // Normalize to canvas
      const y = ((point.y + 10) / 20) * canvas.height;
      const radius = (point.confidence || 0.5) * 50 * intensity;
      
      const pointGradient = ctx.createRadialGradient(x, y, 0, x, y, radius);
      pointGradient.addColorStop(0, `rgba(255, 255, 255, ${point.confidence || 0.5})`);
      pointGradient.addColorStop(1, 'rgba(255, 255, 255, 0)');
      
      ctx.fillStyle = pointGradient;
      ctx.beginPath();
      ctx.arc(x, y, radius, 0, Math.PI * 2);
      ctx.fill();
    });
    
    // Apply color scheme
    const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
    const data32 = new Uint32Array(imageData.data.buffer);
    
    for (let i = 0; i < data32.length; i++) {
      const alpha = (data32[i] >> 24) & 255;
      if (alpha > 0) {
        const colorIndex = Math.floor((alpha / 255) * (colors.length - 1));
        const color = colors[colorIndex] || colors[0];
        data32[i] = (alpha << 24) | (color[2] << 16) | (color[1] << 8) | color[0];
      }
    }
    
    ctx.putImageData(imageData, 0, 0);
    
    const texture = new THREE.CanvasTexture(canvas);
    texture.needsUpdate = true;
    setHeatmapTexture(texture);
  }, [data, colorScheme, intensity]);
  
  return (
    <group>
      {/* Ground plane with heatmap */}
      <mesh ref={meshRef} position={[0, -0.1, 0]} rotation={[-Math.PI / 2, 0, 0]}>
        <planeGeometry args={[20, 20]} />
        <meshBasicMaterial
          map={heatmapTexture}
          transparent={true}
          opacity={0.8}
        />
      </mesh>
      
      {/* Node positions */}
      {showNodes && Object.values(nodePositions).map(node => (
        <mesh key={node.id} position={[node.x, 0.5, node.y]}>
          <sphereGeometry args={[0.3]} />
          <meshStandardMaterial color="#666" />
        </mesh>
      ))}
      
      {/* Current positions */}
      {data.map((point, index) => (
        <mesh key={index} position={[point.x, 1, point.y]}>
          <sphereGeometry args={[0.2]} />
          <meshStandardMaterial 
            color={point.confidence > 0.7 ? '#4caf50' : '#ff9800'}
            emissive={point.confidence > 0.7 ? '#2e7d32' : '#f57800'}
          />
        </mesh>
      ))}
    </group>
  );
};

const HeatmapView = () => {
  const { lastMessage } = useWebSocket();
  const [positionHistory, setPositionHistory] = useState([]);
  const [isPlaying, setIsPlaying] = useState(true);
  const [showNodes, setShowNodes] = useState(true);
  const [showTrail, setShowTrail] = useState(true);
  const [colorScheme, setColorScheme] = useState('viridis');
  const [intensity, setIntensity] = useState(1.0);
  const [timeWindow, setTimeWindow] = useState(300); // 5 minutes
  const [currentTime, setCurrentTime] = useState(Date.now());
  
  // Historical playback state
  const [playbackMode, setPlaybackMode] = useState(false);
  const [playbackPosition, setPlaybackPosition] = useState(0);
  const [playbackSpeed, setPlaybackSpeed] = useState(1);
  
  // Node positions (should come from config)
  const nodePositions = {
    'esp32_001': { x: -5, y: -5, id: 'esp32_001' },
    'esp32_002': { x: 5, y: -5, id: 'esp32_002' },
    'esp32_003': { x: 5, y: 5, id: 'esp32_003' },
    'esp32_004': { x: -5, y: 5, id: 'esp32_004' }
  };
  
  // Update position history from WebSocket
  useEffect(() => {
    if (lastMessage?.type === 'position_update' && !playbackMode) {
      const position = lastMessage.position;
      setPositionHistory(prev => {
        const newHistory = [...prev, {
          timestamp: new Date(position.timestamp),
          x: position.x,
          y: position.y,
          z: position.z || 0,
          confidence: position.confidence,
          id: Date.now()
        }];
        
        // Keep only data within time window
        const cutoffTime = Date.now() - (timeWindow * 1000);
        return newHistory.filter(point => point.timestamp.getTime() > cutoffTime);
      });
    }
  }, [lastMessage, playbackMode, timeWindow]);
  
  // Playback timer
  useEffect(() => {
    if (!playbackMode || !isPlaying) return;
    
    const interval = setInterval(() => {
      setPlaybackPosition(prev => {
        const newPos = prev + (playbackSpeed * 1000); // 1 second increments
        return newPos >= positionHistory.length * 1000 ? 0 : newPos;
      });
    }, 100);
    
    return () => clearInterval(interval);
  }, [playbackMode, isPlaying, playbackSpeed, positionHistory.length]);
  
  // Get visible positions based on current mode
  const getVisiblePositions = useCallback(() => {
    if (!playbackMode) {
      return positionHistory;
    }
    
    const endIndex = Math.floor(playbackPosition / 1000);
    return positionHistory.slice(0, endIndex);
  }, [positionHistory, playbackMode, playbackPosition]);
  
  const visiblePositions = getVisiblePositions();
  
  // Export functionality
  const exportData = (format) => {
    const data = {
      timestamp: new Date().toISOString(),
      nodePositions,
      positionHistory,
      settings: {
        colorScheme,
        intensity,
        timeWindow
      }
    };
    
    let content, filename;
    
    switch (format) {
      case 'json':
        content = JSON.stringify(data, null, 2);
        filename = `csi_positions_${Date.now()}.json`;
        break;
      case 'csv':
        const csvHeader = 'timestamp,x,y,z,confidence\n';
        const csvData = positionHistory.map(point => 
          `${point.timestamp.toISOString()},${point.x},${point.y},${point.z || 0},${point.confidence}`
        ).join('\n');
        content = csvHeader + csvData;
        filename = `csi_positions_${Date.now()}.csv`;
        break;
      default:
        return;
    }
    
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
  };
  
  const clearHistory = () => {
    setPositionHistory([]);
    setPlaybackPosition(0);
  };
  
  const togglePlayback = () => {
    setIsPlaying(!isPlaying);
  };
  
  const enterPlaybackMode = () => {
    setPlaybackMode(true);
    setIsPlaying(false);
    setPlaybackPosition(0);
  };
  
  const exitPlaybackMode = () => {
    setPlaybackMode(false);
    setIsPlaying(true);
  };

  return (
    <Box>
      <Typography variant="h4" gutterBottom>
        Advanced Position Heatmap
      </Typography>
      
      <Grid container spacing={3}>
        {/* Controls */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Grid container spacing={2} alignItems="center">
                {/* View Controls */}
                <Grid item>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={showNodes}
                        onChange={(e) => setShowNodes(e.target.checked)}
                      />
                    }
                    label="Show Nodes"
                  />
                </Grid>
                
                <Grid item>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={showTrail}
                        onChange={(e) => setShowTrail(e.target.checked)}
                      />
                    }
                    label="Show Trail"
                  />
                </Grid>
                
                {/* Color Scheme */}
                <Grid item>
                  <FormControl size="small" sx={{ minWidth: 120 }}>
                    <InputLabel>Color Scheme</InputLabel>
                    <Select
                      value={colorScheme}
                      label="Color Scheme"
                      onChange={(e) => setColorScheme(e.target.value)}
                    >
                      <MenuItem value="viridis">Viridis</MenuItem>
                      <MenuItem value="hot">Hot</MenuItem>
                      <MenuItem value="cool">Cool</MenuItem>
                    </Select>
                  </FormControl>
                </Grid>
                
                {/* Intensity Control */}
                <Grid item xs={12} sm={3}>
                  <Typography gutterBottom>Intensity</Typography>
                  <Slider
                    value={intensity}
                    onChange={(e, newValue) => setIntensity(newValue)}
                    min={0.1}
                    max={3.0}
                    step={0.1}
                    valueLabelDisplay="auto"
                  />
                </Grid>
                
                {/* Time Window */}
                <Grid item xs={12} sm={3}>
                  <Typography gutterBottom>Time Window (seconds)</Typography>
                  <Slider
                    value={timeWindow}
                    onChange={(e, newValue) => setTimeWindow(newValue)}
                    min={30}
                    max={1800}
                    step={30}
                    valueLabelDisplay="auto"
                    disabled={playbackMode}
                  />
                </Grid>
                
                {/* Action Buttons */}
                <Grid item>
                  <ButtonGroup variant="outlined" size="small">
                    <Tooltip title="Clear History">
                      <Button onClick={clearHistory} startIcon={<ClearIcon />}>
                        Clear
                      </Button>
                    </Tooltip>
                    
                    <Tooltip title="Export JSON">
                      <Button onClick={() => exportData('json')} startIcon={<DownloadIcon />}>
                        JSON
                      </Button>
                    </Tooltip>
                    
                    <Tooltip title="Export CSV">
                      <Button onClick={() => exportData('csv')} startIcon={<DownloadIcon />}>
                        CSV
                      </Button>
                    </Tooltip>
                  </ButtonGroup>
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>
        
        {/* Playback Controls */}
        {positionHistory.length > 0 && (
          <Grid item xs={12}>
            <Card>
              <CardContent>
                <Typography variant="h6" gutterBottom>
                  Historical Playback
                </Typography>
                
                <Grid container spacing={2} alignItems="center">
                  <Grid item>
                    <ButtonGroup>
                      <Button
                        onClick={playbackMode ? exitPlaybackMode : enterPlaybackMode}
                        variant={playbackMode ? "contained" : "outlined"}
                        startIcon={<HistoryIcon />}
                      >
                        {playbackMode ? 'Live' : 'Playback'}
                      </Button>
                      
                      {playbackMode && (
                        <Button
                          onClick={togglePlayback}
                          startIcon={isPlaying ? <PauseIcon /> : <PlayIcon />}
                        >
                          {isPlaying ? 'Pause' : 'Play'}
                        </Button>
                      )}
                    </ButtonGroup>
                  </Grid>
                  
                  {playbackMode && (
                    <>
                      <Grid item xs={12} sm={6}>
                        <Typography gutterBottom>Position</Typography>
                        <Slider
                          value={playbackPosition}
                          onChange={(e, newValue) => setPlaybackPosition(newValue)}
                          min={0}
                          max={positionHistory.length * 1000}
                          step={1000}
                          valueLabelDisplay="auto"
                          valueLabelFormat={(value) => `${Math.floor(value / 1000)}s`}
                        />
                      </Grid>
                      
                      <Grid item xs={12} sm={2}>
                        <Typography gutterBottom>Speed</Typography>
                        <Slider
                          value={playbackSpeed}
                          onChange={(e, newValue) => setPlaybackSpeed(newValue)}
                          min={0.1}
                          max={5.0}
                          step={0.1}
                          valueLabelDisplay="auto"
                          valueLabelFormat={(value) => `${value}x`}
                        />
                      </Grid>
                    </>
                  )}
                </Grid>
              </CardContent>
            </Card>
          </Grid>
        )}
        
        {/* 3D Heatmap Visualization */}
        <Grid item xs={12}>
          <Card>
            <Box height={600}>
              <Canvas camera={{ position: [15, 15, 15], fov: 60 }}>
                <ambientLight intensity={0.4} />
                <pointLight position={[10, 10, 10]} />
                <HeatmapMesh 
                  data={visiblePositions}
                  colorScheme={colorScheme}
                  intensity={intensity}
                  showNodes={showNodes}
                  nodePositions={nodePositions}
                />
                <OrbitControls enablePan={true} enableZoom={true} enableRotate={true} />
                
                {/* Grid helper */}
                <gridHelper args={[20, 20]} position={[0, 0, 0]} />
                
                {/* Axis helper */}
                <axesHelper args={[5]} />
              </Canvas>
            </Box>
          </Card>
        </Grid>
        
        {/* Statistics */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Heatmap Statistics
              </Typography>
              
              <Grid container spacing={2}>
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">
                    Total Positions
                  </Typography>
                  <Typography variant="h6">
                    {positionHistory.length.toLocaleString()}
                  </Typography>
                </Grid>
                
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">
                    Visible Positions
                  </Typography>
                  <Typography variant="h6">
                    {visiblePositions.length.toLocaleString()}
                  </Typography>
                </Grid>
                
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">
                    Avg Confidence
                  </Typography>
                  <Typography variant="h6">
                    {visiblePositions.length > 0 
                      ? (visiblePositions.reduce((sum, pos) => sum + (pos.confidence || 0), 0) / visiblePositions.length * 100).toFixed(1)
                      : 0}%
                  </Typography>
                </Grid>
                
                <Grid item xs={6}>
                  <Typography variant="body2" color="textSecondary">
                    Active Nodes
                  </Typography>
                  <Typography variant="h6">
                    {Object.keys(nodePositions).length}
                  </Typography>
                </Grid>
              </Grid>
              
              {playbackMode && (
                <Box mt={2}>
                  <Chip
                    label={`Playback: ${Math.floor(playbackPosition / 1000)}s / ${Math.floor(positionHistory.length)}s`}
                    color="primary"
                  />
                </Box>
              )}
            </CardContent>
          </Card>
        </Grid>
        
        {/* Color Legend */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Color Legend
              </Typography>
              
              <Box mb={2}>
                <Typography variant="body2" gutterBottom>
                  Position Confidence
                </Typography>
                <Box display="flex" alignItems="center" gap={1}>
                  <Box width={20} height={20} bgcolor="#f57c00" borderRadius="50%" />
                  <Typography variant="body2">Low (&lt;70%)</Typography>
                  <Box width={20} height={20} bgcolor="#4caf50" borderRadius="50%" ml={2} />
                  <Typography variant="body2">High (≥70%)</Typography>
                </Box>
              </Box>
              
              <Box mb={2}>
                <Typography variant="body2" gutterBottom>
                  Heatmap Intensity ({colorScheme})
                </Typography>
                <Box 
                  height={20} 
                  width="100%"
                  sx={{
                    background: colorScheme === 'viridis' 
                      ? 'linear-gradient(to right, #440154, #3b528b, #21918c, #5ec962, #fde725)'
                      : colorScheme === 'hot'
                      ? 'linear-gradient(to right, #000000, #800000, #ff0000, #ff8000, #ffff00, #ffffff)'
                      : 'linear-gradient(to right, #00ffff, #8080ff, #ff00ff)',
                    borderRadius: 1
                  }}
                />
                <Box display="flex" justifyContent="space-between" mt={1}>
                  <Typography variant="body2">Low</Typography>
                  <Typography variant="body2">High</Typography>
                </Box>
              </Box>
            </CardContent>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
};

export default HeatmapView;