#!/usr/bin/env python3
"""Verify one durable HaloFPX canary result-authority record."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import re
from pathlib import Path

DOMAIN = b"halofpx.result-authority.v1"
TAG_MARKER = " result_auth_tag="


def verify(record: str, key: bytes) -> dict[str, str]:
    if len(key) != 32 or len(record) > 8192 or record.count("\n") > 1:
        raise ValueError("result authority bounds mismatch")
    line = record.rstrip("\n")
    if line.count(TAG_MARKER) != 1:
        raise ValueError("missing or ambiguous result authentication")
    canonical, tag = line.rsplit(TAG_MARKER, 1)
    if re.fullmatch(r"[0-9a-f]{64}", tag) is None:
        raise ValueError("malformed result authentication")
    expected = hmac.new(key, DOMAIN + canonical.encode(), hashlib.sha256).hexdigest()
    if not hmac.compare_digest(tag, expected):
        raise ValueError("result authentication failure")
    fields: dict[str, str] = {}
    for item in canonical.split():
        if "=" not in item:
            raise ValueError("malformed result field")
        name, value = item.split("=", 1)
        if name in fields:
            raise ValueError(f"duplicate result field: {name}")
        fields[name] = value
    fields["result_auth_tag"] = tag
    return fields


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--record", required=True, type=Path)
    parser.add_argument("--key-file", required=True, type=Path)
    args = parser.parse_args()
    key_text = args.key_file.read_text(encoding="ascii").splitlines()[0]
    key = bytes.fromhex(key_text)
    fields = verify(args.record.read_text(encoding="utf-8"), key)
    print(json.dumps(fields, sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
