#!/usr/bin/env python3
"""Verify one bounded L55 first-chunk status record without exposing key bytes."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import os
import re
import stat
from pathlib import Path


DOMAIN = b"halofpx.l55.first-chunk.v1"
RECORD = re.compile(
    r"^\[halofpx-l55-status\] "
    r"(?P<canonical>phase=(?:capture-chunk|first-chunk)\|decode_status=-?\d+"
    r"(?:\|authority=version=1\|status=failed\|branch=[a-z0-9_]+"
    r"\|execution_sequence=1\|pending=1\|ggml_status=-?\d+"
    r"|\|chunks=1\|n_tokens=512))"
    r"\|auth_tag=(?P<tag>[0-9a-f]{64})$")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--key-file", required=True)
    parser.add_argument("--record", required=True)
    args = parser.parse_args()
    key_path = Path(args.key_file)
    st = key_path.stat()
    expected_uid = os.geteuid() if hasattr(os, "geteuid") else st.st_uid
    if (
        not key_path.is_absolute()
        or not stat.S_ISREG(st.st_mode)
        or st.st_uid != expected_uid
        or (os.name != "nt" and stat.S_IMODE(st.st_mode) != 0o600)
        or st.st_size != 130
    ):
        raise SystemExit("L55 key authority mismatch")
    match = RECORD.fullmatch(args.record)
    if match is None:
        raise SystemExit("L55 status grammar mismatch")
    lines = key_path.read_text(encoding="ascii").splitlines()
    if len(lines) != 2 or any(re.fullmatch(r"[0-9a-f]{64}", line) is None for line in lines):
        raise SystemExit("L55 key format mismatch")
    expected = hmac.new(
        bytes.fromhex(lines[0]), DOMAIN + match["canonical"].encode("utf-8"),
        hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected, match["tag"]):
        raise SystemExit("L55 status authentication failed")
    print(match["canonical"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
