#!/usr/bin/env python3
"""Canonical JSON comparator with JSON Pointer projection and ignored fields.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import argparse, json, sys
from pathlib import Path
from typing import Any


def pointer(doc: Any, ptr: str) -> Any:
    if ptr == "":
        return doc
    if not ptr.startswith("/"):
        raise ValueError("JSON Pointer must be empty or begin with /")
    cur = doc
    for part in ptr[1:].split("/"):
        part = part.replace("~1", "/").replace("~0", "~")
        cur = cur[int(part)] if isinstance(cur, list) else cur[part]
    return cur


def remove_pointer(doc: Any, ptr: str) -> None:
    parts = ptr[1:].split("/") if ptr.startswith("/") else []
    if not parts:
        return
    cur = doc
    for raw in parts[:-1]:
        key = raw.replace("~1", "/").replace("~0", "~")
        cur = cur[int(key)] if isinstance(cur, list) else cur[key]
    key = parts[-1].replace("~1", "/").replace("~0", "~")
    if isinstance(cur, list):
        del cur[int(key)]
    else:
        cur.pop(key, None)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("expected", type=Path)
    ap.add_argument("actual", type=Path)
    ap.add_argument("--pointer", default="")
    ap.add_argument("--ignore", action="append", default=[])
    ns = ap.parse_args()
    expected = json.loads(ns.expected.read_text(encoding="utf-8"))
    actual = json.loads(ns.actual.read_text(encoding="utf-8"))
    for p in ns.ignore:
        remove_pointer(expected, p); remove_pointer(actual, p)
    expected = pointer(expected, ns.pointer); actual = pointer(actual, ns.pointer)
    if expected != actual:
        print(json.dumps({"match": False, "expected": expected, "actual": actual}, ensure_ascii=False, sort_keys=True, indent=2))
        return 1
    print(json.dumps({"match": True}, sort_keys=True))
    return 0
if __name__ == "__main__":
    raise SystemExit(main())
