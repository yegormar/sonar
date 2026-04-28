#!/usr/bin/env python3
"""
Sonar Data Analysis and Model Testing
Analyzes collected sensor data and tests different state detection algorithms
"""

import pandas as pd
import numpy as np
from scipy.signal import medfilt
from collections import deque
import matplotlib.pyplot as plt

class StateDetector:
    """Base class for state detection algorithms"""
    def __init__(self):
        self.states = []
        self.state_names = {0: 'CENTER', 1: 'LEFT', 2: 'RIGHT'}
    
    def process(self, diff_series):
        raise NotImplementedError
    
    def get_states(self):
        return self.states

class SimpleThreshold(StateDetector):
    """Original v1 approach: simple -20/20 threshold"""
    def __init__(self):
        super().__init__()
    
    def process(self, diff_series):
        for diff in diff_series:
            if diff < -20.0:
                self.states.append(1)  # LEFT
            elif diff > 20.0:
                self.states.append(2)  # RIGHT
            else:
                self.states.append(0)  # CENTER

class HysteresisDetector(StateDetector):
    """Hysteresis with different enter/exit thresholds"""
    def __init__(self, left_enter=-30, left_exit=-10, right_enter=30, right_exit=10):
        super().__init__()
        self.left_enter = left_enter
        self.left_exit = left_exit
        self.right_enter = right_enter
        self.right_exit = right_exit
        self.current_state = 0
    
    def process(self, diff_series):
        for diff in diff_series:
            new_state = self.current_state
            
            if self.current_state == 0:  # CENTER
                if diff < self.left_enter:
                    new_state = 1
                elif diff > self.right_enter:
                    new_state = 2
            elif self.current_state == 1:  # LEFT
                if diff > self.left_exit:
                    new_state = 0
            elif self.current_state == 2:  # RIGHT
                if diff < self.right_exit:
                    new_state = 0
            
            self.current_state = new_state
            self.states.append(new_state)

class MovingAverageDetector(StateDetector):
    """Moving average + simple threshold"""
    def __init__(self, window_size=5, threshold=20):
        super().__init__()
        self.window_size = window_size
        self.threshold = threshold
        self.history = deque(maxlen=window_size)
    
    def process(self, diff_series):
        for diff in diff_series:
            self.history.append(diff)
            if len(self.history) < self.window_size:
                avg = diff
            else:
                avg = sum(self.history) / self.window_size
            
            if avg < -self.threshold:
                self.states.append(1)
            elif avg > self.threshold:
                self.states.append(2)
            else:
                self.states.append(0)

class TimeBasedStability(StateDetector):
    """Requires state to be stable for minimum time"""
    def __init__(self, min_stable_time=300, sample_rate=20):
        super().__init__()
        self.min_stable_time = min_stable_time
        self.sample_rate = sample_rate  # ms between samples
        self.min_samples = min_stable_time // sample_rate
        
        self.current_state = 0
        self.state_counter = 0
    
    def process(self, diff_series):
        for diff in diff_series:
            # Determine immediate state
            immediate_state = 0
            if diff < -20:
                immediate_state = 1
            elif diff > 20:
                immediate_state = 2
            
            # Check if we should change state
            if immediate_state != self.current_state:
                self.state_counter += 1
                if self.state_counter >= self.min_samples:
                    self.current_state = immediate_state
                    self.state_counter = 0
            else:
                self.state_counter = 0
            
            self.states.append(self.current_state)

def load_data(filename):
    """Load collected data from CSV file"""
    df = pd.read_csv(filename)
    df.columns = ['timestamp', 'L', 'R', 'diff', 'side', 'leds']
    return df

def analyze_transitions(states, timestamps):
    """Analyze state transition quality"""
    transitions = []
    current_state = states[0]
    transition_times = []
    
    for i, state in enumerate(states[1:], 1):
        if state != current_state:
            transitions.append((timestamps[i-1], current_state, state))
            transition_times.append(timestamps[i] - timestamps[i-1])
            current_state = state
    
    return {
        'transition_count': len(transitions),
        'avg_transition_time': np.mean(transition_times) if transition_times else 0,
        'transitions': transitions
    }

def evaluate_model(model, data, name):
    """Evaluate a detection model"""
    model.process(data['diff'])
    states = model.get_states()
    
    # Add states to dataframe
    data[f'state_{name}'] = states
    
    # Calculate metrics
    metrics = analyze_transitions(states, data['timestamp'])
    
    return metrics

def plot_comparison(data, models):
    """Plot comparison of different models"""
    plt.figure(figsize=(15, 10))
    
    # Plot raw difference
    plt.subplot(4, 1, 1)
    plt.plot(data['timestamp'], data['diff'], label='Raw Diff')
    plt.axhline(-20, color='r', linestyle='--', label='LEFT threshold')
    plt.axhline(20, color='g', linestyle='--', label='RIGHT threshold')
    plt.title('Raw Sensor Difference')
    plt.ylabel('Diff (cm)')
    plt.legend()
    
    # Plot each model's states
    for i, (name, model) in enumerate(models.items(), 2):
        plt.subplot(4, 1, i)
        states = data[f'state_{name}']
        plt.step(data['timestamp'], states, where='post', label=name)
        plt.title(f'{name} State Detection')
        plt.ylabel('State')
        plt.ylim(-0.5, 2.5)
        plt.yticks([0, 1, 2], ['CENTER', 'LEFT', 'RIGHT'])
        plt.legend()
    
    plt.xlabel('Timestamp (ms)')
    plt.tight_layout()
    plt.savefig('model_comparison.png')
    plt.close()

def main():
    # Load data
    print("Loading data...")
    data = load_data('sonar_data.csv')
    
    # Define models to test
    models = {
        'simple': SimpleThreshold(),
        'hyst_30_10': HysteresisDetector(left_enter=-30, left_exit=-10),
        'hyst_25_15': HysteresisDetector(left_enter=-25, left_exit=-15),
        'mov_avg_5': MovingAverageDetector(window_size=5),
        'time_stable': TimeBasedStability(min_stable_time=300)
    }
    
    # Evaluate each model
    results = {}
    for name, model in models.items():
        print(f"Evaluating {name}...")
        results[name] = evaluate_model(model, data, name)
    
    # Generate comparison plot
    print("Generating comparison plot...")
    plot_comparison(data, models)
    
    # Print results
    print("\n=== RESULTS ===")
    for name, result in results.items():
        print(f"\n{name}:")
        print(f"  Transitions: {result['transition_count']}")
        print(f"  Avg transition time: {result['avg_transition_time']:.1f}ms")
        print(f"  Transitions: {len(result['transitions'])}")
        for time, from_state, to_state in result['transitions'][:5]:  # Show first 5
            print(f"    {time}ms: {model.state_names[from_state]} -> {model.state_names[to_state]}")
    
    # Save enhanced data
    data.to_csv('sonar_data_analyzed.csv', index=False)
    print("\nAnalysis complete. Results saved to sonar_data_analyzed.csv")
    print("Comparison plot saved to model_comparison.png")

if __name__ == "__main__":
    main()
