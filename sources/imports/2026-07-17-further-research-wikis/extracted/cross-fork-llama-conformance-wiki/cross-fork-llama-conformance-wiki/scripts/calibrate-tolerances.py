#!/usr/bin/env python3
"""Compute observed numeric deltas without creating a normative tolerance."""
from __future__ import annotations
import argparse, json, math
from pathlib import Path

def load_vector(path: Path, field: str | None):
    obj = json.loads(path.read_text(encoding="utf-8"))
    if field:
        for part in field.split("."):
            obj = obj[int(part)] if isinstance(obj, list) else obj[part]
    if not isinstance(obj, list):
        raise SystemExit(f"{path}: selected value is not an array")
    out = [float(x) for x in obj]
    if not all(math.isfinite(x) for x in out):
        raise SystemExit(f"{path}: vector contains non-finite values")
    return out

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--reference", type=Path, required=True)
    ap.add_argument("--candidate", type=Path, action="append", required=True)
    ap.add_argument("--field")
    ap.add_argument("--profile-id", required=True)
    ap.add_argument("--case-id", action="append", required=True)
    ap.add_argument("--output", type=Path, required=True)
    ns = ap.parse_args()
    ref = load_vector(ns.reference, ns.field)
    runs = []
    maxima = {"max_abs":0.0,"max_rel":0.0,"mean_abs":0.0}
    for path in ns.candidate:
        cand = load_vector(path, ns.field)
        if len(cand) != len(ref):
            raise SystemExit(f"{path}: length {len(cand)} != reference {len(ref)}")
        abs_d = [abs(a-b) for a,b in zip(ref,cand)]
        rel_d = [0.0 if a == b else (abs(a-b)/abs(a) if a else math.inf) for a,b in zip(ref,cand)]
        observed = {
            "path":str(path),
            "max_abs":max(abs_d, default=0.0),
            "max_rel":max(rel_d, default=0.0),
            "mean_abs":sum(abs_d)/len(abs_d) if abs_d else 0.0,
        }
        runs.append(observed)
        for k in maxima:
            maxima[k] = max(maxima[k], observed[k])
    out = {
        "schema_version":"1.0",
        "profile_id":ns.profile_id,
        "status":"PROPOSED",
        "scope":{
            "case_ids":ns.case_id,
            "forks":[],
            "backends":[],
            "model_sha256":None,
            "build_family":None,
        },
        "metrics":{
            "max_abs":None,"max_rel":None,"mean_abs":None,"cosine_distance":None,
            "top1_must_match":None,"top_k_overlap_min":None,
            "distribution_statistic_max":None,"p_value_min":None,"quality_delta_max":None,
        },
        "calibration":{
            "calibration_observations":[str(p) for p in ns.candidate],
            "validation_observations":[],
            "method":"Review observed evidence, justify limits, then validate on disjoint observations.",
            "approved_by":[],
            "approved_at":None,
        },
        "observed_evidence":{"runs":runs,"maxima":maxima},
        "warning":"Observed maxima are evidence, not approved thresholds. Normative metric fields remain null.",
    }
    ns.output.parent.mkdir(parents=True, exist_ok=True)
    ns.output.write_text(json.dumps(out, indent=2)+"\n", encoding="utf-8")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
