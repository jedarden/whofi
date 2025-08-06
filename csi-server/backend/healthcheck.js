#!/usr/bin/env node

/**
 * Health check script for Docker container
 */

const http = require('http');

const options = {
    hostname: 'localhost',
    port: 3000,
    path: '/health',
    method: 'GET',
    timeout: 3000
};

const req = http.request(options, (res) => {
    if (res.statusCode === 200) {
        console.log('Health check passed');
        process.exit(0);
    } else {
        console.error(`Health check failed: ${res.statusCode}`);
        process.exit(1);
    }
});

req.on('error', (err) => {
    console.error(`Health check error: ${err.message}`);
    process.exit(1);
});

req.on('timeout', () => {
    console.error('Health check timeout');
    req.destroy();
    process.exit(1);
});

req.end();