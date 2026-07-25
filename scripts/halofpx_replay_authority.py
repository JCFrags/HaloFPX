#!/usr/bin/env python3
"""Fail-closed verifier for authenticated HaloFPX replay-authority records."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import re
import sys
from pathlib import Path

DOMAIN = b"halofpx.replay-authority.v1"
PREFIX = "[halofpx-replay-authority] "
REQUIRED = {
    "phase", "version", "graph_reused", "scheduler_reset", "graph_nodes",
    "output_count", "output_row", "output_swaps", "logits_backend",
    "flash_attention", "backend_count", "kv_type_k", "kv_type_v",
    "kv_v_trans", "kv_n_stream", "kv_prepare_slots", "kv_apply_slots",
    "kv_heads_before", "kv_heads_after", "kv_n", "kv_positions",
    "kv_sequence_ids", "auth_tag",
}
GRAPH_REQUIRED = {
    "graph_input_count", "graph_input_bytes", "node_assignment_count",
    "inactive_graph_input_count", "node_assignment_sha256",
    "cross_backend_edges",
}


def parse_record(line: str, key: bytes) -> dict[str, object]:
    if not line.startswith(PREFIX):
        raise ValueError("wrong record prefix")
    body = line[len(PREFIX):].strip()
    fields: dict[str, object] = {}
    tensors: list[str] = []
    backends: list[str] = []
    views: list[str] = []
    graph_inputs: list[str] = []
    inactive_graph_inputs: list[str] = []
    canonical_parts: list[str] = []
    for part in body.split("|"):
        if "=" not in part:
            raise ValueError("malformed field")
        name, value = part.split("=", 1)
        if name == "auth_tag":
            if name in fields:
                raise ValueError("duplicate auth tag")
            fields[name] = value
            continue
        canonical_parts.append(part)
        if name == "kv_tensor":
            tensors.append(value)
        elif name == "backend":
            backends.append(value)
        elif name == "attention_view":
            views.append(value)
        elif name == "graph_input":
            graph_inputs.append(value)
        elif name == "inactive_graph_input":
            inactive_graph_inputs.append(value)
        elif name in fields:
            raise ValueError(f"duplicate field: {name}")
        else:
            fields[name] = value
    version = fields.get("version")
    required = REQUIRED | (GRAPH_REQUIRED if version == "2" else set())
    missing = required - fields.keys()
    unknown = set(fields) - required
    if missing or unknown:
        raise ValueError(f"field contract mismatch missing={sorted(missing)} unknown={sorted(unknown)}")
    canonical = "|".join(canonical_parts).encode()
    expected = hmac.new(key, DOMAIN + canonical, hashlib.sha256).hexdigest()
    if not hmac.compare_digest(str(fields["auth_tag"]), expected):
        raise ValueError("authentication failure")
    if fields["version"] not in {"1", "2"}:
        raise ValueError("wrong version")
    if fields["kv_prepare_slots"] != fields["kv_apply_slots"]:
        raise ValueError("prepare/apply slot mismatch")
    if not tensors or not backends or not views:
        raise ValueError("missing tensor/backend/view authority")
    if int(str(fields["backend_count"])) != len(backends):
        raise ValueError("backend count mismatch")
    for name in (
        "graph_reused", "scheduler_reset", "flash_attention", "kv_v_trans",
    ):
        if fields[name] not in {"0", "1"}:
            raise ValueError(f"invalid boolean: {name}")
    for name in (
        "graph_nodes", "output_count", "output_swaps", "backend_count",
        "kv_n_stream", "kv_n",
    ):
        if not str(fields[name]).isdigit() or int(str(fields[name])) <= 0:
            if name not in {"output_swaps"} or fields[name] != "0":
                raise ValueError(f"invalid positive integer: {name}")
    if not re.fullmatch(r"-?\d+", str(fields["output_row"])):
        raise ValueError("invalid output row")
    slot_pattern = r"\d+:\d+(?:,\d+)*(?:;\d+:\d+(?:,\d+)*)*"
    if not re.fullmatch(slot_pattern, str(fields["kv_prepare_slots"])):
        raise ValueError("malformed slot authority")
    for name in ("kv_heads_before", "kv_heads_after", "kv_positions",
                 "kv_sequence_ids"):
        if not re.fullmatch(r"-?\d+(?:,-?\d+)*", str(fields[name])):
            raise ValueError(f"malformed numeric vector: {name}")
    backend_indices = []
    for value in backends:
        parts = value.split(",", 1)
        if len(parts) != 2 or not parts[0].isdigit() or not parts[1]:
            raise ValueError("malformed backend authority")
        backend_indices.append(int(parts[0]))
    if backend_indices != list(range(len(backends))):
        raise ValueError("backend order/index mismatch")
    tensor_keys = set()
    for value in tensors:
        parts = value.split(",")
        if len(parts) != 14 or not parts[0].isdigit() or parts[1] not in {"k", "v"}:
            raise ValueError("malformed KV tensor authority")
        if parts[2] not in {"f16", "bf16", "q8_0"}:
            raise ValueError("unsupported KV tensor type")
        if any(not item.isdigit() or int(item) <= 0 for item in parts[3:11]):
            raise ValueError("invalid KV tensor geometry")
        if not parts[11].isdigit() or not parts[12].isdigit() or int(parts[12]) <= 0:
            raise ValueError("invalid KV tensor range")
        if not parts[13]:
            raise ValueError("missing KV tensor backend")
        key_value = (int(parts[0]), parts[1])
        if key_value in tensor_keys:
            raise ValueError("duplicate KV tensor identity")
        tensor_keys.add(key_value)
    view_keys = set()
    for value in views:
        parts = value.split(",")
        if len(parts) != 11 or not parts[0].isdigit() or parts[1] not in {"k", "v"}:
            raise ValueError("malformed attention view authority")
        if any(not item.isdigit() for item in parts[2:]):
            raise ValueError("invalid attention view geometry")
        if any(int(item) <= 0 for item in parts[3:7] + parts[7:]):
            raise ValueError("nonpositive attention view geometry")
        key_value = (int(parts[0]), parts[1])
        if key_value in view_keys:
            raise ValueError("duplicate attention view identity")
        view_keys.add(key_value)
    if tensor_keys != view_keys:
        raise ValueError("tensor/view identity mismatch")
    if fields["version"] == "2":
        admitted = {
            "attn_inp_kq_mask", "attn_inp_k_rot", "attn_inp_v_rot",
            "attn_scale", "inp_embd", "inp_k_idxs", "inp_out_ids", "inp_pos",
            "inp_tokens", "inp_v_idxs",
        }
        if not graph_inputs or int(str(fields["graph_input_count"])) != len(graph_inputs):
            raise ValueError("graph input count mismatch")
        if not re.fullmatch(r"[0-9a-f]{64}", str(fields["node_assignment_sha256"])):
            raise ValueError("malformed node assignment digest")
        if any(not str(fields[name]).isdigit() for name in (
                "graph_input_bytes", "inactive_graph_input_count",
                "node_assignment_count",
                "cross_backend_edges")):
            raise ValueError("malformed graph authority count")
        inactive = int(str(fields["inactive_graph_input_count"]))
        if inactive not in {0, 1} or len(inactive_graph_inputs) != inactive:
            raise ValueError("inactive graph input count mismatch")
        if inactive_graph_inputs and inactive_graph_inputs != ["inp_embd,token-path"]:
            raise ValueError("unknown inactive graph input")
        graph_names = set()
        total_bytes = 0
        for value in graph_inputs:
            parts = value.split(",")
            if len(parts) != 13 or parts[0] not in admitted:
                raise ValueError("unknown or malformed graph input")
            if parts[0] in graph_names:
                raise ValueError("duplicate graph input")
            graph_names.add(parts[0])
            if parts[1] not in {"i32", "i64", "f16", "f32"}:
                raise ValueError("unsupported graph input type")
            if any(not item.isdigit() for item in parts[2:11]):
                raise ValueError("malformed graph input geometry")
            if int(parts[2]) <= 0 or not parts[11] or not re.fullmatch(
                    r"[0-9a-f]{64}", parts[12]):
                raise ValueError("malformed graph input content authority")
            total_bytes += int(parts[2])
        if total_bytes != int(str(fields["graph_input_bytes"])):
            raise ValueError("graph input byte total mismatch")
        fields["graph_input"] = graph_inputs
        fields["inactive_graph_input"] = inactive_graph_inputs
    fields["kv_tensor"] = tensors
    fields["backend"] = backends
    fields["attention_view"] = views
    return fields


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--record")
    parser.add_argument("--key-hex")
    parser.add_argument("--key-file")
    args = parser.parse_args()
    if bool(args.key_hex) == bool(args.key_file):
        parser.error("exactly one of --key-hex or --key-file is required")
    if args.record:
        record = Path(args.record).read_text(encoding="utf-8").strip()
    else:
        record = sys.stdin.read().strip()
    if args.key_file:
        key_text = Path(args.key_file).read_text(
            encoding="ascii").splitlines()[0]
    else:
        key_text = args.key_hex
    key = bytes.fromhex(key_text)
    if len(key) != 32:
        raise SystemExit("key must be exactly 32 bytes")
    fields = parse_record(record, key)
    print(json.dumps(fields, sort_keys=True, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
