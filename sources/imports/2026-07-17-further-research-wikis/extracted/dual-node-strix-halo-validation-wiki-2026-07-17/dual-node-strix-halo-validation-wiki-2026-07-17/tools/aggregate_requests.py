#!/usr/bin/env python3
"""Aggregate raw request/token JSONL into client-observed latency and throughput metrics."""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import random
import statistics
import sys
from collections import defaultdict
from pathlib import Path
from typing import Iterable


def load_jsonl(path: Path) -> list[dict]:
    rows: list[dict] = []
    with path.open(encoding="utf-8") as fh:
        for lineno, line in enumerate(fh, 1):
            if not line.strip():
                continue
            try:
                rows.append(json.loads(line))
            except json.JSONDecodeError as exc:
                raise ValueError(f"{path}:{lineno}: {exc}") from exc
    return rows


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def percentile(values: list[float], q: float) -> float | None:
    if not values:
        return None
    xs = sorted(values)
    if len(xs) == 1:
        return xs[0]
    pos = (len(xs) - 1) * q
    lo, hi = math.floor(pos), math.ceil(pos)
    if lo == hi:
        return xs[lo]
    return xs[lo] + (xs[hi] - xs[lo]) * (pos - lo)


def bootstrap_ci(values: list[float], statistic: str, samples: int = 2000, seed: int = 20260717) -> list[float] | None:
    if len(values) < 2:
        return None
    rng = random.Random(seed)
    fn = statistics.mean if statistic == "mean" else statistics.median
    n = len(values)
    estimates = [fn(rng.choices(values, k=n)) for _ in range(samples)]
    return [percentile(estimates, 0.025), percentile(estimates, 0.975)]  # type: ignore[list-item]


def stats(values: Iterable[float]) -> dict:
    xs = [float(v) for v in values if v is not None and math.isfinite(float(v))]
    if not xs:
        return {"count": 0, "mean": None, "median": None, "stdev": None, "mad": None, "p90": None, "p95": None, "p99": None, "min": None, "max": None, "mean_ci95": None, "median_ci95": None}
    med = statistics.median(xs)
    return {
        "count": len(xs),
        "mean": statistics.mean(xs),
        "median": med,
        "stdev": statistics.stdev(xs) if len(xs) > 1 else 0.0,
        "mad": statistics.median([abs(x - med) for x in xs]),
        "p90": percentile(xs, 0.90),
        "p95": percentile(xs, 0.95),
        "p99": percentile(xs, 0.99),
        "min": min(xs),
        "max": max(xs),
        "mean_ci95": bootstrap_ci(xs, "mean"),
        "median_ci95": bootstrap_ci(xs, "median"),
    }


def aggregate(requests: list[dict], token_rows: list[dict]) -> dict:
    if not requests:
        raise ValueError("No request records")
    run_ids = {r.get("run_id") for r in requests}
    if len(run_ids) != 1:
        raise ValueError(f"Expected one run_id, found {sorted(run_ids)}")
    run_id = next(iter(run_ids))

    token_times: dict[str, list[int]] = defaultdict(list)
    for row in token_rows:
        if row.get("run_id") != run_id or not row.get("is_content_token", False):
            continue
        token_times[row["request_id"]].append(int(row["client_event_monotonic_ns"]))
    for rid in token_times:
        token_times[rid].sort()

    ttft_ms: list[float] = []
    e2e_ms: list[float] = []
    tpot_ms: list[float] = []
    decode_tps: list[float] = []
    itl_ms: list[float] = []
    successful = 0
    total_prompt = total_output = 0
    eligible = cached = 0
    starts: list[int] = []
    ends: list[int] = []
    invalid_timing: list[str] = []

    for r in requests:
        send = int(r["client_send_monotonic_ns"])
        complete = int(r["client_complete_monotonic_ns"])
        first = r.get("client_first_token_monotonic_ns")
        last = r.get("client_last_token_monotonic_ns")
        rid = r["request_id"]
        starts.append(send)
        ends.append(complete)
        total_prompt += int(r.get("prompt_tokens") or 0)
        total_output += int(r.get("output_tokens") or 0)
        eligible += int(r.get("eligible_prefix_tokens") or 0)
        cached += int(r.get("cached_prompt_tokens") or 0)
        if r.get("success"):
            successful += 1
        if complete < send:
            invalid_timing.append(f"{rid}: complete before send")
            continue
        e2e_ms.append((complete - send) / 1e6)
        if first is not None:
            first = int(first)
            if first < send:
                invalid_timing.append(f"{rid}: first token before send")
            else:
                ttft_ms.append((first - send) / 1e6)
        if last is not None and first is not None:
            last = int(last)
            if last < first:
                invalid_timing.append(f"{rid}: last token before first")
            elif int(r.get("output_tokens") or 0) >= 2 and last > first:
                denom = int(r["output_tokens"]) - 1
                value_ms = (last - first) / 1e6 / denom
                tpot_ms.append(value_ms)
                decode_tps.append(1000.0 / value_ms)
        times = token_times.get(rid, [])
        for left, right in zip(times, times[1:]):
            if right < left:
                invalid_timing.append(f"{rid}: nonmonotonic token event")
            else:
                itl_ms.append((right - left) / 1e6)

    duration_s = (max(ends) - min(starts)) / 1e9 if ends and max(ends) > min(starts) else None
    success_rate = successful / len(requests)
    result = {
        "schema_version": 1,
        "derived_record_type": "request_aggregate",
        "run_id": run_id,
        "request_count": len(requests),
        "successful_requests": successful,
        "request_success_rate": success_rate,
        "measurement_span_s": duration_s,
        "request_throughput_per_s": (successful / duration_s) if duration_s else None,
        "prompt_tokens": total_prompt,
        "output_tokens": total_output,
        "aggregate_prompt_tokens_per_s": (total_prompt / duration_s) if duration_s else None,
        "aggregate_output_tokens_per_s": (total_output / duration_s) if duration_s else None,
        "ttft_ms": stats(ttft_ms),
        "itl_ms": stats(itl_ms),
        "tpot_ms": stats(tpot_ms),
        "decode_tokens_per_s_per_request": stats(decode_tps),
        "e2e_ms": stats(e2e_ms),
        "eligible_prefix_tokens": eligible,
        "cached_prompt_tokens": cached,
        "prefix_token_hit_rate": (cached / eligible) if eligible else None,
        "invalid_timing_records": invalid_timing,
        "interpretation": {
            "ttft": "client send to first non-empty content event",
            "itl": "between consecutive recorded content events; verify one event per token for true token ITL",
            "throughput": "successful completions/tokens over first-submit to last-complete span",
        },
    }
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--requests", type=Path, required=True)
    parser.add_argument("--tokens", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        request_rows = load_jsonl(args.requests)
        token_rows = load_jsonl(args.tokens)
        result = aggregate(request_rows, token_rows)
        result["source_files"] = [
            {"path": str(args.requests), "sha256": sha256_file(args.requests)},
            {"path": str(args.tokens), "sha256": sha256_file(args.tokens)},
        ]
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    rendered = json.dumps(result, indent=2)
    print(rendered)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    return 1 if result["invalid_timing_records"] else 0


if __name__ == "__main__":
    sys.exit(main())
