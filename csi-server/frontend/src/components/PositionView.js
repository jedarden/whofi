/**
 * Position Visualization Component
 * Interactive map showing real-time position tracking
 */

import React, { useState, useEffect, useRef } from 'react';
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
  Paper,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow
} from '@mui/material';
import {
  MyLocation as MyLocationIcon,
  History as HistoryIcon,
  Clear as ClearIcon
} from '@mui/icons-material';
import { MapContainer, TileLayer, Marker, Popup, Polyline, Circle } from 'react-leaflet';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';

import { useCurrentPosition, usePositions } from '../hooks/useApi';
import { useWebSocket } from '../context/WebSocketContext';

// Fix for default markers in react-leaflet
delete L.Icon.Default.prototype._getIconUrl;
L.Icon.Default.mergeOptions({
  iconRetinaUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon-2x.png',
  iconUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-icon.png',
  shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
});

// Custom icons
const currentPositionIcon = L.divIcon({
  html: '<div class="position-marker"></div>',
  className: 'custom-position-marker',
  iconSize: [20, 20],
  iconAnchor: [10, 10]
});

const nodeIcon = L.divIcon({
  html: '<div style="background-color: #666; width: 12px; height: 12px; border-radius: 50%; border: 2px solid white;"></div>',
  className: 'node-marker',
  iconSize: [16, 16],
  iconAnchor: [8, 8]
});

const PositionView = () => {
  const { data: currentPosition } = useCurrentPosition();
  const { lastMessage } = useWebSocket();
  const [positionTrail, setPositionTrail] = useState([]);
  const [showTrail, setShowTrail] = useState(true);
  const [showNodes, setShowNodes] = useState(true);
  const [autoCenter, setAutoCenter] = useState(true);
  const mapRef = useRef(null);

  // Node positions (should come from config)
  const nodePositions = {
    'esp32_001': { x: 0, y: 0, id: 'esp32_001' },
    'esp32_002': { x: 10, y: 0, id: 'esp32_002' },
    'esp32_003': { x: 5, y: 10, id: 'esp32_003' },
    'esp32_004': { x: 0, y: 10, id: 'esp32_004' }
  };

  // Convert coordinate system (adjust for your room layout)
  const convertToMapCoords = (x, y) => {
    // Convert from positioning coordinates to lat/lng for display
    // This is a simple conversion - adjust for your actual coordinate system
    const baseLatLng = [40.7128, -74.0060]; // Example: NYC coordinates
    const scale = 0.0001; // Adjust scale factor
    
    return [
      baseLatLng[0] + (y * scale),
      baseLatLng[1] + (x * scale)
    ];
  };

  const convertFromMapCoords = (lat, lng) => {
    const baseLatLng = [40.7128, -74.0060];
    const scale = 0.0001;
    
    return {
      x: (lng - baseLatLng[1]) / scale,
      y: (lat - baseLatLng[0]) / scale
    };
  };

  // Update position trail from WebSocket
  useEffect(() => {
    if (lastMessage?.type === 'position_update') {
      const position = lastMessage.position;
      const coords = convertToMapCoords(position.x, position.y);
      
      setPositionTrail(prev => {
        const newTrail = [...prev, {
          coords,
          timestamp: new Date(position.timestamp),
          confidence: position.confidence,
          original: { x: position.x, y: position.y }
        }];
        // Keep only last 100 points
        return newTrail.slice(-100);
      });

      // Auto-center map on new position
      if (autoCenter && mapRef.current) {
        mapRef.current.setView(coords, mapRef.current.getZoom());
      }
    }
  }, [lastMessage, autoCenter]);

  const clearTrail = () => {
    setPositionTrail([]);
  };

  const centerOnCurrent = () => {
    if (currentPosition && mapRef.current) {
      const coords = convertToMapCoords(currentPosition.x, currentPosition.y);
      mapRef.current.setView(coords, 18);
    }
  };

  // Get trail line coordinates
  const trailCoords = positionTrail.map(point => point.coords);

  // Get current position coordinates
  const currentCoords = currentPosition 
    ? convertToMapCoords(currentPosition.x, currentPosition.y)
    : null;

  // Get confidence circle radius (in meters, adjust scale)
  const confidenceRadius = currentPosition?.confidence 
    ? (1 - currentPosition.confidence) * 10 // Higher confidence = smaller radius
    : 5;

  return (
    <Box>
      <Typography variant="h4" gutterBottom>
        Position Tracking
      </Typography>

      <Grid container spacing={3}>
        {/* Map Controls */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Box display="flex" alignItems="center" gap={2} flexWrap="wrap">
                <FormControlLabel
                  control={
                    <Switch
                      checked={showTrail}
                      onChange={(e) => setShowTrail(e.target.checked)}
                    />
                  }
                  label="Show Trail"
                />
                
                <FormControlLabel
                  control={
                    <Switch
                      checked={showNodes}
                      onChange={(e) => setShowNodes(e.target.checked)}
                    />
                  }
                  label="Show Nodes"
                />
                
                <FormControlLabel
                  control={
                    <Switch
                      checked={autoCenter}
                      onChange={(e) => setAutoCenter(e.target.checked)}
                    />
                  }
                  label="Auto Center"
                />

                <Button
                  startIcon={<MyLocationIcon />}
                  onClick={centerOnCurrent}
                  variant="outlined"
                  disabled={!currentPosition}
                >
                  Center on Current
                </Button>

                <Button
                  startIcon={<ClearIcon />}
                  onClick={clearTrail}
                  variant="outlined"
                  disabled={positionTrail.length === 0}
                >
                  Clear Trail
                </Button>

                {currentPosition && (
                  <Box>
                    <Chip
                      label={`Position: (${currentPosition.x.toFixed(2)}, ${currentPosition.y.toFixed(2)})`}
                      color="primary"
                    />
                    <Chip
                      label={`Confidence: ${(currentPosition.confidence * 100).toFixed(0)}%`}
                      color={currentPosition.confidence > 0.7 ? 'success' : 'warning'}
                      sx={{ ml: 1 }}
                    />
                  </Box>
                )}
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Interactive Map */}
        <Grid item xs={12} md={8}>
          <Card>
            <Box height={500}>
              <MapContainer
                center={[40.7128, -74.0060]} // Default center
                zoom={18}
                style={{ height: '100%', width: '100%' }}
                ref={mapRef}
              >
                <TileLayer
                  attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
                  url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
                />

                {/* Current Position */}
                {currentCoords && (
                  <>
                    <Marker position={currentCoords} icon={currentPositionIcon}>
                      <Popup>
                        <div>
                          <strong>Current Position</strong><br />
                          X: {currentPosition.x.toFixed(2)}<br />
                          Y: {currentPosition.y.toFixed(2)}<br />
                          Confidence: {(currentPosition.confidence * 100).toFixed(0)}%<br />
                          Time: {new Date(currentPosition.timestamp).toLocaleTimeString()}
                        </div>
                      </Popup>
                    </Marker>

                    {/* Confidence Circle */}
                    <Circle
                      center={currentCoords}
                      radius={confidenceRadius}
                      pathOptions={{
                        color: currentPosition.confidence > 0.7 ? 'green' : 'orange',
                        fillOpacity: 0.1
                      }}
                    />
                  </>
                )}

                {/* Position Trail */}
                {showTrail && trailCoords.length > 1 && (
                  <Polyline
                    positions={trailCoords}
                    pathOptions={{
                      color: '#1976d2',
                      weight: 3,
                      opacity: 0.7
                    }}
                  />
                )}

                {/* Node Positions */}
                {showNodes && Object.values(nodePositions).map(node => {
                  const nodeCoords = convertToMapCoords(node.x, node.y);
                  return (
                    <Marker key={node.id} position={nodeCoords} icon={nodeIcon}>
                      <Popup>
                        <div>
                          <strong>Node: {node.id}</strong><br />
                          Position: ({node.x}, {node.y})
                        </div>
                      </Popup>
                    </Marker>
                  );
                })}
              </MapContainer>
            </Box>
          </Card>
        </Grid>

        {/* Position History Table */}
        <Grid item xs={12} md={4}>
          <Card>
            <CardContent>
              <Typography variant="h6" gutterBottom>
                Recent Positions
              </Typography>
              
              <TableContainer component={Paper} sx={{ maxHeight: 400 }}>
                <Table stickyHeader size="small">
                  <TableHead>
                    <TableRow>
                      <TableCell>Time</TableCell>
                      <TableCell align="right">X</TableCell>
                      <TableCell align="right">Y</TableCell>
                      <TableCell align="right">Conf.</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {positionTrail.slice(-20).reverse().map((point, index) => (
                      <TableRow key={index}>
                        <TableCell>
                          {point.timestamp.toLocaleTimeString()}
                        </TableCell>
                        <TableCell align="right">
                          {point.original.x.toFixed(1)}
                        </TableCell>
                        <TableCell align="right">
                          {point.original.y.toFixed(1)}
                        </TableCell>
                        <TableCell align="right">
                          <Chip
                            label={`${(point.confidence * 100).toFixed(0)}%`}
                            size="small"
                            color={point.confidence > 0.7 ? 'success' : 'warning'}
                          />
                        </TableCell>
                      </TableRow>
                    ))}
                    {positionTrail.length === 0 && (
                      <TableRow>
                        <TableCell colSpan={4} align="center">
                          <Typography color="textSecondary">
                            No position data available
                          </Typography>
                        </TableCell>
                      </TableRow>
                    )}
                  </TableBody>
                </Table>
              </TableContainer>
            </CardContent>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
};

export default PositionView;