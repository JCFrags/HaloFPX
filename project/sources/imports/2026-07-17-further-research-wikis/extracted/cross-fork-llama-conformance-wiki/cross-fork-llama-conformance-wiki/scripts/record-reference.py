#!/usr/bin/env python3
"""Wrap a raw observation in a PROPOSED reference record.

This command never creates an APPROVED reference.
"""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

def canonical(obj) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",",":"), ensure_ascii=False).encode("utf-8")

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("observation", type=Path)
    ap.add_argument("--reference-id", required=True)
    ap.add_argument("--hardware-lane", required=True)
    ap.add_argument("--output", type=Path, required=True)
    ns = ap.parse_args()
    obs = json.loads(ns.observation.read_text(encoding="utf-8"))
    required = ["case_id","source","build","fixtures"]
    missing = [k for k in required if k not in obs]
    if missing:
        raise SystemExit(f"observation missing {missing}")
    model_digests = [
        f["sha256"] for f in obs.get("fixtures", [])
        if f.get("id","").startswith("model.") and f.get("sha256")
    ]
    record = {
        "schema_version":"1.0",
        "status":"PROPOSED",
        "case_id":obs["case_id"],
        "reference_id":ns.reference_id,
        "observation_sha256":hashlib.sha256(canonical(obs)).hexdigest(),
        "observation_path":str(ns.observation),
        "provenance":{
            "source_commit":obs["source"]["commit"],
            "model_digests":model_digests,
            "hardware_lane":ns.hardware_lane,
            "build_id":obs["build"]["build_id"],
        },
        "approval":{
            "independent_runs":[],
            "reviewers":[],
            "approved_at":None,
            "notes":"Promotion requires independent validation and review.",
        },
    }
    ns.output.parent.mkdir(parents=True, exist_ok=True)
    ns.output.write_text(json.dumps(record, indent=2)+"\n", encoding="utf-8")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
