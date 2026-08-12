#!/usr/bin/env python3
"""Recalculate KV-cache and per-node capacity tables.

The script is self-contained and uses only package data.  It regenerates:
  * data/kv_cache.csv and .json
  * data/capacity_budgets.csv and .json
  * data/capacity_summary.csv and .json
  * pages/Per-Node-Budgets.md

All capacity arithmetic is exact under the assumptions declared in
pages/Methodology.md and data/profiles.json.  It is not a replacement for
measured llama.cpp allocator output on the target build and hardware.
"""
from __future__ import annotations

import csv
import json
import math
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CANDIDATE_DOC = json.loads((ROOT / "data/candidates.json").read_text(encoding="utf-8"))
PROFILES = json.loads((ROOT / "data/profiles.json").read_text(encoding="utf-8"))["profiles"]
CANDIDATES = {item["id"]: item for item in CANDIDATE_DOC["candidates"]}
CACHE_TYPES = {
    "f16": (2, 1),
    "q8_0": (34, 32),
    "q4_0": (18, 32),
}


def kv_scalars(candidate_id: str, context_tokens: int, ubatch: int = 512, nseq: int = 1) -> int | None:
    """Return allocated cache scalar count for the declared package profile.

    Hybrid-attention formulas use current llama.cpp interleaved-SWA allocation:
    the SWA cell count is bounded by window*nseq + ubatch and padded to 256.
    MiniMax/Kimi remain measurement gates and deliberately return None.
    """
    if candidate_id == "qwen3-235b-a22b-2507":
        return 94 * context_tokens * 4 * (128 + 128)
    if candidate_id == "glm-4.7":
        return 92 * context_tokens * 8 * (128 + 128)
    if candidate_id in {"tulu3-405b", "nemotron-ultra-253b"}:
        # Nemotron is a conservative full-attention upper bound.  Its Deci/NAS
        # metadata can skip attention in individual blocks, reducing real use.
        return 126 * context_tokens * 8 * (128 + 128)
    if candidate_id == "deepseek-r1-0528":
        # Current MLA absorption cache: 512 latent values + 64 RoPE-key values.
        return 61 * context_tokens * (512 + 64)
    if candidate_id == "mimo-v2-flash":
        swa_cells = math.ceil(min(context_tokens, 128 * nseq + ubatch) / 256) * 256
        return 9 * context_tokens * 4 * (192 + 128) + 39 * swa_cells * 8 * (192 + 128)
    if candidate_id == "step-3.7-flash":
        swa_cells = math.ceil(min(context_tokens, 512 * nseq + ubatch) / 256) * 256
        return 12 * context_tokens * 8 * (128 + 128) + 33 * swa_cells * 8 * (128 + 128)
    return None


def cache_bytes(candidate_id: str, context_tokens: int, cache_type: str) -> int | None:
    scalar_count = kv_scalars(candidate_id, context_tokens)
    if scalar_count is None:
        return None
    numerator, denominator = CACHE_TYPES[cache_type]
    return math.ceil(scalar_count * numerator / denominator)


def quarter_gib_ceiling(byte_count: int) -> float:
    return math.ceil((byte_count / 2**30) * 4) / 4


def contexts_for(candidate: dict[str, Any]) -> list[int]:
    native = int(candidate["context_native"])
    values = [value for value in (32768, 65536, 131072) if value <= native]
    if native not in values:
        values.append(native)
    return values


def split_quarter_gib(total_gib: float, member_count: int) -> list[float]:
    """Split a budget exactly in quarter-GiB planning units."""
    units = math.ceil(total_gib * 4 - 1e-12)
    quotient, remainder = divmod(units, member_count)
    return [(quotient + (1 if index < remainder else 0)) / 4 for index in range(member_count)]


def write_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    if not rows:
        raise ValueError(f"refusing to write empty table: {path}")
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def write_json(path: Path, rows: list[dict[str, Any]]) -> None:
    path.write_text(json.dumps(rows, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def generate_tables() -> tuple[list[dict[str, Any]], list[dict[str, Any]], list[dict[str, Any]]]:
    kv_rows: list[dict[str, Any]] = []
    capacity_rows: list[dict[str, Any]] = []

    for candidate in CANDIDATE_DOC["candidates"]:
        candidate_id = candidate["id"]
        for context_tokens in contexts_for(candidate):
            scalar_count = kv_scalars(candidate_id, context_tokens)
            if scalar_count is None:
                kv_rows.append(
                    {
                        "candidate_id": candidate_id,
                        "context_tokens": context_tokens,
                        "cache_type": "unverified",
                        "scalars": None,
                        "bytes": None,
                        "gib": None,
                        "plan_gib": None,
                        "status": candidate["kv_status"],
                    }
                )
                continue

            for cache_type in CACHE_TYPES:
                byte_count = cache_bytes(candidate_id, context_tokens, cache_type)
                assert byte_count is not None
                kv_rows.append(
                    {
                        "candidate_id": candidate_id,
                        "context_tokens": context_tokens,
                        "cache_type": cache_type,
                        "scalars": scalar_count,
                        "bytes": byte_count,
                        "gib": byte_count / 2**30,
                        "plan_gib": quarter_gib_ceiling(byte_count),
                        "status": candidate["kv_status"],
                    }
                )

            kv_plan_gib = quarter_gib_ceiling(cache_bytes(candidate_id, context_tokens, "q8_0") or 0)
            for profile in PROFILES:
                members = profile["members"]
                weight_splits = split_quarter_gib(candidate["weight_plan_gib"], len(members))
                kv_splits = split_quarter_gib(kv_plan_gib, len(members))
                for index, member in enumerate(members):
                    reserve = (
                        member["reserve_os_gib"]
                        + member["reserve_runtime_gib"]
                        + member["reserve_skew_gib"]
                    )
                    total = weight_splits[index] + kv_splits[index] + reserve
                    capacity_rows.append(
                        {
                            "candidate_id": candidate_id,
                            "candidate_name": candidate["name"],
                            "profile_id": profile["id"],
                            "profile_name": profile["name"],
                            "member": member["name"],
                            "context_tokens": context_tokens,
                            "capacity_gib": member["capacity_gib"],
                            "weight_plan_gib": weight_splits[index],
                            "kv_q8_plan_gib": kv_splits[index],
                            "reserve_os_gib": member["reserve_os_gib"],
                            "reserve_runtime_gib": member["reserve_runtime_gib"],
                            "reserve_skew_gib": member["reserve_skew_gib"],
                            "total_plan_gib": total,
                            "margin_gib": member["capacity_gib"] - total,
                            "utilization": total / member["capacity_gib"],
                            "fit": total <= member["capacity_gib"],
                        }
                    )

    summary_rows: list[dict[str, Any]] = []
    grouped: dict[tuple[str, str, int], list[dict[str, Any]]] = {}
    for row in capacity_rows:
        grouped.setdefault((row["candidate_id"], row["profile_id"], row["context_tokens"]), []).append(row)
    for (candidate_id, profile_id, context_tokens), rows in grouped.items():
        summary_rows.append(
            {
                "candidate_id": candidate_id,
                "profile_id": profile_id,
                "context_tokens": context_tokens,
                "all_members_fit": all(row["fit"] for row in rows),
                "max_member_utilization": max(row["utilization"] for row in rows),
                "min_member_margin_gib": min(row["margin_gib"] for row in rows),
                "total_kv_q8_plan_gib": sum(row["kv_q8_plan_gib"] for row in rows),
                "total_weight_plan_gib": sum(row["weight_plan_gib"] for row in rows),
            }
        )

    return kv_rows, capacity_rows, summary_rows


def generate_per_node_page(capacity_rows: list[dict[str, Any]]) -> None:
    qualified = [candidate for candidate in CANDIDATE_DOC["candidates"] if kv_scalars(candidate["id"], 1) is not None]
    selected_rows: list[str] = [
        "# Exact Per-Node Memory Budgets",
        "",
        "These tables expose the exact member-level arithmetic behind the fit summary. They use Q8_0 KV, one sequence, `ubatch=512`, equal planned split in quarter-GiB units, and the fixed reserves in `data/profiles.json`. The context is 128K, or the model native maximum when lower.",
        "",
        "`Total = weights + KV + OS/services + runtime/graph + split-skew safety`. Negative margin means the configuration does not fit under the declared envelope.",
        "",
    ]
    profile_order = {profile["id"]: index for index, profile in enumerate(PROFILES)}
    member_order = {
        (profile["id"], member["name"]): index
        for profile in PROFILES
        for index, member in enumerate(profile["members"])
    }
    for candidate in qualified:
        context_tokens = min(131072, int(candidate["context_native"]))
        rows = [
            row
            for row in capacity_rows
            if row["candidate_id"] == candidate["id"] and row["context_tokens"] == context_tokens
        ]
        rows.sort(key=lambda row: (profile_order[row["profile_id"]], member_order[(row["profile_id"], row["member"])]))
        selected_rows.extend(
            [
                f"## {candidate['name']}",
                "",
                f"Context: **{context_tokens:,} tokens** · selected artifact: **{candidate['selected_quant']}** · weight plan: **{candidate['weight_plan_gib']} GiB**",
                "",
                "| Profile | Member | Capacity | Weights | KV Q8 | OS | Runtime | Skew | Total | Margin | Fit |",
                "|---|---|---|---|---|---|---|---|---|---|---|",
            ]
        )
        for row in rows:
            fit = "FIT" if row["fit"] else "NO"
            selected_rows.append(
                "| {profile_id} | {member} | {capacity_gib:.2f} | {weight_plan_gib:.2f} | "
                "{kv_q8_plan_gib:.2f} | {reserve_os_gib:.2f} | {reserve_runtime_gib:.2f} | "
                "{reserve_skew_gib:.2f} | {total_plan_gib:.2f} | {margin_gib:+.2f} | {fit_label} |".format(
                    fit_label=fit, **row
                )
            )
        selected_rows.append("")
    selected_rows.extend(
        [
            "## Lower-context and native-context detail",
            "",
            "Every 32K, 64K, 128K, and native-context row is available in [`data/capacity_budgets.csv`](../data/capacity_budgets.csv). The CSV is the authoritative human-auditable output of `scripts/calculate_capacity.py`.",
            "",
            "Actual llama.cpp placement can differ because layers and non-layer tensors are indivisible. Replace the fixed skew reserve with measured startup allocation before final procurement.",
            "",
        ]
    )
    (ROOT / "pages/Per-Node-Budgets.md").write_text("\n".join(selected_rows), encoding="utf-8")


def main() -> None:
    kv_rows, capacity_rows, summary_rows = generate_tables()
    write_csv(ROOT / "data/kv_cache.csv", kv_rows)
    write_json(ROOT / "data/kv_cache.json", kv_rows)
    write_csv(ROOT / "data/capacity_budgets.csv", capacity_rows)
    write_json(ROOT / "data/capacity_budgets.json", capacity_rows)
    write_csv(ROOT / "data/capacity_summary.csv", summary_rows)
    write_json(ROOT / "data/capacity_summary.json", summary_rows)
    generate_per_node_page(capacity_rows)
    print(
        f"wrote {len(kv_rows)} KV rows, {len(capacity_rows)} member rows, "
        f"and {len(summary_rows)} summary rows"
    )


if __name__ == "__main__":
    main()
