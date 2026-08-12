#!/usr/bin/env python3
"""Model cache host writes and illustrative SSD TBW lifetime."""
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


def calculate(row: dict[str, str]) -> dict[str, float | str]:
    state_gib = float(row["state_gib"])
    checkpoints = float(row["checkpoints_per_day"])
    rated_tbw = float(row["rated_tbw"])
    waf = float(row["waf"])
    replicas = float(row.get("replicas", 1))
    overhead = float(row.get("draft_spec_overhead_ratio", 0))
    state_bytes = state_gib * (2 ** 30) * (1 + overhead)
    host_bytes_day = state_bytes * checkpoints * replicas
    host_tb_day = host_bytes_day / 1e12
    annual_host_tb = host_tb_day * 365
    annual_nand_tb = annual_host_tb * waf
    years = rated_tbw / annual_nand_tb if annual_nand_tb > 0 else float("inf")
    return {
        "scenario": row.get("scenario", "custom"),
        "state_gib_including_base": state_gib,
        "optional_overhead_ratio": overhead,
        "checkpoints_per_day": checkpoints,
        "replicas": replicas,
        "host_gib_per_day": host_bytes_day / (2 ** 30),
        "host_tb_per_year": annual_host_tb,
        "modeled_nand_tb_per_year": annual_nand_tb,
        "rated_tbw": rated_tbw,
        "waf": waf,
        "estimated_years_to_rated_tbw": years,
    }


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def render_markdown(results: list[dict]) -> str:
    lines = [
        "# SSD endurance scenario report", "",
        "> Illustrative model. Ratings and WAF are operator-supplied inputs, not drive warranties.", "",
        "| Scenario | State GiB | Checkpoints/day | Replicas | Host GiB/day | Host TB/year | WAF | Rated TBW | Estimated years |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for r in results:
        lines.append(
            f"| {r['scenario']} | {r['state_gib_including_base']:.2f} | {r['checkpoints_per_day']:.0f} | {r['replicas']:.0f} | "
            f"{r['host_gib_per_day']:.1f} | {r['host_tb_per_year']:.1f} | {r['waf']:.2f} | {r['rated_tbw']:.0f} | {r['estimated_years_to_rated_tbw']:.2f} |"
        )
    lines += ["", "Formula:", "", "```text", "years = rated_TBW / (host_TB_per_day × WAF × 365)", "```", ""]
    return "\n".join(lines)


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--scenario-file", type=Path, default=Path("../tables/endurance-scenarios.csv"))
    p.add_argument("--output", type=Path, default=Path("results/endurance-report.md"))
    p.add_argument("--json-output", type=Path, default=Path("results/endurance-report.json"))
    args = p.parse_args()
    results = [calculate(row) for row in load_rows(args.scenario_file)]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_markdown(results), encoding="utf-8")
    args.json_output.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    print(render_markdown(results))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
