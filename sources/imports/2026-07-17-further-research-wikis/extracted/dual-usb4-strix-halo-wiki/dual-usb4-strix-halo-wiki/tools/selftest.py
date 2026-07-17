#!/usr/bin/env python3
"""Offline artifact self-test: data parsing plus loopback transport integrity."""
from __future__ import annotations

import csv
import hashlib
import json
import os
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def free_port() -> int:
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return int(s.getsockname()[1])


def validate_data() -> None:
    for rel in ("manifest.json", "wiki.json", "data/source-index.json", "data/result-schema.json", "data/validation-environment.json"):
        json.loads((ROOT / rel).read_text(encoding="utf-8"))
    with (ROOT / "data/claims.jsonl").open(encoding="utf-8") as fh:
        claims = [json.loads(line) for line in fh if line.strip()]
    assert claims and len({c["id"] for c in claims}) == len(claims)
    for rel in ("data/kernel-feature-matrix.csv", "data/transports.csv", "data/benchmark-matrix.csv"):
        with (ROOT / rel).open(newline="", encoding="utf-8") as fh:
            rows = list(csv.DictReader(fh))
        assert rows, f"empty CSV: {rel}"


def dual_path_loopback() -> str:
    p0, p1 = free_port(), free_port()
    while p1 == p0:
        p1 = free_port()
    with tempfile.TemporaryDirectory(prefix="dual-usb4-selftest-") as td:
        d = Path(td)
        src, dst = d / "source.bin", d / "received.bin"
        src.write_bytes(os.urandom(2 << 20))
        cmd = [sys.executable, str(ROOT / "tools/dual_path_copy.py")]
        server = subprocess.Popen(
            cmd + ["server", "--bind0", f"127.0.0.1:{p0}", "--bind1", f"127.0.0.1:{p1}", "--output", str(dst)],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        try:
            time.sleep(0.25)
            client = subprocess.run(
                cmd + ["client", "--path0", f"127.0.0.1,127.0.0.1:{p0}", "--path1", f"127.0.0.1,127.0.0.1:{p1}", "--input", str(src), "--chunk", "131072", "--queue-depth", "4"],
                check=True, capture_output=True, text=True, timeout=30,
            )
            out, _ = server.communicate(timeout=30)
            if server.returncode != 0:
                raise RuntimeError(out)
        finally:
            if server.poll() is None:
                server.kill()
                server.wait()
        a = hashlib.sha256(src.read_bytes()).hexdigest()
        b = hashlib.sha256(dst.read_bytes()).hexdigest()
        assert a == b
        assert "VERIFIED" in out
        return a



def tbstream_fifo_loopback() -> str:
    """Exercise the raw-stream file framing over a local FIFO."""
    with tempfile.TemporaryDirectory(prefix="tbstream-selftest-") as td:
        d = Path(td)
        fifo = d / "tbstream"
        src, dst = d / "source.bin", d / "received.bin"
        os.mkfifo(fifo)
        src.write_bytes(os.urandom(2 << 20))
        cmd = [sys.executable, str(ROOT / "tools/tbstream_file.py")]
        receiver = subprocess.Popen(
            cmd + ["recv", str(fifo), str(dst)],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        try:
            sender = subprocess.run(
                cmd + ["send", str(fifo), str(src)],
                check=True, capture_output=True, text=True, timeout=30,
            )
            out, _ = receiver.communicate(timeout=30)
            if receiver.returncode != 0:
                raise RuntimeError(out)
        finally:
            if receiver.poll() is None:
                receiver.kill()
                receiver.wait()
        a = hashlib.sha256(src.read_bytes()).hexdigest()
        b = hashlib.sha256(dst.read_bytes()).hexdigest()
        assert a == b
        assert "VERIFIED" in out
        assert "sent" in sender.stdout
        return a

def main() -> int:
    validate_data()
    dual_digest = dual_path_loopback()
    stream_digest = tbstream_fifo_loopback()
    print(
        "data files parsed; "
        f"dual-path loopback verified sha256={dual_digest}; "
        f"tbstream framing loopback verified sha256={stream_digest}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
