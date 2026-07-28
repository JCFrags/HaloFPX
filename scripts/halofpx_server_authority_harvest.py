#!/usr/bin/env python3
"""Authenticate and durably stage one server-owned pre-execute authority."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
from pathlib import Path
import stat
import struct


DOMAIN = b"halofpx.preexecute-authority.v1"
MAX_BYTES = 65536
RECORD_SIZE = 280
PRODUCTIONS = (
    (1, 3, 4, 5, 6, 7),
    (2, 8),
    (1, 8),
    (1, 3, 8),
    (1, 3, 4, 8),
)


def _decode_graph_key(key_file: bytes) -> bytes:
    if len(key_file) != 130:
        raise ValueError("key_format")
    first, second, trailing = key_file.split(b"\n")
    if trailing or len(first) != 64 or len(second) != 64:
        raise ValueError("key_format")
    lowercase_hex = b"0123456789abcdef"
    if any(value not in lowercase_hex for value in first + second):
        raise ValueError("key_format")
    key = bytes.fromhex(first.decode("ascii"))
    if len(key) != 32 or key == bytes(32):
        raise ValueError("key_format")
    return key


def _validated_graph_key(key_file: bytes, expected_sha256: str) -> bytes:
    if hashlib.sha256(key_file).hexdigest() != expected_sha256:
        raise ValueError("key_digest")
    return _decode_graph_key(key_file)


def _fsync_directory(path: Path) -> None:
    fd = os.open(path, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


class SourceAbsent(FileNotFoundError):
    pass


def _open_source(path: Path) -> int:
    try:
        return os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
    except FileNotFoundError as exc:
        raise SourceAbsent(str(path)) from exc


def _key_authority_valid(metadata: os.stat_result, expected_uid: int) -> bool:
    return (
        stat.S_ISREG(metadata.st_mode)
        and metadata.st_uid == expected_uid
        and stat.S_IMODE(metadata.st_mode) == 0o600
        and metadata.st_size == 130
    )


def _read_exact_unchanged(fd: int, expected_size: int) -> bytes:
    data = b""
    while len(data) < expected_size:
        chunk = os.read(fd, expected_size - len(data))
        if not chunk:
            break
        data += chunk
    if len(data) != expected_size or os.read(fd, 1):
        raise ValueError("key_short_or_changed")
    return data


def _decode(raw: bytes) -> dict[str, object]:
    if len(raw) != RECORD_SIZE:
        raise ValueError("record_size")
    values = struct.unpack_from("<HHIIIIQQQQQQQIII", raw)
    offset = struct.calcsize("<HHIIIIQQQQQQQIII")
    names = (
        "major", "minor", "role", "event", "reason", "terminal_branch",
        "record_sequence", "generation", "execution_sequence", "parent_uid",
        "split_uid", "connection_epoch", "allocation_epoch", "backend_ordinal",
        "split_ordinal", "receipt_state",
    )
    result: dict[str, object] = dict(zip(names, values))
    for name in (
        "attempt_nonce", "admission_object_id", "expected_admission_digest",
        "graph_digest", "execute_receipt", "prior_tag",
    ):
        result[name] = raw[offset:offset + 32]
        offset += 32
    return result


def _verify(data: bytes, key: bytes, expected: dict[str, object]) -> dict[str, object]:
    records: list[dict[str, object]] = []
    prior = bytes(32)
    for number, line in enumerate(data.decode("ascii").splitlines(), 1):
        fields = dict(field.split("=", 1) for field in line.split("|"))
        if (
            fields.get("domain") != DOMAIN.decode()
            or fields.get("role") != "server"
            or fields.get("grammar") != "1.0"
        ):
            raise ValueError(f"line_{number}:envelope")
        raw = bytes.fromhex(fields["record"])
        tag = bytes.fromhex(fields["tag"])
        if not hmac.compare_digest(
            hmac.new(key, DOMAIN + raw, hashlib.sha256).digest(), tag
        ):
            raise ValueError(f"line_{number}:hmac")
        record = _decode(raw)
        if (
            record["major"] != 1
            or record["minor"] != 0
            or record["role"] != 2
            or record["record_sequence"] != number
            or record["prior_tag"] != prior
        ):
            raise ValueError(f"line_{number}:chain")
        prior = tag
        records.append(record)
    if not records:
        raise ValueError("records_missing")
    first = records[0]
    binding_names = (
        "generation", "execution_sequence", "parent_uid", "split_uid",
        "connection_epoch", "allocation_epoch", "backend_ordinal",
        "split_ordinal", "receipt_state", "attempt_nonce",
        "admission_object_id", "expected_admission_digest", "graph_digest",
        "execute_receipt",
    )
    if any(record[name] != first[name] for record in records for name in binding_names):
        raise ValueError("cross_binding")
    events = tuple(int(record["event"]) for record in records)
    matches = [index + 1 for index, production in enumerate(PRODUCTIONS)
               if events == production]
    if matches != [int(records[-1]["terminal_branch"])] or any(
        int(record["terminal_branch"]) != matches[0] for record in records
    ):
        raise ValueError("terminal_grammar")
    if matches[0] == 1 and (
        first["receipt_state"] != 1
        or first["execute_receipt"] == bytes(32)
    ):
        raise ValueError("success_receipt")
    exact = {
        "attempt_nonce": bytes.fromhex(str(expected["attempt"])),
        "admission_object_id": bytes.fromhex(str(expected["admission"])),
        "execution_sequence": int(expected["sequence"]),
        "split_uid": int(expected["split_uid"]),
        "split_ordinal": int(expected["split_ordinal"]),
        "backend_ordinal": int(expected["backend"]),
    }
    if any(first[name] != value for name, value in exact.items()):
        raise ValueError("journal_cross_binding")
    return {
        "terminal_branch": matches[0],
        "records": len(records),
        "execute_receipt": bytes(first["execute_receipt"]).hex(),
        "expected_admission_digest":
            bytes(first["expected_admission_digest"]).hex(),
        "graph_digest": bytes(first["graph_digest"]).hex(),
    }


def harvest(
    source: Path, staging: Path, key_file: Path, expected_owner: str,
    expected: dict[str, object], expected_key_sha256: str,
) -> dict[str, object]:
    base = {
        "schema": "halofpx.server-authority-harvest.v1",
        "source": str(source), "staging": str(staging),
    }
    try:
        import pwd
        owner = pwd.getpwnam(expected_owner)
        if not source.is_absolute() or not staging.is_absolute() or source == staging:
            raise ValueError("path_authority")
        parent = staging.parent.stat(follow_symlinks=False)
        if (
            not stat.S_ISDIR(parent.st_mode)
            or parent.st_uid != owner.pw_uid
            or stat.S_IMODE(parent.st_mode) != 0o700
        ):
            raise ValueError("staging_parent_authority")
        fd = _open_source(source)
        try:
            metadata = os.fstat(fd)
            if (
                not stat.S_ISREG(metadata.st_mode)
                or metadata.st_uid != owner.pw_uid
                or stat.S_IMODE(metadata.st_mode) != 0o400
                or not 1 <= metadata.st_size <= MAX_BYTES
            ):
                raise ValueError("source_authority")
            data = b""
            while len(data) < metadata.st_size:
                chunk = os.read(fd, metadata.st_size - len(data))
                if not chunk:
                    break
                data += chunk
            if len(data) != metadata.st_size or os.read(fd, 1):
                raise ValueError("source_short_or_changed")
        finally:
            os.close(fd)
        digest = hashlib.sha256(data).hexdigest()
        if digest != expected["sha256"]:
            raise ValueError("journal_hash")
        key_fd = os.open(
            key_file, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
        try:
            key_metadata = os.fstat(key_fd)
            if not _key_authority_valid(key_metadata, owner.pw_uid):
                raise ValueError("key_authority")
            key = _read_exact_unchanged(key_fd, key_metadata.st_size)
        finally:
            os.close(key_fd)
        graph_key = _validated_graph_key(key, expected_key_sha256)
        verification = _verify(data, graph_key, expected)
        out = os.open(
            staging,
            os.O_WRONLY | os.O_CREAT | os.O_EXCL |
            getattr(os, "O_NOFOLLOW", 0),
            0o600,
        )
        try:
            offset = 0
            while offset < len(data):
                count = os.write(out, data[offset:])
                if count <= 0:
                    raise OSError("short staging write")
                offset += count
            os.fsync(out)
        finally:
            os.close(out)
        _fsync_directory(staging.parent)
        reopened = staging.read_bytes()
        if reopened != data:
            raise ValueError("staging_reopen")
        return {
            **base, "status": "present", "reason": "authenticated",
            "owner": expected_owner, "source_mode": "0400",
            "staging_mode": "0600", "bytes": len(data), "sha256": digest,
            **verification,
        }
    except SourceAbsent:
        return {**base, "status": "missing", "reason": "source_absent"}
    except FileNotFoundError:
        return {**base, "status": "error", "reason": "harvest_dependency_missing"}
    except (KeyError, OSError, UnicodeError, ValueError) as exc:
        return {**base, "status": "error", "reason": str(exc)}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--staging", required=True, type=Path)
    parser.add_argument("--key-file", required=True, type=Path)
    parser.add_argument("--expected-owner", required=True)
    parser.add_argument("--expected-attempt", required=True)
    parser.add_argument("--expected-admission", required=True)
    parser.add_argument("--expected-sequence", required=True, type=int)
    parser.add_argument("--expected-split-uid", required=True, type=int)
    parser.add_argument("--expected-split-ordinal", required=True, type=int)
    parser.add_argument("--expected-backend", required=True, type=int)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--expected-key-sha256", required=True)
    args = parser.parse_args()
    expected = {
        "attempt": args.expected_attempt, "admission": args.expected_admission,
        "sequence": args.expected_sequence, "split_uid": args.expected_split_uid,
        "split_ordinal": args.expected_split_ordinal, "backend": args.expected_backend,
        "sha256": args.expected_sha256,
    }
    result = harvest(
        args.source, args.staging, args.key_file, args.expected_owner, expected,
        args.expected_key_sha256)
    print(json.dumps(result, sort_keys=True, separators=(",", ":")))
    return 0 if result["status"] in {"present", "missing"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
