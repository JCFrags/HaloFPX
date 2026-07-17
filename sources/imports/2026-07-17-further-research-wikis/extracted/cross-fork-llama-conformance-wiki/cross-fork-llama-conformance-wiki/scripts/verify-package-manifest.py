#!/usr/bin/env python3
"""Verify the shipped package manifest and checksum file."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "MANIFEST.json"
CHECKSUMS = ROOT / "MANIFEST.sha256"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    if not MANIFEST.is_file():
        print("error: MANIFEST.json is missing", file=sys.stderr)
        return 2

    try:
        data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"error: cannot read MANIFEST.json: {exc}", file=sys.stderr)
        return 2

    entries = data.get("files")
    if not isinstance(entries, list):
        print("error: MANIFEST.json has no files array", file=sys.stderr)
        return 2

    failures: list[str] = []
    seen: set[str] = set()
    for entry in entries:
        if not isinstance(entry, dict):
            failures.append("manifest entry is not an object")
            continue
        rel = entry.get("path")
        expected_size = entry.get("bytes")
        expected_hash = entry.get("sha256")
        if not isinstance(rel, str) or rel in seen:
            failures.append(f"invalid or duplicate path: {rel!r}")
            continue
        seen.add(rel)
        path = ROOT / rel
        try:
            path.resolve().relative_to(ROOT.resolve())
        except ValueError:
            failures.append(f"path escapes package root: {rel}")
            continue
        if not path.is_file():
            failures.append(f"missing: {rel}")
            continue
        actual_size = path.stat().st_size
        if actual_size != expected_size:
            failures.append(f"size mismatch: {rel}: expected {expected_size}, got {actual_size}")
        actual_hash = sha256(path)
        if actual_hash != expected_hash:
            failures.append(f"SHA-256 mismatch: {rel}: expected {expected_hash}, got {actual_hash}")

    if CHECKSUMS.is_file():
        checksum_map: dict[str, str] = {}
        for line_no, line in enumerate(CHECKSUMS.read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip():
                continue
            try:
                digest, rel = line.split("  ", 1)
            except ValueError:
                failures.append(f"invalid MANIFEST.sha256 line {line_no}")
                continue
            checksum_map[rel] = digest
        expected_manifest_hash = checksum_map.get("MANIFEST.json")
        if expected_manifest_hash is None:
            failures.append("MANIFEST.sha256 does not cover MANIFEST.json")
        elif sha256(MANIFEST) != expected_manifest_hash:
            failures.append("MANIFEST.json checksum does not match MANIFEST.sha256")

    if failures:
        print("package integrity: FAIL", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1

    print(f"package integrity: PASS — {len(entries)} shipped files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
