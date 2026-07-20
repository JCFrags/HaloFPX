#!/usr/bin/env python3
"""Summarize retained exact-primary-model server blocks without dependencies."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import statistics
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("--control", action="append", required=True)
    parser.add_argument("--candidate", action="append", required=True)
    parser.add_argument("--expected-content-sha256", required=True)
    parser.add_argument("--expected-retained-per-block", type=int, default=5)
    return parser.parse_args()


def load_variant(root: Path, blocks: list[str], expected_hash: str, expected_count: int) -> dict:
    samples = []
    block_summaries = []
    for block in blocks:
        block_samples = []
        response_paths = sorted(
            (root / block).glob("retained-*.json"),
            key=lambda path: int(path.stem.removeprefix("retained-")),
        )
        for response_path in response_paths:
            index = response_path.stem.removeprefix("retained-")
            response = json.loads(response_path.read_text(encoding="utf-8"))
            timing = response["timings"]
            content_hash = hashlib.sha256(response["content"].encode("utf-8")).hexdigest()
            if content_hash != expected_hash:
                raise ValueError(f"unexpected content hash in {response_path}: {content_hash}")
            if timing["prompt_n"] != 1129 or timing["predicted_n"] != 128:
                raise ValueError(f"unexpected token counts in {response_path}")
            curl_fields = (root / block / f"retained-{index}.curl").read_text(encoding="utf-8").split()
            if curl_fields[0] != "200":
                raise ValueError(f"non-200 HTTP result for {response_path}")
            sample = {
                "block": block,
                "index": int(index),
                "prompt_tokens_per_second": float(timing["prompt_per_second"]),
                "generation_tokens_per_second": float(timing["predicted_per_second"]),
                "end_to_end_milliseconds": float(curl_fields[1]) * 1000.0,
                "content_sha256": content_hash,
            }
            block_samples.append(sample)
            samples.append(sample)
        if not block_samples:
            raise ValueError(f"no retained samples in {root / block}")
        if len(block_samples) != expected_count:
            raise ValueError(
                f"expected {expected_count} retained samples in {root / block}, "
                f"found {len(block_samples)}"
            )
        block_summaries.append({
            "block": block,
            "n": len(block_samples),
            "prompt_mean": statistics.mean(s["prompt_tokens_per_second"] for s in block_samples),
            "generation_mean": statistics.mean(s["generation_tokens_per_second"] for s in block_samples),
            "end_to_end_mean_ms": statistics.mean(s["end_to_end_milliseconds"] for s in block_samples),
        })
    return {"blocks": block_summaries, "samples": samples}


def describe(samples: list[dict], key: str) -> dict:
    values = [sample[key] for sample in samples]
    return {
        "n": len(values),
        "mean": statistics.mean(values),
        "sample_sd": statistics.stdev(values),
        "minimum": min(values),
        "maximum": max(values),
        "raw": values,
    }


def compare(control: dict, candidate: dict, key: str) -> dict:
    c = describe(control["samples"], key)
    a = describe(candidate["samples"], key)
    delta = a["mean"] - c["mean"]
    standard_error = math.sqrt(c["sample_sd"] ** 2 / c["n"] + a["sample_sd"] ** 2 / a["n"])
    return {
        "control": c,
        "candidate": a,
        "candidate_minus_control": delta,
        "candidate_delta_percent": delta / c["mean"] * 100.0,
        "approximate_normal_ci95": [delta - 1.96 * standard_error, delta + 1.96 * standard_error],
    }


def main() -> None:
    args = parse_args()
    if args.expected_retained_per_block < 2:
        raise ValueError("expected retained samples per block must be at least 2")
    control = load_variant(
        args.root,
        args.control,
        args.expected_content_sha256,
        args.expected_retained_per_block,
    )
    candidate = load_variant(
        args.root,
        args.candidate,
        args.expected_content_sha256,
        args.expected_retained_per_block,
    )
    result = {
        "schema": "halofpx.primary-block-analysis.v1",
        "root": str(args.root),
        "control_blocks": control["blocks"],
        "candidate_blocks": candidate["blocks"],
        "metrics": {
            key: compare(control, candidate, key)
            for key in (
                "prompt_tokens_per_second",
                "generation_tokens_per_second",
                "end_to_end_milliseconds",
            )
        },
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
