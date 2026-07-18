#!/usr/bin/env python3
"""Elementwise numerical comparator for JSON numeric arrays.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import argparse, json, math
from pathlib import Path
from typing import Any


def flatten(x: Any):
    if isinstance(x, list):
        for item in x: yield from flatten(item)
    elif isinstance(x, (int, float)) and not isinstance(x, bool):
        yield float(x)
    else:
        raise TypeError(f"non-numeric value {x!r}")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("expected", type=Path); ap.add_argument("actual", type=Path)
    ap.add_argument("--atol", type=float, default=1e-5); ap.add_argument("--rtol", type=float, default=0.0)
    ns = ap.parse_args()
    a = list(flatten(json.loads(ns.expected.read_text()))); b = list(flatten(json.loads(ns.actual.read_text())))
    if len(a) != len(b):
        print(json.dumps({"match": False, "reason": "length", "expected": len(a), "actual": len(b)})); return 1
    worst = {"index": None, "abs_error": -1.0, "expected": None, "actual": None}
    ok = True
    for i, (x, y) in enumerate(zip(a, b)):
        good = math.isfinite(x) and math.isfinite(y) and abs(x-y) <= ns.atol + ns.rtol*abs(x)
        err = abs(x-y)
        if err > worst["abs_error"]: worst = {"index": i, "abs_error": err, "expected": x, "actual": y}
        ok &= good
    print(json.dumps({"match": ok, "count": len(a), "atol": ns.atol, "rtol": ns.rtol, "worst": worst}, sort_keys=True))
    return 0 if ok else 1
if __name__ == "__main__": raise SystemExit(main())
