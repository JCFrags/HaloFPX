#!/usr/bin/env python3
"""Sign or verify the bounded L48 composed execution result."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import re
import stat
import sys
from pathlib import Path


SCHEMA = "halofpx.l48.composed-result.v1"
DOMAIN = b"halofpx.l48.composed-result.v1\0"
INNER_DOMAIN = b"halofpx.composed-result.v1"
MAX_RECORD_BYTES = 262144
HEX64 = re.compile(r"[0-9a-f]{64}")
EXECUTION_REQUIRED = {
    "phase", "ordinal", "execution_sequence", "graph_uid", "prepared_root",
    "split_mapping_root",
    "prepared_status", "final_status", "scheduler_root", "scheduler_tag",
    "graph_entries", "splits", "rpc_split_count", "copies", "local", "rpc", "mutable_sessions",
    "mutable_status", "mutable_census", "set",
    "set_hash_hit", "set_hash_miss", "mutation_root", "semantic_root",
    "census_root", "receipt_tag", "graph_status", "graph_sequence",
    "graph_digest", "graph_transcript_root", "graph_receipt_tag",
    "parent_uid", "split_ordinal", "split_uid", "reconcile_status",
    "rpc_splits",
}


class ResultError(ValueError):
    pass


def _read_key(path: Path, expected_digest: str, expected_owner: str) -> bytes:
    st = path.lstat()
    if not stat.S_ISREG(st.st_mode) or stat.S_IMODE(st.st_mode) != 0o600:
        raise ResultError("key file must be a regular mode-0600 file")
    raw = path.read_bytes()
    if len(raw) != 130:
        raise ResultError("key file size mismatch")
    lines = raw.splitlines()
    if len(lines) != 2 or any(not re.fullmatch(rb"[0-9a-f]{64}", line) for line in lines):
        raise ResultError("key file format mismatch")
    if HEX64.fullmatch(expected_digest) is None or not hmac.compare_digest(
            hashlib.sha256(raw).hexdigest(), expected_digest):
        raise ResultError("key file digest mismatch")
    if os.name != "nt":
        import pwd
        if pwd.getpwuid(st.st_uid).pw_name != expected_owner:
            raise ResultError("key file owner mismatch")
    return bytes.fromhex(lines[0].decode("ascii"))


def _canonical(payload: object) -> bytes:
    return json.dumps(
        payload, sort_keys=True, separators=(",", ":"), ensure_ascii=True,
    ).encode("ascii")


def _execution(record: object, expected_phase: str, ordinal: int) -> None:
    if not isinstance(record, dict) or set(record) != EXECUTION_REQUIRED:
        raise ResultError("execution field set mismatch")
    if record["phase"] != expected_phase or record["ordinal"] != ordinal:
        raise ResultError("execution phase/order mismatch")
    if type(record["execution_sequence"]) is not int or record["execution_sequence"] <= 0:
        raise ResultError("execution sequence malformed")
    for name in (
        "graph_uid", "graph_entries", "splits", "rpc_split_count", "copies", "local", "rpc",
        "mutable_sessions", "mutable_census", "set", "set_hash_hit", "set_hash_miss",
        "prepared_status", "final_status", "mutable_status", "graph_status",
        "graph_sequence", "parent_uid", "split_ordinal", "split_uid",
        "reconcile_status",
    ):
        if type(record[name]) is not int or record[name] < 0:
            raise ResultError(f"{name} malformed")
    for name in (
        "prepared_root", "split_mapping_root", "scheduler_root", "scheduler_tag", "mutation_root",
        "semantic_root", "census_root", "receipt_tag",
        "graph_digest", "graph_transcript_root", "graph_receipt_tag",
    ):
        if not isinstance(record[name], str) or HEX64.fullmatch(record[name]) is None:
            raise ResultError(f"{name} malformed")
    if (
        record["rpc"] <= 0 or record["mutable_sessions"] <= 0
        or record["mutable_census"] <= 0 or record["set"] <= 0
        or record["set_hash_hit"] + record["set_hash_miss"] <= 0
        or record["prepared_status"] != 1 or record["final_status"] != 1
        or record["mutable_status"] != 1 or record["graph_status"] != 2
        or record["graph_sequence"] != record["execution_sequence"]
        or record["parent_uid"] != record["graph_uid"]
        or record["split_uid"] <= 0 or record["reconcile_status"] != 1
    ):
        raise ResultError("RPC mutable authority is incomplete")
    rpc_splits = record["rpc_splits"]
    if (
        not isinstance(rpc_splits, list) or not rpc_splits
        or len(rpc_splits) != record["rpc_split_count"]
        or record["rpc_split_count"] > record["splits"]
    ):
        raise ResultError("RPC split authority is missing")
    expected_split_fields = {
        "backend_ordinal", "parent_uid", "split_ordinal", "split_uid",
        "reconcile_status", "graph_status", "graph_sequence", "graph_digest",
        "graph_transcript_root", "graph_receipt_tag",
    }
    ordinals: list[int] = []
    uids: set[int] = set()
    for split in rpc_splits:
        if not isinstance(split, dict) or set(split) != expected_split_fields:
            raise ResultError("RPC split authority field set mismatch")
        if (
            split["parent_uid"] != record["graph_uid"]
            or split["graph_sequence"] != record["execution_sequence"]
            or split["reconcile_status"] != 1 or split["graph_status"] != 2
            or type(split["backend_ordinal"]) is not int
            or split["backend_ordinal"] < 0 or split["backend_ordinal"] >= 64
            or type(split["split_uid"]) is not int or split["split_uid"] <= 0
            or type(split["split_ordinal"]) is not int or split["split_ordinal"] < 0
        ):
            raise ResultError("RPC split authority mismatch")
        for name in ("graph_digest", "graph_transcript_root", "graph_receipt_tag"):
            if not isinstance(split[name], str) or HEX64.fullmatch(split[name]) is None:
                raise ResultError("RPC split digest malformed")
        ordinals.append(split["split_ordinal"])
        if split["split_uid"] in uids:
            raise ResultError("RPC split UID is duplicate")
        uids.add(split["split_uid"])
    if ordinals != sorted(ordinals) or len(set(ordinals)) != len(ordinals):
        raise ResultError("RPC split order is invalid")
    first = rpc_splits[0]
    for scalar, split_name in (
        ("parent_uid", "parent_uid"),
        ("split_ordinal", "split_ordinal"),
        ("split_uid", "split_uid"),
        ("reconcile_status", "reconcile_status"),
        ("graph_status", "graph_status"),
        ("graph_sequence", "graph_sequence"),
        ("graph_digest", "graph_digest"),
        ("graph_transcript_root", "graph_transcript_root"),
        ("graph_receipt_tag", "graph_receipt_tag"),
    ):
        if record[scalar] != first[split_name]:
            raise ResultError("RPC split compatibility authority differs")


def validate(payload: object) -> dict[str, object]:
    if not isinstance(payload, dict) or set(payload) != {
        "schema", "attempt", "lineage", "features", "feature_off", "capture", "restore",
        "prompt_chunks", "replay_count", "tokens", "logits", "legacy_state_get_set",
    }:
        raise ResultError("result field set mismatch")
    if payload["schema"] != SCHEMA or HEX64.fullmatch(str(payload["attempt"])) is None:
        raise ResultError("schema/attempt mismatch")
    lineage = payload["lineage"]
    if (
        not isinstance(lineage, dict)
        or set(lineage) != {"capture_worker_invocation", "restore_worker_invocation"}
        or any(re.fullmatch(r"[0-9a-f]{32}", str(value)) is None for value in lineage.values())
        or lineage["capture_worker_invocation"] == lineage["restore_worker_invocation"]
    ):
        raise ResultError("lineage authority malformed")
    expected_attempt = hashlib.sha256(
        (lineage["capture_worker_invocation"] + "|" +
         lineage["restore_worker_invocation"]).encode("ascii")).hexdigest()
    if payload["attempt"] != expected_attempt:
        raise ResultError("attempt does not bind epoch lineage")
    if payload["features"] != {
        "rpc_graph": 1, "scheduler": 2, "mutable_session": 1, "composition": 1,
    } or payload["feature_off"] is not False:
        raise ResultError("feature/version mismatch")
    if payload["prompt_chunks"] != [512, 512, 104] or payload["replay_count"] != 1:
        raise ResultError("prompt/replay structure mismatch")
    capture = payload["capture"]
    restore = payload["restore"]
    if not isinstance(capture, list) or len(capture) != 4:
        raise ResultError("capture execution count mismatch")
    if not isinstance(restore, list) or len(restore) != 1:
        raise ResultError("restore execution count mismatch")
    for index, record in enumerate(capture):
        _execution(record, "capture", index)
    _execution(restore[0], "restore", 0)
    if [record["execution_sequence"] for record in capture] != [1, 2, 3, 4]:
        raise ResultError("capture execution sequence mismatch")
    if restore[0]["execution_sequence"] != 1:
        raise ResultError("restore execution sequence mismatch")
    capture_replay = capture[-1]
    restore_replay = restore[0]
    phase_neutral = (
        "graph_entries", "splits", "rpc_split_count", "copies", "local", "rpc",
        "mutable_sessions", "mutable_census", "set", "set_hash_hit",
        "set_hash_miss", "semantic_root", "census_root", "graph_digest",
    )
    if any(capture_replay[name] != restore_replay[name] for name in phase_neutral):
        raise ResultError("capture/restore phase-neutral authority differs")
    split_phase_neutral = ("backend_ordinal", "split_ordinal", "graph_digest")
    if any(
            left[name] != right[name]
            for left, right in zip(
                capture_replay["rpc_splits"], restore_replay["rpc_splits"])
            for name in split_phase_neutral):
        raise ResultError("capture/restore RPC split authority differs")
    if payload["tokens"] != {"capture": 4245, "restore": 4245}:
        raise ResultError("token authority mismatch")
    logits = payload["logits"]
    if not isinstance(logits, dict) or set(logits) != {"capture", "restore"}:
        raise ResultError("logits authority malformed")
    if any(not isinstance(value, str) or HEX64.fullmatch(value) is None for value in logits.values()):
        raise ResultError("logits digest malformed")
    if logits["capture"] != logits["restore"]:
        raise ResultError("logits authority differs")
    if payload["legacy_state_get_set"] != 0:
        raise ResultError("legacy state transfer observed")
    encoded = _canonical(payload)
    if len(encoded) > MAX_RECORD_BYTES:
        raise ResultError("result exceeds hard bound")
    return payload


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("sign", "verify", "verify-inner"))
    parser.add_argument("--key-file", required=True, type=Path)
    parser.add_argument("--record", type=Path)
    parser.add_argument("--expected-tag")
    parser.add_argument("--expected-key-sha256", required=True)
    parser.add_argument("--expected-owner", required=True)
    args = parser.parse_args()
    try:
        key = _read_key(args.key_file, args.expected_key_sha256, args.expected_owner)
        if args.command == "verify-inner":
            raw = sys.stdin.buffer.read(MAX_RECORD_BYTES + 1)
            if len(raw) > MAX_RECORD_BYTES or not raw:
                raise ResultError("inner result exceeds hard bound or is empty")
            if args.expected_tag is None or HEX64.fullmatch(args.expected_tag) is None:
                raise ResultError("inner expected tag is malformed")
            expected = hmac.new(key, INNER_DOMAIN + raw, hashlib.sha256).hexdigest()
            if not hmac.compare_digest(args.expected_tag, expected):
                raise ResultError("inner result authentication failed")
            print(expected)
            return 0
        if args.command == "sign":
            raw = sys.stdin.buffer.read(MAX_RECORD_BYTES + 1)
            if len(raw) > MAX_RECORD_BYTES:
                raise ResultError("result exceeds hard bound")
            payload = validate(json.loads(raw))
            print(hmac.new(key, DOMAIN + _canonical(payload), hashlib.sha256).hexdigest())
            return 0
        if args.record is None:
            raise ResultError("--record is required for verify")
        st = args.record.lstat()
        if not stat.S_ISREG(st.st_mode) or stat.S_IMODE(st.st_mode) != 0o600:
            raise ResultError("record must be a regular mode-0600 file")
        raw = args.record.read_bytes()
        if len(raw) > MAX_RECORD_BYTES:
            raise ResultError("record exceeds hard bound")
        envelope = json.loads(raw)
        if not isinstance(envelope, dict) or set(envelope) != {"payload", "auth_tag"}:
            raise ResultError("record envelope mismatch")
        payload = validate(envelope["payload"])
        tag = envelope["auth_tag"]
        expected = hmac.new(key, DOMAIN + _canonical(payload), hashlib.sha256).hexdigest()
        if not isinstance(tag, str) or not hmac.compare_digest(tag, expected):
            raise ResultError("record authentication failed")
        print(json.dumps(payload, sort_keys=True, separators=(",", ":")))
        return 0
    except (OSError, ResultError, json.JSONDecodeError) as exc:
        print(f"L48 result refused: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
