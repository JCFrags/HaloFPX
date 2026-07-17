#!/usr/bin/env python3
"""Build a simple Markdown root index from section_index.json."""

from __future__ import annotations

import argparse
import json
from collections import OrderedDict
from pathlib import Path

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("wiki_root", type=Path)
    parser.add_argument(
        "--registry",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "section_index.json",
    )
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()

    registry = json.loads(args.registry.read_text(encoding="utf-8"))
    grouped: OrderedDict[tuple[str, str], list[dict]] = OrderedDict()
    for item in registry:
        grouped.setdefault((item["category_id"], item["category_title"]), []).append(item)

    lines = ["# HaloFPX LLM Wiki — Section Index", ""]
    for (category_id, category_title), items in grouped.items():
        lines.extend([f"## {category_id} — {category_title}", ""])
        for item in items:
            section_readme = args.wiki_root / item["target_path"] / "README.md"
            marker = "complete" if section_readme.is_file() else "pending"
            rel = f"{item['target_path']}/README.md"
            lines.append(f"- [{item['section_id']} — {item['title']}]({rel}) — {marker}")
        lines.append("")

    output = args.output or args.wiki_root / "SECTION_INDEX.md"
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(output)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
