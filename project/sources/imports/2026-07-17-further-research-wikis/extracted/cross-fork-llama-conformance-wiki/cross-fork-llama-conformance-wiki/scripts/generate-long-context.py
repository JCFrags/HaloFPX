#!/usr/bin/env python3
"""Generate token-counted long-context fixtures through a server tokenizer API.

The script never assumes a character/token ratio. It grows deterministic text,
asks the tested server to tokenize it, and stops at the requested relation.
"""
from __future__ import annotations
import argparse, json, urllib.request
from pathlib import Path

UNIT = "alpha beta gamma delta epsilon zeta eta theta iota kappa.\n"

def post_json(url: str, obj: object) -> object:
    data = json.dumps(obj).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers={"content-type":"application/json"}, method="POST")
    with urllib.request.urlopen(req, timeout=30) as res:
        return json.loads(res.read().decode("utf-8"))

def token_count(base_url: str, text: str, add_special: bool) -> int:
    out = post_json(base_url.rstrip("/") + "/tokenize", {"content":text, "add_special":add_special})
    toks = out["tokens"]
    if not isinstance(toks, list) or not all(isinstance(x, int) for x in toks):
        raise RuntimeError("unexpected /tokenize response")
    return len(toks)

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base-url", required=True)
    ap.add_argument("--target-tokens", required=True, type=int)
    ap.add_argument("--relation", choices=["exact","at-least"], default="exact")
    ap.add_argument("--add-special", action="store_true")
    ap.add_argument("--output", required=True, type=Path)
    ns = ap.parse_args()
    if ns.target_tokens < 0:
        raise SystemExit("target must be nonnegative")

    # Grow by doubling, then binary-search whole UNIT repetitions.
    lo, hi = 0, 1
    while token_count(ns.base_url, UNIT * hi, ns.add_special) < ns.target_tokens:
        lo, hi = hi, hi * 2
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if token_count(ns.base_url, UNIT * mid, ns.add_special) < ns.target_tokens:
            lo = mid
        else:
            hi = mid

    candidates = [UNIT * lo, UNIT * hi]
    measured = [(s, token_count(ns.base_url, s, ns.add_special)) for s in candidates]
    if ns.relation == "exact":
        exact = [x for x in measured if x[1] == ns.target_tokens]
        if not exact:
            raise SystemExit(
                "whole-unit generator could not hit exact token count; "
                "extend with a fork/vocab-specific suffix search and promote that input"
            )
        text, n = exact[0]
    else:
        text, n = min((x for x in measured if x[1] >= ns.target_tokens), key=lambda x:x[1])

    ns.output.parent.mkdir(parents=True, exist_ok=True)
    ns.output.write_text(text, encoding="utf-8")
    meta = {
        "target_tokens":ns.target_tokens,
        "actual_tokens":n,
        "relation":ns.relation,
        "add_special":ns.add_special,
        "tokenizer_url":ns.base_url.rstrip("/") + "/tokenize",
    }
    ns.output.with_suffix(ns.output.suffix + ".json").write_text(json.dumps(meta, indent=2)+"\n")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
