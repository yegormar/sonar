#!/usr/bin/env python3
"""
Comprehensive Sonar Data Analysis
Tests ALL brainstormed approaches with parameter variations
"""

import csv
from collections import deque, defaultdict
import math

class StateModel:
    """Base class for all state detection models"""
    def __init__(self, name):
        self.name = name
        self.states = []
        self.transitions = []
        self.metrics = {}
    
    def process(self, data):
        raise NotImplementedError
    
    def calculate_metrics(self, timestamps):
        """Calculate performance metrics"""
        if not self.states:
            return
        
        # Count transitions
        self.metrics['transitions'] = sum(1 for i in range(1, len(self.states)) 
                                         if self.states[i] != self.states[i-1])
        
        # Find stable periods
        periods = []
        current_state = self.states[0]
        start_idx = 0
        
        for i in range(1, len(self.states)):
            if self.states[i] != current_state:
                periods.append({
                    'state': current_state,
                    'start': timestamps[start_idx],
                    'end': timestamps[i-1],
                    'duration': timestamps[i-1] - timestamps[start_idx]
                })
                current_state = self.states[i]
                start_idx = i
        
        # Add last period
        periods.append({
            'state': current_state,
            'start': timestamps[start_idx],
            'end': timestamps[-1],
            'duration': timestamps[-1] - timestamps[start_idx]
        })
        
        self.metrics['stable_periods'] = periods
        
        # Calculate time in each state
        state_times = defaultdict(float)
        for period in periods:
            state_times[period['state']] += period['duration']
        
        self.metrics['state_distribution'] = state_times
        
        # Calculate flicker metric (short stable periods)
        short_periods = [p for p in periods if p['duration'] < 500]
        self.metrics['flicker_count'] = len(short_periods)
        self.metrics['flicker_time'] = sum(p['duration'] for p in short_periods)

# ============================================================================
# MODEL IMPLEMENTATIONS
# ============================================================================

class SimpleThreshold(StateModel):
    """Original approach with variable threshold"""
    def __init__(self, threshold=20):
        super().__init__(f"Simple({threshold})")
        self.threshold = threshold
    
    def process(self, diffs):
        for diff in diffs:
            if diff < -self.threshold:
                self.states.append('LEFT')
            elif diff > self.threshold:
                self.states.append('RIGHT')
            else:
                self.states.append('CENTER')

class HysteresisModel(StateModel):
    """State machine with hysteresis"""
    def __init__(self, left_enter, left_exit):
        super().__init__(f"Hyst({left_enter}/{left_exit})")
        self.left_enter = left_enter
        self.left_exit = left_exit
        self.current = 'CENTER'
    
    def process(self, diffs):
        for diff in diffs:
            if self.current == 'CENTER':
                if diff < self.left_enter:
                    self.current = 'LEFT'
                elif diff > -self.left_enter:  # Symmetric for RIGHT
                    self.current = 'RIGHT'
            elif self.current == 'LEFT':
                if diff > self.left_exit:
                    self.current = 'CENTER'
            elif self.current == 'RIGHT':
                if diff < -self.left_exit:
                    self.current = 'CENTER'
            
            self.states.append(self.current)

class MovingAverageModel(StateModel):
    """Moving average with threshold"""
    def __init__(self, window_size=5, threshold=20):
        super().__init__(f"MovAvg({window_size},{threshold})")
        self.window = deque(maxlen=window_size)
        self.threshold = threshold
    
    def process(self, diffs):
        for diff in diffs:
            self.window.append(diff)
            avg = sum(self.window) / len(self.window) if self.window else diff
            
            if avg < -self.threshold:
                self.states.append('LEFT')
            elif avg > self.threshold:
                self.states.append('RIGHT')
            else:
                self.states.append('CENTER')

class TimeSmoothingModel(StateModel):
    """Requires state to be stable for minimum time"""
    def __init__(self, min_time_ms=300, sample_rate=20):
        super().__init__(f"TimeSmooth({min_time_ms}ms)")
        self.min_samples = min_time_ms // sample_rate
        self.current = 'CENTER'
        self.counter = 0
    
    def process(self, diffs):
        for diff in diffs:
            # Immediate state
            immediate = 'CENTER'
            if diff < -20:
                immediate = 'LEFT'
            elif diff > 20:
                immediate = 'RIGHT'
            
            # State machine with timing
            if immediate != self.current:
                self.counter += 1
                if self.counter >= self.min_samples:
                    self.current = immediate
                    self.counter = 0
            else:
                self.counter = 0
            
            self.states.append(self.current)

class CombinedModel(StateModel):
    """Combine moving average + hysteresis + time smoothing"""
    def __init__(self, avg_window=5, left_enter=-30, left_exit=-10, min_time=300):
        super().__init__(f"Combined(avg{avg_window},hyst{left_enter}/{left_exit},time{min_time}ms)")
        self.avg_window = avg_window
        self.left_enter = left_enter
        self.left_exit = left_exit
        self.min_time = min_time
        self.window = deque(maxlen=avg_window)
        self.current = 'CENTER'
        self.proposed = 'CENTER'
        self.counter = 0
    
    def process(self, diffs):
        for diff in diffs:
            # Moving average
            self.window.append(diff)
            avg = sum(self.window) / len(self.window) if self.window else diff
            
            # Hysteresis logic
            proposed = self.current
            if self.current == 'CENTER':
                if avg < self.left_enter:
                    proposed = 'LEFT'
                elif avg > -self.left_enter:
                    proposed = 'RIGHT'
            elif self.current == 'LEFT':
                if avg > self.left_exit:
                    proposed = 'CENTER'
            elif self.current == 'RIGHT':
                if avg < -self.left_exit:
                    proposed = 'CENTER'
            
            # Time-based stability
            if proposed != self.current:
                if proposed != self.proposed:
                    self.proposed = proposed
                    self.counter = 1
                else:
                    self.counter += 1
                    if self.counter >= (self.min_time // 20):  # Assume 20ms sample rate
                        self.current = proposed
                        self.counter = 0
            else:
                self.counter = 0
            
            self.states.append(self.current)

# ============================================================================
# MAIN ANALYSIS
# ============================================================================

def load_data(filename):
    """Load and parse data file"""
    data = []
    timestamps = []
    diffs = []
    
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('ts') or line.startswith('---'):
                continue
            
            try:
                parts = line.split()
                if len(parts) >= 4:
                    timestamp = int(parts[0].rstrip(','))
                    L = float(parts[1].rstrip(','))
                    R = float(parts[2].rstrip(','))
                    diff = float(parts[3].rstrip(','))
                    
                    timestamps.append(timestamp)
                    diffs.append(diff)
                    data.append({'time': timestamp, 'L': L, 'R': R, 'diff': diff})
            except:
                continue
    
    return data, timestamps, diffs

def run_analysis(filename):
    """Run comprehensive analysis on data"""
    print("Loading data...")
    data, timestamps, diffs = load_data(filename)
    
    if not diffs:
        print("No valid data found!")
        return
    
    print(f"Analyzing {len(diffs)} data points...")
    print(f"Time range: {timestamps[0]}ms to {timestamps[-1]}ms\n")
    
    # Create all models to test
    models = [
        # Simple thresholds
        SimpleThreshold(20),
        SimpleThreshold(25),
        SimpleThreshold(30),
        
        # Hysteresis variations
        HysteresisModel(-30, -10),
        HysteresisModel(-25, -15),
        HysteresisModel(-35, -5),
        HysteresisModel(-40, -20),
        
        # Moving averages
        MovingAverageModel(3, 20),
        MovingAverageModel(5, 20),
        MovingAverageModel(7, 20),
        MovingAverageModel(5, 25),
        
        # Time smoothing
        TimeSmoothingModel(200),
        TimeSmoothingModel(300),
        TimeSmoothingModel(500),
        
        # Combined models
        CombinedModel(3, -30, -10, 200),
        CombinedModel(5, -30, -10, 300),
        CombinedModel(5, -35, -10, 300),
    ]
    
    # Process all models
    print("Testing models...")
    for model in models:
        model.process(diffs)
        model.calculate_metrics(timestamps)
    
    # Generate report
    print("\n" + "="*70)
    print("COMPREHENSIVE ANALYSIS RESULTS")
    print("="*70)
    
    # Find original approach for comparison
    original = SimpleThreshold(20)
    original.process(diffs)
    original.calculate_metrics(timestamps)
    
    print(f"\nOriginal Approach (SimpleThreshold-20):")
    print(f"  Transitions: {original.metrics['transitions']}")
    print(f"  Flicker periods: {original.metrics['flicker_count']}")
    print(f"  Time distribution:")
    for state, time_ms in original.metrics['state_distribution'].items():
        pct = 100 * time_ms / (timestamps[-1] - timestamps[0])
        print(f"    {state}: {time_ms:.1f}ms ({pct:.1f}%)")
    
    # Rank models by transition count (fewer = better)
    ranked = sorted(models, key=lambda m: m.metrics['transitions'])
    
    print(f"\nModel Rankings (by transition count, fewer is better):")
    for i, model in enumerate(ranked[:10], 1):
        trans = model.metrics['transitions']
        reduction = 100 * (original.metrics['transitions'] - trans) / original.metrics['transitions']
        flicker = model.metrics['flicker_count']
        print(f"{i}. {model.name}:")
        print(f"   Transitions: {trans} ({reduction:.1f}% reduction)")
        print(f"   Flicker periods: {flicker}")
        print(f"   LEFT time: {model.metrics['state_distribution'].get('LEFT', 0):.1f}ms")
    
    # Save detailed results
    with open('detailed_analysis.txt', 'w') as f:
        f.write("DETAILED MODEL ANALYSIS\n")
        f.write(f"Original data: {len(diffs)} points, {timestamps[0]}-{timestamps[-1]}ms\n\n")
        
        for model in models:
            f.write(f"\n{model.name}:\n")
            f.write(f"  Transitions: {model.metrics['transitions']}\n")
            f.write(f"  Flicker periods: {model.metrics['flicker_count']}\n")
            f.write(f"  Flicker time: {model.metrics['flicker_time']:.1f}ms\n")
            f.write(f"  State distribution:\n")
            for state, time_ms in model.metrics['state_distribution'].items():
                f.write(f"    {state}: {time_ms:.1f}ms\n")
            
            # Show transition details
            if len(model.metrics['stable_periods']) <= 10:
                f.write(f"  Stable periods:\n")
                for period in model.metrics['stable_periods']:
                    f.write(f"    {period['start']}-{period['end']}ms: {period['state']} ({period['duration']:.1f}ms)\n")
    
    print("\n" + "="*70)
    print("ANALYSIS COMPLETE")
    print("="*70)
    print("\nTop recommendations:")
    print("1. Review detailed_analysis.txt for all model metrics")
    print("2. Look for models with:")
    print("   - Fewest transitions")
    print("   - Lowest flicker count")
    print("   - Accurate LEFT detection time")
    print("3. Best models will be at the top of the ranking list")

if __name__ == "__main__":
    run_analysis('../tools/test_data.csv')
