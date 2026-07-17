#!/usr/bin/env python3
"""Insert a deterministic passkey nonce into an existing long-context text."""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("destination", type=Path)
    ap.add_argument("--case-id", required=True)
    ap.add_argument("--position-fraction", type=float, required=True)
    ns = ap.parse_args()
    if not 0 < ns.position_fraction < 1:
        raise SystemExit("position fraction must be between 0 and 1")
    text = ns.source.read_text(encoding="utf-8")
    nonce = hashlib.sha256(("passkey/"+ns.case_id).encode()).hexdigest()[:32]
    marker = f"\nPASSKEY[{ns.case_id}]={nonce}\n"
    pos = int(len(text) * ns.position_fraction)
    out = text[:pos] + marker + text[pos:]
    ns.destination.parent.mkdir(parents=True, exist_ok=True)
    ns.destination.write_text(out, encoding="utf-8")
    ns.destination.with_suffix(ns.destination.suffix+".json").write_text(json.dumps({
        "case_id":ns.case_id,
        "nonce":nonce,
        "position_fraction":ns.position_fraction,
        "answer_rule":"extract the exact nonce bytes",
    }, indent=2)+"\n")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
