#!/usr/bin/env python3
"""Fit t(V) = ell + V/B to measured application payload observations.

Input CSV columns:
  payload_bytes, elapsed_s
Optional columns are ignored. With --rtt, elapsed_s is divided by two, which is
only a symmetric ping-pong approximation and is labeled as such in the output.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path


def fit(xs: list[float], ys: list[float]) -> dict[str, float | int | str | bool]:
    if len(xs) < 3:
        raise ValueError("at least three observations are required")
    xbar = sum(xs) / len(xs)
    ybar = sum(ys) / len(ys)
    sxx = sum((x - xbar) ** 2 for x in xs)
    if sxx <= 0:
        raise ValueError("payload sizes must vary")
    slope = sum((x - xbar) * (y - ybar) for x, y in zip(xs, ys)) / sxx
    intercept = ybar - slope * xbar
    if slope <= 0:
        raise ValueError("non-positive fitted slope; select a valid size region")
    predictions = [intercept + slope * x for x in xs]
    residuals = [y - p for y, p in zip(ys, predictions)]
    sse = sum(r * r for r in residuals)
    sst = sum((y - ybar) ** 2 for y in ys)
    r2 = 1.0 - sse / sst if sst > 0 else 1.0
    rmse = math.sqrt(sse / len(xs))
    return {
        "evidence_label": "MEASURED INPUT DERIVED BY FIT",
        "observations": len(xs),
        "one_way_fixed_cost_s": intercept,
        "effective_payload_bandwidth_Bps": 1.0 / slope,
        "effective_payload_bandwidth_GBps_decimal": 1.0 / slope / 1e9,
        "r_squared": r2,
        "rmse_s": rmse,
        "negative_intercept_warning": intercept < 0,
    }


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", type=Path)
    p.add_argument("--output", type=Path)
    p.add_argument("--rtt", action="store_true", help="divide elapsed_s by two; symmetric ping-pong approximation")
    p.add_argument("--min-bytes", type=float, default=0)
    p.add_argument("--max-bytes", type=float, default=float("inf"))
    return p


def main() -> int:
    args = parser().parse_args()
    xs: list[float] = []
    ys: list[float] = []
    with args.input.open(newline="", encoding="utf-8") as handle:
        for row in csv.DictReader(handle):
            x = float(row["payload_bytes"])
            if not args.min_bytes <= x <= args.max_bytes:
                continue
            y = float(row["elapsed_s"])
            if args.rtt:
                y /= 2.0
            if x < 0 or y <= 0:
                raise ValueError("payload_bytes >= 0 and elapsed_s > 0 required")
            xs.append(x)
            ys.append(y)
    result = fit(xs, ys)
    result["input"] = str(args.input)
    result["input_time_semantics"] = (
        "RTT divided by two; symmetric-path approximation" if args.rtt else "one-way application elapsed time"
    )
    result["size_region_bytes"] = [args.min_bytes, None if math.isinf(args.max_bytes) else args.max_bytes]
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
