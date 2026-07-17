#!/usr/bin/env python3
"""Generate or verify llms-full.txt from canonical wiki and evidence files."""
from __future__ import annotations

import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "llms-full.txt"

FILES = [
    "README.md",
    "Home.md",
    "_Sidebar.md",
    "_Footer.md",
    "AGENTS.md",
    "CITATIONS.md",
    "SOURCE-SNAPSHOT.md",
    "VALIDATION.md",
    "docs/index.md",
    *[f"docs/{i:02d}-{name}.md" for i, name in [
        (0, "executive-conclusions"),
        (1, "scope-and-terms"),
        (2, "topology-and-enumeration"),
        (3, "kernel-prerequisites"),
        (4, "usb4net-configuration"),
        (5, "routing-and-policy-routing"),
        (6, "bonding-and-ecmp"),
        (7, "mptcp"),
        (8, "mtu-queues-offloads"),
        (9, "cpu-and-numa"),
        (10, "integrity-and-security"),
        (11, "llama-cpp-rpc"),
        (12, "transport-comparison"),
        (13, "usb4stream-and-experimental"),
        (14, "measurement-and-proof"),
        (15, "troubleshooting"),
        (16, "references"),
        (17, "custom-runtime-design"),
        (18, "runbook"),
        (19, "open-questions"),
    ]],
    "docs/glossary.md",
    "examples/address-plan.md",
    "examples/expected-output.md",
    "examples/proof-checklist.md",
    "manifest.json",
    "wiki.json",
    "data/claims.jsonl",
    "data/source-index.json",
    "data/kernel-feature-matrix.csv",
    "data/transports.csv",
    "data/proof-criteria.yaml",
    "data/validation-environment.json",
]

LANG_BY_SUFFIX = {
    ".json": "json",
    ".jsonl": "jsonl",
    ".csv": "csv",
    ".yaml": "yaml",
    ".yml": "yaml",
    ".txt": "text",
}


def render() -> str:
    chunks = [
        "# Dual-USB4 Strix Halo Transport Wiki — full LLM corpus\n",
        "Snapshot: 2026-07-17. Canonical paths are retained as source markers. Source-code facts are revision-pinned; target-hardware results must be measured.\n",
    ]
    for rel in FILES:
        path = ROOT / rel
        if not path.is_file():
            raise FileNotFoundError(rel)
        text = path.read_text(encoding="utf-8").rstrip()
        chunks.append("\n---\n")
        chunks.append(f"\n<!-- source: {rel} -->\n")
        if path.suffix.lower() == ".md":
            chunks.append("\n" + text + "\n")
        else:
            language = LANG_BY_SUFFIX.get(path.suffix.lower(), "text")
            chunks.append(f"\n## Source file: `{rel}`\n\n```{language}\n{text}\n```\n")
    return "".join(chunks).rstrip() + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail when llms-full.txt is stale")
    args = parser.parse_args()
    expected = render()
    if args.check:
        actual = OUTPUT.read_text(encoding="utf-8") if OUTPUT.exists() else ""
        if actual != expected:
            raise SystemExit("llms-full.txt is stale; run tools/build_llm_corpus.py")
        print("llms-full.txt matches canonical wiki/evidence files")
        return 0
    OUTPUT.write_text(expected, encoding="utf-8")
    print(f"wrote {OUTPUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
