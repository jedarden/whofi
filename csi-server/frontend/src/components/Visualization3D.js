/**
 * @file Visualization3D.js
 * @brief 3D visualization of ESP32 nodes and detected people
 */

import React, { useRef, useEffect, useState } from 'react';
import * as THREE from 'three';
import { Canvas, useFrame, useThree } from '@react-three/fiber';
import { OrbitControls, Text, Box, Sphere, Line, Html } from '@react-three/drei';
import { useWebSocket } from '../hooks/useWebSocket';

// ESP32 Node Component
const NodeMesh = ({ position, nodeId, isOnline, signalStrength }) => {
    const meshRef = useRef();
    const [hovered, setHovered] = useState(false);

    useFrame((state) => {
        if (meshRef.current) {
            meshRef.current.rotation.y += 0.01;
            // Pulse effect when online
            if (isOnline) {
                meshRef.current.scale.setScalar(1 + Math.sin(state.clock.elapsedTime * 2) * 0.1);
            }
        }
    });

    return (
        <group position={position}>
            {/* Node box */}
            <Box
                ref={meshRef}
                args={[0.3, 0.3, 0.1]}
                onPointerOver={() => setHovered(true)}
                onPointerOut={() => setHovered(false)}
            >
                <meshStandardMaterial 
                    color={isOnline ? '#4CAF50' : '#757575'}
                    emissive={isOnline ? '#4CAF50' : '#000000'}
                    emissiveIntensity={hovered ? 0.5 : 0.2}
                />
            </Box>
            
            {/* Signal range indicator */}
            {isOnline && (
                <Sphere args={[signalStrength * 5, 16, 16]}>
                    <meshBasicMaterial 
                        color="#4CAF50" 
                        transparent 
                        opacity={0.1} 
                        wireframe 
                    />
                </Sphere>
            )}
            
            {/* Node label */}
            <Text
                position={[0, 0.5, 0]}
                fontSize={0.2}
                color="white"
                anchorX="center"
                anchorY="middle"
            >
                {nodeId}
            </Text>
            
            {/* Hover info */}
            {hovered && (
                <Html distanceFactor={10}>
                    <div style={{
                        background: 'rgba(0,0,0,0.8)',
                        color: 'white',
                        padding: '5px 10px',
                        borderRadius: '4px',
                        fontSize: '12px',
                        whiteSpace: 'nowrap'
                    }}>
                        <div>Node: {nodeId}</div>
                        <div>Status: {isOnline ? 'Online' : 'Offline'}</div>
                        <div>Position: ({position[0].toFixed(1)}, {position[1].toFixed(1)}, {position[2].toFixed(1)})</div>
                    </div>
                </Html>
            )}
        </group>
    );
};

// Detected Person Component
const PersonMesh = ({ position, id, confidence, trail }) => {
    const meshRef = useRef();
    const [showInfo, setShowInfo] = useState(false);

    useFrame((state) => {
        if (meshRef.current) {
            // Breathing effect
            const scale = 1 + Math.sin(state.clock.elapsedTime * 3) * 0.05;
            meshRef.current.scale.set(scale, scale, scale);
        }
    });

    const color = confidence > 0.8 ? '#2196F3' : confidence > 0.5 ? '#FF9800' : '#F44336';

    return (
        <group position={position}>
            {/* Person sphere */}
            <Sphere
                ref={meshRef}
                args={[0.4, 32, 32]}
                onPointerOver={() => setShowInfo(true)}
                onPointerOut={() => setShowInfo(false)}
            >
                <meshStandardMaterial 
                    color={color}
                    emissive={color}
                    emissiveIntensity={0.3}
                    metalness={0.3}
                    roughness={0.4}
                />
            </Sphere>
            
            {/* Confidence ring */}
            <mesh rotation={[Math.PI / 2, 0, 0]} position={[0, -0.5, 0]}>
                <ringGeometry args={[0.5, 0.6, 32]} />
                <meshBasicMaterial 
                    color={color} 
                    transparent 
                    opacity={confidence} 
                />
            </mesh>
            
            {/* Movement trail */}
            {trail && trail.length > 1 && (
                <Line
                    points={trail}
                    color={color}
                    lineWidth={2}
                    transparent
                    opacity={0.5}
                />
            )}
            
            {/* Person info */}
            {showInfo && (
                <Html distanceFactor={10}>
                    <div style={{
                        background: 'rgba(0,0,0,0.9)',
                        color: 'white',
                        padding: '8px 12px',
                        borderRadius: '4px',
                        fontSize: '12px'
                    }}>
                        <div>ID: {id}</div>
                        <div>Confidence: {(confidence * 100).toFixed(0)}%</div>
                        <div>Position: ({position[0].toFixed(1)}, {position[1].toFixed(1)}, {position[2].toFixed(1)})</div>
                    </div>
                </Html>
            )}
        </group>
    );
};

// Grid and Axes Helper
const SceneHelpers = ({ gridSize = 20 }) => {
    return (
        <>
            {/* Grid */}
            <gridHelper args={[gridSize, gridSize]} />
            
            {/* Axes */}
            <axesHelper args={[gridSize / 2]} />
            
            {/* Walls (optional) */}
            <Line
                points={[
                    [-gridSize/2, 0, -gridSize/2],
                    [gridSize/2, 0, -gridSize/2],
                    [gridSize/2, 0, gridSize/2],
                    [-gridSize/2, 0, gridSize/2],
                    [-gridSize/2, 0, -gridSize/2]
                ]}
                color="gray"
                lineWidth={1}
            />
        </>
    );
};

// Connection Lines Between Nodes
const NodeConnections = ({ nodes }) => {
    const connections = [];
    
    // Create connections between nearby nodes
    nodes.forEach((node1, i) => {
        nodes.forEach((node2, j) => {
            if (i < j && node1.online && node2.online) {
                const distance = Math.sqrt(
                    Math.pow(node1.position.x - node2.position.x, 2) +
                    Math.pow(node1.position.y - node2.position.y, 2) +
                    Math.pow(node1.position.z - node2.position.z, 2)
                );
                
                if (distance < 10) { // Within 10 meters
                    connections.push({
                        start: [node1.position.x, node1.position.y, node1.position.z],
                        end: [node2.position.x, node2.position.y, node2.position.z],
                        strength: 1 - (distance / 10)
                    });
                }
            }
        });
    });

    return (
        <>
            {connections.map((conn, i) => (
                <Line
                    key={i}
                    points={[conn.start, conn.end]}
                    color="#4CAF50"
                    lineWidth={1}
                    transparent
                    opacity={conn.strength * 0.3}
                />
            ))}
        </>
    );
};

// Main 3D Visualization Component
const Visualization3D = () => {
    const [nodes, setNodes] = useState([]);
    const [people, setPeople] = useState([]);
    const [trails, setTrails] = useState({});
    const ws = useWebSocket();

    useEffect(() => {
        // Fetch initial node positions
        fetchNodes();
        
        // Subscribe to real-time updates
        if (ws) {
            ws.on('position-update', handlePositionUpdate);
            ws.on('node-status', handleNodeStatus);
        }

        return () => {
            if (ws) {
                ws.off('position-update', handlePositionUpdate);
                ws.off('node-status', handleNodeStatus);
            }
        };
    }, [ws]);

    const fetchNodes = async () => {
        try {
            const response = await fetch('/api/nodes');
            const data = await response.json();
            setNodes(data.map(node => ({
                id: node.id,
                position: {
                    x: node.position_x || 0,
                    y: node.position_z || 1, // Height
                    z: node.position_y || 0  // Swap Y/Z for Three.js
                },
                online: node.online,
                signalStrength: 1.0
            })));
        } catch (error) {
            console.error('Failed to fetch nodes:', error);
        }
    };

    const handlePositionUpdate = (data) => {
        // Update person position
        setPeople(current => {
            const existing = current.find(p => p.id === data.mac);
            if (existing) {
                return current.map(p => 
                    p.id === data.mac 
                        ? { ...p, ...data.position, confidence: data.confidence }
                        : p
                );
            } else {
                return [...current, {
                    id: data.mac,
                    x: data.position.x,
                    y: data.position.z || 1,
                    z: data.position.y,
                    confidence: data.confidence || 0.5
                }];
            }
        });

        // Update trail
        setTrails(current => {
            const trail = current[data.mac] || [];
            const newPoint = [data.position.x, data.position.z || 1, data.position.y];
            return {
                ...current,
                [data.mac]: [...trail.slice(-20), newPoint] // Keep last 20 points
            };
        });
    };

    const handleNodeStatus = (data) => {
        setNodes(current => 
            current.map(node => 
                node.id === data.nodeId 
                    ? { ...node, online: data.online }
                    : node
            )
        );
    };

    return (
        <div style={{ width: '100%', height: '600px' }}>
            <Canvas
                camera={{ position: [15, 15, 15], fov: 50 }}
                shadows
            >
                {/* Lighting */}
                <ambientLight intensity={0.5} />
                <pointLight position={[10, 10, 10]} />
                
                {/* Controls */}
                <OrbitControls 
                    enablePan={true}
                    enableZoom={true}
                    enableRotate={true}
                />
                
                {/* Scene helpers */}
                <SceneHelpers gridSize={20} />
                
                {/* Node connections */}
                <NodeConnections nodes={nodes} />
                
                {/* ESP32 Nodes */}
                {nodes.map(node => (
                    <NodeMesh
                        key={node.id}
                        nodeId={node.id}
                        position={[node.position.x, node.position.y, node.position.z]}
                        isOnline={node.online}
                        signalStrength={node.signalStrength}
                    />
                ))}
                
                {/* Detected People */}
                {people.map(person => (
                    <PersonMesh
                        key={person.id}
                        id={person.id}
                        position={[person.x, person.y, person.z]}
                        confidence={person.confidence}
                        trail={trails[person.id]}
                    />
                ))}
                
                {/* Stats */}
                <Html position={[-9, 9, 0]}>
                    <div style={{
                        background: 'rgba(0,0,0,0.7)',
                        color: 'white',
                        padding: '10px',
                        borderRadius: '4px',
                        minWidth: '150px'
                    }}>
                        <h4 style={{ margin: '0 0 10px 0' }}>System Status</h4>
                        <div>Nodes: {nodes.filter(n => n.online).length}/{nodes.length}</div>
                        <div>People: {people.length}</div>
                    </div>
                </Html>
            </Canvas>
        </div>
    );
};

export default Visualization3D;