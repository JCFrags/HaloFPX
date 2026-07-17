#!/usr/bin/env python3
from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

REQUIRED = {
    "README.md",
    "facts_and_constraints.md",
    "design_implications.md",
    "procedures_and_checks.md",
    "open_questions.md",
    "sources.md",
    "section.yaml",
    "license_compatibility_matrix.md",
    "repository_snapshots.md",
    "provenance_map.md",
    "legal_review_register.md",
}

CSV_HEADERS = {
    "data/source_ledger.csv": {"source_id", "claim_label", "url", "accessed_at_utc"},
    "data/provenance_ledger.csv": {"provenance_id", "repository", "commit", "path", "license_expression"},
    "data/license_matrix.csv": {"license_or_material", "copy_into_mit_oriented_repo", "obligations"},
    "data/legal_review_register.csv": {"id", "severity", "area", "status"},
    "data/ancestry_edges.csv": {"edge_id", "child_commit", "parent_index", "parent_commit", "evidence_url"},
    "data/open_questions.csv": {"id", "priority", "question"},
    "data/resolved_findings.csv": {"id", "claim_label", "finding"},
}


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[1])
    errors: list[str] = []
    missing = sorted(name for name in REQUIRED if not (root / name).is_file())
    if missing:
        errors.append(f"missing required files: {missing}")

    section = (root / "section.yaml").read_text(encoding="utf-8") if (root / "section.yaml").exists() else ""
    for key in ("id:", "title:", "pages:", "research_cutoff_utc:"):
        if key not in section:
            errors.append(f"section.yaml missing {key}")

    for rel, required in CSV_HEADERS.items():
        path = root / rel
        if not path.exists():
            errors.append(f"missing {rel}")
            continue
        with path.open(newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            headers = set(reader.fieldnames or [])
            absent = required - headers
            if absent:
                errors.append(f"{rel} missing headers {sorted(absent)}")
            for row_no, row in enumerate(reader, 2):
                url = row.get("url") or row.get("source_url")
                if url and not url.startswith("https://"):
                    errors.append(f"{rel}:{row_no} non-https URL {url}")

    sha40 = re.compile(r"^[0-9a-f]{40}$")
    for rel, field_names in {
        "data/ancestry_edges.csv": ("child_commit", "parent_commit"),
        "data/provenance_ledger.csv": ("commit",),
    }.items():
        path = root / rel
        if not path.exists():
            continue
        with path.open(newline="", encoding="utf-8") as f:
            for row_no, row in enumerate(csv.DictReader(f), 2):
                for field in field_names:
                    value = (row.get(field) or "").strip()
                    if value and not sha40.fullmatch(value):
                        errors.append(f"{rel}:{row_no} {field} is not a 40-char lowercase SHA: {value}")

    link_pattern = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
    for md in root.rglob("*.md"):
        text = md.read_text(encoding="utf-8")
        for target in link_pattern.findall(text):
            target = target.split("#", 1)[0]
            if not target or "://" in target or target.startswith("mailto:"):
                continue
            resolved = (md.parent / target).resolve()
            try:
                resolved.relative_to(root.resolve())
            except ValueError:
                errors.append(f"{md.relative_to(root)} link escapes root: {target}")
                continue
            if not resolved.exists():
                errors.append(f"{md.relative_to(root)} broken link: {target}")

    if errors:
        for err in errors:
            print(f"ERROR: {err}")
        return 1
    print(f"Validated wiki at {root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
