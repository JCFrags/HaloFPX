#!/usr/bin/env python3
"""Generate simple byte-level negative GGUF fixtures.

This intentionally does not guess variable-length GGUF structure offsets.
Use upstream tests/test-gguf.cpp for KV and tensor descriptor corruption.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import struct

MAGIC = b"GGUF"

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("destination", type=Path)
    ap.add_argument("operation", choices=["bad-magic","set-version","future-version","truncate","flip"])
    ap.add_argument("--value", type=int)
    ap.add_argument("--bytes-to-remove", type=int, default=1)
    ap.add_argument("--offset", type=str, default="middle")
    ns = ap.parse_args()

    data = bytearray(ns.source.read_bytes())
    if len(data) < 8 or bytes(data[:4]) != MAGIC:
        raise SystemExit("source is not a minimally recognizable GGUF file")

    if ns.operation == "bad-magic":
        data[:4] = b"FUGG"
    elif ns.operation == "set-version":
        if ns.value is None or not 0 <= ns.value <= 0xFFFFFFFF:
            raise SystemExit("--value must be a uint32")
        data[4:8] = struct.pack("<I", ns.value)
    elif ns.operation == "future-version":
        version, = struct.unpack("<I", data[4:8])
        if version == 0xFFFFFFFF:
            raise SystemExit("cannot increment maximum version")
        data[4:8] = struct.pack("<I", version + 1)
    elif ns.operation == "truncate":
        n = ns.bytes_to_remove
        if n <= 0 or n >= len(data):
            raise SystemExit("--bytes-to-remove must retain at least one byte")
        del data[-n:]
    elif ns.operation == "flip":
        off = len(data) // 2 if ns.offset == "middle" else int(ns.offset, 0)
        if not 0 <= off < len(data):
            raise SystemExit("offset outside source")
        data[off] ^= 0x01

    ns.destination.parent.mkdir(parents=True, exist_ok=True)
    ns.destination.write_bytes(data)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
