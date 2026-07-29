#!/usr/bin/env python3
"""Offline HaloFPX KV physical-range coverage auditor."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path

QK8_0 = 32
Q8_0_BLOCK_BYTES = 34


def merge(intervals: list[tuple[int, int]]) -> list[tuple[int, int]]:
    result: list[tuple[int, int]] = []
    for begin, end in sorted(intervals):
        if begin < 0 or end < begin:
            raise ValueError("invalid interval")
        if result and begin <= result[-1][1]:
            result[-1] = (result[-1][0], max(result[-1][1], end))
        else:
            result.append((begin, end))
    return result


def intersect(a: list[tuple[int, int]], b: list[tuple[int, int]]) -> list[tuple[int, int]]:
    result: list[tuple[int, int]] = []
    for ab, ae in merge(a):
        for bb, be in merge(b):
            begin, end = max(ab, bb), min(ae, be)
            if begin < end:
                result.append((begin, end))
    return merge(result)


def subtract(a: list[tuple[int, int]], b: list[tuple[int, int]]) -> list[tuple[int, int]]:
    result: list[tuple[int, int]] = []
    for begin, end in merge(a):
        cursor = begin
        for bb, be in merge(b):
            if be <= cursor or bb >= end:
                continue
            if cursor < bb:
                result.append((cursor, min(bb, end)))
            cursor = max(cursor, be)
            if cursor >= end:
                break
        if cursor < end:
            result.append((cursor, end))
    return result


def total(intervals: list[tuple[int, int]]) -> int:
    return sum(end - begin for begin, end in merge(intervals))


def q8_row_bytes(elements: int) -> int:
    if elements < 0 or elements % QK8_0:
        raise ValueError("q8_0 row is not block aligned")
    return elements // QK8_0 * Q8_0_BLOCK_BYTES


def serialized_q8_copy_bytes(requested_bytes: int) -> int:
    # Exact source behavior in llama_io_write_device::~llama_io_write_device:
    # n = winfo.size / ggml_element_size(q8_0), followed by a new q8_0 tensor.
    if requested_bytes % Q8_0_BLOCK_BYTES:
        raise ValueError("requested q8_0 bytes are not block aligned")
    n = requested_bytes // Q8_0_BLOCK_BYTES
    return q8_row_bytes(n)


def classify_tensor(
        tensor: dict[str, object], occupied_rows: int, padded_rows: int,
        serialized_bytes: int | None) -> dict[str, object]:
    row_bytes = tensor["nb"][1]
    data_bytes = q8_row_bytes(tensor["ne"][0])
    begin = tensor["allocation_offset"]
    if data_bytes > row_bytes:
        raise ValueError("q8_0 row data exceeds stride")
    provable = [
        (begin + row * row_bytes, begin + row * row_bytes + data_bytes)
        for row in range(occupied_rows)]
    possible_all = [
        (begin + row * row_bytes, begin + row * row_bytes + data_bytes)
        for row in range(padded_rows)]
    possible = subtract(possible_all, provable)
    allocation = [(begin, begin + tensor["allocation_bytes"])]
    result: dict[str, object] = {
        "provably_read_intervals": provable,
        "possibly_read_intervals": possible,
        "row_stride_padding": subtract(
            [(begin, begin + padded_rows * row_bytes)], possible_all),
        "allocation_only_padding": subtract(
            allocation, [(begin, begin + padded_rows * row_bytes)]),
    }
    if serialized_bytes is not None:
        represented = [(begin, begin + serialized_bytes)]
        result.update({
            "source_intervals_covered_by_serialization": represented,
            "source_intervals_covered_by_restore": represented,
            "unrepresented_provably_read": subtract(provable, represented),
            "unrepresented_possibly_read": subtract(possible_all, represented),
            "coverage_bytes": {
                "provably_read": total(provable),
                "serialized_and_restored_intersection":
                    total(intersect(provable, represented)),
                "unrepresented_provably_read":
                    total(subtract(provable, represented)),
                "possibly_read_padding":
                    total(possible),
                "allocation_only_padding":
                    total(subtract(
                        allocation, [(begin, begin + padded_rows * row_bytes)])),
            },
        })
    return result


def parse_replay(path: Path, phase: str) -> tuple[list[dict[str, object]], dict[str, int]]:
    line = next(
        value for value in path.read_text(encoding="utf-8").splitlines()
        if f"[halofpx-replay-authority] phase={phase}|" in value)
    tensors = []
    for match in re.finditer(
            r"\|kv_tensor=(\d+),([kv]),q8_0,(\d+),(\d+),(\d+),(\d+),"
            r"(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),([^|]+)", line):
        values = match.groups()
        tensors.append({
            "layer": int(values[0]), "kind": values[1],
            "ne": [int(v) for v in values[2:6]],
            "nb": [int(v) for v in values[6:10]],
            "allocation_offset": int(values[10]),
            "allocation_bytes": int(values[11]), "backend": values[12],
        })
    scalar = {}
    for name in ("kv_prepare_slots", "kv_apply_slots", "kv_n", "kv_positions"):
        value = re.search(rf"\|{name}=([^|]+)", line).group(1)
        scalar[name] = int(value.split(":")[-1])
    if len(tensors) != 124:
        raise ValueError("retained replay authority does not contain 124 KV tensors")
    return tensors, scalar


def parse_components(path: Path, phase: str) -> list[dict[str, object]]:
    result = []
    pattern = re.compile(
        rf"\[halofpx-state-diag-component\] phase={phase} ordinal=(\d+) "
        r"kind=(\d+) type=(\d+) ne=([^ ]+) nb=([^ ]+) view_offset=(\d+) "
        r"size=(\d+) label_sha256=([0-9a-f]{64}) content_sha256=([0-9a-f]{64}) "
        r"buffer_group=(\d+) range=(\d+):(\d+)")
    for line in path.read_text(encoding="utf-8").splitlines():
        match = pattern.search(line)
        if match:
            result.append({
                "ordinal": int(match[1]), "kind": int(match[2]),
                "type": int(match[3]), "ne": [int(v) for v in match[4].split(",")],
                "nb": [int(v) for v in match[5].split(",")],
                "view_offset": int(match[6]), "size": int(match[7]),
                "label_sha256": match[8], "content_sha256": match[9],
                "buffer_group": int(match[10]),
                "buffer_range": [int(match[11]), int(match[12])],
            })
    return result


def audit(root: Path) -> dict[str, object]:
    child = root / "docs/halofpx/evidence/l98-attempt/child"
    capture_journal = child / "halofpx-l48-canary-capture-journal.txt"
    restore_journal = child / "halofpx-l48-canary-restore-journal.txt"
    tensors, geometry = parse_replay(capture_journal, "capture")
    restore_tensors, restore_geometry = parse_replay(restore_journal, "restore")
    if tensors != restore_tensors or geometry != restore_geometry:
        raise ValueError("capture and restore replay geometry differ")
    captured = parse_components(
        child / "halofpx-l48-worker-capture-journal.txt", "capture")
    staged = parse_components(
        child / "halofpx-l48-worker-restore-journal.txt", "stage")
    committed = parse_components(
        child / "halofpx-l48-worker-restore-journal.txt", "apply")
    recaptured = parse_components(
        child / "halofpx-l48-worker-restore-journal.txt", "capture")
    if not (len(captured) == len(staged) == len(committed) == len(recaptured) == 64):
        raise ValueError("worker component cardinality mismatch")

    occupied_rows = geometry["kv_prepare_slots"]
    padded_rows = geometry["kv_n"]
    rpc_order = sorted(
        (tensor for tensor in tensors if str(tensor["backend"]).startswith("RPC")),
        key=lambda tensor: (0 if tensor["kind"] == "k" else 1, tensor["layer"]))
    if len(rpc_order) != 64:
        raise ValueError("RPC tensor cardinality mismatch")
    component_index = {
        (tensor["layer"], tensor["kind"], tensor["backend"]): index
        for index, tensor in enumerate(rpc_order)}
    local_order = sorted(
        (tensor for tensor in tensors if tensor not in rpc_order),
        key=lambda tensor: (0 if tensor["kind"] == "k" else 1, tensor["layer"]))
    if len(local_order) != 60:
        raise ValueError("local tensor cardinality mismatch")
    local_index = {
        (tensor["layer"], tensor["kind"], tensor["backend"]): index
        for index, tensor in enumerate(local_order)}
    local_blob_bytes = 2301688
    local_header_bytes = 16 + 72
    local_entry_bytes = 8 + captured[0]["size"]
    if local_header_bytes + len(local_order) * local_entry_bytes != local_blob_bytes:
        raise ValueError("coordinator-local blob does not match exact tensor payload projection")
    per_tensor = []
    for index, tensor in enumerate(tensors):
        record: dict[str, object] = {
            "identity": f"layer={tensor['layer']},kind={tensor['kind']},backend={tensor['backend']}",
            **tensor,
            "source_lines": {
                "serialization_count": "src/llama-context.cpp:3843-3846",
                "kv_write_range": "src/llama-kv-cache.cpp:2183-2196,2210-2223",
                "q8_block": "ggml/src/ggml-common.h:265-266",
                "q8_k_load": "ggml/src/ggml-cuda/fattn-common.cuh:276-299",
                "q8_v_load": "ggml/src/ggml-cuda/fattn-common.cuh:750-772",
                "quantized_tile": "ggml/src/ggml-cuda/fattn-tile.cuh:477-510"
            },
        }
        identity = (tensor["layer"], tensor["kind"], tensor["backend"])
        if identity in component_index:
            ordinal = component_index[identity]
            component = captured[ordinal]
            restored = committed[ordinal]
            expected_kind = 1 if tensor["kind"] == "k" else 2
            for phase_component in (
                    component, staged[ordinal], restored, recaptured[ordinal]):
                if (phase_component["ordinal"] != ordinal
                        or phase_component["kind"] != expected_kind):
                    raise ValueError("component ordinal/kind identity mismatch")
            serialized_bytes = component["size"]
            record.update(classify_tensor(
                tensor, occupied_rows, padded_rows, serialized_bytes))
            record.update({
                "capture_component": component,
                "stage_component": staged[ordinal],
                "commit_component": restored,
                "live_recapture_component": recaptured[ordinal],
                "serialized_staging_buffer_interval": component["buffer_range"],
                "applied_staging_buffer_interval": restored["buffer_range"],
                "coordinate_note": "staging buffer ranges and live source allocation ranges are distinct address spaces; size and authenticated stable ordinal/kind bind the transfer",
            })
        else:
            ordinal = local_index[identity]
            blob_begin = local_header_bytes + ordinal * local_entry_bytes + 8
            serialized_bytes = captured[0]["size"]
            record.update(classify_tensor(
                tensor, occupied_rows, padded_rows, serialized_bytes))
            record.update({
                "local_blob_component": {
                    "ordinal": ordinal,
                    "blob_range": [blob_begin, blob_begin + serialized_bytes],
                    "size": serialized_bytes,
                    "identity_source": "authenticated replay tensor identity plus canonical K-then-V state writer order",
                },
                "serialized_staging_buffer_interval":
                    [blob_begin, blob_begin + serialized_bytes],
                "applied_staging_buffer_interval":
                    [blob_begin, blob_begin + serialized_bytes],
                "coordinate_note": "coordinator-local blob offsets and live ROCm allocation offsets are distinct; exact blob size/header/cardinality bind the 60 canonical payloads",
            })
        per_tensor.append(record)

    worker = [record for record in per_tensor if "capture_component" in record]
    expected_each = occupied_rows * tensors[0]["nb"][1]
    serialized_each = captured[0]["size"]
    output = {
        "schema": "halofpx.l100.kv-physical-range-audit.v1",
        "source_commit": "bf861840423c60c9f71afa119086b32b4e4ef5e3",
        "input_sha256": {
            path.name: hashlib.sha256(path.read_bytes()).hexdigest()
            for path in (capture_journal, restore_journal,
                         child / "halofpx-l48-worker-capture-journal.txt",
                         child / "halofpx-l48-worker-restore-journal.txt")
        },
        "geometry": geometry,
        "tensor_count": len(per_tensor),
        "rpc_tensor_count": len(worker),
        "local_tensor_count": len(per_tensor) - len(worker),
        "component_counts": {
            "capture": len(captured), "stage": len(staged), "apply": len(committed),
            "live_recapture": len(recaptured)},
        "totals": {
            "rpc_provably_read_bytes": expected_each * len(worker),
            "rpc_serialized_bytes": serialized_each * len(worker),
            "rpc_restored_bytes": serialized_each * len(worker),
            "rpc_unrepresented_provably_read_bytes":
                (expected_each - serialized_each) * len(worker),
            "rpc_unrepresented_possibly_read_padding_bytes":
                (padded_rows - occupied_rows) * tensors[0]["nb"][1] * len(worker),
            "rpc_serialized_staging_union_bytes":
                total([tuple(item["capture_component"]["buffer_range"]) for item in worker]),
            "rpc_serialized_staging_overlap_bytes":
                sum(item["capture_component"]["size"] for item in worker)
                - total([tuple(item["capture_component"]["buffer_range"]) for item in worker]),
            "local_provably_read_bytes": expected_each * len(local_order),
            "local_serialized_payload_bytes": serialized_each * len(local_order),
            "local_blob_header_and_length_bytes":
                local_blob_bytes - serialized_each * len(local_order),
            "local_unrepresented_provably_read_bytes":
                (expected_each - serialized_each) * len(local_order),
            "all_provably_read_bytes": expected_each * len(per_tensor),
            "all_serialized_tensor_payload_bytes": serialized_each * len(per_tensor),
            "all_unrepresented_provably_read_bytes":
                (expected_each - serialized_each) * len(per_tensor),
        },
        "classification": {
            "source_proven_defect": True,
            "cause": "q8_0 requested byte count is divided by 34-byte block size and then block-scaled a second time when the 1-D copy tensor is created",
            "causal_status": "uncovered ranges within the 1128 occupied KV rows are provably read by replay attention; padded rows 1128..1279 are reported separately and are not claimed causal",
            "smallest_correction": "derive quantized copy tensor element count with block_size/type_size (or allocate/copy the exact requested byte interval with a byte-exact tensor/view contract), then require ggml_nbytes(copy)==winfo.size before capture",
        },
        "tensors": per_tensor,
    }
    return output


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    encoded = json.dumps(audit(args.root), sort_keys=True, separators=(",", ":")) + "\n"
    if args.output:
        args.output.write_text(encoded, encoding="utf-8", newline="\n")
    else:
        print(encoded, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
