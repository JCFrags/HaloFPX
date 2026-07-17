#!/usr/bin/env python3
"""Generate or verify CITATIONS.md from the machine-readable claim/source index."""
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "CITATIONS.md"


def render() -> str:
    index = json.loads((ROOT / "data/source-index.json").read_text(encoding="utf-8"))
    sources = {source["id"]: source for source in index["sources"]}
    claims = [
        json.loads(line)
        for line in (ROOT / "data/claims.jsonl").read_text(encoding="utf-8").splitlines()
        if line.strip()
    ]
    claims.sort(key=lambda claim: claim["id"])

    lines = [
        "# Citation map",
        "",
        "**Snapshot:** 2026-07-17. Claims are atomic retrieval units. Source-code claims are pinned to the revisions and blob IDs in [SOURCE-SNAPSHOT.md](SOURCE-SNAPSHOT.md).",
        "",
        "## Claims",
        "",
        "| ID | Kind | Confidence | Claim | Sources |",
        "|---|---|---|---|---|",
    ]
    for claim in claims:
        links: list[str] = []
        for source_id in claim.get("sources", []):
            source = sources.get(source_id)
            if source:
                links.append(f"[{source_id}]({source['url']})")
            else:
                links.append(f"`{source_id}` (missing from source index)")
        claim_text = claim["claim"].replace("|", "\\|")
        source_text = ", ".join(links) if links else "Project-defined"
        lines.append(
            f"| {claim['id']} | {claim['kind']} | {claim['confidence']} | {claim_text} | {source_text} |"
        )

    lines.extend(
        [
            "",
            "## Source catalog",
            "",
            "| Source ID | Type | Title |",
            "|---|---|---|",
        ]
    )
    for source in index["sources"]:
        title = source["title"].replace("|", "\\|")
        lines.append(f"| `{source['id']}` | {source['type']} | [{title}]({source['url']}) |")

    lines.extend(
        [
            "",
            "## Interpretation",
            "",
            "- **Upstream fact:** documented by a project, vendor, or standard body.",
            "- **Source-code fact:** observed in the pinned implementation and subject to later change.",
            "- **Engineering inference:** a reasoned conclusion that still requires target-platform validation.",
            "- **Heuristic:** a project acceptance threshold, not a protocol guarantee.",
            "",
            "See also: [source snapshot](SOURCE-SNAPSHOT.md) and [full references](docs/16-references.md).",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail when CITATIONS.md is stale")
    args = parser.parse_args()
    expected = render()
    if args.check:
        actual = OUTPUT.read_text(encoding="utf-8") if OUTPUT.exists() else ""
        if actual != expected:
            raise SystemExit("CITATIONS.md is stale; run tools/build_citations.py")
        print("CITATIONS.md matches data/claims.jsonl and data/source-index.json")
        return 0
    OUTPUT.write_text(expected, encoding="utf-8")
    print(f"wrote {OUTPUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
