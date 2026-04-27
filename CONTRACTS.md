# Module Contracts

## 1. Input Layer

### Purpose
Provide raw distance measurements from ultrasonic sensors.

### Public Interfaces
- `measureDistance(int trigPin, int echoPin)`: Returns distance in cm or -1.0 on error.
- `getAverageDistance(int trigPin, int echoPin)`: Returns averaged distance over multiple samples.

### Invariants
- Distance readings must be non-negative or -1.0 (error).
- Sensor pins must be correctly initialized (TRIG as OUTPUT, ECHO as INPUT).

### Stability
- **Stable**: Sensor reading logic is mature and unlikely to change.

---

## 2. Processing Layer

### Purpose
Convert raw sensor data into feedback signals.

### Public Interfaces
- `calculateSpatialFeedback(float L, float R)`: Computes feedback signals from distances.
- `calculateFeedback(float L, float R)`: Computes feedback signals (alternative implementation).

### Invariants
- Feedback signals must be computed from valid sensor readings.
- Calibration constants must be applied consistently.

### Stability
- **Evolving**: Feedback logic may be refined based on user testing.

---

## 3. Output Layer

### Purpose
Emit audio and visual feedback based on processed signals.

### Public Interfaces
- `updateBuzzer()`: Updates buzzer state based on current feedback signals.
- `updateBuzzers()`: Updates both buzzers (dual buzzer implementation).

### Invariants
- Buzzer must not emit sound when `buzzerEnabled` is false.
- LEDs must reflect the current direction state.

### Stability
- **Stable**: Output logic is mature and unlikely to change.

---

## 4. Calibration Layer

### Purpose
Capture reference points for system configuration.

### Public Interfaces
- `captureNear1()`, `captureNear2()`, `captureFar()`, `captureLeft()`, `captureRight()`: Guide users through calibration steps.
- `printResults()`: Output calibration constants for use in the main sketch.

### Invariants
- Calibration constants must be valid (non-zero, non-negative where applicable).
- Calibration steps must be completed in order.

### Stability
- **Stable**: Calibration process is mature and unlikely to change.

---

## Stability Expectations

| Module          | Stability  | Notes                                  |
|-----------------|------------|----------------------------------------|
| Input Layer     | Stable     | Sensor reading logic is mature.        |
| Processing Layer| Evolving   | Feedback logic may be refined.         |
| Output Layer    | Stable     | Output logic is mature.                |
| Calibration Layer| Stable     | Calibration process is mature.         |
