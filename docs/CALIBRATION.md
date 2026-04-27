# Sonar System — Calibration Guide

---

# 1. Overview

Calibration teaches the system what your specific setup considers "close", "far", "center", "hard left", and "hard right" — using real objects in real positions, not formulas or measurements.

You will place an object in five positions. The system records what it sees at each position and builds all its signalling thresholds from those observations. No numbers to enter, no ruler needed.

---

# 2. What You Need

- The assembled hardware (same wiring as the main sketch — no changes)
- Any flat, hard object to use as a target (a book, clipboard, or box works well)
- Arduino IDE with Serial Monitor open at **9600 baud**
- About 5 minutes

---

# 3. The Five Positions

You will be asked to place the object at each of these positions in order:

| Step | Position | What It Defines |
|------|----------|-----------------|
| 1 | Closest possible — directly in front | The "alarm" threshold — constant beep |
| 2 | Closest center again — hold still | Confirms near reading, checks for noise |
| 3 | Farthest center — still detectable | The "wake up" threshold — where beeping begins |
| 4 | Far left edge — move it sideways until detection gets unreliable | Maximum left angle |
| 5 | Far right edge — same on the other side | Maximum right angle |

The system does not need to know how far away anything is. It only needs to see the contrast between positions.

---

# 4. Running the Calibration

Upload `sonar_calibration.ino` to your board. Open Serial Monitor at **9600 baud**. The sketch will guide you through each position step by step.

At each step:
- Read the instruction
- Place the object in the described position
- Send any character (e.g. press `s` and Enter) to capture the reading
- The sketch confirms what it recorded before moving on

At the end, the sketch prints a ready-to-copy block of constants. Paste them into the top of your main sketch.

---

# 5. What the Constants Mean (Plain Language)

After calibration, the main sketch uses these five reference points internally. You never need to think about the numbers — but here is what each one represents:

| Constant | Comes From | Used For |
|----------|------------|----------|
| `CAL_NEAR` | Steps 1+2 averaged | Triggers constant beep — maximum urgency |
| `CAL_FAR` | Step 3 | Below this = silence. Closer = beeping begins |
| `CAL_LEFT` | Step 4 | Maps to lowest pitch and full-left LED |
| `CAL_RIGHT` | Step 5 | Maps to highest pitch and full-right LED |

Everything in between is interpolated. An object halfway to the left gets a pitch halfway between center and max-left. An object halfway to the near threshold gets a beep rate halfway between slow and constant.

---

# 6. Tips for Good Calibration

**Steps 1 & 2 (Near):** Get as close as the sensors can reliably read — typically 5–10 cm. Do not go so close that readings become erratic or zero. If the Serial Monitor shows an instability warning, back off a centimeter or two.

**Step 3 (Far):** Move the object back slowly until beeping would feel unnecessary — the point where you'd want silence. This is a judgment call. A typical useful range ends around 100–150 cm.

**Steps 4 & 5 (Left/Right):** Move the object sideways at roughly the same distance as Step 3. Stop when detection starts feeling unreliable — readings will become noisy or start jumping. Back up just slightly from that edge. Do both sides at a similar distance for symmetry.

**Consistency matters more than precision.** The system compares readings against each other. A consistent but imprecise calibration beats a precise but inconsistent one.

---

# 7. When to Re-Calibrate

- The sensor pair is physically repositioned or the board is moved to a new environment
- The system is used in a very different space (a large open room vs. a narrow corridor behave differently)
- Left/right response feels asymmetric during normal use
- Near/far thresholds feel wrong after a period of reliable use

Re-calibration takes about 5 minutes and is done by simply uploading the calibration sketch again.

---

# 8. Troubleshooting

| Symptom | What to Do |
|---------|------------|
| Serial Monitor shows garbage text | Set Serial Monitor to exactly 9600 baud |
| Instability warning at Steps 1 or 2 | Object is too close — back off 1–2 cm |
| "No echo" warning | Object out of sensor range or surface angle too steep — try a flatter object |
| Left and right feel unequal | Redo Steps 4 & 5 at the same distance on both sides |
| Beeping starts too early or too late | Redo Step 3 — move the object to where you personally want beeping to begin |
