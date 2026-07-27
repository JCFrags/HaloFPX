#!/usr/bin/env python3
"""Durably stage one bounded RPC response-boundary stream before cleanup."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import stat


MAX_BYTES = 65536


def fsync_directory(path: Path) -> None:
    flags = os.O_RDONLY | getattr(os, "O_DIRECTORY", 0)
    fd = os.open(path, flags)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def capture(source: Path, staging: Path, expected_owner: str) -> dict[str, object]:
    result: dict[str, object] = {
        "schema": "halofpx.rpc-response-harvest.v1",
        "source": str(source),
        "staging": str(staging),
    }
    if not source.is_absolute() or not staging.is_absolute() or source == staging:
        return {**result, "status": "error", "reason": "path_authority"}
    try:
        owner = __import__("pwd").getpwnam(expected_owner)
    except (ImportError, KeyError):
        return {**result, "status": "error", "reason": "owner_authority"}
    try:
        parent_stat = staging.parent.stat(follow_symlinks=False)
    except OSError:
        return {**result, "status": "error", "reason": "staging_parent_missing"}
    if (
        not stat.S_ISDIR(parent_stat.st_mode)
        or parent_stat.st_uid != owner.pw_uid
        or stat.S_IMODE(parent_stat.st_mode) != 0o700
    ):
        return {**result, "status": "error", "reason": "staging_parent_authority"}
    flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
    try:
        source_fd = os.open(source, flags)
    except FileNotFoundError:
        return {**result, "status": "missing", "reason": "source_absent"}
    except OSError:
        return {**result, "status": "error", "reason": "source_open"}
    try:
        metadata = os.fstat(source_fd)
        if (
            not stat.S_ISREG(metadata.st_mode)
            or metadata.st_uid != owner.pw_uid
            or stat.S_IMODE(metadata.st_mode) != 0o600
            or not 1 <= metadata.st_size <= MAX_BYTES
        ):
            return {**result, "status": "error", "reason": "source_authority"}
        data = bytearray()
        while len(data) < metadata.st_size:
            chunk = os.read(source_fd, metadata.st_size - len(data))
            if not chunk:
                break
            data.extend(chunk)
        if len(data) != metadata.st_size or os.read(source_fd, 1):
            return {**result, "status": "error", "reason": "source_short_or_changed"}
    finally:
        os.close(source_fd)
    digest = hashlib.sha256(data).hexdigest()
    try:
        staging_fd = os.open(
            staging,
            os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0),
            0o600,
        )
    except OSError:
        return {**result, "status": "error", "reason": "staging_create"}
    try:
        offset = 0
        while offset < len(data):
            written = os.write(staging_fd, data[offset:])
            if written <= 0:
                return {
                    **result, "status": "error", "reason": "staging_write",
                    "copyable": False,
                }
            offset += written
        os.fsync(staging_fd)
    except OSError:
        return {
            **result, "status": "error", "reason": "staging_write_or_fsync",
            "copyable": False,
        }
    finally:
        os.close(staging_fd)
    try:
        fsync_directory(staging.parent)
    except OSError:
        return {
            **result,
            "status": "error",
            "reason": "staging_directory_fsync",
            "copyable": True,
            "bytes": len(data),
            "sha256": digest,
            "mode": "0600",
            "owner": expected_owner,
        }
    return {
        **result,
        "status": "present",
        "reason": "captured",
        "copyable": True,
        "bytes": len(data),
        "sha256": digest,
        "mode": "0600",
        "owner": expected_owner,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--staging", required=True)
    parser.add_argument("--expected-owner", required=True)
    args = parser.parse_args()
    result = capture(Path(args.source), Path(args.staging), args.expected_owner)
    print(json.dumps(result, sort_keys=True, separators=(",", ":")))
    return 0 if result["status"] in {"present", "missing"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
