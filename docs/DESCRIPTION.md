# Dual Ultrasonic Spatial Feedback System

---

# 1. Project Overview

This system is an assistive spatial awareness prototype that uses two ultrasonic distance sensors to detect obstacles in front of a user and translate spatial information into:

- Audio feedback (buzzer)
- Visual feedback (LEDs)

The goal is to represent:
- **Distance (how close an object is)**
- **Direction (left, right, or center bias)**

using simple, interpretable signals suitable for accessibility applications.

---

# 2. System Concept

## Input Layer

Two ultrasonic sensors measure distance independently:

- Left sensor → left spatial field
- Right sensor → right spatial field

Each sensor outputs a time-of-flight measurement, converted into distance.

---

## Computed Values

- `L` = distance from left sensor
- `R` = distance from right sensor
- `D` = known fixed distance between the two sensors (baseline)

From this:

- **Distance (proximity):** based on the smaller of L and R
- **Angle:** derived from the differential measurement (see Section 2a)
- **Direction:** derived from the computed angle

---

## 2a. Angle Calculation

With two forward-facing sensors separated by a known baseline distance `D`, the horizontal angle to a detected object can be estimated using the difference in measured distances.

### Principle

If an object is directly ahead, both sensors return the same distance. If it is off-center, one sensor will return a shorter distance than the other. The difference encodes the lateral offset.

### Formula

```
delta = L - R

angle = arcsin( delta / D )
```

- `delta` is the difference in measured distances (meters or cm, consistent with D)
- `D` is the fixed sensor separation (baseline)
- `angle` is in radians; convert to degrees by multiplying by (180 / π)
- Positive angle → object is to the right
- Negative angle → object is to the left
- Zero → object is straight ahead

### Validity Constraint

This formula is geometrically valid when the object is in the far field relative to the baseline (distance >> D). For close objects, the approximation remains practically useful for the purpose of direction classification.

### Arduino Example

```cpp
float delta = L - R;           // difference in cm
float ratio = delta / D;       // D = sensor separation in cm
ratio = constrain(ratio, -1.0, 1.0); // clamp for safety
float angle_rad = asin(ratio);
float angle_deg = angle_rad * (180.0 / PI);
```

---

## Direction Model

The computed angle maps **continuously** to pitch frequency across the full left-to-right range. There are no discrete LEFT / CENTER / RIGHT states for audio — the pitch shifts smoothly as the object moves.

A dead zone threshold of ±20° is used only to stabilize the LED indicators, preventing them from flickering when the object is nearly centered. The buzzer pitch is unaffected by the dead zone and always reflects the current angle directly.

---

# 3. Output Mapping

## A) Distance → Buzzer Tempo

- Far → slow beeps
- Close → fast beeps

This encodes urgency. The beep interval is mapped from the minimum of L and R across the operational range of 0.5 m to 5 m:

```cpp
int interval = map(min(L, R), 50, 500, 100, 1000); // cm: 50cm→constant, 500cm→slow
```

---

## B) Direction → Buzzer Pitch

Pitch (tone frequency) encodes direction as an independent audio dimension, mapped **continuously** from the computed angle. As the object moves from hard left to hard right, pitch shifts smoothly from low to high with no discrete steps:

| Angle | Frequency |
|-------|-----------|
| Hard left (−60°) | ~300 Hz |
| Center (0°) | ~750 Hz |
| Hard right (+60°) | ~1200 Hz |

The mapping is linear between these endpoints:

```cpp
int freq = map((int)angle_deg, -60, 60, 300, 1200);
tone(BUZZER_PIN, freq, 50); // 50 ms beep duration
delay(interval);
```

This design uses two orthogonal audio dimensions — **tempo for distance, pitch for direction** — so the signals do not interfere with each other and require no learned decoding.

---

## C) Direction → LEDs

| State  | Red LED | Blue LED |
|--------|---------|----------|
| LEFT   | ON      | OFF      |
| RIGHT  | OFF     | ON       |
| CENTER | ON      | ON       |

LEDs provide immediate visual confirmation of system state, useful for debugging and demonstration.

---

# 4. Hardware Components

- Arduino Uno R3
- 2 × HC-SR04 ultrasonic sensors
- 1 × passive buzzer
- 1 × red LED
- 1 × blue LED
- Resistors (220Ω–330Ω for LEDs)
- Breadboard + jumper wires

---

# 5. Final Pin Assignment (Critical Reference)

## Left HC-SR04
- VCC → J53 (5V rail)
- GND → J50
- TRIG → D9 (J52)
- ECHO → D10 (J51)

---

## Right HC-SR04
- VCC → A53 (5V rail)
- GND → A50
- TRIG → D7 (A52)
- ECHO → D8 (A51)

---

## Output Devices

### Buzzer
- GND → A33
- SIGNAL → D6 (A35)

### Red LED (Right indicator)
- GND → A35
- SIGNAL → D4 (A36)

### Blue LED (Left indicator)
- GND → J35
- SIGNAL → D5 (J36)

---

# 6. Electrical Notes (Important)

- All GND rails must be electrically connected (common ground required)
- LEDs must use current-limiting resistors (220Ω–330Ω)
- Sensors must be powered from stable 5V supply

---

# 7. Software Structure (Planned Architecture)

## Step 1: Se   nsor Reading
- Trigger both HC-SR04 sensors
- Measure echo pulse duration using `pulseIn()`
- Convert to distance in cm

## Step 2: Filtering
- Optional averaging of readings to reduce noise

## Step 3: Angle and Direction Calculation
- Compute `delta = L - R`
- Compute `angle = arcsin(delta / D)` where D is the sensor baseline
- Apply threshold to classify state: LEFT, RIGHT, or CENTER

## Step 4: Output Mapping
- Update LEDs based on direction state
- Compute beep interval from proximity (min of L and R)
- Compute tone frequency from angle
- Call `tone(pin, freq, duration)` then `delay(interval)`

---

# 8. Key Design Justification

## Why Two Sensors
- Enables differential spatial comparison
- Provides directional awareness (left vs right bias)
- Enables geometric angle estimation via baseline separation

## Why Ultrasonic Sensing
- Low cost
- Real-time distance estimation
- Simple microcontroller integration

## Why Pitch Encodes Direction (not pattern)
- Pitch and tempo are orthogonal audio dimensions — they do not interfere
- Direction was already being considered for pattern-based encoding, but pattern and tempo are easily confused under movement
- Pitch is immediately interpretable: low = left, high = right
- Requires no learned decoding; matches natural spatial intuition
- Implemented with a single `tone()` call on Arduino — no extra logic

## Why LEDs
- Immediate visual confirmation of system state
- Useful for debugging and demonstration clarity

---

# 9. Expected System Behavior

| Scenario | Tempo | Pitch | Red LED | Blue LED |
|----------|-------|-------|---------|----------|
| Object at 5 m, center (0°) | slow | 750 Hz | ON | ON |
| Object at 0.5 m, center (0°) | constant | 750 Hz | ON | ON |
| Object at 5 m, left (−60°) | slow | 300 Hz | ON | OFF |
| Object at 0.5 m, left (−60°) | constant | 300 Hz | ON | OFF |
| Object at 5 m, right (+60°) | slow | 1200 Hz | OFF | ON |
| Object at 0.5 m, right (+60°) | constant | 1200 Hz | OFF | ON |
| Object at 2.5 m, slightly left (−30°) | medium | 525 Hz | ON | ON |

---

# 10. Limitations

- Measures only nearest reflective surface
- Angle calculation assumes far-field geometry; less accurate for very close objects
- Direction is inferred, not directly measured
- Sensitive to surface angle and material
- Requires filtering for stable output

---

# 11. Extension Ideas (Optional)

- Add hysteresis to stabilize direction switching
- Add calibration mode for environment setup
- Add vibration motor for tactile feedback
- Add OLED display for debugging raw distances and computed angle

---

# 12. Core Concept Summary

This system converts spatial information from two ultrasonic sensors into a multimodal feedback system (audio + visual), enabling simplified environmental awareness through differential distance measurement and temporal/tonal signal encoding. Distance is communicated via beep tempo; direction is communicated via beep pitch — two independent audio channels on a single buzzer, requiring no stereo hardware.
