#!/usr/bin/env python3
"""Verify and compare bounded HaloFPX worker component diagnostics."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


DOMAIN = b"halofpx.rpc-local-state.v1\0"
HEX = r"[0-9a-f]{64}"
COMPONENT = re.compile(
    rf".*\[halofpx-state-diag-component\] phase=(capture|stage|apply) "
    rf"ordinal=(\d+) kind=(\d+) type=(\d+) ne=(\d+),(\d+),(\d+),(\d+) "
    rf"nb=(\d+),(\d+),(\d+),(\d+) view_offset=(\d+) size=(\d+) "
    rf"label_sha256=({HEX}) content_sha256=({HEX}) buffer_group=(\d+) "
    rf"range=(\d+):(\d+) leaf_sha256=({HEX})$"
)
SUMMARY = re.compile(
    rf".*\[halofpx-state-diag\] phase=(capture|stage|apply) components=(\d+) "
    rf"bytes=(\d+) descriptor_content_sha256=({HEX}) merkle_sha256=({HEX}) "
    rf"auth_tag=({HEX})$"
)


class DiagnosticError(RuntimeError):
    pass


def state_hmac(key: bytes, data: bytes) -> bytes:
    if len(key) != 32:
        raise DiagnosticError("control key must be exactly 32 bytes")
    padded = key + bytes(32)
    inner_pad = bytes(value ^ 0x36 for value in padded)
    outer_pad = bytes(value ^ 0x5C for value in padded)
    inner = hashlib.sha256(inner_pad + DOMAIN + data).digest()
    return hashlib.sha256(outer_pad + inner).digest()

def load_control_key(path: Path) -> bytes:
    encoded = path.read_bytes()
    if len(encoded) == 32:
        return encoded
    lines = encoded.splitlines()
    if (
        len(encoded) == 130 and len(lines) == 2
        and all(re.fullmatch(b"[0-9a-f]{64}", line) for line in lines)
    ):
        return bytes.fromhex(lines[0].decode("ascii"))
    raise DiagnosticError("key file does not have the frozen 32-byte or two-line format")


def merkle_root(leaves: list[bytes]) -> bytes:
    if not leaves:
        return hashlib.sha256(b"").digest()
    level = leaves
    while len(level) > 1:
        level = [
            hashlib.sha256(level[index] + level[min(index + 1, len(level) - 1)]).digest()
            for index in range(0, len(level), 2)
        ]
    return level[0]


def component_bytes(record: dict[str, object]) -> bytes:
    descriptor = struct.pack(
        "<QQQQIIII4I4I32s",
        0, 0, int(record["view_offset"]), int(record["size"]),
        int(record["ordinal"]), int(record["kind"]), int(record["type"]), 0,
        *record["ne"], *record["nb"], bytes.fromhex(str(record["label_sha256"])),
    )
    return descriptor + bytes.fromhex(str(record["content_sha256"])) + struct.pack(
        "<IIQQ", int(record["buffer_group"]), 0,
        int(record["range_begin"]), int(record["range_end"]),
    )


def summary_bytes(record: dict[str, object]) -> bytes:
    phase = str(record["phase"]).encode("ascii")[:7].ljust(8, b"\0")
    return struct.pack(
        "<8sIIQ32s32s", phase, int(record["components"]), 0, int(record["bytes"]),
        bytes.fromhex(str(record["aggregate_sha256"])),
        bytes.fromhex(str(record["merkle_sha256"])),
    )


def parse(text: str, key: bytes) -> dict[str, dict[str, object]]:
    phases: dict[str, dict[str, object]] = {}
    for line in text.splitlines():
        match = COMPONENT.fullmatch(line)
        if match:
            values = match.groups()
            phase = values[0]
            record: dict[str, object] = {
                "ordinal": int(values[1]), "kind": int(values[2]), "type": int(values[3]),
                "ne": [int(value) for value in values[4:8]],
                "nb": [int(value) for value in values[8:12]],
                "view_offset": int(values[12]), "size": int(values[13]),
                "label_sha256": values[14], "content_sha256": values[15],
                "buffer_group": int(values[16]), "range_begin": int(values[17]),
                "range_end": int(values[18]), "leaf_sha256": values[19],
            }
            if hashlib.sha256(component_bytes(record)).hexdigest() != record["leaf_sha256"]:
                raise DiagnosticError(f"{phase}: component leaf mismatch at ordinal {record['ordinal']}")
            phase_record = phases.setdefault(phase, {"components": [], "summary": None})
            phase_record["components"].append(record)
            continue
        match = SUMMARY.fullmatch(line)
        if match:
            phase, count, byte_count, aggregate, merkle, tag = match.groups()
            phase_record = phases.setdefault(phase, {"components": [], "summary": None})
            if phase_record["summary"] is not None:
                raise DiagnosticError(f"{phase}: duplicate summary")
            summary = {
                "phase": phase, "components": int(count), "bytes": int(byte_count),
                "aggregate_sha256": aggregate, "merkle_sha256": merkle, "auth_tag": tag,
            }
            if state_hmac(key, summary_bytes(summary)).hex() != tag:
                raise DiagnosticError(f"{phase}: summary authentication failed")
            phase_record["summary"] = summary
            continue
        if "[halofpx-state-diag-component]" in line or "[halofpx-state-diag]" in line:
            raise DiagnosticError("malformed or ambiguous diagnostic record")
    if set(phases) != {"capture", "stage", "apply"}:
        raise DiagnosticError(f"incomplete phases: {sorted(phases)}")
    for phase, record in phases.items():
        components = record["components"]
        summary = record["summary"]
        if summary is None:
            raise DiagnosticError(f"{phase}: missing summary")
        components.sort(key=lambda item: item["ordinal"])
        if [item["ordinal"] for item in components] != list(range(len(components))):
            raise DiagnosticError(f"{phase}: ordinal sequence is incomplete or ambiguous")
        if summary["components"] != len(components):
            raise DiagnosticError(f"{phase}: component count mismatch")
        if summary["bytes"] != sum(item["size"] for item in components):
            raise DiagnosticError(f"{phase}: byte count mismatch")
        leaves = [bytes.fromhex(item["leaf_sha256"]) for item in components]
        aggregate = hashlib.sha256(
            b"".join(component_bytes(item)[:144] for item in components)
        ).hexdigest()
        if aggregate != summary["aggregate_sha256"]:
            raise DiagnosticError(f"{phase}: descriptor/content aggregate mismatch")
        if merkle_root(leaves).hex() != summary["merkle_sha256"]:
            raise DiagnosticError(f"{phase}: Merkle root mismatch")
        for group in sorted({item["buffer_group"] for item in components}):
            ranges = sorted(
                (item["range_begin"], item["range_end"], item["ordinal"])
                for item in components if item["buffer_group"] == group
            )
            for index, (begin, end, ordinal) in enumerate(ranges):
                if begin >= end:
                    raise DiagnosticError(f"{phase}: empty/reversed range at ordinal {ordinal}")
                if index and begin < ranges[index - 1][1]:
                    raise DiagnosticError(
                        f"{phase}: overlapping ranges at ordinals {ranges[index - 1][2]} and {ordinal}")
    return phases


def identity(component: dict[str, object]) -> tuple[object, ...]:
    return (
        component["ordinal"], component["kind"], component["type"],
        *component["ne"], *component["nb"], component["view_offset"],
        component["size"], component["label_sha256"],
    )


def compare(phases: dict[str, dict[str, object]], key: bytes) -> dict[str, object]:
    result: dict[str, object] = {"schema": "halofpx.state-component-diagnostic.v1", "phases": {}}
    for phase in ("capture", "stage", "apply"):
        summary = phases[phase]["summary"]
        result["phases"][phase] = {
            "components": summary["components"], "bytes": summary["bytes"],
            "aggregate_sha256": summary["aggregate_sha256"],
            "merkle_sha256": summary["merkle_sha256"],
            "auth_tag": summary["auth_tag"],
        }
    mismatches = []
    for left, right in (("capture", "stage"), ("stage", "apply")):
        left_components = phases[left]["components"]
        right_components = phases[right]["components"]
        if len(left_components) != len(right_components):
            mismatches.append({"boundary": f"{left}_to_{right}", "reason": "component_count"})
            continue
        for first, second in zip(left_components, right_components):
            if identity(first) != identity(second):
                mismatches.append({
                    "boundary": f"{left}_to_{right}", "ordinal": first["ordinal"],
                    "reason": "component_identity",
                })
                break
            if first["content_sha256"] != second["content_sha256"]:
                mismatches.append({
                    "boundary": f"{left}_to_{right}", "ordinal": first["ordinal"],
                    "reason": "content", "kind": first["kind"], "type": first["type"],
                    "size": first["size"], "label_sha256": first["label_sha256"],
                    "left_content_sha256": first["content_sha256"],
                    "right_content_sha256": second["content_sha256"],
                })
                break
    result["mismatches"] = mismatches
    canonical = json.dumps(result, sort_keys=True, separators=(",", ":")).encode()
    result["report_auth_tag"] = state_hmac(key, canonical).hex()
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--key-file", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    key = load_control_key(args.key_file)
    report = compare(parse(args.log.read_text(encoding="utf-8"), key), key)
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(encoded, encoding="utf-8")
    else:
        print(encoded, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
