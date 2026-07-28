#!/usr/bin/env python3
"""Verify immutable L67 pre-execute attempt files and exact terminal productions."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import struct
import tempfile
from collections import defaultdict
from pathlib import Path


DOMAIN = b"halofpx.preexecute-authority.v1"
RECORD_SIZE = 236
EVENT_COUNT = 12


def take(fmt: str, data: bytes, offset: int) -> tuple[tuple[int, ...], int]:
    size = struct.calcsize(fmt)
    return struct.unpack_from(fmt, data, offset), offset + size


def decode(data: bytes) -> dict[str, object]:
    if len(data) != RECORD_SIZE:
        raise ValueError(f"record_size:{len(data)}")
    offset = 0
    (major, minor), offset = take("<HH", data, offset)
    (event, reason, transport, opcode), offset = take("<IIII", data, offset)
    values, offset = take("<QQQQQQQQ", data, offset)
    sequence, generation, execution, parent, split_uid, client_epoch, server_epoch, allocation_epoch = values
    (backend, split_ordinal), offset = take("<II", data, offset)
    (expected, actual), offset = take("<QQ", data, offset)
    (error_number, terminal_branch, expected_register, expected_exclude), offset = take("<IIII", data, offset)
    counts, offset = take("<" + "I" * EVENT_COUNT, data, offset)
    nonce = data[offset : offset + 32]
    prior = data[offset + 32 : offset + 64]
    if offset + 64 != len(data):
        raise ValueError("record_layout")
    return {
        "major": major, "minor": minor, "event": event, "reason": reason,
        "transport": transport, "opcode": opcode, "sequence": sequence,
        "generation": generation, "execution": execution, "parent": parent,
        "split_uid": split_uid, "client_epoch": client_epoch,
        "server_epoch": server_epoch, "allocation_epoch": allocation_epoch,
        "backend": backend, "split_ordinal": split_ordinal,
        "expected": expected, "actual": actual, "error_number": error_number,
        "terminal_branch": terminal_branch, "expected_register": expected_register,
        "expected_exclude": expected_exclude, "counts": counts,
        "nonce": nonce, "prior": prior, "raw": data,
    }


def transport_transition(prior: int, nxt: int) -> bool:
    allowed = {
        1: {0}, 2: {0, 11, 14}, 3: {2}, 4: {3}, 5: {4},
        6: {5}, 7: {6}, 8: {7}, 9: {8}, 10: {9}, 11: {10},
        12: {2, 4, 6, 8, 10}, 13: {2, 4, 6, 8, 10}, 14: {9, 11},
    }
    return prior in allowed.get(nxt, set())


FULL_TRANSPORT = (2, 3, 4, 5, 6, 7, 8, 9, 10, 11)
FAILURE_TRANSPORT = (
    (2, 13),
    (2, 3, 4, 13),
    (2, 3, 4, 5, 6, 13),
    (2, 3, 4, 5, 6, 7, 8, 13),
    (2, 3, 4, 5, 6, 7, 8, 9, 10, 13),
    FULL_TRANSPORT + (2, 13),
    FULL_TRANSPORT + (2, 3, 4, 13),
    FULL_TRANSPORT + (2, 3, 4, 5, 6, 13),
    FULL_TRANSPORT + (2, 3, 4, 5, 6, 7, 8, 13),
)
PRODUCTIONS = (
    (1, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE",
         "DECISION", "TRANSPORT_COMPLETE", "TRANSPORT_COMPLETE", "COMMIT", "END")),
    (2, ("BEGIN", "L42", "L44", "ABORT")),
    (4, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN",
         "PREPARE", "ABORT")),
    (4, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "COMMIT", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_0", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_1", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_2", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_3", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_4", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_5", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_6", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_7", "ABORT")),
    (3, ("BEGIN", "L42", "L44", "REGISTER_PLAN", "EXCLUDE_PLAN", "PREPARE", "DECISION", "TRANSPORT_FAIL_8", "ABORT")),
)


def production_matches(
        production: tuple[str, ...],
        records: list[dict[str, object]],
        registers: int,
        excludes: int) -> bool:
    event_index = 0
    transport_index = 0
    event_for = {
        "BEGIN": 1, "L42": 2, "L44": 3, "REGISTER_PLAN": 4,
        "EXCLUDE_PLAN": 5, "PREPARE": 6, "COMMIT": 7,
        "DECISION": 8, "ABORT": 10, "END": 11,
    }
    for token in production:
        if token == "REGISTER_PLAN":
            count = registers
        elif token == "EXCLUDE_PLAN":
            count = excludes
        elif token == "TRANSPORT_COMPLETE":
            count = 10
            states = FULL_TRANSPORT
        elif token.startswith("TRANSPORT_FAIL_"):
            states = FAILURE_TRANSPORT[int(token.rsplit("_", 1)[1])]
            count = len(states)
        else:
            count = 1
        event = 9 if token.startswith("TRANSPORT_") else event_for[token]
        if event_index + count > len(records) or any(
                record["event"] != event
                for record in records[event_index:event_index + count]):
            return False
        if event == 9:
            observed = tuple(
                record["transport"]
                for record in records[event_index:event_index + count])
            if observed != states:
                return False
            transport_index += count
        event_index += count
    return event_index == len(records) and \
        transport_index == sum(record["event"] == 9 for record in records)


def terminal_valid(records: list[dict[str, object]]) -> bool:
    last = records[-1]
    branch = last["terminal_branch"]
    registers = last["expected_register"]
    excludes = last["expected_exclude"]
    prepare_index = next(
        (index for index, record in enumerate(records) if record["event"] == 6),
        len(records),
    )
    if any(record["expected_register"] != 0 or record["expected_exclude"] != 0
           for record in records[:prepare_index]) or \
       any(record["expected_register"] != registers or
           record["expected_exclude"] != excludes
           for record in records[prepare_index:]):
        return False
    matches = [
        production for production_branch, production in PRODUCTIONS
        if production_branch == branch and
        production_matches(production, records, registers, excludes)
    ]
    if len(matches) != 1:
        return False
    if branch == 1:
        transports = [record for record in records if record["event"] == 9]
        return transports[0]["opcode"] in (23, 24) and transports[10]["opcode"] == 25
    return True


def verify(path: Path, key: bytes) -> dict[str, object]:
    streams: dict[tuple[bytes, int, int], list[dict[str, object]]] = defaultdict(list)
    files = sorted(path.glob("*.authority")) if path.is_dir() else [path]
    if not files:
        raise ValueError("records_missing")
    sourced_lines: list[tuple[Path, int, str]] = []
    for source in files:
        sourced_lines.extend(
            (source, number, line)
            for number, line in enumerate(source.read_text(encoding="ascii").splitlines(), 1)
        )
    for source, line_number, line in sourced_lines:
        fields = dict(field.split("=", 1) for field in line.split("|"))
        if fields.get("domain") != DOMAIN.decode():
            raise ValueError(f"line_{line_number}:domain")
        raw = bytes.fromhex(fields["record"])
        tag = bytes.fromhex(fields["tag"])
        if not hmac.compare_digest(hmac.new(key, DOMAIN + raw, hashlib.sha256).digest(), tag):
            raise ValueError(f"line_{line_number}:hmac")
        record = decode(raw)
        if record["major"] != 1 or record["minor"] != 0:
            raise ValueError(f"line_{line_number}:version")
        record["tag"] = tag
        record["line"] = f"{source.name}:{line_number}"
        # Parent UID is intentionally populated only after the L42 admission
        # event; generation plus the connection-scoped recorder identity is
        # stable for the complete attempt.
        identity = (record["nonce"], record["generation"], record["client_epoch"])
        streams[identity].append(record)

    terminal = 0
    abort = 0
    transport_commands = 0
    for records in streams.values():
        prior = bytes(32)
        counts = [0] * EVENT_COUNT
        phases: dict[int, int] = defaultdict(int)
        for sequence, record in enumerate(records, 1):
            if record["sequence"] != sequence or record["prior"] != prior:
                raise ValueError(f"line_{record['line']}:chain_or_sequence")
            event = record["event"]
            if not 1 <= event < EVENT_COUNT:
                raise ValueError(f"line_{record['line']}:event")
            counts[event] += 1
            if tuple(counts) != record["counts"]:
                raise ValueError(f"line_{record['line']}:cardinality")
            if event == 9:
                opcode = record["opcode"]
                if opcode == 0 or not transport_transition(phases[opcode], record["transport"]):
                    raise ValueError(f"line_{record['line']}:transport_order")
                phases[opcode] = record["transport"]
                transport_commands += record["transport"] == 2
            prior = record["tag"]
            if record["terminal_branch"] and sequence != len(records):
                raise ValueError(f"line_{record['line']}:post_terminal")
        last = records[-1]
        if not last["terminal_branch"]:
            raise ValueError(f"line_{last['line']}:unterminated")
        if not terminal_valid(records):
            raise ValueError(f"line_{last['line']}:terminal_grammar")
        terminal += 1
        abort += last["terminal_branch"] != 1
    return {
        "schema": "halofpx.preexecute-verification.v2",
        "status": "pass",
        "files": len(files),
        "records": len(sourced_lines),
        "attempts": len(streams),
        "terminal_attempts": terminal,
        "aborted_attempts": abort,
        "transport_commands": transport_commands,
    }


def structural_self_test(path: Path, key: bytes) -> dict[str, object]:
    source = sorted(path.glob("*.authority"))[0]
    lines = source.read_text(encoding="ascii").splitlines()
    if len(lines) < 6:
        raise ValueError("self_test_fixture")
    variants = {
        "unterminated": lines[:-1],
        "extra": lines + [lines[0]],
        "missing": lines[:3] + lines[4:],
        "reordered": lines[:3] + [lines[4], lines[3]] + lines[5:],
        "post_terminal": lines + [lines[-1]],
    }
    fields = dict(field.split("=", 1) for field in lines[3].split("|"))
    raw = bytearray.fromhex(fields["record"])
    struct.pack_into("<I", raw, 4, 0xFFFFFFFF)
    fields["record"] = raw.hex()
    fields["tag"] = hmac.new(key, DOMAIN + raw, hashlib.sha256).hexdigest()
    variants["unknown"] = ["|".join(f"{name}={value}" for name, value in fields.items())]
    refused: list[str] = []
    with tempfile.TemporaryDirectory(prefix="halofpx-l67-grammar-") as root:
        for name, content in variants.items():
            candidate = Path(root, f"{name}.authority")
            candidate.write_text("\n".join(content) + "\n", encoding="ascii")
            try:
                verify(candidate, key)
            except (KeyError, ValueError):
                refused.append(name)
    if set(refused) != set(variants):
        raise ValueError(f"structural_acceptance:{sorted(set(variants) - set(refused))}")
    return {
        "schema": "halofpx.preexecute-structural-self-test.v1",
        "status": "pass",
        "refused": sorted(refused),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--records", type=Path, required=True)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--key-hex")
    group.add_argument("--key-file", type=Path)
    parser.add_argument("--structural-self-test", action="store_true")
    args = parser.parse_args()
    key_hex = args.key_hex
    if args.key_file:
        key_hex = args.key_file.read_text(encoding="ascii").splitlines()[0]
    key = bytes.fromhex(key_hex)
    if len(key) != 32 or not any(key):
        raise ValueError("key")
    result = structural_self_test(args.records, key) if args.structural_self_test \
        else verify(args.records, key)
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
