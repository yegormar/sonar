#!/usr/bin/env python3
"""
Sonar Data Analysis - V2 (Comprehensive)
=========================================
Re-designed to handle the actual sensor behaviour seen in the data:
  - HC-SR04 returns ~800-805 cm when no echo is heard (timeout).
  - When both sensors time-out the person is genuinely out of range.
  - When exactly one sensor times-out the person is on the timeout side.
  - Real echoes give consistent left/right pairs that allow exact geometry.

Goal: data-backed recommendation for the on-Arduino algorithm.

Sensor geometry
---------------
  Left sensor  at  (-6, 0)    Right sensor at  (+6, 0)
  dL^2 = (x+6)^2 + y^2          dR^2 = (x-6)^2 + y^2
  x   = (dL^2 - dR^2) / 24
  y   = sqrt(dL^2 - (x+6)^2)
  angle = atan2(x, y)   (degrees)   positive = right, negative = left

Data-derived constants
----------------------
  NO_ECHO_FLOOR        ~ 800 cm   (anything >= is treated as timeout)
  BASELINE             = 12 cm
  MAX_DETECTABLE_RANGE ~ 400 cm   (beyond this geometry breaks)
  Typical sample dt    ~ 50 ms  (direction_clean.ino delay)

Four sensor states observed in the data
-----------------------------------------
  1. BOTH_OK      -> exact angle, y-position reconstructable
  2. LEFT_ONLY    -> right sensor timed out, person far-left of right FOV
  3. RIGHT_ONLY   -> left  sensor timed out, person far-right of left FOV
  4. BOTH_TIMEOUT -> person absent or beyond useful range

Enter/Exit detection logic (target: blind-user assistant)
  - Transition ABSENT -> {BOTH, LEFT, RIGHT}   = someone ENTERED
  - Transition {BOTH, LEFT, RIGHT} -> ABSENT     = someone EXITED
  - Transition LEFT -> RIGHT without ABSENT      = CROSSING left-to-right
  - Transition RIGHT -> LEFT without ABSENT      = CROSSING right-to-left
  - No transitions for > ~300 ms                 = no activity in cone

Approaches compared
  1. Raw diff angle    (L-R)/12           - lightweight, exact only near center
  2. Geometric atan2  (exact x/y)         - more accurate off-center
  3. Median   (window 5)                  - kills single-sample glitches
  4. EWMA     (alpha=0.3)                 - light smooth
  5. 2-state Kalman (angle, angular_vel)  - best smoothness, modest compute

Output
  PNG plots for every experiment
  Markdown report with quantitative comparison
"""
from __future__ import annotations

import math
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
OUT_DIR  = Path(__file__).resolve().parent.parent / "analysis_output"
OUT_DIR.mkdir(exist_ok=True)

BASE_CM          = 12.0
HALF             = BASE_CM / 2.0
NO_ECHO_FLOOR    = 760.0
MAX_RANGE        = 400.0
PRESENCE_ZONE    = 15.0
SAMPLE_DT        = 0.050

STATE_BOTH   = "BOTH"
STATE_LEFT   = "LEFT"
STATE_RIGHT  = "RIGHT"
STATE_ABSENT = "ABSENT"


class RawRow:
    def __init__(self, line_no, raw, valid, ms=None, dL=None, dR=None, ang=None, error=None):
        self.line_no = line_no; self.raw = raw; self.valid = valid
        self.ms = ms; self.dL = dL; self.dR = dR; self.ang = ang; self.error = error


def load(path):
    out = []
    with open(path, "r", encoding="utf-8") as fh:
        lines = [line.rstrip("\n").rstrip("\r") for line in fh]
    for i, raw in enumerate(lines, start=1):
        line = raw.strip()
        if not line:
            out.append(RawRow(i, raw, False, error="empty")); continue
        parts = line.split(",")
        if len(parts) != 4:
            out.append(RawRow(i, raw, False, error=f"cols={len(parts)}")); continue
        try:
            ms, dL, dR, ang = map(float, parts)
        except ValueError:
            out.append(RawRow(i, raw, False, error="bad_num")); continue
        out.append(RawRow(i, raw, True, ms, dL, dR, ang))
    return out


def exact_angle(dL, dR):
    if dL <= 0 or dR <= 0:
        return None, None, None
    x = (dL*dL - dR*dR) / (4.0 * HALF)
    inner = dL*dL - (x + HALF)**2
    if inner < 0:
        inner = 0.0
    y = math.sqrt(inner)
    if y == 0:
        return None, x, y
    return math.degrees(math.atan2(x, y)), x, y


def classify(dL, dR):
    l_ok = 0 < dL < NO_ECHO_FLOOR
    r_ok = 0 < dR < NO_ECHO_FLOOR
    if l_ok and r_ok:
        return (STATE_BOTH, max(dL, dR))
    if l_ok and not r_ok:
        return (STATE_LEFT, dL)
    if not l_ok and r_ok:
        return (STATE_RIGHT, dR)
    return (STATE_ABSENT, 0.0)


class Frame:
    def __init__(self, ms, dL, dR, state, dist_max, angle_exact, angle_diff, angle_fb):
        self.ms = ms; self.dL = dL; self.dR = dR; self.state = state
        self.dist_max = dist_max; self.angle_exact = angle_exact
        self.angle_diff = angle_diff; self.angle_fallback = angle_fb


def build_frames(rows):
    valid = [r for r in rows if r.valid and r.ms is not None and r.dL is not None and r.dR is not None]
    if len(valid) < 3:
        return []
    valid = valid[1:-1]
    out = []
    for r in valid:
        st, dmax = classify(r.dL, r.dR)
        ang_exact, x, y = exact_angle(r.dL, r.dR) if st == STATE_BOTH else (None, None, None)
        ang_diff = (r.dL - r.dR) / BASE_CM
        if st == STATE_BOTH and ang_exact is not None:
            ang_fb = ang_exact
        elif st == STATE_LEFT:
            ang_fb = -89.0
        elif st == STATE_RIGHT:
            ang_fb = +89.0
        else:
            ang_fb = 0.0
        out.append(Frame(r.ms, r.dL, r.dR, st, dmax, ang_exact, ang_diff, ang_fb))
    return out


class Kalman2D:
    """Same maths as direction_kalman.ino, pure floats."""
    def __init__(self, dt=SAMPLE_DT, q0=0.1, q1=0.1, r=25.0):
        self.dt = dt; self.q0 = q0; self.q1 = q1; self.r = r; self.reset()
    def reset(self):
        self.a = 0.0; self.v = 0.0
        self.p00 = 100.0; self.p01 = 0.0; self.p10 = 0.0; self.p11 = 100.0
        self.started = False
    def update(self, z):
        if not self.started:
            self.a = z; self.v = 0.0; self.started = True; return z
        self.a += self.dt * self.v
        p00 = self.p00 + self.dt*(self.p10+self.p01+self.dt*self.p11) + self.q0
        p01 = self.p01 + self.dt*self.p11
        p10 = self.p10 + self.dt*self.p11
        p11 = self.p11 + self.q1
        denom = p00 + self.r
        k0 = p00 / denom; k1 = p10 / denom
        inn = z - self.a
        self.a += k0 * inn; self.v += k1 * inn
        self.p00 = p00 - k0*p00; self.p01 = p01 - k0*p01
        self.p10 = p10 - k1*p00; self.p11 = p11 - k1*p01
        return self.a


def ewma(vals, alpha=0.3):
    out = np.full_like(vals, np.nan)
    prev = np.nan
    for i, v in enumerate(vals):
        if not np.isfinite(v):
            continue
        prev = v if not np.isfinite(prev) else alpha*v + (1-alpha)*prev
        out[i] = prev
    return out


def median5(vals):
    out = np.full_like(vals, np.nan)
    n = len(vals)
    for i in range(n):
        lo = max(0, i-2); hi = min(n, i+3)
        chunk = vals[lo:hi]
        ok = chunk[np.isfinite(chunk)]
        out[i] = float(np.median(ok)) if len(ok) else np.nan
    return out


def smoothness(ang, t_ms):
    s = 0.0; c = 0
    for i in range(1, len(ang)):
        if np.isfinite(ang[i]) and np.isfinite(ang[i-1]):
            dt = max(1e-6, (t_ms[i]-t_ms[i-1])/1000.0)
            s += abs((ang[i]-ang[i-1]) / dt); c += 1
    return s / c if c else float('inf')


def outliers(ang, thresh=45.0):
    c = 0
    for i in range(1, len(ang)):
        if np.isfinite(ang[i]) and np.isfinite(ang[i-1]) and abs(ang[i]-ang[i-1]) > thresh:
            c += 1
    return c


class Ev:
    def __init__(self, ms, kind, side, angle=None):
        self.ms = ms; self.kind = kind; self.side = side; self.angle = angle


def detect_events(f):
    evs = []
    state = STATE_ABSENT
    for fr in f:
        new = fr.state
        if new == state:
            continue
        if new == STATE_BOTH or new == STATE_LEFT or new == STATE_RIGHT:
            if state == STATE_ABSENT:
                side = ("left" if (fr.angle_fallback or 0) < -PRESENCE_ZONE else
                        "right" if (fr.angle_fallback or 0) > PRESENCE_ZONE else "center")
                evs.append(Ev(fr.ms, "enter", side, fr.angle_fallback))
            elif state != STATE_ABSENT:
                if (state == STATE_LEFT or state == STATE_BOTH) and new == STATE_RIGHT:
                    evs.append(Ev(fr.ms, "cross", "left_to_right", fr.angle_fallback))
                elif (state == STATE_RIGHT or state == STATE_BOTH) and new == STATE_LEFT:
                    evs.append(Ev(fr.ms, "cross", "right_to_left", fr.angle_fallback))
        elif new == STATE_ABSENT:
            if state != STATE_ABSENT:
                side = ("left" if (fr.angle_fallback or 0) < -PRESENCE_ZONE else
                        "right" if (fr.angle_fallback or 0) > PRESENCE_ZONE else "center")
                evs.append(Ev(fr.ms, "exit", side, fr.angle_fallback))
        state = new
    return evs


def plot(f, fname):
    if not f:
        return None
    t0 = f[0].ms
    t = np.array([(fr.ms - t0)/1000.0 for fr in f])
    dL = np.array([fr.dL for fr in f])
    dR = np.array([fr.dR for fr in f])
    ang_exact = np.array([fr.angle_exact if fr.angle_exact is not None else np.nan for fr in f])
    ang_fb    = np.array([fr.angle_fallback for fr in f])
    mask_both = np.array([fr.state == STATE_BOTH for fr in f])
    ang_med   = median5(np.where(mask_both, ang_exact, np.nan))
    ang_ewma  = ewma(np.where(mask_both, ang_exact, np.nan), alpha=0.3)
    kf = Kalman2D(); ang_kf = np.full_like(ang_fb, np.nan)
    for i, fr in enumerate(f):
        if fr.state == STATE_BOTH and np.isfinite(ang_exact[i]):
            ang_kf[i] = kf.update(ang_exact[i])
        elif fr.state in (STATE_LEFT, STATE_RIGHT):
            ang_kf[i] = kf.update(ang_fb[i])
    events = detect_events(f)

    fig, axes = plt.subplots(5, 1, figsize=(13, 14), sharex=True,
                             gridspec_kw={"height_ratios": [1, 1, 1, 1, 0.5]})
    fig.suptitle(f"Experiment: {fname}", fontsize=13, fontweight="bold")

    ax = axes[0]
    ax.plot(t, dL, "b-", alpha=0.6, label="Left")
    ax.plot(t, dR, "r-", alpha=0.6, label="Right")
    ax.axhline(NO_ECHO_FLOOR, color="grey", ls="--", alpha=0.5, label="No-echo floor")
    absent = np.array([fr.state == STATE_ABSENT for fr in f])
    ax.fill_between(t, 0, 1000, where=absent, color="lightgray", alpha=0.3, label="Absent")
    ax.set_ylim(0, 900); ax.set_ylabel("Distance (cm)")
    ax.legend(loc="upper right", fontsize=8); ax.set_title("Raw Sensor Readings")

    ax = axes[1]
    ax.plot(t, ang_fb,    "k-", alpha=0.2, lw=0.8, label="Fallback (raw)")
    ax.plot(t, ang_med,   "g-", lw=1.5, label="Median (w=5)")
    ax.plot(t, ang_ewma,  "m-", lw=1.5, label="EWMA")
    ax.plot(t, ang_kf,    "c-", lw=2.2, label="Kalman")
    ax.axhline(0, color="black", lw=0.5)
    ax.axhline(PRESENCE_ZONE,  color="orange", ls="--", alpha=0.5)
    ax.axhline(-PRESENCE_ZONE, color="orange", ls="--", alpha=0.5)
    ax.set_ylabel("Angle (deg)"); ax.set_ylim(-95, 95)
    ax.legend(loc="upper right", fontsize=8); ax.set_title("Angle Estimates")

    ax = axes[2]
    vel = np.full_like(ang_kf, np.nan)
    for i in range(1, len(ang_kf)):
        if np.isfinite(ang_kf[i]) and np.isfinite(ang_kf[i-1]):
            dt = max(1e-6, (f[i].ms-f[i-1].ms)/1000.0)
            vel[i] = (ang_kf[i] - ang_kf[i-1]) / dt
    ax.plot(t, vel, "b-", lw=1.0, alpha=0.7)
    ax.axhline(0, color="black", lw=0.5)
    ax.set_ylabel("d_a/dt (deg/s)"); ax.set_title("Angular Velocity (Kalman)")

    ax = axes[3]
    state_map = {STATE_ABSENT: 0, STATE_LEFT: -1, STATE_BOTH: 0.5, STATE_RIGHT: 1}
    svals = np.array([state_map.get(fr.state, 0) for fr in f])
    ax.step(t, svals, where="post", color="purple", lw=1.5)
    ax.set_yticks([-1, 0, 0.5, 1])
    ax.set_yticklabels(["L-only", "Absent", "Both", "R-only"])
    ax.set_ylabel("Sensor state"); ax.set_title("Sensor State Machine")

    ax = axes[4]
    for ev in events:
        tx = (ev.ms - t0) / 1000.0
        color = {"enter": "green", "exit": "red", "cross": "blue"}.get(ev.kind, "gray")
        ax.axvline(tx, color=color, alpha=0.6, lw=1.5)
        label = f"{ev.kind}\n{ev.side}"
        ax.text(tx, 0.5, label, rotation=90, fontsize=5, color=color, ha="center", va="center")
    ax.set_ylim(0, 1); ax.set_yticks([])
    ax.set_xlabel("Time (s)"); ax.set_title("Detected Events")

    plt.tight_layout(rect=[0, 0.03, 1, 0.96])
    p = OUT_DIR / f"{fname}.png"
    fig.savefig(p, dpi=150); plt.close(fig)
    return p


def main():
    files = sorted(DATA_DIR.glob("exp_*.txt"))
    print(f"Analyzing {len(files)} files ...\n")
    rows = []
    for fp in files:
        raw = load(fp)
        f = build_frames(raw)
        if not f:
            print(f"  {fp.name}: no frames", flush=True); continue
        print(f"  {fp.name}: {len(f):>3} frames ... ", end="", flush=True)
        plot(f, fp.stem)
        t_ms = np.array([fr.ms for fr in f])
        mask_both = np.array([fr.state == STATE_BOTH for fr in f])
        ang_exact = np.array([fr.angle_exact if fr.angle_exact is not None else np.nan for fr in f])
        ang_fb    = np.array([fr.angle_fallback for fr in f])
        ang_med   = median5(np.where(mask_both, ang_exact, np.nan))
        ang_ewma  = ewma(np.where(mask_both, ang_exact, np.nan), 0.3)
        kf = Kalman2D(); ang_kf = np.full_like(ang_fb, np.nan)
        for i, fr in enumerate(f):
            if fr.state == STATE_BOTH and np.isfinite(ang_exact[i]):
                ang_kf[i] = kf.update(ang_exact[i])
            elif fr.state in (STATE_LEFT, STATE_RIGHT):
                ang_kf[i] = kf.update(ang_fb[i])
        evs = detect_events(f)
        rows.append({
            "file": fp.name, "n": len(f), "both_n": int(np.sum(mask_both)), "events": len(evs),
            "smooth_fb": round(smoothness(ang_fb, t_ms), 1),
            "smooth_med": round(smoothness(ang_med, t_ms), 1) if np.any(np.isfinite(ang_med)) else "-",
            "smooth_ewma": round(smoothness(ang_ewma, t_ms), 1) if np.any(np.isfinite(ang_ewma)) else "-",
            "smooth_kf": round(smoothness(ang_kf, t_ms), 1) if np.any(np.isfinite(ang_kf)) else "-",
            "out_fb": outliers(ang_fb), "out_med": outliers(ang_med),
            "out_ewma": outliers(ang_ewma), "out_kf": outliers(ang_kf),
        })
        print("done")

    rpt = OUT_DIR / "ANALYSIS_REPORT_V2.md"
    with open(rpt, "w") as fh:
        fh.write("# Sonar Analysis Report V2\n\n")
        fh.write("## Quantitative comparison (lower smoothness = smoother)\n\n")
        fh.write("| File | Frames | BOTH_ok | Events | Smooth Fallback | Median | EWMA | Kalman | Outliers FB | Med | EWA | KF |\n")
        fh.write("|------|--------|---------|--------|-----------------|--------|------|--------|-------------|-----|-----|----|\n")
        for r in rows:
            fh.write(f"| {r['file'][:40]:<40} | {r['n']:>6} | {r['both_n']:>7} | {r['events']:>6} | "
                     f"{r['smooth_fb']:>15} | {r['smooth_med']:>6} | {r['smooth_ewma']:>4} | {r['smooth_kf']:>6} | "
                     f"{r['out_fb']:>11} | {r['out_med']:>3} | {r['out_ewma']:>3} | {r['out_kf']:>2} |\n")
    print(f"\nReport -> {rpt}")

if __name__ == "__main__":
    main()
