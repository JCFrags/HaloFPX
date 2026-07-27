#!/usr/bin/env python3
"""Verify bounded L58 RPC response-boundary records without exposing key bytes."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import re
import stat
from pathlib import Path


DOMAIN = b"halofpx.rpc-response-boundary.v1"
PHASES = {
    "request_opcode", "request_header", "request_body", "response_header",
    "response_size_mismatch", "response_body", "handler_entry",
    "backend_complete", "handler_exit", "response_header_publish",
    "response_body_publish", "handler_validation", "client_decode",
    "client_receipt_validation", "receipt_construction",
}
SIDES = {"client", "server"}
HEX64 = re.compile(r"[0-9a-f]{64}")
MAX_FILE_BYTES = 65536
MAX_LINE_BYTES = 2048


def load_key(path: Path) -> bytes:
    metadata = path.stat()
    expected_uid = os.geteuid() if hasattr(os, "geteuid") else metadata.st_uid
    if (
        not path.is_absolute()
        or not stat.S_ISREG(metadata.st_mode)
        or metadata.st_uid != expected_uid
        or (os.name != "nt" and stat.S_IMODE(metadata.st_mode) != 0o600)
        or metadata.st_size != 130
    ):
        raise ValueError("key authority mismatch")
    lines = path.read_text(encoding="ascii").splitlines()
    if len(lines) != 2 or any(HEX64.fullmatch(line) is None for line in lines):
        raise ValueError("key format mismatch")
    return bytes.fromhex(lines[0])


def parse_record(line: str, key: bytes) -> dict[str, object]:
    if not 1 <= len(line.encode("ascii")) <= MAX_LINE_BYTES:
        raise ValueError("record line size is outside the closed bound")
    fields = line.rstrip("\n").split("|")
    if len(fields) != 21 or fields[-1].split("=", 1)[0] != "tag":
        raise ValueError("record field count/order mismatch")
    expected_names = [
        "domain", "version", "event", "side", "phase", "opcode", "parent_uid",
        "split_uid", "exec_sequence", "backend_ordinal", "attempt",
        "connection_epoch", "expected", "actual", "rc", "errno", "eof",
        "status", "wall_ns", "mono_ns", "tag",
    ]
    parsed: dict[str, str] = {}
    for expected, field in zip(expected_names, fields, strict=True):
        name, separator, value = field.partition("=")
        if separator != "=" or name != expected or name in parsed:
            raise ValueError("record grammar mismatch")
        parsed[name] = value
    if (
        parsed["domain"] != DOMAIN.decode("ascii")
        or parsed["version"] != "1"
        or parsed["side"] not in SIDES
        or parsed["phase"] not in PHASES
        or HEX64.fullmatch(parsed["attempt"]) is None
        or HEX64.fullmatch(parsed["connection_epoch"]) is None
        or HEX64.fullmatch(parsed["tag"]) is None
    ):
        raise ValueError("record authority mismatch")
    numeric = (
        "event", "opcode", "parent_uid", "split_uid", "exec_sequence",
        "backend_ordinal", "expected", "actual", "status", "wall_ns", "mono_ns",
    )
    signed_numeric = ("rc", "errno")
    for name in numeric:
        if not parsed[name].isdecimal():
            raise ValueError(f"{name} is not canonical unsigned decimal")
    for name in signed_numeric:
        if re.fullmatch(r"-?(?:0|[1-9][0-9]*)", parsed[name]) is None:
            raise ValueError(f"{name} is not canonical signed decimal")
    if parsed["eof"] not in {"0", "1"}:
        raise ValueError("EOF authority mismatch")
    canonical = "|".join(fields[:-1]).encode("ascii")
    expected_tag = hmac.new(key, DOMAIN + canonical, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(expected_tag, parsed["tag"]):
        raise ValueError("record authentication failed")
    return {
        **parsed,
        **{name: int(parsed[name]) for name in numeric + signed_numeric + ("eof",)},
    }


def validate_stream(records: list[dict[str, object]], side: str) -> None:
    phases = [str(record["phase"]) for record in records]
    if side == "client":
        required = ["request_opcode", "request_header", "request_body", "response_header"]
        if phases[:len(required)] != required:
            raise ValueError("client request/response-header lifecycle mismatch")
        if records[3]["rc"] == 0:
            if len(records) != 4:
                raise ValueError("client response-header failure has trailing events")
        elif len(records) == 5 and phases[4] == "response_size_mismatch":
            if records[4]["rc"] != 0:
                raise ValueError("response-size mismatch reports success")
        else:
            if len(records) < 5 or phases[4] != "response_body":
                raise ValueError("client response-body lifecycle mismatch")
            if records[4]["rc"] == 0:
                if len(records) != 5:
                    raise ValueError("client body failure has trailing events")
            elif len(records) < 6 or phases[5] != "client_decode":
                raise ValueError("client decode lifecycle mismatch")
            elif records[5]["rc"] == 0:
                if len(records) != 6:
                    raise ValueError("client decode failure has trailing events")
            elif phases[6:] != ["client_receipt_validation"] or len(records) != 7:
                raise ValueError("client receipt-validation lifecycle mismatch")
        for record in records:
            phase = str(record["phase"])
            expected = int(record["expected"])
            actual = int(record["actual"])
            rc = int(record["rc"])
            if phase == "response_size_mismatch":
                if rc != 0 or expected == actual:
                    raise ValueError("response-size mismatch authority is inconsistent")
            elif phase in {"client_decode", "client_receipt_validation"}:
                if expected != 0 or actual != 0 or rc not in {0, 1}:
                    raise ValueError("client validation authority is inconsistent")
            elif actual > expected or (rc == 1 and actual != expected):
                raise ValueError("client transport byte authority is inconsistent")
    else:
        required = ["handler_entry", "handler_validation"]
        if phases[:2] != required:
            raise ValueError("server handler lifecycle mismatch")
        if records[1]["rc"] == 0:
            if len(records) != 2:
                raise ValueError("server handler refusal has trailing events")
        elif len(records) == 2:
            # Admissible only as the retained prefix when the process exits
            # during backend compute; unit/journal authority classifies exit.
            return
        else:
            suffix = [
                "backend_complete", "receipt_construction", "handler_exit",
                "response_header_publish", "response_body_publish",
            ]
            if phases[2:] != suffix[:len(phases) - 2]:
                raise ValueError("server completion/publication lifecycle mismatch")
            if len(records) > 7:
                raise ValueError("server lifecycle has extra events")
            for index, record in enumerate(records[2:], start=2):
                if int(record["rc"]) == 0 and index != len(records) - 1:
                    raise ValueError("server failure has trailing events")
        for record in records:
            phase = str(record["phase"])
            expected = int(record["expected"])
            actual = int(record["actual"])
            rc = int(record["rc"])
            if phase in {
                    "handler_entry", "handler_validation", "backend_complete",
                    "receipt_construction", "handler_exit"}:
                if expected != 0 or actual != 0 or rc not in {0, 1}:
                    raise ValueError("server handler authority is inconsistent")
            elif actual > expected or (rc == 1 and actual != expected):
                raise ValueError("server publication byte authority is inconsistent")
    for record in records:
        if bool(record["eof"]) and (
                record["rc"] != 0 or record["errno"] != 0):
            raise ValueError("EOF authority is inconsistent")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--key-file", required=True)
    parser.add_argument("--record-file", action="append", required=True)
    args = parser.parse_args()
    key = load_key(Path(args.key_file))
    output: dict[str, list[dict[str, object]]] = {}
    for value in args.record_file:
        path = Path(value)
        if path.stat().st_size > MAX_FILE_BYTES:
            raise SystemExit("record file size is outside the closed bound")
        lines = path.read_text(encoding="ascii").splitlines()
        if not 1 <= len(lines) <= 64:
            raise SystemExit("record count is outside the closed bound")
        records = [parse_record(line, key) for line in lines]
        if [record["event"] for record in records] != list(range(1, len(records) + 1)):
            raise SystemExit("record event sequence is missing, duplicate, or out of order")
        sides = {str(record["side"]) for record in records}
        if len(sides) != 1:
            raise SystemExit("record file mixes client and server authority")
        side = next(iter(sides))
        if side in output:
            raise SystemExit("duplicate side record file")
        validate_stream(records, side)
        output[side] = records
    if set(output) != SIDES:
        raise SystemExit("exactly one client and one server stream are required")
    client = output["client"]
    server = output["server"]
    for field in (
            "split_uid", "exec_sequence", "backend_ordinal", "attempt",
            "connection_epoch", "opcode"):
        values = {
            str(record[field]) for record in client + server
        }
        if len(values) != 1:
            raise SystemExit(f"cross-side {field} authority mismatch")
    if any(record["parent_uid"] != 0 for record in server):
        raise SystemExit("server-local parent UID must be canonical zero")
    if len({str(record["parent_uid"]) for record in client}) != 1 or client[0]["parent_uid"] == 0:
        raise SystemExit("client parent UID authority is missing or inconsistent")
    print(json.dumps(output, sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
