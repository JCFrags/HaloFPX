#!/usr/bin/env python3
"""Reference tokenizer for the self-generated PFIR10 byte-BPE vocabulary.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import argparse, json
from pathlib import Path

SPECIAL = ["<unk>", "<s>", "</s>", "<|eot_id|>", "<|pad|>", "<|tool_call|>", "<|end_tool_call|>"]
BASE = len(SPECIAL)


def tokenize(raw: bytes, parse_special: bool, add_bos: bool) -> list[int]:
    out = [1] if add_bos else []
    markers = sorted(((s.encode(), i) for i, s in enumerate(SPECIAL)), key=lambda x: (-len(x[0]), x[1]))
    i = 0
    while i < len(raw):
        hit = next(((m, tid) for m, tid in markers if parse_special and raw.startswith(m, i)), None)
        if hit:
            m, tid = hit; out.append(tid); i += len(m)
        else:
            out.append(BASE + raw[i]); i += 1
    return out


def main() -> int:
    ap = argparse.ArgumentParser(); ap.add_argument("cases", type=Path)
    ns = ap.parse_args(); failures = []; count = 0
    for line_no, line in enumerate(ns.cases.read_text(encoding="utf-8").splitlines(), 1):
        if not line: continue
        row = json.loads(line); count += 1; raw = bytes.fromhex(row["input_hex"])
        if "expected_ids_no_special" in row:
            checks = [("no-special", tokenize(raw, False, False), row["expected_ids_no_special"])]
            if raw.hex() != row["expected_roundtrip_hex"]:
                failures.append({"line": line_no, "id": row.get("id"), "check": "roundtrip-hex", "expected": row["expected_roundtrip_hex"], "actual": raw.hex()})
        else:
            checks = [
                ("parse-special-false", tokenize(raw, False, False), row["parse_special_false"]),
                ("parse-special-true", tokenize(raw, True, False), row["parse_special_true"]),
                ("add-bos-true", tokenize(raw, True, True), row["add_bos_true_prefix"]),
            ]
        for check, actual, expected in checks:
            if actual != expected:
                failures.append({"line": line_no, "id": row.get("id"), "check": check, "expected": expected, "actual": actual})
    print(json.dumps({"match": not failures, "cases": count, "failures": failures}, ensure_ascii=False, sort_keys=True, indent=2))
    return 0 if not failures else 1
if __name__ == "__main__": raise SystemExit(main())
