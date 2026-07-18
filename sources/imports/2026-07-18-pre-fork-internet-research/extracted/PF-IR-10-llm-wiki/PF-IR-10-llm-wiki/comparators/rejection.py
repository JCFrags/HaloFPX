#!/usr/bin/env python3
"""Classify process/API rejection evidence without fixing diagnostic wording.

SPDX-License-Identifier: CC0-1.0
"""
from __future__ import annotations
import argparse, json, re
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("record", type=Path, help="JSON record with exit_code/http_status/stderr/body")
    ap.add_argument("--class", dest="expected", required=True, choices=["reject-load", "reject-request", "reject-template", "reject-grammar", "reject-unsupported-type"])
    ns = ap.parse_args()
    rec = json.loads(ns.record.read_text(encoding="utf-8"))
    exit_code = rec.get("exit_code")
    status = rec.get("http_status")
    text = "\n".join(str(rec.get(k, "")) for k in ("stderr", "stdout", "body")).lower()
    rejected = (isinstance(exit_code, int) and exit_code != 0) or (isinstance(status, int) and 400 <= status < 500)
    hints = {
        "reject-load": r"gguf|model|load|magic|metadata|tensor|truncat|offset|alignment",
        "reject-request": r"request|json|schema|parameter|invalid|unprocessable|bad request",
        "reject-template": r"template|jinja|role|filter|parse",
        "reject-grammar": r"grammar|gbnf|schema|parse|invalid",
        "reject-unsupported-type": r"type|unsupported|unknown|turbo|quant",
    }
    hinted = bool(re.search(hints[ns.expected], text))
    ok = rejected and hinted
    print(json.dumps({"match": ok, "expected_class": ns.expected, "rejected": rejected, "diagnostic_hint": hinted}, sort_keys=True))
    return 0 if ok else 1
if __name__ == "__main__": raise SystemExit(main())
