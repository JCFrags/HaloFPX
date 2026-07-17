#!/usr/bin/env python3
"""Generate bounded nested Jinja source for resource-limit tests."""
from __future__ import annotations
import argparse
from pathlib import Path

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--depth", type=int, required=True)
    ap.add_argument("--output", type=Path, required=True)
    ns = ap.parse_args()
    if not 1 <= ns.depth <= 100000:
        raise SystemExit("depth outside generator safety bound")
    source = "".join("{% if true %}" for _ in range(ns.depth))
    source += "x"
    source += "".join("{% endif %}" for _ in range(ns.depth))
    ns.output.parent.mkdir(parents=True, exist_ok=True)
    ns.output.write_text(source, encoding="utf-8")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
