#!/usr/bin/env python3
"""
Simple Sonar Data Analysis - No external dependencies
Analyzes the collected CSV data and tests different approaches
"""

import csv
from collections import defaultdict

def load_data(filename):
    """Load data - handle both CSV and space-separated formats"""
    data = []
    with open(filename, 'r') as f:
        for line in f:
            # Skip header lines and empty lines
            if 'ts(ms)' in line or 'DATA COLLECTION' in line or 'Negative' in line or not line.strip():
                continue
            
            # Parse the line
            parts = line.strip().split()
            if len(parts) >= 6:
                try:
                    row = {
                        'timestamp': parts[0].rstrip(','),
                        'L': parts[1].rstrip(','),
                        'R': parts[2].rstrip(','),
                        'diff': parts[3].rstrip(','),
                        'side': parts[4],
                        'leds': parts[5]
                    }
                    data.append(row)
                except:
                    continue
    return data

def simple_threshold(data):
    """Original approach: -20/20 threshold"""
    states = []
    for row in data:
        diff = float(row['diff'])
        if diff < -20.0:
            states.append('LEFT')
        elif diff > 20.0:
            states.append('RIGHT')
        else:
            states.append('CENTER')
    return states

def hysteresis_model(data, left_enter=-30, left_exit=-10):
    """Hysteresis with different enter/exit thresholds"""
    states = []
    current = 'CENTER'
    
    for row in data:
        diff = float(row['diff'])
        
        if current == 'CENTER':
            if diff < left_enter:
                current = 'LEFT'
            elif diff > (left_enter * -1):  # Symmetric for RIGHT
                current = 'RIGHT'
        elif current == 'LEFT':
            if diff > left_exit:
                current = 'CENTER'
        elif current == 'RIGHT':
            if diff < (left_exit * -1):
                current = 'CENTER'
        
        states.append(current)
    
    return states

def count_transitions(states):
    """Count state transitions"""
    transitions = 0
    current = states[0]
    
    for state in states[1:]:
        if state != current:
            transitions += 1
            current = state
    
    return transitions

def find_stable_periods(states, timestamps):
    """Find periods where state is stable"""
    periods = []
    current_state = states[0]
    start_time = timestamps[0]
    
    for i, state in enumerate(states[1:], 1):
        if state != current_state:
            # End of stable period
            periods.append({
                'state': current_state,
                'start': start_time,
                'end': timestamps[i-1],
                'duration': float(timestamps[i-1]) - float(start_time)
            })
            current_state = state
            start_time = timestamps[i]
    
    # Add final period
    periods.append({
        'state': current_state,
        'start': start_time,
        'end': timestamps[-1],
        'duration': float(timestamps[-1]) - float(start_time)
    })
    
    return periods

def analyze_data(filename):
    """Main analysis function"""
    print("Loading data...")
    data = load_data(filename)
    
    if not data:
        print("No data found!")
        return
    
    # Extract timestamps and diffs
    timestamps = [row['timestamp'] for row in data]
    diffs = [float(row['diff']) for row in data]
    original_sides = [row['side'] for row in data]
    
    print(f"\nAnalyzing {len(data)} data points...")
    print(f"Time range: {timestamps[0]}ms to {timestamps[-1]}ms")
    
    # Test different models
    models = {
        'Original (-20/20)': simple_threshold(data),
        'Hysteresis (-30/-10)': hysteresis_model(data, -30, -10),
        'Hysteresis (-25/-15)': hysteresis_model(data, -25, -15),
        'Hysteresis (-35/-5)': hysteresis_model(data, -35, -5)
    }
    
    # Analyze each model
    print("\n=== MODEL COMPARISON ===")
    for name, states in models.items():
        trans = count_transitions(states)
        stable = find_stable_periods(states, timestamps)
        
        # Count time in each state
        state_times = defaultdict(float)
        for period in stable:
            state_times[period['state']] += period['duration']
        
        print(f"\n{name}:")
        print(f"  Transitions: {trans}")
        print(f"  Time in states:")
        for state, time_ms in state_times.items():
            print(f"    {state}: {time_ms:.1f}ms ({100*time_ms/float(timestamps[-1]):.1f}%)")
        
        # Show stable periods
        print(f"  Stable periods:")
        for period in stable[:5]:  # Show first 5
            print(f"    {period['start']}-{period['end']}ms: {period['state']} ({period['duration']:.1f}ms)")
    
    # Compare with original
    print(f"\n=== COMPARISON WITH ORIGINAL ===")
    original_trans = count_transitions(original_sides)
    print(f"Original approach had {original_trans} transitions")
    
    for name, states in models.items():
        trans = count_transitions(states)
        reduction = 100 * (original_trans - trans) / original_trans if original_trans > 0 else 0
        print(f"{name}: {trans} transitions ({reduction:.1f}% reduction)")
    
    # Save simplified analysis
    with open('analysis_results.txt', 'w') as f:
        f.write("SONAR DATA ANALYSIS RESULTS\n")
        f.write(f"Data points: {len(data)}\n")
        f.write(f"Time range: {timestamps[0]}ms to {timestamps[-1]}ms\n\n")
        
        for name, states in models.items():
            f.write(f"{name}:\n")
            f.write(f"  Transitions: {count_transitions(states)}\n")
            stable = find_stable_periods(states, timestamps)
            for state, time_ms in defaultdict(float):
                for period in stable:
                    state_times[period['state']] += period['duration']
                for state, time_ms in state_times.items():
                    f.write(f"  {state}: {time_ms:.1f}ms\n")

if __name__ == "__main__":
    analyze_data('test_data.csv')
    print("\nAnalysis complete. Results saved to analysis_results.txt")
