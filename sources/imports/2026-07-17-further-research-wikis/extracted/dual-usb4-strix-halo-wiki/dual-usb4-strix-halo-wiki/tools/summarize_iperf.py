#!/usr/bin/env python3
"""Summarize isolated and concurrent iperf3 JSON files."""
from __future__ import annotations
import json
import sys
from pathlib import Path


def bps(path: Path) -> float | None:
    if not path.exists():
        return None
    data = json.loads(path.read_text())
    end = data.get("end", {})
    for key in ("sum_received", "sum_sent", "sum"):
        value = end.get(key, {}).get("bits_per_second")
        if value is not None:
            return float(value)
    return None


def fmt(value: float | None) -> str:
    return "n/a" if value is None else f"{value / 1e9:.3f} Gbit/s"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: summarize_iperf.py RESULT_DIR", file=sys.stderr)
        return 2
    d = Path(sys.argv[1])
    b0 = bps(d / "baseline-link0.json")
    b1 = bps(d / "baseline-link1.json")
    d0 = bps(d / "dual-link0.json")
    d1 = bps(d / "dual-link1.json")
    agg = None if d0 is None or d1 is None else d0 + d1
    best = max(v for v in (b0, b1) if v is not None) if any(v is not None for v in (b0, b1)) else None
    ratio = agg / best if agg is not None and best else None
    r0 = d0 / b0 if d0 is not None and b0 else None
    r1 = d1 / b1 if d1 is not None and b1 else None
    heuristic = bool(ratio is not None and ratio >= 1.7 and r0 is not None and r0 >= .8 and r1 is not None and r1 >= .8)

    print("# Dual-link iperf3 summary\n")
    print("| Measurement | Result |")
    print("|---|---:|")
    print(f"| Isolated link 0 | {fmt(b0)} |")
    print(f"| Isolated link 1 | {fmt(b1)} |")
    print(f"| Concurrent link 0 | {fmt(d0)} |")
    print(f"| Concurrent link 1 | {fmt(d1)} |")
    print(f"| Concurrent aggregate | {fmt(agg)} |")
    print(f"| Aggregate / best isolated | {'n/a' if ratio is None else f'{ratio:.3f}×'} |")
    print(f"| Link 0 retention | {'n/a' if r0 is None else f'{r0:.1%}'} |")
    print(f"| Link 1 retention | {'n/a' if r1 is None else f'{r1:.1%}'} |")
    print(f"| Project 1.7× / 80% heuristic | {'PASS' if heuristic else 'NOT PROVEN'} |")
    print("\nThe heuristic is an engineering threshold, not a protocol or hardware standard.")

    payload = {
        "baseline_link0_bps": b0,
        "baseline_link1_bps": b1,
        "concurrent_link0_bps": d0,
        "concurrent_link1_bps": d1,
        "concurrent_aggregate_bps": agg,
        "aggregate_to_best_ratio": ratio,
        "link0_retention": r0,
        "link1_retention": r1,
        "heuristic_pass": heuristic,
    }
    (d / "summary.json").write_text(json.dumps(payload, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
