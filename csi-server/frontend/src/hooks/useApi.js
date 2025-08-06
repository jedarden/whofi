/**
 * Custom hook for API calls using React Query
 */

import { useQuery, useMutation, useQueryClient } from 'react-query';
import axios from 'axios';

// Configure axios defaults
const api = axios.create({
  baseURL: process.env.REACT_APP_API_BASE_URL || '',
  timeout: 10000,
});

// Request interceptor
api.interceptors.request.use(
  (config) => {
    // Add auth headers if needed
    return config;
  },
  (error) => {
    return Promise.reject(error);
  }
);

// Response interceptor
api.interceptors.response.use(
  (response) => response.data,
  (error) => {
    console.error('API Error:', error);
    return Promise.reject(error);
  }
);

// Hook for GET requests
export const useApi = (url, options = {}) => {
  return useQuery(
    url,
    () => api.get(url),
    {
      staleTime: 30000,
      cacheTime: 300000,
      ...options,
    }
  );
};

// Hook for mutations
export const useApiMutation = () => {
  const queryClient = useQueryClient();
  
  return useMutation(
    ({ method = 'POST', url, data }) => {
      switch (method.toLowerCase()) {
        case 'post':
          return api.post(url, data);
        case 'put':
          return api.put(url, data);
        case 'patch':
          return api.patch(url, data);
        case 'delete':
          return api.delete(url);
        default:
          throw new Error(`Unsupported method: ${method}`);
      }
    },
    {
      onSuccess: (data, variables) => {
        // Invalidate related queries
        queryClient.invalidateQueries();
      },
      onError: (error) => {
        console.error('Mutation error:', error);
      },
    }
  );
};

// Specific API hooks
export const useNodes = () => useApi('/api/nodes');
export const usePositions = (params = {}) => {
  const queryString = new URLSearchParams(params).toString();
  return useApi(`/api/positions${queryString ? `?${queryString}` : ''}`);
};
export const useCurrentPosition = () => useApi('/api/positions/current', {
  refetchInterval: 1000,
});
export const useStats = () => useApi('/api/stats', {
  refetchInterval: 5000,
});
export const useConfig = () => useApi('/api/config');

// CSI data hooks
export const useCSIData = (nodeId, params = {}) => {
  const queryString = new URLSearchParams(params).toString();
  return useApi(`/api/csi/${nodeId}${queryString ? `?${queryString}` : ''}`, {
    enabled: !!nodeId,
  });
};

export default api;