#!/usr/bin/env python3
"""
Data validation script for sonar project experiment files.

Parses all .txt files in the data directory and extracts the 3 important columns:
  1. milliseconds since start
  2. left sensor measurement (cm)
  3. right sensor measurement (cm)

The 4th column (angle) is ignored because it is the baseline quantity we are trying to improve.

Special handling rules
----------------------
1. First and last lines of every file are considered potentially incomplete and are
   ignored in the valid/failure statistics. They are still parsed for reference.
2. Distance readings > 400 cm are treated as unreliable (likely far wall or no echo).
   These lines are flagged but retained so their pattern can be inspected.
3. Corrupted / partial lines that appear in the middle of a file are marked as
   ``failed``. Their timestamp is estimated by averaging the nearest surrounding
   valid timestamps so that the sequence stays continuous.

Sensor geometry
---------------
- Horizontal sensor separation: 12 cm.
"""
import os
import sys
from pathlib import Path

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
EXPECTED_COLUMNS = 4  # millis, left, right, angle
MAX_RELIABLE_CM = 400.0


def _strip_newline(line: str) -> str:
    # Files appear to contain CR+LF (Windows-style); strip both.
    return line.rstrip("\n").rstrip("\r")


def parse_line(raw_line: str):
    """
    Attempt to split a CSV line into the four expected columns.

    Returns
    -------
    tuple | None, str | None
        On success: (millis, left, right), None
        On failure: None, error_reason
    """
    line = _strip_newline(raw_line).strip()
    if not line:
        return None, "empty line"

    parts = line.split(",")
    if len(parts) < EXPECTED_COLUMNS:
        return None, f"only {len(parts)} column(s)"
    if len(parts) != EXPECTED_COLUMNS:
        return None, f"unexpected column count ({len(parts)})"

    try:
        millis = float(parts[0].strip())
        left = float(parts[1].strip())
        right = float(parts[2].strip())
    except ValueError:
        return None, "non-numeric value"

    return (millis, left, right), None


def read_and_validate_file(filepath: Path):
    """
    Read a single experiment file and build a record for every line.

    First and last lines are marked ``is_border=True`` and excluded from
    valid/fail aggregates.  Failed interior lines receive an interpolated
    timestamp in a second pass.
    """
    with open(filepath, "r", encoding="utf-8") as f:
        raw_lines = [_strip_newline(line) for line in f]

    total_lines = len(raw_lines)
    records = []

    for line_no, raw in enumerate(raw_lines, start=1):
        is_border = line_no == 1 or line_no == total_lines
        parsed, error = parse_line(raw)

        record = {
            "line_no": line_no,
            "raw": raw,
            "is_border": is_border,
            "valid_interior": False,
            "error": error,
            "millis": parsed[0] if parsed else None,
            "left": parsed[1] if parsed else None,
            "right": parsed[2] if parsed else None,
            "unreliable": False,
            "millis_est": None,
        }

        # Unreliable readings: > 400 cm (either sensor)
        if parsed and (parsed[1] > MAX_RELIABLE_CM or parsed[2] > MAX_RELIABLE_CM):
            record["unreliable"] = True

        # Interior lines: valid only when parsed AND not border
        if not is_border and error is None:
            record["valid_interior"] = True

        records.append(record)

    # Second pass: estimate timestamps for every failed interior row.
    n = len(records)
    for i, r in enumerate(records):
        if r["valid_interior"]:
            r["millis_est"] = r["millis"]
            continue

        prev_millis = None
        for j in range(i - 1, -1, -1):
            if records[j]["valid_interior"]:
                prev_millis = records[j]["millis"]
                break

        next_millis = None
        for j in range(i + 1, n):
            if records[j]["valid_interior"]:
                next_millis = records[j]["millis"]
                break

        if prev_millis is not None and next_millis is not None:
            r["millis_est"] = (prev_millis + next_millis) / 2.0
        elif prev_millis is not None:
            r["millis_est"] = prev_millis
        elif next_millis is not None:
            r["millis_est"] = next_millis
        else:
            r["millis_est"] = None

    interior_records = [r for r in records if not r["is_border"]]
    valid_interior = [r for r in interior_records if r["valid_interior"]]
    failed_interior = [r for r in interior_records if not r["valid_interior"]]
    unreliable_interior = [r for r in valid_interior if r["unreliable"]]

    return {
        "path": filepath,
        "total_lines": total_lines,
        "ignored_border": 2 if total_lines >= 2 else total_lines,
        "valid_interior": len(valid_interior),
        "failed_interior": len(failed_interior),
        "unreliable_interior": len(unreliable_interior),
        "records": records,
        "valid_interior_records": valid_interior,
        "failed_interior_records": failed_interior,
    }


def print_summary(results: list):
    """Print a formatted summary table."""
    header = (
        f"{'File':<45} {'Total':>6} {'Ign':>4} {'Valid':>6} {'Fail':>5} "
        f"{'Fail%':>6} {'Unrbl':>6} {'Millis Range (valid)':>24}"
    )
    print("=" * len(header))
    print(header)
    print("-" * len(header))

    total_all = valid_all = failed_all = ignored_all = unreliable_all = 0

    for r in results:
        recs = r["valid_interior_records"]
        total = r["total_lines"]
        ignored = r["ignored_border"]
        valid = r["valid_interior"]
        failed = r["failed_interior"]
        unreliable = r["unreliable_interior"]
        fail_pct = (failed / (valid + failed) * 100) if (valid + failed) else 0.0

        total_all += total
        valid_all += valid
        failed_all += failed
        ignored_all += ignored
        unreliable_all += unreliable

        if recs:
            millis_min = min(x["millis"] for x in recs)
            millis_max = max(x["millis"] for x in recs)
            range_str = f"{millis_min:,.0f} -- {millis_max:,.0f}"
        else:
            range_str = "N/A"

        print(
            f"{r['path'].name:<45} {total:>6} {ignored:>4} {valid:>6} {failed:>5} "
            f"{fail_pct:>5.1f}% {unreliable:>6} {range_str:>24}"
        )

        if recs:
            left_min = min(x["left"] for x in recs)
            left_max = max(x["left"] for x in recs)
            right_min = min(x["right"] for x in recs)
            right_max = max(x["right"] for x in recs)
            print(
                f"{'':<45} "
                f"L={left_min:.1f}..{left_max:.1f}  R={right_min:.1f}..{right_max:.1f}"
            )

    print("-" * len(header))
    fail_pct_all = (
        (failed_all / (valid_all + failed_all) * 100) if (valid_all + failed_all) else 0.0
    )
    print(
        f"{'TOTAL':<45} {total_all:>6} {ignored_all:>4} {valid_all:>6} {failed_all:>5} "
        f"{fail_pct_all:>5.1f}% {unreliable_all:>6}"
    )
    print("=" * len(header))

    # Failed-line samples
    print("\nFailed interior-line samples (first 3 per file, with estimated timestamp):")
    for r in results:
        fails = r["failed_interior_records"][:3]
        if not fails:
            continue
        print(f"  {r['path'].name}:")
        for f in fails:
            est = f"{f['millis_est']:,.0f}" if f["millis_est"] is not None else "N/A"
            print(
                f"      line {f['line_no']:>4}: est_ts={est:<12} "
                f"reason={f['error']:<25} raw=\"{f['raw']}\""
            )

    # Global failure reasons
    global_reasons = {}
    for r in results:
        for f in r["failed_interior_records"]:
            reason = f["error"]
            global_reasons[reason] = global_reasons.get(reason, 0) + 1
    if global_reasons:
        print("\nGlobal failure reasons (interior lines only):")
        for reason, count in sorted(global_reasons.items(), key=lambda kv: -kv[1]):
            print(f"  - {reason}: {count}")


def main():
    if not DATA_DIR.exists():
        print(f"ERROR: Data directory not found: {DATA_DIR}")
        sys.exit(1)

    files = sorted(DATA_DIR.glob("exp_*.txt"))
    if not files:
        print(f"WARNING: No experiment files (exp_*.txt) found in {DATA_DIR}")
        sys.exit(0)

    print(f"Found {len(files)} experiment file(s) in {DATA_DIR}\n")

    results = [read_and_validate_file(fp) for fp in files]
    print_summary(results)

    bad_files = [r["path"].name for r in results if r["valid_interior"] == 0]
    if bad_files:
        print(f"\nERROR: {len(bad_files)} file(s) contain no valid interior records:")
        for name in bad_files:
            print(f"  - {name}")
        sys.exit(1)

    print("\nAll files validated successfully.")


if __name__ == "__main__":
    main()
