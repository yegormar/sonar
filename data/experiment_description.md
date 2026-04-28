# Experiment Descriptions

This directory contains raw sensor data files produced by the `direction_clean` Arduino sketch as part of data collection for the sonar project.

## File Name Convention

| Component | Meaning |
|-----------|---------|
| `exp_XXX` | Sequential experiment identifier |
| `walk_...` | Indicates a walking/person motion experiment |
| `left_to_right` / `right_to_left` | Single lateral pass across the sensors |
| `left_right_left` / `left_center_left` / `right_center_right` | Multi-waypoint lateral path |
| `center` / `left` (at start) | Fixed lateral position while changing depth |
| `close` / `medium` / `far` | Distance from the sensor plane |
| `fast` | Increased walking speed (normal speed if omitted) |

## Experiments Table

| # | Filename | Lateral Movement | Depth Range | Speed | Description |
|---|----------|------------------|-------------|-------|-------------|
| 1 | `exp_001_walk_left_to_right_close.txt` | Left → Right | Close | Normal | Single lateral pass at close distance |
| 2 | `exp_002_walk_left_to_right_medium.txt` | Left → Right | Medium | Normal | Single lateral pass at medium distance |
| 3 | `exp_003_walk_left_to_right_far.txt` | Left → Right | Far | Normal | Single lateral pass at far distance |
| 4 | `exp_004_walk_left_right_left_medium.txt` | Left → Right → Left | Medium | Normal | Lateral back-and-forth at medium distance |
| 5 | `exp_005_walk_left_center_left_medium.txt` | Left → Center → Left | Medium | Normal | Lateral partial pass: left to center and back |
| 6 | `exp_006_walk_right_center_right_medium.txt` | Right → Center → Right | Medium | Normal | Lateral partial pass: right to center and back |
| 7 | `exp_007_walk_right_to_left_medium_fast.txt` | Right → Left | Medium | Fast | Single lateral pass, fast speed (take 1/3) |
| 8 | `exp_008_walk_right_to_left_medium_fast.txt` | Right → Left | Medium | Fast | Single lateral pass, fast speed (take 2/3) |
| 9 | `exp_009_walk_right_to_left_medium_fast.txt` | Right → Left | Medium | Fast | Single lateral pass, fast speed (take 3/3) |
| 10 | `exp_010_walk_left_to_right_medium_fast.txt` | Left → Right | Medium | Fast | Single lateral pass, fast speed (take 1/3) |
| 11 | `exp_011_walk_left_to_right_medium_fast.txt` | Left → Right | Medium | Fast | Single lateral pass, fast speed (take 2/3) |
| 12 | `exp_012_walk_left_to_right_medium_fast.txt` | Left → Right | Medium | Fast | Single lateral pass, fast speed (take 3/3) |
| 13 | `exp_013_walk_left_center_left_far.txt` | Left → Center → Left | Far | Normal | Lateral partial pass at far distance (take 1/4) |
| 14 | `exp_014_walk_left_center_left_far.txt` | Left → Center → Left | Far | Normal | Lateral partial pass at far distance (take 2/4) |
| 15 | `exp_015_walk_left_center_left_far.txt` | Left → Center → Left | Far | Normal | Lateral partial pass at far distance (take 3/4) |
| 16 | `exp_016_walk_left_center_left_far.txt` | Left → Center → Left | Far | Normal | Lateral partial pass at far distance (take 4/4) |
| 17 | `exp_017_walk_center_close_far_close.txt` | Center (fixed) | Close → Far → Close | Normal | Depth sweep from center: close, far, back (take 1/3) |
| 18 | `exp_018_walk_center_close_far_close.txt` | Center (fixed) | Close → Far → Close | Normal | Depth sweep from center: close, far, back (take 2/3) |
| 19 | `exp_019_walk_center_close_far_close.txt` | Center (fixed) | Close → Far → Close | Normal | Depth sweep from center: close, far, back (take 3/3) |
| 20 | `exp_020_walk_left_center_left_close.txt` | Left → Center → Left | Close | Normal | Lateral partial pass at close distance (take 1/3) |
| 21 | `exp_021_walk_left_center_left_close.txt` | Left → Center → Left | Close | Normal | Lateral partial pass at close distance (take 2/3) |
| 22 | `exp_022_walk_left_center_left_close.txt` | Left → Center → Left | Close | Normal | Lateral partial pass at close distance (take 3/3) |
| 23 | `exp_023_walk_center_close_medium_far.txt` | Center (fixed) | Close → Medium → Far | Normal | Forward depth sweep from center |
| 24 | `exp_024_walk_left_close_medium_far.txt` | Left (fixed) | Close → Medium → Far | Normal | Forward depth sweep from left side (take 1/2) |
| 25 | `exp_025_walk_left_close_medium_far.txt` | Left (fixed) | Close → Medium → Far | Normal | Forward depth sweep from left side (take 2/2) |

## Data Format

Each `.txt` file contains columns recorded by the `direction_clean` sketch:

1. **Milliseconds** since experiment start
2. **Left sensor** distance measurement (cm)
3. **Right sensor** distance measurement (cm)
4. **Angle measurement** — can be ignored (this is a baseline to be improved)

> **Sensor separation**: 12 cm horizontal distance between the two ultrasonic sensors.

## Purpose

These recordings capture a person performing various motions between the sensors to provide training and validation data for:
- **Angle detection** algorithms
- **Angle change over time** estimation

Multiple takes of the same label help capture repeatability, sensor interference, and room/object reflection variability.
