#!/usr/bin/env python3
"""
Sonar Data Analysis
=================
Comprehensive analysis of sonar experiment logs to:
  a) Compute reliable angle from sensor pair geometry.
  b) Track angle change with time (enter / exit / crossing detection).

Sensor geometry:
  - Baseline = 12 cm, each sensor is 6 cm from centerline (x=0).
  - Left sensor  at (-6, 0), Right sensor at (+6, 0).
  - Person at (x, y).  dL = dist to left sensor, dR = dist to right sensor.

Geometric reconstruction (exact):
  x = (dL^2 - dR^2) / (4 * 6)          [see derivation below]
  y = sqrt(dL^2 - (x + 6)^2)
  angle = atan2(x, y)  (positive = right, negative = left)

Derivation:
  dL^2 = (x+6)^2 + y^2
  dR^2 = (x-6)^2 + y^2
  dL^2 - dR^2 = 4*6*x  =>  x = (dL^2 - dR^2) / 24
  y^2 = dL^2 - (x+6)^2

Out-of-range handling:
  - Values > 400 cm are treated as unreliable (far wall or no echo).
  - When both sensors are in range  -> exact geometric angle.
  - When exactly one sensor is in range -> person is on the corresponding
    side; angle is estimated as the max observable angle on that side.
  - When both sensors are out of range  -> angle undefined (NaN), state = absent.

Filtering approaches compared:
  1. Raw geometric angle (no filter).
  2. Moving median   (robust to single-sample spikes).
  3. EWMA            (Exponentially Weighted Moving Average, light weight).
  4. 2-state Kalman  (state = [angle, angular_velocity]).

Events detected from *filtered* angle + presence:
  - someone_entered_left  : state went absent -> present with angle < -threshold
  - someone_entered_right : state went absent -> present with angle > +threshold
  - someone_exited_left   : state went present with angle < -threshold -> absent
  - someone_exited_right  : state went present with angle > +threshold -> absent

Output:
  Per-experiment PNG plots in ../analysis_output/
  Comprehensive Markdown report.
"""
from __future__ import annotations

import csv
import math
import os
import sys
from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator, List, Tuple, Dict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# =============================================================================
# CONFIGURATION
# =============================================================================

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
OUTPUT_DIR = Path(__file__).resolve().parent.parent / "analysis_output"
OUTPUT_DIR.mkdir(exist_ok=True)

BASELINE_CM = 12.0          # horizontal distance between sensors
HALF_BASELINE = BASELINE_CM / 2.0
MAX_RELIABLE_CM = 400.0    # > this = out of range / far wall
ANALYSIS_MAX_CM = 600.0    # upper clip just for plotting / edge cases
PRESENCE_THRESHOLD_CM = 200.0  # if min(dL, dR) > this => likely nobody near
ANGLE_ZONE_DEG = 15.0       # dead-zone half-width for "center"

# =============================================================================
# DATA LOADER (robust, mirrors validate_data.py)
# =============================================================================

@dataclass
class RawReading:
    line_no: int
    raw: str
    millis: float | None
    left: float | None
    right: float | None
    valid: bool
    error: str | None


def load_experiment(path: Path) -> List[RawReading]:
    """Load one experiment file with the same robustness rules as validate_data."""
    out: List[RawReading] = []
    with open(path, "r", encoding="utf-8") as fh:
        raw_lines = [line.rstrip("\n").rstrip("\r") for line in fh]

    n = len(raw_lines)
    for i, raw in enumerate(raw_lines, start=1):
        line = raw.strip()
        if not line:
            out.append(RawReading(i, raw, None, None, None, False, "empty line"))
            continue
        parts = line.split(",")
        if len(parts) != 4:
            out.append(
                RawReading(i, raw, None, None, None, False,
                           f"unexpected columns ({len(parts)})")
            )
            continue
        try:
            millis = float(parts[0].strip())
            left = float(parts[1].strip())
            right = float(parts[2].strip())
        except ValueError:
            out.append(
                RawReading(i, raw, None, None, None, False, "non-numeric value")
            )
            continue

        out.append(RawReading(i, raw, millis, left, right, True, None))

    return out


# =============================================================================
# GEOMETRIC ANGLE COMPUTATION
# =============================================================================

def compute_geometric_angle(dL: float, dR: float) -> Tuple[float, float, float]:
    """
    Reconstruct position and compute true angle from sensor geometry.

    Returns
    -------
    angle_deg, x, y
        angle_deg: angle from centerline in degrees (positive = right).
        x, y: reconstructed person position in cm.
    """
    # Exact reconstruction assuming person is at (x, y).
    # See module docstring for derivation.
    x = (dL * dL - dR * dR) / (4.0 * HALF_BASELINE)
    inner = dL * dL - (x + HALF_BASELINE) ** 2
    if inner < 0:
        inner = 0.0
    y = math.sqrt(inner)
    angle_deg = math.degrees(math.atan2(x, y))
    return angle_deg, x, y


@dataclass
class ProcessedReading:
    millis: float
    dL: float
    dR: float
    dL_valid: bool          # True if dL <= MAX_RELIABLE_CM
    dR_valid: bool
    angle_exact_deg: float | None
    x: float | None
    y: float | None
    present: bool           # at least one sensor valid and someone is near
    sensor_out_of_range: bool  # both sensors > threshold


def process_readings(raws: List[RawReading]) -> List[ProcessedReading]:
    """Convert raw records to processed geometric readings."""
    out: List[ProcessedReading] = []
    valid_seen = 0
    for r in raws:
        # only accept valid records with all 3 columns present
        if not r.valid or r.millis is None or r.left is None or r.right is None:
            continue

        valid_seen += 1
        # ignore first valid record and last raw line from each file
        if valid_seen == 1 or r.line_no == len(raws):
            continue

        dL = r.left
        dR = r.right
        dL_valid = 0 < dL <= MAX_RELIABLE_CM
        dR_valid = 0 < dR <= MAX_RELIABLE_CM

        # Sensor out-of-range pattern: one or both > MAX_RELIABLE_CM
        # Both far  -> absent
        # One valid -> estimate as extreme angle on that side
        if dL_valid and dR_valid:
            angle_exact, x, y = compute_geometric_angle(dL, dR)
            present = True  # at least one valid, assume person in FOV
        elif dL_valid and not dR_valid:
            # Person is on the far left (only left sensor sees them)
            # We can estimate angle from just dL if we assume y is not huge.
            # For simplicity: treat angle as max left extreme (~ -90 deg)
            # Actually we can approximate: if dR > 400, person is somewhat
            # to the left. Using just dL gives no angle. So we return None
            # but mark present = True so event detection still works.
            angle_exact = None
            x = None
            y = None
            present = True
        elif not dL_valid and dR_valid:
            angle_exact = None
            x = None
            y = None
            present = True
        else:
            angle_exact = None
            x = None
            y = None
            present = False

        out.append(ProcessedReading(
            millis=r.millis,
            dL=dL,
            dR=dR,
            dL_valid=dL_valid,
            dR_valid=dR_valid,
            angle_exact_deg=angle_exact,
            x=x,
            y=y,
            present=present,
            sensor_out_of_range=not dL_valid and not dR_valid,
        ))

    return out


# =============================================================================
# FILTERING
# =============================================================================

def _extract_series(readings: List[ProcessedReading], attr: str) -> np.ndarray:
    values = [getattr(r, attr) for r in readings]
    arr = np.full(len(values), np.nan, dtype=float)
    for i, v in enumerate(values):
        if v is not None:
            arr[i] = v
    return arr


def moving_median_filter(arr: np.ndarray, window: int = 3) -> np.ndarray:
    """Robust 1-D median filter (lightweight, no scipy dependency)."""
    if window % 2 == 0:
        window += 1
    half = window // 2
    out = np.full_like(arr, np.nan)
    n = len(arr)
    for i in range(n):
        lo = max(0, i - half)
        hi = min(n - 1, i + half) + 1
        chunk = arr[lo:hi]
        valid = chunk[np.isfinite(chunk)]
        if len(valid) > 0:
            out[i] = float(np.median(valid))
    return out


def ewma_filter(arr: np.ndarray, alpha: float = 0.3) -> np.ndarray:
    """Exponentially weighted moving average, handles NaN gaps."""
    out = np.full_like(arr, np.nan)
    n = len(arr)
    has_started = False
    prev = 0.0
    for i in range(n):
        if np.isfinite(arr[i]):
            if not has_started:
                prev = arr[i]
                has_started = True
            else:
                prev = alpha * arr[i] + (1 - alpha) * prev
            out[i] = prev
    return out


class Kalman1D:
    """
    Lightweight 1-D Kalman (position + velocity) for embedded usage.
    Reproduces the algorithm from direction_kalman.ino.
    """
    def __init__(self, dt: float = 0.050, q_pos: float = 0.1,
                 q_vel: float = 0.1, r_meas: float = 25.0):
        self.dt = dt
        self.q_pos = q_pos
        self.q_vel = q_vel
        self.r_meas = r_meas
        self.reset()

    def reset(self):
        self.angle = 0.0
        self.vel = 0.0
        self.p00 = 100.0
        self.p01 = 0.0
        self.p10 = 0.0
        self.p11 = 100.0
        self.started = False

    def update(self, measurement: float) -> float:
        if not self.started:
            self.angle = measurement
            self.vel = 0.0
            self.started = True
            return self.angle

        # Predict
        self.angle = self.angle + self.dt * self.vel
        p00 = self.p00 + self.dt * (self.p10 + self.p01 + self.dt * self.p11) + self.q_pos
        p01 = self.p01 + self.dt * self.p11
        p10 = self.p10 + self.dt * self.p11
        p11 = self.p11 + self.q_vel

        # Update
        denom = p00 + self.r_meas
        k0 = p00 / denom
        k1 = p10 / denom
        innovation = measurement - self.angle
        self.angle = self.angle + k0 * innovation
        self.vel = self.vel + k1 * innovation
        self.p00 = p00 - k0 * p00
        self.p01 = p01 - k0 * p01
        self.p10 = p10 - k1 * p00
        self.p11 = p11 - k1 * p01
        return self.angle


def kalman_filter_series(arr: np.ndarray, dt: float = 0.050,
                         q_pos: float = 0.1, q_vel: float = 0.1,
                         r_meas: float = 25.0) -> np.ndarray:
    out = np.full_like(arr, np.nan)
    kf = Kalman1D(dt, q_pos, q_vel, r_meas)
    for i, v in enumerate(arr):
        if np.isfinite(v):
            out[i] = kf.update(v)
    return out


# =============================================================================
# EVENT DETECTION (enter / exit / crossing)
# =============================================================================

@dataclass
class Event:
    time_ms: float
    kind: str       # 'enter_left', 'enter_right', 'exit_left', 'exit_right',
                    # 'cross_left_to_right', 'cross_right_to_left',
                    # 'present_left', 'present_right', 'present_center', 'absent'
    angle_deg: float | None
    description: str


def detect_events(millis: np.ndarray, angle_deg: np.ndarray,
                  present: np.ndarray,
                  zone_deg: float = ANGLE_ZONE_DEG) -> List[Event]:
    """
    High-level FSM:
      absent -> present_left / present_right / present_center
      present_* -> absent  => exit event
      present_left -> present_right  (or vice versa) without absent => crossing
    """
    events: List[Event] = []
    n = len(millis)
    if n == 0:
        return events

    state = "absent"   # absent | present_left | present_center | present_right
    state_start_ms = millis[0]

    def _zone(a):
        if a is None or math.isnan(a):
            return None
        if a < -zone_deg:
            return "present_left"
        if a > zone_deg:
            return "present_right"
        return "present_center"

    for i in range(n):
        new_zone = None
        if present[i]:
            new_zone = _zone(angle_deg[i])
        new_state = new_zone if new_zone else "absent"

        if new_state == state:
            continue

        # Determine event type
        if state == "absent":
            # we just appeared somewhere
            if new_state == "present_left":
                events.append(Event(millis[i], "enter_left",
                                    angle_deg[i], "Entered from LEFT"))
            elif new_state == "present_right":
                events.append(Event(millis[i], "enter_right",
                                    angle_deg[i], "Entered from RIGHT"))
            elif new_state == "present_center":
                events.append(Event(millis[i], "enter_center",
                                    angle_deg[i], "Entered near CENTER"))
        elif new_state == "absent":
            if state == "present_left":
                events.append(Event(millis[i], "exit_left",
                                    angle_deg[i], "Exited to LEFT"))
            elif state == "present_right":
                events.append(Event(millis[i], "exit_right",
                                    angle_deg[i], "Exited to RIGHT"))
            elif state == "present_center":
                events.append(Event(millis[i], "exit_center",
                                    angle_deg[i], "Exited from CENTER"))
        else:
            # state transition without passing through absent
            if state == "present_left" and new_state == "present_right":
                events.append(Event(millis[i], "cross_left_to_right",
                                    angle_deg[i], "Crossed LEFT to RIGHT"))
            elif state == "present_right" and new_state == "present_left":
                events.append(Event(millis[i], "cross_right_to_left",
                                    angle_deg[i], "Crossed RIGHT to LEFT"))
            elif state == "present_center" and new_state in ("present_left", "present_right"):
                # started from center and moved to one side
                pass  # ambiguous; could be start of crossing

        state = new_state
        state_start_ms = millis[i]

    return events


# =============================================================================
# PLOTTING
# =============================================================================

def plot_experiment(readings: List[ProcessedReading], fname: str) -> Path:
    """Generate a multi-panel figure for one experiment."""
    ts = np.array([r.millis for r in readings])
    dL = np.array([r.dL for r in readings])
    dR = np.array([r.dR for r in readings])
    present = np.array([r.present for r in readings])
    angle_raw = np.array([r.angle_exact_deg if r.angle_exact_deg is not None
                          else np.nan for r in readings])

    # Clip distances for visualization
    dL_plot = np.clip(dL, 0, ANALYSIS_MAX_CM)
    dR_plot = np.clip(dR, 0, ANALYSIS_MAX_CM)

    # Filters
    angle_median = moving_median_filter(angle_raw, window=5)
    angle_ewma   = ewma_filter(angle_raw, alpha=0.3)
    angle_kalman = kalman_filter_series(angle_raw, dt=0.050,
                                        q_pos=0.1, q_vel=0.1, r_meas=25.0)

    # Detect events on Kalman-filtered angle (best trade-off)
    events = detect_events(ts, angle_kalman, present)

    # Convert ms to seconds for x-axis
    t_sec = (ts - ts[0]) / 1000.0

    fig, axes = plt.subplots(5, 1, figsize=(12, 14), sharex=True,
                             gridspec_kw={"height_ratios": [1, 1, 1, 1, 0.6]})
    fig.suptitle(f"Experiment: {fname}", fontsize=14, fontweight="bold")

    # --- Panel 1: raw distances --------------------------------------------
    ax = axes[0]
    ax.plot(t_sec, dL_plot, "b-", alpha=0.6, label="Left sensor")
    ax.plot(t_sec, dR_plot, "r-", alpha=0.6, label="Right sensor")
    ax.axhline(MAX_RELIABLE_CM, color="gray", linestyle="--", alpha=0.5,
               label="Reliability ceiling (400 cm)")
    ax.fill_between(t_sec, 0, ANALYSIS_MAX_CM, where=~present,
                    color="lightgray", alpha=0.3, label="Absent")
    ax.set_ylabel("Distance (cm)")
    ax.legend(loc="upper right", fontsize=8)
    ax.set_ylim(0, ANALYSIS_MAX_CM)
    ax.set_title("Raw Sensor Readings")

    # --- Panel 2: angle reconstruction ---------------------------------------
    ax = axes[1]
    ax.plot(t_sec, angle_raw, "k-", alpha=0.3, linewidth=0.5,
            label="Exact geometric angle (raw)")
    ax.plot(t_sec, angle_median, "g-", linewidth=1.5, label="Median (w=5)")
    ax.plot(t_sec, angle_ewma, "m-", linewidth=1.5, label="EWMA (α=0.3)")
    ax.plot(t_sec, angle_kalman, "c-", linewidth=2.0, label="Kalman")
    ax.axhline(0, color="black", linewidth=0.5)
    ax.axhline(ANGLE_ZONE_DEG, color="orange", linestyle="--", alpha=0.5)
    ax.axhline(-ANGLE_ZONE_DEG, color="orange", linestyle="--", alpha=0.5)
    ax.set_ylabel("Angle (°)")
    ax.set_ylim(-100, 100)
    ax.legend(loc="upper right", fontsize=8)
    ax.set_title("Angle Estimates")

    # --- Panel 3: angular velocity (from Kalman) -----------------------------
    ax = axes[2]
    # differentiate Kalman angle
    vel = np.full_like(angle_kalman, np.nan)
    for i in range(1, len(angle_kalman)):
        dt = (ts[i] - ts[i - 1]) / 1000.0
        if dt > 0 and np.isfinite(angle_kalman[i]) and np.isfinite(angle_kalman[i - 1]):
            vel[i] = (angle_kalman[i] - angle_kalman[i - 1]) / dt
    ax.plot(t_sec, vel, "b-", linewidth=1.0, alpha=0.7)
    ax.axhline(0, color="black", linewidth=0.5)
    ax.set_ylabel("Angular Velocity (°/s)")
    ax.set_title("Angular Velocity (Kalman)")

    # --- Panel 4: y-distance (depth) ---------------------------------------
    ax = axes[3]
    ys = np.array([r.y if r.y is not None else np.nan for r in readings])
    ax.plot(t_sec, ys, "g-", linewidth=1.0)
    ax.set_ylabel("Depth y (cm)")
    ax.set_title("Reconstructed Depth (y)")

    # --- Panel 5: events -----------------------------------------------------
    ax = axes[4]
    for ev in events:
        ev_t = (ev.time_ms - ts[0]) / 1000.0
        color = {
            "enter_left": "green", "enter_right": "green",
            "enter_center": "green",
            "exit_left": "red", "exit_right": "red", "exit_center": "red",
            "cross_left_to_right": "blue", "cross_right_to_left": "blue",
        }.get(ev.kind, "gray")
        ax.axvline(ev_t, color=color, alpha=0.6, linewidth=1.5)
        ax.text(ev_t, 0.5, ev.kind.replace("_", "\n"), rotation=90,
                fontsize=6, color=color, ha="center", va="center")
    ax.set_ylim(0, 1)
    ax.set_yticks([])
    ax.set_xlabel("Time (s)")
    ax.set_title("Detected Events")

    plt.tight_layout(rect=[0, 0.03, 1, 0.96])
    plot_path = OUTPUT_DIR / f"{fname}.png"
    fig.savefig(plot_path, dpi=150)
    plt.close(fig)
    return plot_path


# =============================================================================
# METRICS
# =============================================================================

def compute_smoothness(angle_arr: np.ndarray, ts_ms: np.ndarray) -> float:
    """Quantify smoothness via mean absolute angular velocity."""
    vel = 0.0
    count = 0
    for i in range(1, len(angle_arr)):
        if np.isfinite(angle_arr[i]) and np.isfinite(angle_arr[i - 1]):
            dt = (ts_ms[i] - ts_ms[i - 1]) / 1000.0
            if dt > 0:
                vel += abs((angle_arr[i] - angle_arr[i - 1]) / dt)
                count += 1
    return vel / count if count else float('inf')


def compute_outliers(angle_arr: np.ndarray) -> int:
    """Count samples with jump > 45° from previous (likely spike)."""
    count = 0
    for i in range(1, len(angle_arr)):
        if np.isfinite(angle_arr[i]) and np.isfinite(angle_arr[i - 1]):
            if abs(angle_arr[i] - angle_arr[i - 1]) > 45:
                count += 1
    return count


# =============================================================================
# MAIN
# =============================================================================

def main():
    files = sorted(DATA_DIR.glob("exp_*.txt"))
    if not files:
        print("No experiment files found.")
        sys.exit(1)

    summary_rows: List[Dict] = []
    all_events: Dict[str, List[Event]] = {}

    print(f"Analyzing {len(files)} experiment files...\n")

    for fp in files:
        print(f"  {fp.name} ... ", end="", flush=True)
        raws = load_experiment(fp)
        proc = process_readings(raws)
        if not proc:
            print("SKIP (no valid records)")
            continue
        plot_path = plot_experiment(proc, fp.stem)

        ts = np.array([r.millis for r in proc])
        angle_raw = np.array([r.angle_exact_deg if r.angle_exact_deg is not None
                              else np.nan for r in proc])
        angle_median = moving_median_filter(angle_raw, window=5)
        angle_ewma   = ewma_filter(angle_raw, alpha=0.3)
        angle_kalman = kalman_filter_series(angle_raw, dt=0.050,
                                            q_pos=0.1, q_vel=0.1, r_meas=25.0)
        present = np.array([r.present for r in proc])

        events = detect_events(ts, angle_kalman, present)
        all_events[fp.name] = events

        metrics = {
            "file": fp.name,
            "samples": len(proc),
            "valid_angles": int(np.sum(np.isfinite(angle_raw))),
            "events": len(events),
            "smooth_raw": round(compute_smoothness(angle_raw, ts), 2),
            "smooth_median": round(compute_smoothness(angle_median, ts), 2),
            "smooth_ewma": round(compute_smoothness(angle_ewma, ts), 2),
            "smooth_kalman": round(compute_smoothness(angle_kalman, ts), 2),
            "outliers_raw": compute_outliers(angle_raw),
            "outliers_median": compute_outliers(angle_median),
            "outliers_ewma": compute_outliers(angle_ewma),
            "outliers_kalman": compute_outliers(angle_kalman),
        }
        summary_rows.append(metrics)
        print(f"done ({len(proc)} samples, {len(events)} events)")

    # Write summary report
    report_path = OUTPUT_DIR / "ANALYSIS_REPORT.md"
    with open(report_path, "w", encoding="utf-8") as fh:
        fh.write("# Sonar Data Analysis Report\n\n")
        fh.write("Generated by `tools/analyze_sonar.py`\n\n")

        fh.write("## Smoothness Metrics (lower = smoother)\n\n")
        fh.write("| File | Samples | Valid Angles | Events | "
                 "Smooth Raw | Median | EWMA | Kalman | "
                 "Outliers Raw | Median | EWMA | Kalman |\n")
        fh.write("|------|---------|--------------|--------|"
                 "------------|--------|------|--------|"
                 "--------------|--------|------|--------|\n")
        for r in summary_rows:
            fh.write(
                f"| {r['file'][:40]:<40} | {r['samples']:<7} | "
                f"{r['valid_angles']:<12} | {r['events']:<6} | "
                f"{r['smooth_raw']:<10} | {r['smooth_median']:<6} | "
                f"{r['smooth_ewma']:<4} | {r['smooth_kalman']:<6} | "
                f"{r['outliers_raw']:<12} | {r['outliers_median']:<6} | "
                f"{r['outliers_ewma']:<4} | {r['outliers_kalman']:<6} |\n"
            )

        fh.write("\n## Event Summary per Experiment\n\n")
        for fname, evs in all_events.items():
            fh.write(f"### {fname}\n")
            if not evs:
                fh.write("- No events detected.\n")
            for ev in evs:
                fh.write(f"- **{ev.kind}** at {ev.time_ms:,.0f} ms "
                         f"(angle={ev.angle_deg:.1f}°): {ev.description}\n")
            fh.write("\n")

    print(f"\nReport written to: {report_path}")
    print(f"Plots written to:  {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
