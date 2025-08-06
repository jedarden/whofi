#!/usr/bin/env python3
"""
CSI vs RSSI Visualization
Shows the dramatic difference in information content
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Simulate WiFi signal with person walking
def simulate_wifi_signal(time, person_position):
    """Simulate how WiFi signal is affected by a person"""
    # 52 subcarriers for 20 MHz WiFi
    subcarriers = 52
    frequencies = np.linspace(2.412e9, 2.432e9, subcarriers)  # 2.412-2.432 GHz
    
    # Base channel response (empty room)
    base_magnitude = np.ones(subcarriers) * 10
    base_phase = np.zeros(subcarriers)
    
    # Person effect on each subcarrier
    csi_magnitude = np.zeros(subcarriers)
    csi_phase = np.zeros(subcarriers)
    
    for i, freq in enumerate(frequencies):
        # Wavelength
        wavelength = 3e8 / freq
        
        # Path difference due to person (simplified)
        direct_path = 5.0  # 5 meters
        reflected_path = np.sqrt((5 - person_position)**2 + 1.5**2) + \
                        np.sqrt(person_position**2 + 1.5**2)
        
        # Phase shift due to path difference
        phase_shift = 2 * np.pi * (reflected_path - direct_path) / wavelength
        
        # Magnitude change due to absorption/reflection
        absorption = 0.3 * np.exp(-((person_position - 2.5)**2) / 2)
        
        # Complex channel response
        direct = base_magnitude[i] * np.exp(1j * base_phase[i])
        reflected = base_magnitude[i] * 0.5 * absorption * np.exp(1j * phase_shift)
        
        # Total response (superposition)
        total = direct + reflected
        
        csi_magnitude[i] = np.abs(total)
        csi_phase[i] = np.angle(total)
    
    # RSSI is just average power
    rssi = 10 * np.log10(np.mean(csi_magnitude**2))
    
    return rssi, csi_magnitude, csi_phase

# Visualization
def create_visualization():
    fig, axes = plt.subplots(3, 1, figsize=(12, 10))
    fig.suptitle('CSI vs RSSI: Information Content Comparison', fontsize=16)
    
    # Time series data
    time_window = 100
    time_data = np.linspace(0, 10, time_window)
    rssi_history = []
    
    # Person walking across room
    person_positions = 2.5 + 2.5 * np.sin(time_data * 0.5)
    
    # Subcarrier indices
    subcarrier_indices = np.arange(52)
    
    # Initialize plots
    # RSSI plot
    ax1 = axes[0]
    ax1.set_ylabel('RSSI (dBm)')
    ax1.set_xlabel('Time (s)')
    ax1.set_title('RSSI: Single Value Over Time')
    ax1.set_ylim(-60, -40)
    ax1.grid(True)
    rssi_line, = ax1.plot([], [], 'b-', linewidth=2)
    
    # CSI Magnitude plot
    ax2 = axes[1]
    ax2.set_ylabel('Subcarrier Index')
    ax2.set_xlabel('Time (s)')
    ax2.set_title('CSI Magnitude: 52 Values Over Time (Heatmap)')
    
    # CSI Phase plot
    ax3 = axes[2]
    ax3.set_ylabel('Phase (radians)')
    ax3.set_xlabel('Subcarrier Index')
    ax3.set_title('CSI Phase: Current Snapshot Across Subcarriers')
    ax3.set_ylim(-np.pi, np.pi)
    ax3.grid(True)
    phase_line, = ax3.plot([], [], 'g-', linewidth=2)
    phase_scatter = ax3.scatter([], [], c=[], cmap='viridis', s=50)
    
    # Initialize data storage
    csi_magnitude_history = np.zeros((52, time_window))
    
    def animate(frame):
        # Get current data
        person_pos = person_positions[frame % len(person_positions)]
        rssi, csi_mag, csi_phase = simulate_wifi_signal(time_data[frame], person_pos)
        
        # Update RSSI
        rssi_history.append(rssi)
        if len(rssi_history) > time_window:
            rssi_history.pop(0)
        
        rssi_line.set_data(time_data[:len(rssi_history)], rssi_history)
        
        # Update CSI magnitude history
        csi_magnitude_history[:, :-1] = csi_magnitude_history[:, 1:]
        csi_magnitude_history[:, -1] = csi_mag
        
        # Clear and replot heatmap
        ax2.clear()
        im = ax2.imshow(csi_magnitude_history, aspect='auto', cmap='hot',
                       extent=[0, 10, 0, 52], origin='lower')
        ax2.set_ylabel('Subcarrier Index')
        ax2.set_xlabel('Time (s)')
        ax2.set_title(f'CSI Magnitude: 52 Values Over Time (Person at {person_pos:.1f}m)')
        
        # Update CSI phase
        phase_line.set_data(subcarrier_indices, csi_phase)
        colors = (csi_phase + np.pi) / (2 * np.pi)  # Normalize to 0-1
        phase_scatter.set_offsets(np.c_[subcarrier_indices, csi_phase])
        phase_scatter.set_array(colors)
        
        # Add information text
        info_text = f"RSSI: {rssi:.1f} dBm (1 value)\n"
        info_text += f"CSI: {len(csi_mag)} magnitude + {len(csi_phase)} phase values"
        ax1.text(0.02, 0.95, info_text, transform=ax1.transAxes,
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5),
                verticalalignment='top')
        
        return rssi_line, phase_line, phase_scatter
    
    anim = FuncAnimation(fig, animate, interval=100, blit=False)
    plt.tight_layout()
    plt.show()

# Example: Information content calculation
def compare_information_content():
    """Compare the information content of RSSI vs CSI"""
    print("=== Information Content Comparison ===\n")
    
    # RSSI information
    rssi_bits = 8  # 8-bit value (-128 to +127 dBm)
    rssi_rate = 10  # 10 Hz sampling
    rssi_bps = rssi_bits * rssi_rate
    
    print(f"RSSI Information:")
    print(f"  - Bits per sample: {rssi_bits}")
    print(f"  - Samples per second: {rssi_rate}")
    print(f"  - Information rate: {rssi_bps} bits/second")
    print(f"  - Data size per minute: {rssi_bps * 60 / 8} bytes\n")
    
    # CSI information
    subcarriers = 52  # 20 MHz channel
    magnitude_bits = 8  # 8-bit magnitude
    phase_bits = 8  # 8-bit phase
    csi_rate = 100  # 100 Hz sampling (10x RSSI)
    csi_bits_per_sample = subcarriers * (magnitude_bits + phase_bits)
    csi_bps = csi_bits_per_sample * csi_rate
    
    print(f"CSI Information:")
    print(f"  - Subcarriers: {subcarriers}")
    print(f"  - Bits per subcarrier: {magnitude_bits + phase_bits} (mag + phase)")
    print(f"  - Bits per sample: {csi_bits_per_sample}")
    print(f"  - Samples per second: {csi_rate}")
    print(f"  - Information rate: {csi_bps} bits/second")
    print(f"  - Data size per minute: {csi_bps * 60 / 8 / 1024:.1f} KB\n")
    
    print(f"CSI provides {csi_bps / rssi_bps:.0f}x more information than RSSI")
    print(f"CSI data rate: {csi_bits_per_sample / rssi_bits:.0f}x more data per sample")
    
    # Practical implications
    print("\n=== Practical Implications ===\n")
    
    applications = [
        ("Presence Detection", "Good (85%)", "Excellent (98%)"),
        ("People Counting", "Poor (±1)", "Good (±0.2)"),
        ("Location Accuracy", "3-5 meters", "0.5-2 meters"),
        ("Gesture Recognition", "Impossible", "Good (85%)"),
        ("Breathing Detection", "Nearly Impossible", "Good (80%)"),
        ("Through-wall Sensing", "Poor (20%)", "Moderate (60%)"),
    ]
    
    print(f"{'Application':<20} {'RSSI':<20} {'CSI':<20}")
    print("-" * 60)
    for app, rssi_perf, csi_perf in applications:
        print(f"{app:<20} {rssi_perf:<20} {csi_perf:<20}")

if __name__ == "__main__":
    # Show information comparison
    compare_information_content()
    
    # Create visualization
    print("\nGenerating visualization...")
    create_visualization()