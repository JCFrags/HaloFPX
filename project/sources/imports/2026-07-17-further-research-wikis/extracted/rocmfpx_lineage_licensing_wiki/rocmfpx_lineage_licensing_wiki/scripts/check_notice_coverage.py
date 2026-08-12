#!/usr/bin/env python3
from __future__ import annotations

import csv
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: check_notice_coverage.py provenance_ledger.csv THIRD_PARTY_NOTICES.md", file=sys.stderr)
        return 2
    ledger = Path(sys.argv[1])
    notice = Path(sys.argv[2])
    text = notice.read_text(encoding="utf-8", errors="replace").lower()
    missing: list[tuple[str, str, str]] = []
    with ledger.open(newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            if row.get("notice_required", "").lower() != "yes":
                continue
            token = row.get("notice_token", "").strip()
            if token and token.lower() not in text:
                missing.append((row.get("provenance_id", ""), token, row.get("path", "")))
    if missing:
        print("Missing expected notice tokens:")
        for pid, token, path in missing:
            print(f"- {pid}: {token!r} for {path}")
        return 1
    print("All ledger notice tokens were found. Manual legal review is still required.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
