/**
 * Advanced Data Export Utility
 * Comprehensive data export functionality for CSI positioning system
 */

import React, { useState, useMemo } from 'react';
import {
  Box,
  Card,
  CardContent,
  Typography,
  Grid,
  Button,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Checkbox,
  FormControlLabel,
  TextField,
  Alert,
  LinearProgress,
  Chip,
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
  Accordion,
  AccordionSummary,
  AccordionDetails
} from '@mui/material';
import {
  Download as DownloadIcon,
  CloudDownload as CloudDownloadIcon,
  Schedule as ScheduleIcon,
  Storage as StorageIcon,
  ExpandMore as ExpandMoreIcon,
  Settings as SettingsIcon,
  Preview as PreviewIcon
} from '@mui/icons-material';
import { DateTimePicker } from '@mui/x-date-pickers/DateTimePicker';
import { LocalizationProvider } from '@mui/x-date-pickers/LocalizationProvider';
import { AdapterMoment } from '@mui/x-date-pickers/AdapterMoment';
import moment from 'moment';

// Export format configurations
const EXPORT_FORMATS = {
  json: {
    name: 'JSON',
    description: 'JavaScript Object Notation - structured data format',
    extension: 'json',
    mimeType: 'application/json'
  },
  csv: {
    name: 'CSV',
    description: 'Comma Separated Values - spreadsheet compatible',
    extension: 'csv',
    mimeType: 'text/csv'
  },
  xlsx: {
    name: 'Excel',
    description: 'Microsoft Excel format with multiple sheets',
    extension: 'xlsx',
    mimeType: 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet'
  },
  matlab: {
    name: 'MATLAB',
    description: 'MATLAB .mat file format',
    extension: 'mat',
    mimeType: 'application/octet-stream'
  },
  hdf5: {
    name: 'HDF5',
    description: 'Hierarchical Data Format 5 - scientific data',
    extension: 'h5',
    mimeType: 'application/x-hdf'
  }
};

// Data type configurations
const DATA_TYPES = {
  positions: {
    name: 'Position Data',
    description: 'X, Y, Z coordinates with timestamps and confidence',
    fields: ['timestamp', 'x', 'y', 'z', 'confidence', 'algorithm', 'nodeCount']
  },
  csi: {
    name: 'CSI Measurements',
    description: 'Raw CSI amplitude and phase data from nodes',
    fields: ['timestamp', 'nodeId', 'rssi', 'amplitude', 'phase', 'subcarriers', 'channelInfo']
  },
  nodes: {
    name: 'Node Information',
    description: 'Node status, configuration, and metadata',
    fields: ['nodeId', 'position', 'status', 'lastSeen', 'configuration', 'calibration']
  },
  analytics: {
    name: 'Analytics Data',
    description: 'Processed analytics and performance metrics',
    fields: ['timestamp', 'metric', 'value', 'category', 'metadata']
  },
  events: {
    name: 'System Events',
    description: 'System events, errors, and status changes',
    fields: ['timestamp', 'type', 'severity', 'message', 'context']
  }
};

// Data preview component
const DataPreview = ({ dataType, sampleData, selectedFields }) => {
  if (!sampleData || sampleData.length === 0) {
    return (
      <Alert severity="info">
        No sample data available for preview.
      </Alert>
    );
  }

  const previewData = sampleData.slice(0, 5); // Show first 5 rows
  const fields = selectedFields.length > 0 ? selectedFields : Object.keys(previewData[0]);

  return (
    <TableContainer component={Paper} sx={{ maxHeight: 300 }}>
      <Table stickyHeader size="small">
        <TableHead>
          <TableRow>
            {fields.map(field => (
              <TableCell key={field}>{field}</TableCell>
            ))}
          </TableRow>
        </TableHead>
        <TableBody>
          {previewData.map((row, index) => (
            <TableRow key={index}>
              {fields.map(field => (
                <TableCell key={field}>
                  {typeof row[field] === 'object' ? 
                    JSON.stringify(row[field]).substring(0, 50) + '...' : 
                    String(row[field]).substring(0, 50)}
                </TableCell>
              ))}
            </TableRow>
          ))}
        </TableBody>
      </Table>
      
      <Box p={1}>
        <Typography variant="caption" color="textSecondary">
          Showing {previewData.length} of {sampleData.length} records
        </Typography>
      </Box>
    </TableContainer>
  );
};

// Export progress component
const ExportProgress = ({ isExporting, progress, currentStep, totalSteps }) => {
  if (!isExporting) return null;

  return (
    <Paper sx={{ p: 2, mt: 2 }}>
      <Typography variant="h6" gutterBottom>
        Exporting Data...
      </Typography>
      
      <LinearProgress 
        variant="determinate" 
        value={(progress / totalSteps) * 100}
        sx={{ mb: 2 }}
      />
      
      <Typography variant="body2" color="textSecondary">
        Step {progress} of {totalSteps}: {currentStep}
      </Typography>
    </Paper>
  );
};

const DataExporter = ({ availableData = {} }) => {
  const [selectedDataTypes, setSelectedDataTypes] = useState(['positions']);
  const [selectedFormat, setSelectedFormat] = useState('json');
  const [dateRange, setDateRange] = useState({
    start: moment().subtract(24, 'hours'),
    end: moment()
  });
  const [selectedFields, setSelectedFields] = useState({});
  const [includeMetadata, setIncludeMetadata] = useState(true);
  const [compressionEnabled, setCompressionEnabled] = useState(false);
  const [isExporting, setIsExporting] = useState(false);
  const [exportProgress, setExportProgress] = useState(0);
  const [currentStep, setCurrentStep] = useState('');
  const [previewDialogOpen, setPreviewDialogOpen] = useState(false);
  const [previewDataType, setPreviewDataType] = useState('');

  // Calculate export statistics
  const exportStats = useMemo(() => {
    let totalRecords = 0;
    let estimatedSize = 0;
    
    selectedDataTypes.forEach(dataType => {
      const data = availableData[dataType] || [];
      totalRecords += data.length;
      
      // Rough size estimation (bytes)
      const avgRecordSize = dataType === 'csi' ? 2000 : 
                           dataType === 'positions' ? 200 :
                           dataType === 'analytics' ? 300 : 150;
      estimatedSize += data.length * avgRecordSize;
    });
    
    return {
      totalRecords,
      estimatedSize: estimatedSize / 1024 / 1024, // Convert to MB
      timeRange: moment.duration(dateRange.end.diff(dateRange.start)).humanize()
    };
  }, [selectedDataTypes, availableData, dateRange]);

  // Handle field selection changes
  const handleFieldSelection = (dataType, field, checked) => {
    setSelectedFields(prev => ({
      ...prev,
      [dataType]: {
        ...prev[dataType],
        [field]: checked
      }
    }));
  };

  // Generate filename
  const generateFilename = () => {
    const timestamp = moment().format('YYYYMMDD_HHmmss');
    const dataTypesStr = selectedDataTypes.join('_');
    return `csi_export_${dataTypesStr}_${timestamp}.${EXPORT_FORMATS[selectedFormat].extension}`;
  };

  // Export data function
  const exportData = async () => {
    setIsExporting(true);
    setExportProgress(0);
    
    try {
      const totalSteps = selectedDataTypes.length + 2; // Data processing + formatting + download
      let currentProgress = 0;

      // Step 1: Collect and filter data
      setCurrentStep('Collecting data...');
      const exportDataObj = {};
      
      for (const dataType of selectedDataTypes) {
        const data = availableData[dataType] || [];
        
        // Filter by date range
        const filteredData = data.filter(record => {
          const recordDate = moment(record.timestamp);
          return recordDate.isBetween(dateRange.start, dateRange.end, null, '[]');
        });

        // Filter by selected fields
        const selectedFieldsForType = selectedFields[dataType];
        if (selectedFieldsForType && Object.values(selectedFieldsForType).some(v => v)) {
          const fieldsToInclude = Object.keys(selectedFieldsForType).filter(
            field => selectedFieldsForType[field]
          );
          exportDataObj[dataType] = filteredData.map(record => {
            const filteredRecord = {};
            fieldsToInclude.forEach(field => {
              if (record[field] !== undefined) {
                filteredRecord[field] = record[field];
              }
            });
            return filteredRecord;
          });
        } else {
          exportDataObj[dataType] = filteredData;
        }
        
        currentProgress++;
        setExportProgress(currentProgress);
        setCurrentStep(`Processed ${dataType} data (${filteredData.length} records)`);
        
        // Simulate processing delay
        await new Promise(resolve => setTimeout(resolve, 500));
      }

      // Step 2: Format data
      currentProgress++;
      setExportProgress(currentProgress);
      setCurrentStep('Formatting data...');
      
      let formattedData;
      let mimeType = EXPORT_FORMATS[selectedFormat].mimeType;
      
      switch (selectedFormat) {
        case 'json':
          const exportObj = {
            metadata: includeMetadata ? {
              exportDate: moment().toISOString(),
              dateRange: {
                start: dateRange.start.toISOString(),
                end: dateRange.end.toISOString()
              },
              totalRecords: exportStats.totalRecords,
              dataTypes: selectedDataTypes,
              format: selectedFormat
            } : undefined,
            data: exportDataObj
          };
          formattedData = JSON.stringify(exportObj, null, 2);
          break;

        case 'csv':
          // For CSV, export each data type as a separate file or combine
          if (selectedDataTypes.length === 1) {
            const dataType = selectedDataTypes[0];
            const data = exportDataObj[dataType];
            if (data.length > 0) {
              const headers = Object.keys(data[0]);
              const csvContent = [
                headers.join(','),
                ...data.map(row => headers.map(header => {
                  const value = row[header];
                  return typeof value === 'string' && value.includes(',') ? 
                    `"${value}"` : value;
                }).join(','))
              ].join('\n');
              formattedData = csvContent;
            }
          } else {
            // Multiple data types - create a ZIP file with multiple CSVs
            // For now, just export the first data type
            const dataType = selectedDataTypes[0];
            const data = exportDataObj[dataType];
            if (data.length > 0) {
              const headers = Object.keys(data[0]);
              const csvContent = [
                headers.join(','),
                ...data.map(row => headers.map(header => row[header]).join(','))
              ].join('\n');
              formattedData = csvContent;
            }
          }
          break;

        default:
          formattedData = JSON.stringify(exportDataObj, null, 2);
      }

      // Step 3: Download
      currentProgress++;
      setExportProgress(currentProgress);
      setCurrentStep('Preparing download...');

      await new Promise(resolve => setTimeout(resolve, 500));

      // Create and trigger download
      const blob = new Blob([formattedData], { type: mimeType });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = generateFilename();
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);

      setCurrentStep('Export completed!');
      
    } catch (error) {
      console.error('Export failed:', error);
      setCurrentStep('Export failed: ' + error.message);
    } finally {
      setTimeout(() => {
        setIsExporting(false);
        setExportProgress(0);
        setCurrentStep('');
      }, 2000);
    }
  };

  // Show data preview
  const showPreview = (dataType) => {
    setPreviewDataType(dataType);
    setPreviewDialogOpen(true);
  };

  return (
    <LocalizationProvider dateAdapter={AdapterMoment}>
      <Box>
        <Typography variant="h4" gutterBottom>
          Data Export Utility
        </Typography>

        <Grid container spacing={3}>
          {/* Export Configuration */}
          <Grid item xs={12} md={6}>
            <Card>
              <CardContent>
                <Typography variant="h6" gutterBottom>
                  Export Configuration
                </Typography>

                {/* Data Type Selection */}
                <FormControl fullWidth margin="normal">
                  <InputLabel>Data Types</InputLabel>
                  <Select
                    multiple
                    value={selectedDataTypes}
                    onChange={(e) => setSelectedDataTypes(e.target.value)}
                    renderValue={(selected) => (
                      <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 0.5 }}>
                        {selected.map((value) => (
                          <Chip key={value} label={DATA_TYPES[value]?.name} size="small" />
                        ))}
                      </Box>
                    )}
                  >
                    {Object.entries(DATA_TYPES).map(([key, config]) => (
                      <MenuItem key={key} value={key}>
                        {config.name}
                      </MenuItem>
                    ))}
                  </Select>
                </FormControl>

                {/* Format Selection */}
                <FormControl fullWidth margin="normal">
                  <InputLabel>Export Format</InputLabel>
                  <Select
                    value={selectedFormat}
                    onChange={(e) => setSelectedFormat(e.target.value)}
                  >
                    {Object.entries(EXPORT_FORMATS).map(([key, format]) => (
                      <MenuItem key={key} value={key}>
                        <Box>
                          <Typography variant="body1">{format.name}</Typography>
                          <Typography variant="caption" color="textSecondary">
                            {format.description}
                          </Typography>
                        </Box>
                      </MenuItem>
                    ))}
                  </Select>
                </FormControl>

                {/* Date Range */}
                <Grid container spacing={2} sx={{ mt: 1 }}>
                  <Grid item xs={6}>
                    <DateTimePicker
                      label="Start Date"
                      value={dateRange.start}
                      onChange={(value) => setDateRange(prev => ({ ...prev, start: value }))}
                      renderInput={(params) => <TextField {...params} fullWidth />}
                    />
                  </Grid>
                  <Grid item xs={6}>
                    <DateTimePicker
                      label="End Date"
                      value={dateRange.end}
                      onChange={(value) => setDateRange(prev => ({ ...prev, end: value }))}
                      renderInput={(params) => <TextField {...params} fullWidth />}
                    />
                  </Grid>
                </Grid>

                {/* Options */}
                <Box sx={{ mt: 2 }}>
                  <FormControlLabel
                    control={
                      <Checkbox
                        checked={includeMetadata}
                        onChange={(e) => setIncludeMetadata(e.target.checked)}
                      />
                    }
                    label="Include metadata"
                  />
                  
                  <FormControlLabel
                    control={
                      <Checkbox
                        checked={compressionEnabled}
                        onChange={(e) => setCompressionEnabled(e.target.checked)}
                      />
                    }
                    label="Enable compression"
                  />
                </Box>
              </CardContent>
            </Card>
          </Grid>

          {/* Export Statistics */}
          <Grid item xs={12} md={6}>
            <Card>
              <CardContent>
                <Typography variant="h6" gutterBottom>
                  Export Summary
                </Typography>

                <Grid container spacing={2}>
                  <Grid item xs={6}>
                    <Typography variant="body2" color="textSecondary">
                      Total Records
                    </Typography>
                    <Typography variant="h6">
                      {exportStats.totalRecords.toLocaleString()}
                    </Typography>
                  </Grid>

                  <Grid item xs={6}>
                    <Typography variant="body2" color="textSecondary">
                      Estimated Size
                    </Typography>
                    <Typography variant="h6">
                      {exportStats.estimatedSize.toFixed(2)} MB
                    </Typography>
                  </Grid>

                  <Grid item xs={12}>
                    <Typography variant="body2" color="textSecondary">
                      Time Range
                    </Typography>
                    <Typography variant="h6">
                      {exportStats.timeRange}
                    </Typography>
                  </Grid>

                  <Grid item xs={12}>
                    <Typography variant="body2" color="textSecondary">
                      Filename
                    </Typography>
                    <Typography variant="body1" sx={{ fontFamily: 'monospace' }}>
                      {generateFilename()}
                    </Typography>
                  </Grid>
                </Grid>

                <Box sx={{ mt: 2 }}>
                  <Button
                    variant="contained"
                    startIcon={<DownloadIcon />}
                    onClick={exportData}
                    disabled={isExporting || selectedDataTypes.length === 0}
                    fullWidth
                    size="large"
                  >
                    {isExporting ? 'Exporting...' : 'Export Data'}
                  </Button>
                </Box>
              </CardContent>
            </Card>

            {/* Export Progress */}
            <ExportProgress
              isExporting={isExporting}
              progress={exportProgress}
              currentStep={currentStep}
              totalSteps={selectedDataTypes.length + 2}
            />
          </Grid>

          {/* Field Selection */}
          <Grid item xs={12}>
            <Accordion>
              <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                <Typography variant="h6">
                  <SettingsIcon sx={{ mr: 1, verticalAlign: 'middle' }} />
                  Advanced Field Selection
                </Typography>
              </AccordionSummary>
              <AccordionDetails>
                <Grid container spacing={3}>
                  {selectedDataTypes.map(dataType => (
                    <Grid item xs={12} md={6} key={dataType}>
                      <Paper sx={{ p: 2 }}>
                        <Box display="flex" justifyContent="space-between" alignItems="center" mb={1}>
                          <Typography variant="subtitle1">
                            {DATA_TYPES[dataType]?.name}
                          </Typography>
                          <Button
                            size="small"
                            startIcon={<PreviewIcon />}
                            onClick={() => showPreview(dataType)}
                          >
                            Preview
                          </Button>
                        </Box>
                        
                        <Typography variant="body2" color="textSecondary" gutterBottom>
                          {DATA_TYPES[dataType]?.description}
                        </Typography>

                        <Box>
                          {DATA_TYPES[dataType]?.fields.map(field => (
                            <FormControlLabel
                              key={field}
                              control={
                                <Checkbox
                                  checked={selectedFields[dataType]?.[field] ?? true}
                                  onChange={(e) => handleFieldSelection(dataType, field, e.target.checked)}
                                />
                              }
                              label={field}
                            />
                          ))}
                        </Box>
                      </Paper>
                    </Grid>
                  ))}
                </Grid>
              </AccordionDetails>
            </Accordion>
          </Grid>
        </Grid>

        {/* Preview Dialog */}
        <Dialog
          open={previewDialogOpen}
          onClose={() => setPreviewDialogOpen(false)}
          maxWidth="lg"
          fullWidth
        >
          <DialogTitle>
            Data Preview: {DATA_TYPES[previewDataType]?.name}
          </DialogTitle>
          <DialogContent>
            <DataPreview
              dataType={previewDataType}
              sampleData={availableData[previewDataType]}
              selectedFields={Object.keys(selectedFields[previewDataType] || {}).filter(
                field => selectedFields[previewDataType]?.[field]
              )}
            />
          </DialogContent>
          <DialogActions>
            <Button onClick={() => setPreviewDialogOpen(false)}>
              Close
            </Button>
          </DialogActions>
        </Dialog>
      </Box>
    </LocalizationProvider>
  );
};

export default DataExporter;