# System Architecture

## Overall System Purpose
This system is an assistive spatial awareness prototype that uses two ultrasonic sensors to detect obstacles and translate spatial information into audio (buzzer) and visual (LED) feedback. It is designed for accessibility applications, enabling users to perceive distance and direction without visual input.

## High-Level Component Diagram

```
┌───────────────────────────────────────────────────────┐
│                 Sonar System                          │
├───────────────────┬───────────────────┬───────────────┤
│   Input Layer     │  Processing Layer │  Output Layer │
├───────────────────┼───────────────────┼───────────────┤
│  - Left Sensor    │  - Distance       │  - Buzzer      │
│  - Right Sensor   │    Calculation    │  - LEDs        │
└───────────────────┴───────────────────┴───────────────┘
```

## Module Boundaries and Responsibilities

### 1. Input Layer
- **Components**: `TRIG_L`, `ECHO_L`, `TRIG_R`, `ECHO_R`
- **Responsibilities**:
  - Trigger ultrasonic sensors and measure echo pulse duration.
  - Convert pulse duration to distance in centimeters.
  - Provide raw distance readings for left and right sensors.

### 2. Processing Layer
- **Components**: `calculateSpatialFeedback`, `calculateFeedback`
- **Responsibilities**:
  - Compute differential distance (`diff = L - R`).
  - Map differential to direction (left, right, center).
  - Map proximity to beep interval and direction to pitch.
  - Apply calibration constants to normalize behavior.

### 3. Output Layer
- **Components**: Buzzer, LEDs
- **Responsibilities**:
  - Emit audio feedback (pitch for direction, tempo for proximity).
  - Display visual feedback (LEDs for direction).

### 4. Calibration Layer
- **Components**: `sonar_calibration*.ino`
- **Responsibilities**:
  - Guide users through calibration steps.
  - Capture reference points for near, far, left, and right positions.
  - Generate calibration constants for the main sketch.

## Explicit Layering Rules

1. **Input Layer** → **Processing Layer**: Raw sensor data flows into processing.
2. **Processing Layer** → **Output Layer**: Processed feedback signals drive output devices.
3. **Calibration Layer** → **Processing Layer**: Calibration constants configure processing behavior.

### Forbidden Dependencies
- **Output Layer** must not depend on **Input Layer**.
- **Processing Layer** must not depend on **Output Layer**.
- **Calibration Layer** must not depend on **Output Layer**.

### Anti-Patterns
- Direct coupling between sensor readings and output devices.
- Hardcoding calibration values in processing logic.
- Mixing feedback logic with sensor reading logic.

## Design Principles

1. **Separation of Concerns**: Each layer handles a distinct aspect of the system (input, processing, output).
2. **Calibration-Driven**: Behavior is configured via calibration, not hardcoded values.
3. **Orthogonal Feedback**: Distance and direction are encoded independently (tempo vs. pitch).
4. **Non-Blocking**: Feedback updates do not block sensor readings or other operations.
5. **Consistency**: All sketches use the same pin assignments and calibration constants.

## Justification

- **Why Two Sensors**: Enables differential spatial comparison and directional awareness.
- **Why Ultrasonic Sensing**: Low cost, real-time distance estimation, and simple microcontroller integration.
- **Why Pitch Encodes Direction**: Pitch and tempo are orthogonal audio dimensions that do not interfere.
- **Why LEDs**: Immediate visual confirmation for debugging and demonstration.
