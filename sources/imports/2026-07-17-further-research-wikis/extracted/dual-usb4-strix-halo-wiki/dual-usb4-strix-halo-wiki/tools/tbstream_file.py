#!/usr/bin/env python3
"""Send or receive one integrity-checked file over /dev/tbstreamX.

This is a lab tool, not an authenticated or encrypted protocol.
"""
from __future__ import annotations
import argparse, hashlib, os, struct, sys
from pathlib import Path

MAGIC = b"DUSB4SF1"
HEADER = struct.Struct("!8sQ32s")


def write_all(fd: int, data: bytes | memoryview) -> None:
    view = memoryview(data)
    while view:
        n = os.write(fd, view)
        if n <= 0:
            raise OSError("write made no progress")
        view = view[n:]


def read_exact(fd: int, size: int) -> bytes:
    out = bytearray()
    while len(out) < size:
        chunk = os.read(fd, size - len(out))
        if not chunk:
            raise EOFError(f"unexpected EOF after {len(out)} of {size} bytes")
        out.extend(chunk)
    return bytes(out)


def hash_file(path: Path, block: int = 4 << 20) -> tuple[int, bytes]:
    h = hashlib.sha256(); total = 0
    with path.open("rb") as f:
        while data := f.read(block): h.update(data); total += len(data)
    return total, h.digest()


def send(device: str, src: Path) -> None:
    total, digest = hash_file(src)
    fd = os.open(device, os.O_WRONLY)
    try:
        write_all(fd, HEADER.pack(MAGIC, total, digest))
        with src.open("rb") as f:
            while data := f.read(4 << 20):
                write_all(fd, data)
    finally: os.close(fd)
    print(f"sent {total} bytes sha256={digest.hex()}")


def recv(device: str, dst: Path) -> None:
    fd = os.open(device, os.O_RDONLY)
    try:
        magic, total, expected = HEADER.unpack(read_exact(fd, HEADER.size))
        if magic != MAGIC: raise ValueError("bad magic")
        h = hashlib.sha256(); remaining = total
        with dst.open("wb") as f:
            while remaining:
                data = read_exact(fd, min(4 << 20, remaining)); f.write(data); h.update(data); remaining -= len(data)
    finally: os.close(fd)
    actual = h.digest()
    if actual != expected:
        raise SystemExit(f"hash mismatch expected={expected.hex()} actual={actual.hex()}")
    print(f"received {total} bytes sha256={actual.hex()} VERIFIED")


def main() -> None:
    p = argparse.ArgumentParser()
    sub = p.add_subparsers(dest="mode", required=True)
    s = sub.add_parser("send"); s.add_argument("device"); s.add_argument("file", type=Path)
    r = sub.add_parser("recv"); r.add_argument("device"); r.add_argument("file", type=Path)
    a = p.parse_args()
    send(a.device, a.file) if a.mode == "send" else recv(a.device, a.file)

if __name__ == "__main__": main()
