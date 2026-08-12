#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
from datetime import date
from pathlib import Path
import sys
import yaml

ROOT = Path(__file__).resolve().parents[1]
VERSION = (ROOT / "VERSION").read_text().strip() if (ROOT / "VERSION").exists() else "2026.07.17"
errors: list[str] = []


def load_json(p): return json.loads(p.read_text(encoding="utf-8"))
def load_yaml(p): return yaml.safe_load(p.read_text(encoding="utf-8"))

def check_unique(rows, label):
    ids = [r.get("id") for r in rows]
    if len(ids) != len(set(ids)): errors.append(f"duplicate {label} IDs")
    if any(not x for x in ids): errors.append(f"empty {label} ID")

sources = load_json(ROOT / "sources/source-registry.json")
source_ids = {s["id"] for s in sources}
check_unique(sources, "source")
profiles_j = load_json(ROOT / f"data/compatibility-matrix-{VERSION}.json")
profiles_y = load_yaml(ROOT / f"data/compatibility-matrix-{VERSION}.yaml")
if profiles_j != profiles_y: errors.append("compatibility JSON/YAML differ")
check_unique(profiles_j, "profile")

with (ROOT / f"data/compatibility-matrix-{VERSION}.csv").open(encoding="utf-8", newline="") as f:
    csv_rows = list(csv.DictReader(f))
if [r["id"] for r in csv_rows] != [r["id"] for r in profiles_j]: errors.append("compatibility CSV IDs/order differ")

for p in profiles_j:
    for key in ["classification","evidence_level","status","scope","kernel","rocm","llama_cpp","sources","last_verified"]:
        if not p.get(key): errors.append(f"{p.get('id')}: missing {key}")
    missing = set(p.get("sources", [])) - source_ids
    if missing: errors.append(f"{p['id']}: unknown sources {sorted(missing)}")
    try:
        if date.fromisoformat(p["last_verified"]) > date(2026,7,17): errors.append(f"{p['id']}: future verification date")
    except Exception: errors.append(f"{p['id']}: invalid last_verified")
    if p["classification"] == "official-supported":
        official = [source_ids and next((s for s in sources if s["id"] == sid), None) for sid in p["sources"]]
        if not any(s and s["classification"].startswith("official") for s in official):
            errors.append(f"{p['id']}: official-supported without official source")

for filename in [f"regressions-{VERSION}.json", "environment-variables.json", f"component-versions-{VERSION}.json"]:
    rows = load_json(ROOT / "data" / filename)
    for idx, row in enumerate(rows):
        missing = set(row.get("sources", [])) - source_ids
        if missing: errors.append(f"{filename}[{idx}]: unknown sources {sorted(missing)}")

current = load_json(ROOT / "data/compatibility-matrix-current.json")
if current != profiles_j: errors.append("current matrix pointer differs from dated JSON")

metadata = load_json(ROOT / "metadata.json")
if metadata.get("version") != VERSION: errors.append("metadata version differs from VERSION")
if metadata.get("llama_cpp_pin", {}).get("commit") != "86d86ed4396b4130922f7b9af26e3d9fc11a591b": errors.append("unexpected llama.cpp pin")

if errors:
    print("Validation failed:", file=sys.stderr)
    for e in errors: print(f"- {e}", file=sys.stderr)
    sys.exit(1)
print(f"Validated {len(profiles_j)} profiles, {len(sources)} sources, {len(csv_rows)} CSV rows")
