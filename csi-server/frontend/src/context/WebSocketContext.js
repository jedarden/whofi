/**
 * WebSocket Context for real-time data updates
 */

import React, { createContext, useContext, useEffect, useState, useRef } from 'react';

const WebSocketContext = createContext();

export const useWebSocket = () => {
  const context = useContext(WebSocketContext);
  if (!context) {
    throw new Error('useWebSocket must be used within a WebSocketProvider');
  }
  return context;
};

export const WebSocketProvider = ({ children, onNotification }) => {
  const [isConnected, setIsConnected] = useState(false);
  const [lastMessage, setLastMessage] = useState(null);
  const [connectionError, setConnectionError] = useState(null);
  const wsRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const reconnectAttempts = useRef(0);

  const wsUrl = process.env.REACT_APP_WS_URL || 
    `${window.location.protocol === 'https:' ? 'wss:' : 'ws:'}//${window.location.host}/ws/`;

  const connect = () => {
    try {
      console.log('Connecting to WebSocket:', wsUrl);
      wsRef.current = new WebSocket(wsUrl);

      wsRef.current.onopen = () => {
        console.log('WebSocket connected');
        setIsConnected(true);
        setConnectionError(null);
        reconnectAttempts.current = 0;
        
        if (onNotification) {
          onNotification({
            type: 'success',
            message: 'Connected to real-time data stream'
          });
        }
      };

      wsRef.current.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          setLastMessage(data);
          
          // Handle different message types
          switch (data.type) {
            case 'position_update':
              // Position update will be handled by components
              break;
            case 'csi_data':
              // CSI data update
              break;
            case 'node_status':
              if (onNotification) {
                onNotification({
                  type: 'info',
                  message: `Node ${data.nodeId} status: ${data.status}`
                });
              }
              break;
            case 'error':
              if (onNotification) {
                onNotification({
                  type: 'error',
                  message: data.message || 'System error occurred'
                });
              }
              break;
            default:
              console.log('Received message:', data);
          }
        } catch (error) {
          console.error('Error parsing WebSocket message:', error);
        }
      };

      wsRef.current.onclose = (event) => {
        console.log('WebSocket disconnected:', event.code, event.reason);
        setIsConnected(false);
        
        // Only attempt reconnect if it wasn't a clean close
        if (event.code !== 1000) {
          attemptReconnect();
        }
      };

      wsRef.current.onerror = (error) => {
        console.error('WebSocket error:', error);
        setConnectionError(error);
        setIsConnected(false);
        
        if (onNotification) {
          onNotification({
            type: 'error',
            message: 'Lost connection to real-time data stream'
          });
        }
      };
    } catch (error) {
      console.error('Failed to create WebSocket connection:', error);
      setConnectionError(error);
      attemptReconnect();
    }
  };

  const attemptReconnect = () => {
    if (reconnectAttempts.current < 5) {
      const delay = Math.min(1000 * Math.pow(2, reconnectAttempts.current), 30000);
      reconnectAttempts.current++;
      
      console.log(`Attempting to reconnect in ${delay}ms (attempt ${reconnectAttempts.current})`);
      
      reconnectTimeoutRef.current = setTimeout(() => {
        connect();
      }, delay);
    } else {
      console.error('Max reconnection attempts reached');
      if (onNotification) {
        onNotification({
          type: 'error',
          message: 'Unable to reconnect to server. Please refresh the page.'
        });
      }
    }
  };

  const disconnect = () => {
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current);
    }
    
    if (wsRef.current) {
      wsRef.current.close(1000, 'User initiated disconnect');
      wsRef.current = null;
    }
    
    setIsConnected(false);
    setLastMessage(null);
    setConnectionError(null);
  };

  const sendMessage = (message) => {
    if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(message));
      return true;
    }
    return false;
  };

  useEffect(() => {
    connect();

    return () => {
      disconnect();
    };
  }, []);

  // Handle page visibility changes
  useEffect(() => {
    const handleVisibilityChange = () => {
      if (document.visibilityState === 'visible' && !isConnected && !wsRef.current) {
        connect();
      }
    };

    document.addEventListener('visibilitychange', handleVisibilityChange);
    return () => {
      document.removeEventListener('visibilitychange', handleVisibilityChange);
    };
  }, [isConnected]);

  const value = {
    isConnected,
    lastMessage,
    connectionError,
    connect,
    disconnect,
    sendMessage,
    reconnect: () => {
      disconnect();
      setTimeout(connect, 1000);
    }
  };

  return (
    <WebSocketContext.Provider value={value}>
      {children}
    </WebSocketContext.Provider>
  );
};