#!/usr/bin/env python3
"""Create or verify the authenticated L28 capture-epoch audit sidecar."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import re
from pathlib import Path


SCHEMA = "halofpx.l28.capture-epoch-auth.v1"


def read_key(path: Path) -> bytes:
    stat = path.stat(follow_symlinks=False)
    if not path.is_file() or stat.st_mode & 0o077 or stat.st_size != 130:
        raise ValueError("channel key authority mismatch")
    data = path.read_bytes()
    if not re.fullmatch(rb"[0-9a-f]{64}\n[0-9a-f]{64}\n", data):
        raise ValueError("channel key format mismatch")
    return data


def payload(
        object_digest: str, worker_pid: int, worker_invocation_id: str,
        coordinator_pid: int) -> dict[str, object]:
    if not re.fullmatch(r"[0-9a-f]{64}", object_digest):
        raise ValueError("object digest mismatch")
    if not re.fullmatch(r"[0-9a-f]{32}", worker_invocation_id):
        raise ValueError("worker InvocationID mismatch")
    if worker_pid <= 0 or coordinator_pid <= 0:
        raise ValueError("PID authority mismatch")
    return {
        "schema": SCHEMA,
        "object_sha256": object_digest,
        "worker_pid": worker_pid,
        "worker_invocation_id": worker_invocation_id,
        "coordinator_pid": coordinator_pid,
    }


def canonical(value: dict[str, object]) -> bytes:
    return json.dumps(
        value, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("ascii")


def seal(value: dict[str, object], key: bytes) -> dict[str, object]:
    return {
        "payload": value,
        "tag": hmac.new(key, canonical(value), hashlib.sha256).hexdigest(),
    }


def verify(
        receipt: dict[str, object], expected: dict[str, object], key: bytes) -> None:
    if set(receipt) != {"payload", "tag"} or receipt["payload"] != expected:
        raise ValueError("capture epoch receipt payload mismatch")
    tag = receipt["tag"]
    if not isinstance(tag, str) or not hmac.compare_digest(
            tag, hmac.new(key, canonical(expected), hashlib.sha256).hexdigest()):
        raise ValueError("capture epoch receipt authentication mismatch")


def atomic_write(path: Path, value: dict[str, object]) -> None:
    if not path.is_absolute() or path.exists():
        raise ValueError("receipt output must be a fresh absolute path")
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("x", encoding="utf-8", newline="\n") as output:
        json.dump(value, output, indent=2, sort_keys=True)
        output.write("\n")
        output.flush()
        os.fsync(output.fileno())
    os.replace(temporary, path)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("operation", choices=("create", "verify"))
    parser.add_argument("--key", required=True, type=Path)
    parser.add_argument("--receipt", required=True, type=Path)
    parser.add_argument("--object-digest", required=True)
    parser.add_argument("--worker-pid", required=True, type=int)
    parser.add_argument("--worker-invocation-id", required=True)
    parser.add_argument("--coordinator-pid", required=True, type=int)
    args = parser.parse_args()
    expected = payload(
        args.object_digest, args.worker_pid, args.worker_invocation_id,
        args.coordinator_pid)
    key = read_key(args.key)
    if args.operation == "create":
        atomic_write(args.receipt, seal(expected, key))
    else:
        receipt = json.loads(args.receipt.read_text(encoding="utf-8"))
        verify(receipt, expected, key)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
