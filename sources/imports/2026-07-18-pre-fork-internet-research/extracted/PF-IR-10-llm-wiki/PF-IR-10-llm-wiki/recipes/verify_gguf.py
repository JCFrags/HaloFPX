#!/usr/bin/env python3
"""Independent stdlib-only GGUF structural verifier for PF-IR-10.

SPDX-License-Identifier: CC0-1.0

This verifier does not import or execute any candidate implementation.  It checks
only the format invariants required by the generated corpus and emits stable
error classes for malformed fixtures.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

TYPE_NAMES = {
    0: "F32",
    1: "F16",
    2: "Q4_0",
    3: "Q4_1",
    6: "Q5_0",
    7: "Q5_1",
    8: "Q8_0",
    9: "Q8_1",
    10: "Q2_K",
    11: "Q3_K",
    12: "Q4_K",
    13: "Q5_K",
    14: "Q6_K",
    15: "Q8_K",
    30: "BF16",
    34: "TQ1_0",
    35: "TQ2_0",
    39: "MXFP4",
    40: "NVFP4",
    41: "Q1_0",
    100: "Q4_0_ROCMFP4",
    101: "Q4_0_ROCMFP4_FAST",
    102: "Q6_0_ROCMFPX",
    103: "Q8_0_ROCMFPX",
    104: "Q3_0_ROCMFPX",
    105: "TURBO3_0",
    106: "TURBO4_0",
    107: "Q2_0_ROCMFPX",
}

# (logical values per block, bytes per block).  Only types emitted by the
# generator need exact entries.  F16/BF16 are included for evidence parsing.
TYPE_LAYOUTS = {
    0: (1, 4),
    1: (1, 2),
    30: (1, 2),
    106: (32, 18),
}

META_SCALAR_SIZES = {
    0: ("<B", 1),
    1: ("<b", 1),
    2: ("<H", 2),
    3: ("<h", 2),
    4: ("<I", 4),
    5: ("<i", 4),
    6: ("<f", 4),
    7: ("<?", 1),
    10: ("<Q", 8),
    11: ("<q", 8),
    12: ("<d", 8),
}


class GGUFError(Exception):
    def __init__(self, code: str, message: str, offset: int | None = None):
        super().__init__(message)
        self.code = code
        self.message = message
        self.offset = offset


class Reader:
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0

    def need(self, size: int, code: str = "truncated") -> None:
        if size < 0 or self.pos + size > len(self.data):
            raise GGUFError(code, f"need {size} bytes at offset {self.pos}, file length {len(self.data)}", self.pos)

    def read(self, size: int) -> bytes:
        self.need(size)
        out = self.data[self.pos:self.pos + size]
        self.pos += size
        return out

    def unpack(self, fmt: str) -> Any:
        size = struct.calcsize(fmt)
        return struct.unpack(fmt, self.read(size))[0]

    def string(self) -> tuple[str, int, int]:
        start = self.pos
        length = self.unpack("<Q")
        if length > len(self.data):
            raise GGUFError("invalid-length", f"string length {length} exceeds file length", start)
        raw = self.read(length)
        try:
            value = raw.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise GGUFError("invalid-utf8", f"invalid UTF-8 string: {exc}", start) from exc
        return value, start, self.pos


def align_up(value: int, alignment: int) -> int:
    return value + ((alignment - value % alignment) % alignment)


def read_meta_value(r: Reader, value_type: int, depth: int = 0) -> Any:
    if depth > 8:
        raise GGUFError("metadata-depth", "metadata array nesting exceeds 8", r.pos)
    if value_type in META_SCALAR_SIZES:
        fmt, _ = META_SCALAR_SIZES[value_type]
        return r.unpack(fmt)
    if value_type == 8:  # string
        return r.string()[0]
    if value_type == 9:  # array
        elem_type = r.unpack("<I")
        if elem_type == 9:
            raise GGUFError("invalid-array-type", "GGUF arrays may not contain arrays", r.pos - 4)
        if elem_type not in {*META_SCALAR_SIZES, 8}:
            raise GGUFError("unknown-metadata-type", f"unknown array element type {elem_type}", r.pos - 4)
        count = r.unpack("<Q")
        if count > 10_000_000:
            raise GGUFError("invalid-length", f"array count {count} is unreasonable", r.pos - 8)
        return [read_meta_value(r, elem_type, depth + 1) for _ in range(count)]
    raise GGUFError("unknown-metadata-type", f"unknown metadata value type {value_type}", r.pos - 4)


@dataclass
class TensorInfo:
    name: str
    dims: list[int]
    ggml_type: int
    relative_offset: int
    info_start: int
    info_end: int


def expected_tensor_bytes(tensor: TensorInfo) -> int | None:
    layout = TYPE_LAYOUTS.get(tensor.ggml_type)
    if layout is None:
        return None
    block_values, block_bytes = layout
    n = math.prod(tensor.dims)
    if n % block_values:
        raise GGUFError(
            "invalid-block-shape",
            f"tensor {tensor.name} has {n} values, not divisible by block size {block_values}",
            tensor.info_start,
        )
    return (n // block_values) * block_bytes


def parse_gguf(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    r = Reader(data)
    if len(data) < 4:
        raise GGUFError("truncated-header", "file shorter than GGUF magic", 0)
    magic = r.read(4)
    if magic != b"GGUF":
        raise GGUFError("bad-magic", f"expected GGUF, got {magic!r}", 0)
    try:
        version = r.unpack("<I")
        tensor_count = r.unpack("<Q")
        metadata_count = r.unpack("<Q")
    except GGUFError as exc:
        raise GGUFError("truncated-header", exc.message, exc.offset) from exc
    if version != 3:
        raise GGUFError("unsupported-version", f"expected version 3, got {version}", 4)
    if tensor_count > 1_000_000 or metadata_count > 1_000_000:
        raise GGUFError("invalid-count", "unreasonable tensor or metadata count", 8)

    metadata: dict[str, Any] = {}
    metadata_locators: dict[str, dict[str, int]] = {}
    for _ in range(metadata_count):
        start = r.pos
        try:
            key, _, _ = r.string()
            value_type_offset = r.pos
            value_type = r.unpack("<I")
            value = read_meta_value(r, value_type)
        except GGUFError as exc:
            if exc.code == "truncated":
                raise GGUFError("truncated-metadata", exc.message, exc.offset) from exc
            raise
        if key in metadata:
            raise GGUFError("duplicate-metadata-key", f"duplicate key {key}", start)
        metadata[key] = value
        metadata_locators[key] = {
            "start": start,
            "end": r.pos,
            "value_type_offset": value_type_offset,
            "value_type": value_type,
        }

    tensors: list[TensorInfo] = []
    names: set[str] = set()
    for _ in range(tensor_count):
        start = r.pos
        try:
            name, _, _ = r.string()
            n_dims = r.unpack("<I")
            if not 1 <= n_dims <= 4:
                raise GGUFError("invalid-dimension-count", f"tensor {name} has {n_dims} dimensions", r.pos - 4)
            dims = [r.unpack("<Q") for _ in range(n_dims)]
            if any(d == 0 for d in dims):
                raise GGUFError("zero-dimension", f"tensor {name} has a zero dimension", start)
            ggml_type = r.unpack("<I")
            relative_offset = r.unpack("<Q")
        except GGUFError as exc:
            if exc.code == "truncated":
                raise GGUFError("truncated-tensor-info", exc.message, exc.offset) from exc
            raise
        if name in names:
            raise GGUFError("duplicate-tensor-name", f"duplicate tensor {name}", start)
        names.add(name)
        tensors.append(TensorInfo(name, dims, ggml_type, relative_offset, start, r.pos))

    alignment = metadata.get("general.alignment", 32)
    if not isinstance(alignment, int) or alignment < 8 or alignment % 8:
        raise GGUFError("invalid-alignment", f"invalid alignment {alignment!r}", metadata_locators.get("general.alignment", {}).get("start"))
    data_start = align_up(r.pos, alignment)
    if data_start > len(data):
        raise GGUFError("truncated-padding", "file ends before aligned tensor-data section", r.pos)
    padding = data[r.pos:data_start]
    if any(padding):
        raise GGUFError("nonzero-padding", "non-zero bytes in GGUF header padding", r.pos)

    tensor_rows: list[dict[str, Any]] = []
    for tensor in tensors:
        if tensor.relative_offset % alignment:
            raise GGUFError(
                "misaligned-tensor-offset",
                f"tensor {tensor.name} relative offset {tensor.relative_offset} is not aligned to {alignment}",
                tensor.info_start,
            )
        absolute_start = data_start + tensor.relative_offset
        size = expected_tensor_bytes(tensor)
        if size is not None:
            absolute_end = absolute_start + size
            if absolute_end > len(data):
                raise GGUFError(
                    "truncated-tensor-data",
                    f"tensor {tensor.name} needs bytes [{absolute_start},{absolute_end}), file length {len(data)}",
                    absolute_start,
                )
        else:
            absolute_end = None
        tensor_rows.append({
            "name": tensor.name,
            "dims": tensor.dims,
            "ggml_type": tensor.ggml_type,
            "ggml_type_name": TYPE_NAMES.get(tensor.ggml_type, "UNKNOWN"),
            "relative_offset": tensor.relative_offset,
            "absolute_start": absolute_start,
            "expected_bytes": size,
            "absolute_end": absolute_end,
        })

    return {
        "path": str(path),
        "sha256": hashlib.sha256(data).hexdigest(),
        "length": len(data),
        "magic": "GGUF",
        "version": version,
        "tensor_count": tensor_count,
        "metadata_count": metadata_count,
        "alignment": alignment,
        "tensor_data_start": data_start,
        "metadata": metadata,
        "metadata_locators": metadata_locators,
        "tensors": tensor_rows,
    }


def classify(path: Path) -> dict[str, Any]:
    try:
        parsed = parse_gguf(path)
        return {"path": str(path), "valid": True, "parsed": parsed}
    except GGUFError as exc:
        return {
            "path": str(path),
            "valid": False,
            "error": {"code": exc.code, "message": exc.message, "offset": exc.offset},
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None,
            "length": path.stat().st_size if path.exists() else None,
        }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", type=Path)
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()
    results = [classify(path) for path in args.paths]
    text = json.dumps(results, ensure_ascii=False, sort_keys=True, indent=2) + "\n"
    if args.json_out:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(text, encoding="utf-8", newline="\n")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
