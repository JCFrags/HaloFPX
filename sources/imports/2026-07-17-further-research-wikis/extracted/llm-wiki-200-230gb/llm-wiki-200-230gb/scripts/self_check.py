#!/usr/bin/env python3
"""Offline consistency checks for the packaged wiki."""
from __future__ import annotations

import csv
import json
import math
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = [
    "Home.md",
    "_Sidebar.md",
    "_Footer.md",
    "README.md",
    "SUMMARY.md",
    "data/candidates.json",
    "data/kv_cache.csv",
    "data/capacity_budgets.csv",
    "data/capacity_summary.csv",
    "site/index.html",
]


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def as_bool(value: str) -> bool:
    if value == "True":
        return True
    if value == "False":
        return False
    fail(f"unexpected bool encoding: {value!r}")
    return False


def check_local_html_links() -> int:
    href_re = re.compile(r'href=["\']([^"\']+)["\']')
    broken: list[str] = []
    html_files = sorted((ROOT / "site").rglob("*.html"))
    for html_path in html_files:
        text = html_path.read_text(encoding="utf-8")
        for href in href_re.findall(text):
            if href.startswith(("http://", "https://", "mailto:", "#", "javascript:")):
                continue
            path = unquote(urlsplit(href).path)
            if not path:
                continue
            target = (html_path.parent / path).resolve()
            if not target.exists():
                broken.append(f"{html_path.relative_to(ROOT)} -> {href}")
    if broken:
        fail("broken local HTML links:\n" + "\n".join(broken[:20]))
    return len(html_files)


def main() -> None:
    missing = [path for path in REQUIRED if not (ROOT / path).exists()]
    if missing:
        fail(f"missing required files: {missing}")

    candidate_doc = json.loads((ROOT / "data/candidates.json").read_text(encoding="utf-8"))
    candidates = candidate_doc["candidates"]
    candidate_ids = {candidate["id"] for candidate in candidates}
    if len(candidate_ids) != len(candidates):
        fail("duplicate candidate IDs")
    for candidate in candidates:
        size = float(candidate["size_gb_decimal"])
        if not 200 <= size <= 230:
            fail(f"candidate outside declared band: {candidate['id']}={size}")
        expected_plan = math.ceil(size * 1_000_000_000 / 2**30) + 1
        if int(candidate["weight_plan_gib"]) != expected_plan:
            fail(f"weight plan mismatch for {candidate['id']}: {candidate['weight_plan_gib']} != {expected_plan}")
        if not (ROOT / f"candidates/{candidate['id']}.md").exists():
            fail(f"missing candidate card: {candidate['id']}")
        if not (ROOT / f"manifests/{candidate['id']}.json").exists():
            fail(f"missing provenance manifest: {candidate['id']}")

    capacity_rows = list(csv.DictReader((ROOT / "data/capacity_budgets.csv").open(encoding="utf-8")))
    if not capacity_rows:
        fail("empty capacity table")
    grouped: dict[tuple[str, str, int], list[dict[str, str]]] = {}
    for row in capacity_rows:
        if row["candidate_id"] not in candidate_ids:
            fail(f"unknown candidate in capacity table: {row['candidate_id']}")
        capacity = float(row["capacity_gib"])
        total = float(row["total_plan_gib"])
        margin = float(row["margin_gib"])
        utilization = float(row["utilization"])
        fit = as_bool(row["fit"])
        if capacity <= 0:
            fail("non-positive capacity")
        if abs((capacity - total) - margin) > 1e-9:
            fail(f"margin arithmetic mismatch: {row}")
        if abs(total / capacity - utilization) > 1e-12:
            fail(f"utilization mismatch: {row}")
        if fit != (total <= capacity):
            fail(f"fit flag mismatch: {row}")
        key = (row["candidate_id"], row["profile_id"], int(row["context_tokens"]))
        grouped.setdefault(key, []).append(row)

    summary_rows = list(csv.DictReader((ROOT / "data/capacity_summary.csv").open(encoding="utf-8")))
    if len(summary_rows) != len(grouped):
        fail(f"summary row count mismatch: {len(summary_rows)} != {len(grouped)}")
    for summary in summary_rows:
        key = (summary["candidate_id"], summary["profile_id"], int(summary["context_tokens"]))
        rows = grouped.get(key)
        if rows is None:
            fail(f"orphan capacity summary row: {key}")
        expected_fit = all(as_bool(row["fit"]) for row in rows)
        if as_bool(summary["all_members_fit"]) != expected_fit:
            fail(f"summary fit mismatch: {key}")
        expected_util = max(float(row["utilization"]) for row in rows)
        expected_margin = min(float(row["margin_gib"]) for row in rows)
        expected_kv = sum(float(row["kv_q8_plan_gib"]) for row in rows)
        expected_weights = sum(float(row["weight_plan_gib"]) for row in rows)
        if abs(float(summary["max_member_utilization"]) - expected_util) > 1e-12:
            fail(f"summary utilization mismatch: {key}")
        if abs(float(summary["min_member_margin_gib"]) - expected_margin) > 1e-9:
            fail(f"summary margin mismatch: {key}")
        if abs(float(summary["total_kv_q8_plan_gib"]) - expected_kv) > 1e-9:
            fail(f"summary KV mismatch: {key}")
        if abs(float(summary["total_weight_plan_gib"]) - expected_weights) > 1e-9:
            fail(f"summary weight mismatch: {key}")

    html_count = check_local_html_links()
    print(
        f"OK: {len(candidates)} candidates, {len(capacity_rows)} capacity rows, "
        f"{len(summary_rows)} summaries, {html_count} HTML pages"
    )


if __name__ == "__main__":
    main()
