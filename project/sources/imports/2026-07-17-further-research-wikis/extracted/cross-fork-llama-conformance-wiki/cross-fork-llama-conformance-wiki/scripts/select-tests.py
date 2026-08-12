#!/usr/bin/env python3
from __future__ import annotations
import argparse, json
from pathlib import Path

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--matrix", type=Path, default=Path("matrix/test-matrix.json"))
    ap.add_argument("--area", action="append", default=[])
    ap.add_argument("--tier", action="append", default=[])
    ap.add_argument("--fork", choices=["upstream","rocmfpx","cachyllama","integration"])
    ap.add_argument("--backend")
    ap.add_argument("--include-failure-injection", action="store_true")
    ap.add_argument("--json", action="store_true")
    ns = ap.parse_args()
    matrix = json.loads(ns.matrix.read_text(encoding="utf-8"))
    selected = []
    for case in matrix["cases"]:
        if ns.area and case["area"] not in ns.area:
            continue
        if ns.tier and case["ci_tier"] not in ns.tier:
            continue
        if ns.fork and case["applicability"][ns.fork] == "not-applicable":
            continue
        if ns.backend and ns.backend not in case["backend_scope"] and "gpu" not in case["backend_scope"]:
            continue
        if case["failure_injection"] and not ns.include_failure_injection:
            continue
        selected.append(case)
    if ns.json:
        print(json.dumps(selected, indent=2))
    else:
        for case in selected:
            print(case["id"])
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
