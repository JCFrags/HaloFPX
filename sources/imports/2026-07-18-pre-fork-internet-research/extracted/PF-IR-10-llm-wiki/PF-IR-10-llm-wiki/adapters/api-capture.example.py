#!/usr/bin/env python3
"""UNEXECUTED-EVIDENCE: minimal HTTP capture skeleton for an isolated workspace."""
# SPDX-License-Identifier: CC0-1.0
# This module is deliberately inert unless imported and called by a reviewed harness.
from __future__ import annotations
import hashlib, json, urllib.request
from pathlib import Path


def capture(url: str, request_file: Path, output_dir: Path) -> None:
    payload = request_file.read_bytes()
    req = urllib.request.Request(url, data=payload, headers={"Content-Type": "application/json"})
    output_dir.mkdir(parents=True, exist_ok=False)
    try:
        with urllib.request.urlopen(req, timeout=30) as response:
            body = response.read()
            status = response.status
            headers = dict(response.headers.items())
    except Exception as exc:  # capture raw failure; qualification policy classifies it later
        (output_dir / "exception.txt").write_text(repr(exc) + "\n", encoding="utf-8")
        raise
    (output_dir / "request.json").write_bytes(payload)
    (output_dir / "response.body").write_bytes(body)
    (output_dir / "response.json").write_text(json.dumps({"status": status, "headers": headers, "request_sha256": hashlib.sha256(payload).hexdigest()}, sort_keys=True, indent=2) + "\n")
