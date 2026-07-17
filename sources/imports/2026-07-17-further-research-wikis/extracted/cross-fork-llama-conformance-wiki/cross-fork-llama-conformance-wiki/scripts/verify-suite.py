#!/usr/bin/env python3
"""Validate suite integrity and reference-safety rules."""
from __future__ import annotations
import json, re, sys, hashlib
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CASE_RE = re.compile(r"^[A-Z]+-[0-9]{3}$")
HEX64 = re.compile(r"^[0-9a-f]{64}$")
REQUIRED_AREAS = {
    "GGUF parsing","Tokenizer","Chat templates","Logits","Sampling",
    "Deterministic runs","Quantized kernels","Long context","Cache save/restore",
    "Cache rejection","Server APIs","MTP/speculative decoding","RPC",
    "Cancellation","Expected error behavior",
}
NUMERIC_KEYS = {
    "max_abs","max_rel","mean_abs","cosine_distance","top_k_overlap_min",
    "distribution_statistic_max","p_value_min","quality_delta_max",
}
errors: list[str] = []
warnings: list[str] = []

def load(rel: str):
    return json.loads((ROOT/rel).read_text(encoding="utf-8"))

matrix = load("matrix/test-matrix.json")
cases = matrix.get("cases", [])
ids = [c.get("id") for c in cases]
if len(ids) != len(set(ids)):
    errors.append("duplicate case IDs")
for case in cases:
    if not CASE_RE.fullmatch(str(case.get("id",""))):
        errors.append(f"invalid case ID: {case.get('id')!r}")
    for fork in ("upstream","rocmfpx","cachyllama","integration"):
        if fork not in case.get("applicability",{}):
            errors.append(f"{case.get('id')}: missing applicability for {fork}")
    if not case.get("fixtures"):
        errors.append(f"{case.get('id')}: no fixtures")
    if not case.get("expected_behavior"):
        errors.append(f"{case.get('id')}: no expected behavior")
    numeric = "numeric" in str(case.get("oracle","")).lower() or "calibrated" in str(case.get("comparison_mode","")).lower()
    if numeric and "approved numeric reference" not in case.get("prerequisites",[]) and case.get("oracle") != "native-test":
        errors.append(f"{case['id']}: numeric comparison lacks approved numeric reference prerequisite")
    distributional = "distribution" in str(case.get("oracle","")).lower() or "statistical" in str(case.get("comparison_mode","")).lower()
    if distributional and "approved distributional reference" not in case.get("prerequisites",[]) and "paired-statistical" not in str(case.get("comparison_mode","")):
        # paired statistical cases still need the prerequisite; enforce below.
        errors.append(f"{case['id']}: distributional comparison lacks approved distributional reference prerequisite")
    if "paired-statistical" in str(case.get("comparison_mode","")) and "approved distributional reference" not in case.get("prerequisites",[]):
        errors.append(f"{case['id']}: paired statistical comparison lacks approved distributional reference prerequisite")

areas = {c.get("area") for c in cases}
missing_areas = sorted(REQUIRED_AREAS - areas)
if missing_areas:
    errors.append(f"missing requested areas: {missing_areas}")

fixture_manifest = load("fixtures/manifest.json")
fixture_entries = fixture_manifest.get("fixtures", [])
fixture_by_id = {f["id"]:f for f in fixture_entries}
if len(fixture_by_id) != len(fixture_entries):
    errors.append("duplicate fixture IDs")
used_fixture_ids = {f for c in cases for f in c["fixtures"]}
missing_fixtures = sorted(used_fixture_ids - set(fixture_by_id))
if missing_fixtures:
    errors.append(f"matrix fixtures missing from manifest: {missing_fixtures}")
for fid, entry in fixture_by_id.items():
    rel = entry.get("path")
    if rel and not (ROOT/rel).exists():
        errors.append(f"fixture {fid}: path does not exist: {rel}")
    digest = entry.get("sha256")
    if digest is not None and not HEX64.fullmatch(str(digest)):
        errors.append(f"fixture {fid}: invalid SHA-256")
    if digest is not None and rel and entry.get("materialization") in {"included","recipe"}:
        h = hashlib.sha256()
        with (ROOT/rel).open("rb") as f:
            for chunk in iter(lambda:f.read(1024*1024), b""):
                h.update(chunk)
        actual = h.hexdigest()
        if actual != digest:
            errors.append(f"fixture {fid}: manifest digest {digest} != current file digest {actual}")
    if entry.get("materialization") in {"download","operator-supplied"} and digest is None:
        warnings.append(f"fixture {fid}: unresolved digest; run is forbidden until locally locked")

model_manifest = load("fixtures/models/model-manifest.json")
for model in model_manifest.get("models",[]):
    mat = model.get("materialization")
    digest = model.get("sha256")
    if digest is not None and not HEX64.fullmatch(str(digest)):
        errors.append(f"model {model.get('id')}: invalid SHA-256")
    if mat == "download" and not digest:
        errors.append(f"download model {model.get('id')}: missing SHA-256")

# Numeric profiles: non-approved files may contain observed evidence, but normative fields remain null.
for path in sorted((ROOT/"references/tolerances").glob("*.json")):
    profile = json.loads(path.read_text(encoding="utf-8"))
    status = profile.get("status")
    metrics = profile.get("metrics",{})
    populated = {k:v for k,v in metrics.items() if k in NUMERIC_KEYS and v is not None}
    if status != "APPROVED" and populated:
        errors.append(f"{path.relative_to(ROOT)}: non-approved profile has normative numeric values: {sorted(populated)}")
    if status == "APPROVED":
        cal = profile.get("calibration",{})
        if not populated:
            errors.append(f"{path.relative_to(ROOT)}: approved profile has no normative values")
        if not cal.get("approved_by") or not cal.get("approved_at"):
            errors.append(f"{path.relative_to(ROOT)}: approved profile lacks approval")
        a = set(cal.get("calibration_observations",[]))
        b = set(cal.get("validation_observations",[]))
        if not a or not b or a & b:
            errors.append(f"{path.relative_to(ROOT)}: calibration/validation sets missing or overlap")

# Approved references must not be examples or placeholders.
for path in sorted((ROOT/"references").rglob("*.json")):
    if "tolerances" in path.parts:
        continue
    obj = json.loads(path.read_text(encoding="utf-8"))
    if obj.get("status") == "APPROVED":
        if obj.get("observation_sha256") == "0"*64:
            errors.append(f"{path.relative_to(ROOT)}: approved reference uses placeholder digest")
        approval = obj.get("approval",{})
        if not approval.get("reviewers") or not approval.get("approved_at"):
            errors.append(f"{path.relative_to(ROOT)}: approved reference lacks reviewers/date")

snapshot = matrix.get("research_snapshot",{})
for fork in ("upstream","rocmfpx"):
    if not snapshot.get(fork,{}).get("repository") or not snapshot.get(fork,{}).get("ref"):
        errors.append(f"research snapshot lacks pin for {fork}")
cachy = snapshot.get("cachyllama",{})
if cachy.get("requested_status") != "not resolved during research":
    errors.append("CachyLLama alias ambiguity marker was removed without updating policy")
if snapshot.get("integration",{}).get("ref") is not None:
    warnings.append("integration fork is pinned in the matrix snapshot; update Forks-and-Pinning.md")

# Optional JSON Schema validation when jsonschema is installed.
try:
    import jsonschema
except Exception:
    warnings.append("jsonschema package unavailable; structural checks only")
else:
    case_schema = load("matrix/schemas/case.schema.json")
    for case in cases:
        try:
            jsonschema.validate(case, case_schema)
        except Exception as exc:
            errors.append(f"{case.get('id')}: schema validation failed: {exc.message if hasattr(exc,'message') else exc}")
    try:
        jsonschema.validate(fixture_manifest, load("matrix/schemas/fixture-manifest.schema.json"))
    except Exception as exc:
        errors.append(f"fixture manifest schema validation failed: {exc.message if hasattr(exc,'message') else exc}")

print(f"cases: {len(cases)}")
print(f"areas: {len(areas)}")
print(f"failure-injection cases: {sum(bool(c.get('failure_injection')) for c in cases)}")
print(f"fixture IDs: {len(fixture_entries)}")
print("area counts:")
for area, count in sorted(Counter(c["area"] for c in cases).items()):
    print(f"  {area}: {count}")
for warning in warnings:
    print(f"warning: {warning}", file=sys.stderr)
if errors:
    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    raise SystemExit(1)
print("suite integrity: PASS")
