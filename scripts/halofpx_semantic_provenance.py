#!/usr/bin/env python3
"""Verify one canonical HaloFPX semantic-provenance record."""

import argparse
import hashlib
import hmac
import re
from pathlib import Path
import sys


HEX64 = re.compile(r"[0-9a-f]{64}")
DOMAIN = b"halofpx.semantic-provenance.v1"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--key-file", required=True, type=Path)
    parser.add_argument("--expected-tag", required=True)
    args = parser.parse_args()
    if HEX64.fullmatch(args.expected_tag) is None:
        return 2
    raw = args.key_file.read_bytes()
    lines = raw.decode("ascii").splitlines()
    if (
        len(raw) != 130
        or len(lines) != 2
        or any(HEX64.fullmatch(line) is None for line in lines)
    ):
        return 2
    record = sys.stdin.buffer.read()
    try:
        record.decode("ascii")
    except UnicodeDecodeError:
        return 2
    actual = hmac.new(
        bytes.fromhex(lines[0]), DOMAIN + record, hashlib.sha256
    ).hexdigest()
    if not hmac.compare_digest(actual, args.expected_tag):
        return 3
    print(actual, flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
