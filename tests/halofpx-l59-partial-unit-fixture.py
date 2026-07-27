#!/usr/bin/env python3
"""Write one authenticated RPC response prefix, fsync it, then fail."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import os
from pathlib import Path


DOMAIN = b"halofpx.rpc-response-boundary.v1"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--key-file", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    key_lines = Path(args.key_file).read_bytes().splitlines()
    if len(key_lines) != 2:
        return 17
    key = bytes.fromhex(key_lines[0].decode("ascii"))
    if hashlib.sha256(key).hexdigest().encode("ascii") != key_lines[1]:
        return 18
    canonical = (
        "domain=halofpx.rpc-response-boundary.v1|version=1|event=1|side=server"
        "|phase=handler_entry|opcode=25|parent_uid=0|split_uid=27"
        "|exec_sequence=1|backend_ordinal=0"
        f"|attempt={'11' * 32}|connection_epoch={'22' * 32}"
        "|expected=0|actual=0|rc=1|errno=0|eof=0|status=0"
        "|wall_ns=100|mono_ns=50"
    )
    tag = hmac.new(key, DOMAIN + canonical.encode("ascii"), hashlib.sha256).hexdigest()
    output = Path(args.output)
    fd = os.open(output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    try:
        os.write(fd, f"{canonical}|tag={tag}\n".encode("ascii"))
        os.fsync(fd)
    finally:
        os.close(fd)
    directory = os.open(output.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(directory)
    finally:
        os.close(directory)
    return 19


if __name__ == "__main__":
    raise SystemExit(main())
