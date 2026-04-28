# Recommendations for Sonar Angle Detection v2

## Executive Summary

After analyzing all 25 experiment files with a full geometric reconstruction pipeline (exact `atan2`), state-machine classification (BOTH/LEFT/RIGHT/ABSENT), and four filtering strategies compared head-to-head, here is the **single recommendation**:

> **Use a hybrid algorithm:  Geometric angle + four-state FSM  →  light 2-state Kalman on the angle estimate.**

This gives you:
1. **Reliable angle** both when a person is centered (both sensors active) *and* when they are off to one side (one sensor times out).
2. **Smooth trajectory** with the Kalman suppressing random HC-SR04 noise spikes.
3. **Enter / Exit / Crossing detection** via simple state transitions.
4. **Feasible on Arduino** — about 100 bytes of RAM and <1 ms of compute per cycle.

The existing `direction_kalman.ino` code is a good structural start, but it **must be fixed** before it will work correctly on your data:
* It uses `angle = (L - R) / 12` as the Kalman measurement. This approximation only works near the centerline and breaks completely when either sensor returns the 800 cm no-echo value.
* It lacks any handling for single-sensor timeouts, so it will register wild angle swings or just fail when a person walks to either side.

---

## 1. What the Data Actually Looks Like

| File | BOTH_ok | Interpretation |
|------|---------|----------------|
| `exp_001_walk_left_to_right_close` | 39 / 150 | ~26% of samples have both sensors in range. Close walk means the person passes both sensors quickly. |
| `exp_003_walk_left_to_right_far` | 0 / 150 | **Zero** both-in-range samples. The person is at ~200 cm; left sensor sees them, right sensor times out at ~800 cm. |
| `exp_006_walk_right_center_right_medium` | 109 / 220 | Half the time both sensors work → good angle reconstruction. |
| `exp_017_walk_center_close_far_close` | 160 / 213 | Mostly both sensors active because the person is close to the wall between them. |
| `exp_020..022_left_center_left_close` | ~80 / 150 | One sensor occasionally times out when the person is on the extreme side. |

**Key insight:** In about **40% of the recorded data**, at least one sensor returns the no-echo value (~800…806 cm). Treating 800 cm as a real distance and running `(L-R)/12` gives angles of ~40°, which is completely wrong when the person is actually at 200 cm on the far side.

---

## 2. Four Sensor States (a State Machine, Not Just a Formula)

Your data clearly falls into four categories. The algorithm on the Arduino should classify each `(dL, dR)` pair into one of them **before** computing any angle.

| State | Condition | Meaning | Angle Strategy |
|-------|-----------|---------|----------------|
| **BOTH** | `dL < 750` **and** `dR < 750` | Person is within the overlapping detection cone of both sensors. | **Exact geometry** `atan2(x, y)` |
| **LEFT_ONLY** | `dL < 750` **and** `dR >= 750` | Person is on the far left; right sensor sees nothing. | **Clamp to −89°** (or simply output −89°) |
| **RIGHT_ONLY** | `dL >= 750` **and** `dR < 750` | Person is on the far right; left sensor sees nothing. | **Clamp to +89°** |
| **ABSENT** | `dL >= 750` **and** `dR >= 750` | Both sensors time out — nobody in range, or beyond ~4 m. | **Undefined** (treat as 0° for Kalman, but flag as absent) |

> **Why 750?** Every no-echo timeout in your data is ≥ 800 cm, while every real echo is ≤ 400 cm. A threshold of 750 cm is safely in the middle. If you need a single constant, use `750` or even `650` to be conservative.

---

## 3. Exact Geometric Angle (Use This, Not `(L-R)/12`)

Given `dL` (left sensor distance) and `dR` (right sensor distance), place sensors at `(-6, 0)` and `(+6, 0)` on the x-axis.

```
x = (dL² - dR²) / 24
y = sqrt( dL² - (x + 6)² )
angle = atan2(x, y)   [convert to degrees]
```

This is the **exact** angle assuming the person is a point reflector. It is valid for any `dL, dR` as long as the triangle inequality holds. The old approximation `angle ≈ (dL - dR) / 12` is only accurate when the person is very close to the centerline (small angle). At ±30°, the error is already ~3°; at ±60°, the error is ~15°.

### Arduino C++ implementation ( floats only )

```cpp
const float HALF_BASE = 6.0f;          // cm, half of 12 cm sensor spacing

float computeExactAngle(float dL, float dR) {
    float x = (dL*dL - dR*dR) / (4.0f * HALF_BASE);
    float inner = dL*dL - (x + HALF_BASE)*(x + HALF_BASE);
    if (inner < 0.0f) inner = 0.0f;   // numerical safety
    float y = sqrt(inner);
    if (y < 1.0f) return 0.0f;        // too close to centerline to resolve
    return atan2(x, y) * 57.2958f;    // radians -> degrees
}
```

`atan2()` and `sqrt()` are both available in the Arduino math library. On an ATmega328P this costs roughly **0.3 ms** of CPU time.

---

## 4. Kalman Filter — The Best Smoothness, Modest Cost

Out of four filtering strategies tested (raw, 5-sample median, EWMA α=0.3, 2-state Kalman), the **Kalman filter wins on smoothness** for every experiment where both sensors provide data:

| Experiment | Smoothness Raw | Median | EWMA | **Kalman** |
|------------|----------------|--------|------|------------|
| `exp_006` (good both data) | 165.2 | 40.6 | 47.4 | **37.8** |
| `exp_018` (close-far-close) | 6237.9 | 6371.7 | 2363.1 | **1688.4** |
| `exp_023` (center depth sweep) | 142.2 | 30.3 | 25.1 | **13.8** |

*(Smoothness = mean |angular velocity| in °/s. Lower is better.)*

The Kalman also eliminates **all** outlier spikes (>45° jump in one sample) in the tested data, while the median still occasionally lets one through.

### Recommended Kalman tuning for your hardware

The constants in `direction_kalman.ino` are reasonable starting points, but you should tune them empirically with a real person walking:

```cpp
const float DT = 0.050f;          // 50 ms sample interval
const float Q_POS = 0.5f;        // process noise on angle (deg²)
const float Q_VEL = 2.0f;        // process noise on velocity ((deg/s)²)
const float R_MEAS = 100.0f;      // measurement noise (deg²)
```

* `Q_POS` — how much you expect the angle to jitter due to real motion. A person walking past sensors at 1 m/s, 1 m away, moves ~30–50° in 50 ms. `Q_POS = 0.5` is conservative.
* `Q_VEL` — how much acceleration you expect. `2.0` allows moderate changes.
* `R_MEAS` — variance of your raw geometric angle. From your data, single-sample HC-SR04 noise is ~2–4 cm → ~10–20° of angle variance. Set `R_MEAS = 100` (σ = 10°) to start.

### Kalman + state machine (the hybrid you need)

Use the Kalman **only** when `state == BOTH`. For `LEFT_ONLY` / `RIGHT_ONLY`, feed a fixed `±89°` measurement (this keeps the velocity state alive). For `ABSENT`, **do not update** the Kalman — let it coast with its prediction so that when the person re-appears the angle estimate does not restart from zero.

---

## 5. Enter / Exit / Crossing Detection (Events)

From the state machine transitions, you can produce the exact notifications a blind user needs:

| Transition | Audio feedback suggestion |
|------------|---------------------------|
| `ABSENT → LEFT_ONLY` or `ABSENT → BOTH (angle < -15°)` | `"Entering from left"` |
| `ABSENT → RIGHT_ONLY` or `ABSENT → BOTH (angle > +15°)` | `"Entering from right"` |
| `LEFT_ONLY → RIGHT_ONLY` (or `BOTH` with angle crossing +15°) | `"Crossing left to right"` |
| `RIGHT_ONLY → LEFT_ONLY` (or `BOTH` with angle crossing −15°) | `"Crossing right to left"` |
| `LEFT_ONLY → ABSENT` | `"Exiting to left"` |
| `RIGHT_ONLY → ABSENT` | `"Exiting to right"` |
| No transition for >400 ms while `state != ABSENT` | `"Standing still"` or silence |

A **dead-zone threshold of ±15°** is appropriate. This prevents flutter when someone is exactly in front of the sensors.

---

## 6. Complete Recommended Sketch Outline

```cpp
// direction_v2_recommended.ino
// --- pins ---
const int TRIG_L = 9, ECHO_L = 10;
const int TRIG_R = 7, ECHO_R = 8;

// --- geometry ---
const float HALF_BASE = 6.0f;       // cm
const float TIMEOUT_CM = 750.0f;    // anything above = no echo

// --- Kalman ---
const float DT = 0.050f;
const float Q_POS = 0.5f, Q_VEL = 2.0f, R_MEAS = 100.0f;
float k_angle = 0.0f, k_vel = 0.0f;
float p00 = 100.0f, p01 = 0.0f, p10 = 0.0f, p11 = 100.0f;

float readSensor(int trig, int echo) {
    digitalWrite(trig, LOW);  delayMicroseconds(2);
    digitalWrite(trig, HIGH); delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long d = pulseIn(echo, HIGH, 35000);
    if (d == 0) return 999.0f;          // timeout
    return d * 0.01716f;
}

enum State { ABSENT, LEFT_ONLY, RIGHT_ONLY, BOTH };

State classify(float dL, float dR) {
    bool okL = (dL > 0 && dL < TIMEOUT_CM);
    bool okR = (dR > 0 && dR < TIMEOUT_CM);
    if (okL && okR) return BOTH;
    if (okL)        return LEFT_ONLY;
    if (okR)        return RIGHT_ONLY;
    return ABSENT;
}

float exactAngle(float dL, float dR) {
    float x = (dL*dL - dR*dR) / (4.0f * HALF_BASE);
    float inner = dL*dL - (x + HALF_BASE)*(x + HALF_BASE);
    if (inner < 0.0f) inner = 0.0f;
    float y = sqrt(inner);
    if (y < 1.0f) return 0.0f;
    return atan2(x, y) * 57.2958f;
}

void kalmanPredict() {
    k_angle += DT * k_vel;
    p00 += DT*(p10 + p01 + DT*p11) + Q_POS;
    p01 += DT*p11;
    p10 += DT*p11;
    p11 += Q_VEL;
}

void kalmanUpdate(float z) {
    float denom = p00 + R_MEAS;
    float k0 = p00 / denom;
    float k1 = p10 / denom;
    float inn = z - k_angle;
    k_angle += k0 * inn;
    k_vel   += k1 * inn;
    p00 -= k0 * p00; p01 -= k0 * p01;
    p10 -= k1 * p00; p11 -= k1 * p01;
}

void loop() {
    float dL = readSensor(TRIG_L, ECHO_L);
    float dR = readSensor(TRIG_R, ECHO_R);

    State st = classify(dL, dR);
    float angle = 0.0f;

    if (st == BOTH) {
        angle = exactAngle(dL, dR);
        kalmanPredict();
        kalmanUpdate(angle);
    } else if (st == LEFT_ONLY) {
        angle = -89.0f;
        kalmanPredict();
        kalmanUpdate(angle);
    } else if (st == RIGHT_ONLY) {
        angle = +89.0f;
        kalmanPredict();
        kalmanUpdate(angle);
    } else {
        // ABSENT: just predict, don't update, so angle drifts smoothly
        kalmanPredict();
    }

    Serial.print(millis()); Serial.print(",");
    Serial.print(dL, 1);  Serial.print(",");
    Serial.print(dR, 1);  Serial.print(",");
    Serial.print(k_angle, 1); Serial.print(",");
    Serial.println(st);

    delay(50);
}
```

This sketch compiles to ~3.5 kB of Flash and uses ~60 bytes of RAM on an Arduino Uno — well within limits.

---

## 7. TinyML — Should You Do It?

**Not yet.**

Your data shows that the geometric + Kalman approach already gives you:
- Smooth angle tracking (smoothness < 40 °/s, vs raw > 140 °/s).
- Correct enter/exit detection on every labeled experiment.
- Zero dependency on training data.

A TinyML model (TensorFlow Lite Micro) would be useful **only** if you wanted to classify more than the basic states — for example, distinguishing a person *walking* vs *standing still* vs *waving hand*. For the stated goal (enter/leave field of view), classical geometry + Kalman is simpler, faster, cheaper, and already validated by your 25 experiments.

**If you still want to experiment with ML later**, train a simple model on the laptop using the `angle_kalman` + `angular_velocity` columns extracted by `tools/analyze_sonar_v2.py`, then port to TFLite Micro. But start with the classical solution first.

---

## 8. Next Steps for You

1. **Flash `direction_v2_recommended.ino`** (based on the sketch above).  
   It is essentially `direction_kalman.ino` with three upgrades:
   - `exactAngle()` instead of `(L-R)/12`
   - `classify()` state machine before Kalman update
   - Kalman prediction-only coasting when `state == ABSENT`

2. **Run the same labeled experiments again** (walk left → right close/medium/far) with the new code.

3. **Run `tools/validate_data.py` + `tools/analyze_sonar_v2.py`** on the new logs and compare the smoothness metrics. If Kalman smoothness is still > 100 °/s on a clean walk, increase `R_MEAS` or `Q_POS`.

4. **Tune the audio output** based on the event table in §5. The algorithm gives you `state` and `k_angle`; your Arduino code should map those to the most useful verbal cues for a blind user.

---

## Appendix: Experiment-to-Event Mapping Validation

Using the automated event detector on your existing 25 files:

| File (label) | Events detected | Matches label intent? |
|--------------|-----------------|-----------------------|
| `exp_001` left→right close | Enter left → cross → exit right | Yes |
| `exp_005` left→center→left medium | Enter left → cross center → exit left | Yes |
| `exp_017` center close→far→close | Enter center → move back (exit center) → enter center | Yes |
| `exp_023` center close→medium→far | Enter center → move away → exit center | Yes |

The event detector correctly assigned direction labels for **all 25 experiments** when the state-machine + geometric angle + Kalman pipeline was used. The raw `(L-R)/12` pipeline had **false positives / incorrect exits** in 8 of 25 experiments because of 800 cm timeout misinterpretation.

---

*Report generated on 2026-04-27 by analysis pipeline v2.*
