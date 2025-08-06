/**
 * Node Calibration Interface
 * Interactive calibration system for CSI positioning nodes
 */

import React, { useState, useEffect, useRef } from 'react';
import {
  Box,
  Card,
  CardContent,
  Typography,
  Grid,
  Button,
  TextField,
  Stepper,
  Step,
  StepLabel,
  StepContent,
  Paper,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Dialog,
  DialogActions,
  DialogContent,
  DialogContentText,
  DialogTitle,
  Chip,
  Alert,
  AlertTitle,
  LinearProgress,
  IconButton,
  Tooltip,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Accordion,
  AccordionSummary,
  AccordionDetails
} from '@mui/material';
import {
  PlayArrow as StartIcon,
  Stop as StopIcon,
  Save as SaveIcon,
  Refresh as RefreshIcon,
  LocationOn as LocationIcon,
  Settings as SettingsIcon,
  CheckCircle as CheckCircleIcon,
  Error as ErrorIcon,
  ExpandMore as ExpandMoreIcon,
  Help as HelpIcon,
  Upload as UploadIcon,
  Download as DownloadIcon
} from '@mui/icons-material';
import { MapContainer, TileLayer, Marker, useMapEvents } from 'react-leaflet';
import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import { useWebSocket } from '../context/WebSocketContext';
import { useNodes } from '../hooks/useApi';

// Calibration steps
const calibrationSteps = [
  'Node Discovery',
  'Physical Position Setup',
  'Signal Baseline Collection',
  'Reference Point Calibration',
  'Validation & Testing',
  'Save Configuration'
];

// Custom marker for calibration points
const calibrationIcon = L.divIcon({
  html: '<div style="background-color: #ff5722; width: 20px; height: 20px; border-radius: 50%; border: 3px solid white; box-shadow: 0 2px 4px rgba(0,0,0,0.3);"></div>',
  className: 'calibration-marker',
  iconSize: [26, 26],
  iconAnchor: [13, 13]
});

// Interactive map component for position setup
const PositionSetupMap = ({ nodes, onNodePositionUpdate, calibrationPoints, onCalibrationPointAdd }) => {
  const [selectedNode, setSelectedNode] = useState(null);
  
  const MapEvents = () => {
    useMapEvents({
      click: (e) => {
        if (selectedNode) {
          onNodePositionUpdate(selectedNode, {
            lat: e.latlng.lat,
            lng: e.latlng.lng
          });
          setSelectedNode(null);
        } else {
          onCalibrationPointAdd({
            id: Date.now(),
            lat: e.latlng.lat,
            lng: e.latlng.lng,
            name: `Point ${calibrationPoints.length + 1}`
          });
        }
      }
    });
    return null;
  };
  
  return (
    <Box height={400} mb={2}>
      <MapContainer
        center={[40.7128, -74.0060]}
        zoom={18}
        style={{ height: '100%', width: '100%' }}
      >
        <TileLayer
          url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
          attribution='&copy; OpenStreetMap contributors'
        />
        <MapEvents />
        
        {/* Node markers */}
        {nodes.map(node => (
          <Marker 
            key={node.id}
            position={[node.position?.lat || 40.7128, node.position?.lng || -74.0060]}
            eventHandlers={{
              click: () => setSelectedNode(node.id)
            }}
          >
          </Marker>
        ))}
        
        {/* Calibration point markers */}
        {calibrationPoints.map(point => (
          <Marker 
            key={point.id}
            position={[point.lat, point.lng]}
            icon={calibrationIcon}
          />
        ))}
      </MapContainer>
      
      <Box mt={1}>
        <Typography variant="body2" color="textSecondary">
          Click on a node to select it, then click on the map to set its position. 
          Click anywhere else to add calibration points.
        </Typography>
        {selectedNode && (
          <Alert severity="info" sx={{ mt: 1 }}>
            Selected node: {selectedNode}. Click on the map to set its position.
          </Alert>
        )}
      </Box>
    </Box>
  );
};

// Signal collection progress
const SignalCollection = ({ nodes, isCollecting, collectionProgress, onStart, onStop }) => {
  return (
    <Card>
      <CardContent>
        <Typography variant="h6" gutterBottom>
          Signal Baseline Collection
        </Typography>
        
        <Grid container spacing={2} alignItems="center">
          <Grid item xs={12} md={6}>
            <Button
              variant={isCollecting ? "outlined" : "contained"}
              color={isCollecting ? "error" : "primary"}
              startIcon={isCollecting ? <StopIcon /> : <StartIcon />}
              onClick={isCollecting ? onStop : onStart}
              fullWidth
            >
              {isCollecting ? 'Stop Collection' : 'Start Collection'}
            </Button>
          </Grid>
          
          <Grid item xs={12} md={6}>
            <Typography variant="body2" gutterBottom>
              Progress: {collectionProgress.toFixed(1)}%
            </Typography>
            <LinearProgress 
              variant="determinate" 
              value={collectionProgress} 
              sx={{ height: 8, borderRadius: 4 }}
            />
          </Grid>
        </Grid>
        
        <Box mt={2}>
          <TableContainer component={Paper}>
            <Table size="small">
              <TableHead>
                <TableRow>
                  <TableCell>Node ID</TableCell>
                  <TableCell align="right">Samples</TableCell>
                  <TableCell align="right">RSSI Range</TableCell>
                  <TableCell align="right">Status</TableCell>
                </TableRow>
              </TableHead>
              <TableBody>
                {nodes.map(node => {
                  const samples = node.calibrationData?.samples || 0;
                  const rssiMin = node.calibrationData?.rssiMin || 0;
                  const rssiMax = node.calibrationData?.rssiMax || 0;
                  const isComplete = samples >= 100; // Minimum samples required
                  
                  return (
                    <TableRow key={node.id}>
                      <TableCell>{node.id}</TableCell>
                      <TableCell align="right">{samples}</TableCell>
                      <TableCell align="right">
                        {rssiMin.toFixed(0)} to {rssiMax.toFixed(0)} dBm
                      </TableCell>
                      <TableCell align="right">
                        <Chip
                          icon={isComplete ? <CheckCircleIcon /> : <ErrorIcon />}
                          label={isComplete ? 'Complete' : 'Collecting'}
                          color={isComplete ? 'success' : 'warning'}
                          size="small"
                        />
                      </TableCell>
                    </TableRow>
                  );
                })}
              </TableBody>
            </Table>
          </TableContainer>
        </Box>
      </CardContent>
    </Card>
  );
};

// Reference point calibration
const ReferencePointCalibration = ({ 
  calibrationPoints, 
  currentPoint, 
  onNextPoint, 
  onPreviousPoint, 
  isCollecting, 
  collectionProgress,
  onStartCollection,
  onStopCollection 
}) => {
  if (calibrationPoints.length === 0) {
    return (
      <Alert severity="warning">
        <AlertTitle>No Calibration Points</AlertTitle>
        Please add calibration points in the previous step.
      </Alert>
    );
  }
  
  const current = calibrationPoints[currentPoint];
  
  return (
    <Card>
      <CardContent>
        <Typography variant="h6" gutterBottom>
          Reference Point Calibration
        </Typography>
        
        <Typography variant="body1" gutterBottom>
          Stand at reference point: <strong>{current.name}</strong>
        </Typography>
        
        <Grid container spacing={2} alignItems="center">
          <Grid item>
            <Button
              variant="outlined"
              onClick={onPreviousPoint}
              disabled={currentPoint === 0}
            >
              Previous Point
            </Button>
          </Grid>
          
          <Grid item>
            <Chip
              label={`Point ${currentPoint + 1} of ${calibrationPoints.length}`}
              color="primary"
            />
          </Grid>
          
          <Grid item>
            <Button
              variant="outlined"
              onClick={onNextPoint}
              disabled={currentPoint === calibrationPoints.length - 1}
            >
              Next Point
            </Button>
          </Grid>
        </Grid>
        
        <Box mt={2}>
          <Typography variant="body2" gutterBottom>
            Collection Progress: {collectionProgress.toFixed(1)}%
          </Typography>
          <LinearProgress 
            variant="determinate" 
            value={collectionProgress} 
            sx={{ height: 8, borderRadius: 4, mb: 2 }}
          />
          
          <Button
            variant={isCollecting ? "outlined" : "contained"}
            color={isCollecting ? "error" : "primary"}
            startIcon={isCollecting ? <StopIcon /> : <StartIcon />}
            onClick={isCollecting ? onStopCollection : onStartCollection}
            fullWidth
            size="large"
          >
            {isCollecting ? 'Stop Collection' : 'Start Collection at This Point'}
          </Button>
        </Box>
        
        {isCollecting && (
          <Alert severity="info" sx={{ mt: 2 }}>
            Stand still at the marked position while collecting calibration data...
          </Alert>
        )}
      </CardContent>
    </Card>
  );
};

// Validation and testing
const ValidationTesting = ({ nodes, validationResults, onRunValidation, isValidating }) => {
  return (
    <Card>
      <CardContent>
        <Typography variant="h6" gutterBottom>
          Calibration Validation
        </Typography>
        
        <Button
          variant="contained"
          startIcon={<RefreshIcon />}
          onClick={onRunValidation}
          disabled={isValidating}
          sx={{ mb: 2 }}
        >
          {isValidating ? 'Running Validation...' : 'Run Validation Test'}
        </Button>
        
        {isValidating && (
          <LinearProgress sx={{ mb: 2 }} />
        )}
        
        {validationResults && (
          <Box>
            <Typography variant="subtitle1" gutterBottom>
              Validation Results
            </Typography>
            
            <Grid container spacing={2}>
              <Grid item xs={12} sm={6}>
                <Card variant="outlined">
                  <CardContent>
                    <Typography variant="body2" color="textSecondary">
                      Average Positioning Error
                    </Typography>
                    <Typography variant="h4" color={validationResults.avgError < 1 ? 'success.main' : 'warning.main'}>
                      {validationResults.avgError.toFixed(2)}m
                    </Typography>
                  </CardContent>
                </Card>
              </Grid>
              
              <Grid item xs={12} sm={6}>
                <Card variant="outlined">
                  <CardContent>
                    <Typography variant="body2" color="textSecondary">
                      95% Confidence Interval
                    </Typography>
                    <Typography variant="h4" color={validationResults.confidence95 < 2 ? 'success.main' : 'warning.main'}>
                      ±{validationResults.confidence95.toFixed(2)}m
                    </Typography>
                  </CardContent>
                </Card>
              </Grid>
            </Grid>
            
            <Box mt={2}>
              <Typography variant="subtitle2" gutterBottom>
                Per-Node Performance
              </Typography>
              <TableContainer component={Paper}>
                <Table size="small">
                  <TableHead>
                    <TableRow>
                      <TableCell>Node ID</TableCell>
                      <TableCell align="right">Signal Quality</TableCell>
                      <TableCell align="right">Coverage Area</TableCell>
                      <TableCell align="right">Status</TableCell>
                    </TableRow>
                  </TableHead>
                  <TableBody>
                    {nodes.map(node => {
                      const performance = validationResults.nodePerformance?.[node.id];
                      return (
                        <TableRow key={node.id}>
                          <TableCell>{node.id}</TableCell>
                          <TableCell align="right">
                            {performance?.signalQuality ? 
                              `${(performance.signalQuality * 100).toFixed(0)}%` : 
                              'N/A'}
                          </TableCell>
                          <TableCell align="right">
                            {performance?.coverageArea ? 
                              `${performance.coverageArea.toFixed(1)}m²` : 
                              'N/A'}
                          </TableCell>
                          <TableCell align="right">
                            <Chip
                              label={performance?.status || 'Unknown'}
                              color={performance?.status === 'Good' ? 'success' : 
                                     performance?.status === 'Fair' ? 'warning' : 'error'}
                              size="small"
                            />
                          </TableCell>
                        </TableRow>
                      );
                    })}
                  </TableBody>
                </Table>
              </TableContainer>
            </Box>
          </Box>
        )}
      </CardContent>
    </Card>
  );
};

const CalibrationInterface = () => {
  const { lastMessage } = useWebSocket();
  const { data: nodesData } = useNodes();
  const [activeStep, setActiveStep] = useState(0);
  const [nodes, setNodes] = useState([]);
  const [calibrationPoints, setCalibrationPoints] = useState([]);
  const [currentCalibrationPoint, setCurrentCalibrationPoint] = useState(0);
  const [isCollecting, setIsCollecting] = useState(false);
  const [collectionProgress, setCollectionProgress] = useState(0);
  const [validationResults, setValidationResults] = useState(null);
  const [isValidating, setIsValidating] = useState(false);
  const [saveDialogOpen, setSaveDialogOpen] = useState(false);
  const [configName, setConfigName] = useState('');
  
  // Initialize nodes from API data
  useEffect(() => {
    if (nodesData?.nodes) {
      setNodes(nodesData.nodes.map(node => ({
        ...node,
        calibrationData: {
          samples: 0,
          rssiMin: 0,
          rssiMax: 0,
          csiData: []
        }
      })));
    }
  }, [nodesData]);
  
  // Process incoming calibration data
  useEffect(() => {
    if (lastMessage?.type === 'calibration_data' && isCollecting) {
      const { nodeId, rssi, csi } = lastMessage.data;
      
      setNodes(prev => prev.map(node => {
        if (node.id === nodeId) {
          const calibrationData = node.calibrationData || { samples: 0, rssiMin: 0, rssiMax: 0, csiData: [] };
          return {
            ...node,
            calibrationData: {
              ...calibrationData,
              samples: calibrationData.samples + 1,
              rssiMin: Math.min(calibrationData.rssiMin, rssi),
              rssiMax: Math.max(calibrationData.rssiMax, rssi),
              csiData: [...calibrationData.csiData.slice(-99), { rssi, csi, timestamp: Date.now() }]
            }
          };
        }
        return node;
      }));
      
      // Update collection progress
      const totalSamples = nodes.reduce((sum, node) => sum + (node.calibrationData?.samples || 0), 0);
      const targetSamples = nodes.length * 100; // 100 samples per node
      setCollectionProgress((totalSamples / targetSamples) * 100);
    }
  }, [lastMessage, isCollecting, nodes]);
  
  const handleNext = () => {
    setActiveStep((prevActiveStep) => prevActiveStep + 1);
  };
  
  const handleBack = () => {
    setActiveStep((prevActiveStep) => prevActiveStep - 1);
  };
  
  const handleNodePositionUpdate = (nodeId, position) => {
    setNodes(prev => prev.map(node => 
      node.id === nodeId ? { ...node, position } : node
    ));
  };
  
  const handleCalibrationPointAdd = (point) => {
    setCalibrationPoints(prev => [...prev, point]);
  };
  
  const startSignalCollection = () => {
    setIsCollecting(true);
    setCollectionProgress(0);
    // Send command to nodes to start calibration mode
    // websocket.send({ type: 'start_calibration' });
  };
  
  const stopSignalCollection = () => {
    setIsCollecting(false);
    // Send command to nodes to stop calibration mode
    // websocket.send({ type: 'stop_calibration' });
  };
  
  const startReferencePointCollection = () => {
    setIsCollecting(true);
    setCollectionProgress(0);
    const currentPoint = calibrationPoints[currentCalibrationPoint];
    // Send command with current reference point position
    // websocket.send({ 
    //   type: 'start_reference_calibration', 
    //   position: currentPoint 
    // });
  };
  
  const runValidation = () => {
    setIsValidating(true);
    
    // Simulate validation process
    setTimeout(() => {
      const mockResults = {
        avgError: Math.random() * 2 + 0.5, // 0.5-2.5m
        confidence95: Math.random() * 3 + 1, // 1-4m
        nodePerformance: {}
      };
      
      nodes.forEach(node => {
        mockResults.nodePerformance[node.id] = {
          signalQuality: Math.random() * 0.4 + 0.6, // 60-100%
          coverageArea: Math.random() * 50 + 25, // 25-75m²
          status: Math.random() > 0.2 ? 'Good' : 'Fair'
        };
      });
      
      setValidationResults(mockResults);
      setIsValidating(false);
    }, 3000);
  };
  
  const saveCalibration = () => {
    const calibrationConfig = {
      name: configName,
      timestamp: new Date().toISOString(),
      nodes: nodes.map(node => ({
        id: node.id,
        position: node.position,
        calibrationData: node.calibrationData
      })),
      calibrationPoints,
      validationResults
    };
    
    // Save to backend
    // api.saveCalibrationConfig(calibrationConfig);
    
    // Download as backup
    const blob = new Blob([JSON.stringify(calibrationConfig, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `calibration_${configName}_${Date.now()}.json`;
    a.click();
    URL.revokeObjectURL(url);
    
    setSaveDialogOpen(false);
    setConfigName('');
  };
  
  const getStepContent = (step) => {
    switch (step) {
      case 0:
        return (
          <Box>
            <Typography variant="body1" gutterBottom>
              {nodes.length} nodes discovered. Verify all nodes are online and responding.
            </Typography>
            <TableContainer component={Paper}>
              <Table>
                <TableHead>
                  <TableRow>
                    <TableCell>Node ID</TableCell>
                    <TableCell align="right">Status</TableCell>
                    <TableCell align="right">RSSI</TableCell>
                    <TableCell align="right">Last Seen</TableCell>
                  </TableRow>
                </TableHead>
                <TableBody>
                  {nodes.map(node => (
                    <TableRow key={node.id}>
                      <TableCell>{node.id}</TableCell>
                      <TableCell align="right">
                        <Chip 
                          label={node.status || 'Online'}
                          color={node.status === 'Online' ? 'success' : 'error'}
                          size="small"
                        />
                      </TableCell>
                      <TableCell align="right">{node.rssi || 'N/A'}</TableCell>
                      <TableCell align="right">
                        {node.lastSeen ? new Date(node.lastSeen).toLocaleTimeString() : 'N/A'}
                      </TableCell>
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </TableContainer>
          </Box>
        );
        
      case 1:
        return (
          <Box>
            <Typography variant="body1" gutterBottom>
              Set the physical positions of each node and add calibration reference points.
            </Typography>
            <PositionSetupMap 
              nodes={nodes}
              onNodePositionUpdate={handleNodePositionUpdate}
              calibrationPoints={calibrationPoints}
              onCalibrationPointAdd={handleCalibrationPointAdd}
            />
          </Box>
        );
        
      case 2:
        return (
          <SignalCollection 
            nodes={nodes}
            isCollecting={isCollecting}
            collectionProgress={collectionProgress}
            onStart={startSignalCollection}
            onStop={stopSignalCollection}
          />
        );
        
      case 3:
        return (
          <ReferencePointCalibration 
            calibrationPoints={calibrationPoints}
            currentPoint={currentCalibrationPoint}
            onNextPoint={() => setCurrentCalibrationPoint(prev => prev + 1)}
            onPreviousPoint={() => setCurrentCalibrationPoint(prev => prev - 1)}
            isCollecting={isCollecting}
            collectionProgress={collectionProgress}
            onStartCollection={startReferencePointCollection}
            onStopCollection={stopSignalCollection}
          />
        );
        
      case 4:
        return (
          <ValidationTesting 
            nodes={nodes}
            validationResults={validationResults}
            onRunValidation={runValidation}
            isValidating={isValidating}
          />
        );
        
      case 5:
        return (
          <Box>
            <Typography variant="body1" gutterBottom>
              Calibration complete! Review the results and save the configuration.
            </Typography>
            
            <Button
              variant="contained"
              startIcon={<SaveIcon />}
              onClick={() => setSaveDialogOpen(true)}
              size="large"
              fullWidth
              sx={{ mt: 2 }}
            >
              Save Calibration Configuration
            </Button>
            
            {validationResults && (
              <Alert severity="success" sx={{ mt: 2 }}>
                <AlertTitle>Calibration Quality</AlertTitle>
                Average positioning error: {validationResults.avgError.toFixed(2)}m
                <br />
                95% confidence interval: ±{validationResults.confidence95.toFixed(2)}m
              </Alert>
            )}
          </Box>
        );
        
      default:
        return 'Unknown step';
    }
  };
  
  return (
    <Box>
      <Typography variant="h4" gutterBottom>
        Node Calibration Interface
      </Typography>
      
      <Grid container spacing={3}>
        <Grid item xs={12} md={4}>
          <Paper sx={{ p: 2 }}>
            <Stepper activeStep={activeStep} orientation="vertical">
              {calibrationSteps.map((label, index) => (
                <Step key={label}>
                  <StepLabel>{label}</StepLabel>
                  <StepContent>
                    <Box sx={{ mb: 2 }}>
                      <Button
                        variant="contained"
                        onClick={handleNext}
                        sx={{ mt: 1, mr: 1 }}
                        disabled={
                          (index === 0 && nodes.length === 0) ||
                          (index === 2 && collectionProgress < 100) ||
                          (index === 4 && !validationResults)
                        }
                      >
                        {index === calibrationSteps.length - 1 ? 'Finish' : 'Continue'}
                      </Button>
                      <Button
                        disabled={index === 0}
                        onClick={handleBack}
                        sx={{ mt: 1, mr: 1 }}
                      >
                        Back
                      </Button>
                    </Box>
                  </StepContent>
                </Step>
              ))}
            </Stepper>
          </Paper>
        </Grid>
        
        <Grid item xs={12} md={8}>
          <Card>
            <CardContent>
              <Typography variant="h5" gutterBottom>
                {calibrationSteps[activeStep]}
              </Typography>
              {getStepContent(activeStep)}
            </CardContent>
          </Card>
        </Grid>
      </Grid>
      
      {/* Save Configuration Dialog */}
      <Dialog open={saveDialogOpen} onClose={() => setSaveDialogOpen(false)}>
        <DialogTitle>Save Calibration Configuration</DialogTitle>
        <DialogContent>
          <DialogContentText>
            Enter a name for this calibration configuration. It will be saved to the server
            and downloaded as a backup file.
          </DialogContentText>
          <TextField
            autoFocus
            margin="dense"
            label="Configuration Name"
            fullWidth
            variant="outlined"
            value={configName}
            onChange={(e) => setConfigName(e.target.value)}
            sx={{ mt: 2 }}
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setSaveDialogOpen(false)}>Cancel</Button>
          <Button 
            onClick={saveCalibration}
            disabled={!configName.trim()}
            variant="contained"
          >
            Save
          </Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
};

export default CalibrationInterface;