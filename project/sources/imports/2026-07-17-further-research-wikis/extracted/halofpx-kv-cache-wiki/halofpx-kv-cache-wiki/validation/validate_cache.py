#!/usr/bin/env python3
"""Fail-closed outer validators for legacy CachyLLama and HaloFPX cache files.

The legacy parser proves bounded structural consistency for an explicit
little-endian LP64 layout. Because the upstream format has no payload digest,
a structurally valid legacy file is never labeled eligible for a trusted hit.
The HaloFPX manifest validator returns CATALOG_ENTRY_VALID after keyed catalog
authentication and immutable-object validation, and IMPORT_CANDIDATE_VALID only
after caller-supplied current-request bindings also match. It never emits a
public hit; principal authorization and isolated engine import remain external gates.
"""
from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import os
import re
import stat
import struct
import sys
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Iterable

EXIT_VALID = 0
EXIT_ERROR = 1
EXIT_USAGE = 2
EXIT_MISS = 10

MISS = "MISS_RECOMPUTE"
LEGACY_VALID = "LEGACY_STRUCTURALLY_VALID_UNAUTHENTICATED"
OBJECT_VALID = "OBJECT_VALID"
CATALOG_ENTRY_VALID = "CATALOG_ENTRY_VALID"
IMPORT_CANDIDATE_VALID = "IMPORT_CANDIDATE_VALID"
HIT_VERIFIED = "HIT_VERIFIED"  # reserved for an integrated successful engine import

CACHY_RECORD_MAGIC = 0x4B565243
CACHY_INDEX_MAGIC = 0x4B564944
CACHY_SYSTEM_MAGIC = 0x4B565359
CACHY_RECORD_VERSION = 3
CACHY_SYSTEM_VERSION = 1
CACHY_TOKEN_PREFIX_MAX = 4096

# Explicit conventional little-endian LP64 interpretation of native structs.
CACHY_RECORD_FMT = "<IIQIii4xQI4xQQI4xQ4096IQQ"
CACHY_INDEX_FMT = "<IIQQ12Q"
CACHY_SYSTEM_FMT = "<IIQIIQQQII4096I"
CACHY_RECORD_SIZE = struct.calcsize(CACHY_RECORD_FMT)  # 16480
CACHY_INDEX_SIZE = struct.calcsize(CACHY_INDEX_FMT)    # 120
CACHY_SYSTEM_SIZE = struct.calcsize(CACHY_SYSTEM_FMT)  # 16440

HALO_MAGIC = b"HFPXKVC1"
HALO_MAJOR = 1
HALO_MINOR_MAX = 0
HALO_HEADER_FMT = "<8sHHIQQII32s32s"
HALO_HEADER_SIZE = struct.calcsize(HALO_HEADER_FMT)    # 104
HALO_KNOWN_FLAGS = 0
MANIFEST_AUTH_DOMAIN = b"halofpx.manifest-auth/v1\0"
HEX64_RE = re.compile(r"^[0-9a-f]{64}$")
CKPT_NAME_RE = re.compile(r"^ckpt-(\d+)\.bin$")
SYS_NAME_RE = re.compile(r"^sys-([0-9a-fA-F]{16})\.bin$")
OBJECT_NAME_RE = re.compile(r"^([0-9a-f]{64})\.hkv$")

DEFAULT_MAX_OBJECT_BYTES = 1 << 40       # 1 TiB hard analysis cap
DEFAULT_MAX_METADATA_BYTES = 8 << 20     # 8 MiB
DEFAULT_MAX_TOKENS = 10_000_000
DEFAULT_MAX_SEGMENTS = 128


@dataclass
class ValidationResult:
    status: str
    reason: str
    path: str
    eligible_for_hit: bool = False
    eligible_for_engine_import: bool = False
    details: dict[str, Any] = field(default_factory=dict)

    @property
    def is_miss(self) -> bool:
        return self.status == MISS

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def ok(
    path: Path,
    status: str,
    reason: str,
    *,
    eligible: bool = False,
    import_candidate: bool = False,
    **details: Any,
) -> ValidationResult:
    return ValidationResult(status, reason, str(path), eligible, import_candidate, details)


def miss(path: Path, reason: str, **details: Any) -> ValidationResult:
    return ValidationResult(MISS, reason, str(path), False, False, details)


def parse_int(value: str | int | None) -> int | None:
    if value is None or isinstance(value, int):
        return value
    return int(value, 0)


def canonical_json_bytes(obj: Any) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path, *, offset: int = 0, length: int | None = None, chunk_size: int = 4 << 20) -> str:
    h = hashlib.sha256()
    with path.open("rb", buffering=0) as f:
        if offset:
            f.seek(offset)
        remaining = length
        while remaining is None or remaining > 0:
            want = chunk_size if remaining is None else min(chunk_size, remaining)
            block = f.read(want)
            if not block:
                break
            h.update(block)
            if remaining is not None:
                remaining -= len(block)
        if remaining not in (None, 0):
            raise EOFError(f"short read while hashing: {remaining} bytes missing")
    return h.hexdigest()


def fnv1a64_tokens(tokens: Iterable[int]) -> int:
    h = 14695981039346656037
    for token in tokens:
        v = int(token) & 0xFFFFFFFF
        for shift in (0, 8, 16, 24):
            h ^= (v >> shift) & 0xFF
            h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


def _safe_regular_file(path: Path) -> tuple[os.stat_result | None, ValidationResult | None]:
    try:
        if path.is_symlink():
            return None, miss(path, "PATH_SYMLINK")
        st = path.stat()
    except FileNotFoundError:
        return None, miss(path, "NO_ENTRY")
    except OSError as exc:
        return None, miss(path, "IO_STAT_ERROR", error=str(exc))
    if not stat.S_ISREG(st.st_mode):
        return None, miss(path, "NOT_REGULAR_FILE", mode=oct(st.st_mode))
    return st, None


def _read_prefix(path: Path, size: int) -> tuple[bytes | None, ValidationResult | None]:
    try:
        with path.open("rb", buffering=0) as f:
            data = f.read(size)
    except OSError as exc:
        return None, miss(path, "IO_READ_ERROR", error=str(exc))
    if len(data) != size:
        return None, miss(path, "SHORT_HEADER", expected=size, actual=len(data))
    return data, None


def validate_cachyllama_checkpoint(
    path: Path,
    *,
    expected_compat: int | None = None,
    expected_token_hash: int | None = None,
    max_object_bytes: int = DEFAULT_MAX_OBJECT_BYTES,
    max_tokens: int = DEFAULT_MAX_TOKENS,
) -> ValidationResult:
    st, err = _safe_regular_file(path)
    if err:
        return err
    assert st is not None
    if st.st_size > max_object_bytes:
        return miss(path, "OBJECT_TOO_LARGE", size=st.st_size, cap=max_object_bytes)
    header, err = _read_prefix(path, CACHY_RECORD_SIZE)
    if err:
        return err
    assert header is not None
    try:
        values = struct.unpack(CACHY_RECORD_FMT, header)
    except struct.error as exc:
        return miss(path, "HEADER_UNPACK_ERROR", error=str(exc))

    magic, version, checkpoint_id, slot_id, pos_min, pos_max = values[:6]
    n_tokens, turn_created, target_size, token_hash, token_count, compat_hash = values[6:12]
    prefix = values[12:12 + CACHY_TOKEN_PREFIX_MAX]
    draft_size, spec_size = values[-2:]

    if magic != CACHY_RECORD_MAGIC:
        return miss(path, "BAD_MAGIC", actual=hex(magic), expected=hex(CACHY_RECORD_MAGIC))
    if version != CACHY_RECORD_VERSION:
        return miss(path, "UNSUPPORTED_VERSION", actual=version, expected=CACHY_RECORD_VERSION)
    if token_count > CACHY_TOKEN_PREFIX_MAX:
        return miss(path, "TOKEN_COUNT_OVERFLOW", token_count=token_count, maximum=CACHY_TOKEN_PREFIX_MAX)
    if n_tokens > max_tokens:
        return miss(path, "TOKEN_LIMIT_EXCEEDED", n_tokens=n_tokens, maximum=max_tokens)
    if token_count > n_tokens:
        return miss(path, "TOKEN_COUNT_EXCEEDS_TOTAL", token_count=token_count, n_tokens=n_tokens)
    for name, size in (("target", target_size), ("draft", draft_size), ("speculative", spec_size)):
        if size > max_object_bytes:
            return miss(path, "SEGMENT_TOO_LARGE", segment=name, size=size, cap=max_object_bytes)
    payload_size = target_size + draft_size + spec_size
    expected_size = CACHY_RECORD_SIZE + payload_size
    if expected_size > max_object_bytes:
        return miss(path, "OBJECT_TOO_LARGE", expected_size=expected_size, cap=max_object_bytes)
    if st.st_size != expected_size:
        return miss(path, "LENGTH_MISMATCH", actual=st.st_size, expected=expected_size)
    m = CKPT_NAME_RE.match(path.name)
    if m and int(m.group(1)) != checkpoint_id:
        return miss(path, "FILENAME_ID_MISMATCH", filename_id=int(m.group(1)), header_id=checkpoint_id)
    if expected_compat is not None and compat_hash != expected_compat:
        return miss(path, "FINGERPRINT_MISMATCH", actual=hex(compat_hash), expected=hex(expected_compat))
    if expected_token_hash is not None and token_hash != expected_token_hash:
        return miss(path, "PROMPT_HASH_MISMATCH", actual=hex(token_hash), expected=hex(expected_token_hash))
    if token_count == n_tokens and fnv1a64_tokens(prefix[:token_count]) != token_hash:
        return miss(path, "TOKEN_HASH_SELF_MISMATCH", stored=hex(token_hash))

    return ok(
        path,
        LEGACY_VALID,
        "NO_PAYLOAD_INTEGRITY",
        eligible=False,
        format="cachyllama.kv_ssd_record/v3-lp64le",
        header_size=CACHY_RECORD_SIZE,
        checkpoint_id=checkpoint_id,
        slot_id=slot_id,
        pos_min=pos_min,
        pos_max=pos_max,
        n_tokens=n_tokens,
        turn_created=turn_created,
        token_count=token_count,
        token_hash=hex(token_hash),
        compatibility_hash=hex(compat_hash),
        segment_sizes={"target": target_size, "draft": draft_size, "speculative": spec_size},
        payload_integrity_verified=False,
        keyed_authentication_verified=False,
        warning="Outer format cannot detect same-length payload corruption; do not treat as trusted hit.",
    )


def validate_cachyllama_index(
    path: Path,
    *,
    expected_compat: int | None = None,
) -> ValidationResult:
    st, err = _safe_regular_file(path)
    if err:
        return err
    assert st is not None
    if st.st_size != CACHY_INDEX_SIZE:
        return miss(path, "LENGTH_MISMATCH", actual=st.st_size, expected=CACHY_INDEX_SIZE)
    header, err = _read_prefix(path, CACHY_INDEX_SIZE)
    if err:
        return err
    assert header is not None
    magic, version, next_id, compat_hash, *_ = struct.unpack(CACHY_INDEX_FMT, header)
    if magic != CACHY_INDEX_MAGIC:
        return miss(path, "BAD_MAGIC", actual=hex(magic), expected=hex(CACHY_INDEX_MAGIC))
    if version != CACHY_RECORD_VERSION:
        return miss(path, "UNSUPPORTED_VERSION", actual=version, expected=CACHY_RECORD_VERSION)
    if next_id < 1:
        return miss(path, "INVALID_NEXT_ID", next_id=next_id)
    if expected_compat is not None and compat_hash != expected_compat:
        return miss(path, "FINGERPRINT_MISMATCH", actual=hex(compat_hash), expected=hex(expected_compat))
    return ok(
        path,
        LEGACY_VALID,
        "NO_INDEX_AUTHENTICATION",
        eligible=False,
        format="cachyllama.kv_ssd_index/v3-lp64le",
        next_id=next_id,
        compatibility_hash=hex(compat_hash),
        payload_integrity_verified=False,
        keyed_authentication_verified=False,
    )


def validate_cachyllama_system(
    path: Path,
    *,
    expected_compat: int | None = None,
    expected_token_hash: int | None = None,
    max_object_bytes: int = DEFAULT_MAX_OBJECT_BYTES,
    max_tokens: int = DEFAULT_MAX_TOKENS,
) -> ValidationResult:
    st, err = _safe_regular_file(path)
    if err:
        return err
    assert st is not None
    if st.st_size > max_object_bytes:
        return miss(path, "OBJECT_TOO_LARGE", size=st.st_size, cap=max_object_bytes)
    header, err = _read_prefix(path, CACHY_SYSTEM_SIZE)
    if err:
        return err
    assert header is not None
    values = struct.unpack(CACHY_SYSTEM_FMT, header)
    magic, version, token_hash, n_tokens, data_size, compat_hash, created_at, last_used, access_count, token_count = values[:10]
    prefix = values[10:]
    if magic != CACHY_SYSTEM_MAGIC:
        return miss(path, "BAD_MAGIC", actual=hex(magic), expected=hex(CACHY_SYSTEM_MAGIC))
    if version != CACHY_SYSTEM_VERSION:
        return miss(path, "UNSUPPORTED_VERSION", actual=version, expected=CACHY_SYSTEM_VERSION)
    if token_count > CACHY_TOKEN_PREFIX_MAX:
        return miss(path, "TOKEN_COUNT_OVERFLOW", token_count=token_count, maximum=CACHY_TOKEN_PREFIX_MAX)
    if n_tokens > max_tokens:
        return miss(path, "TOKEN_LIMIT_EXCEEDED", n_tokens=n_tokens, maximum=max_tokens)
    if token_count > n_tokens:
        return miss(path, "TOKEN_COUNT_EXCEEDS_TOTAL", token_count=token_count, n_tokens=n_tokens)
    expected_size = CACHY_SYSTEM_SIZE + data_size
    if expected_size > max_object_bytes:
        return miss(path, "OBJECT_TOO_LARGE", expected_size=expected_size, cap=max_object_bytes)
    if st.st_size != expected_size:
        return miss(path, "LENGTH_MISMATCH", actual=st.st_size, expected=expected_size)
    m = SYS_NAME_RE.match(path.name)
    if m and int(m.group(1), 16) != token_hash:
        return miss(path, "FILENAME_HASH_MISMATCH", filename_hash=m.group(1).lower(), header_hash=f"{token_hash:016x}")
    if expected_compat is not None and compat_hash != expected_compat:
        return miss(path, "FINGERPRINT_MISMATCH", actual=hex(compat_hash), expected=hex(expected_compat))
    if expected_token_hash is not None and token_hash != expected_token_hash:
        return miss(path, "PROMPT_HASH_MISMATCH", actual=hex(token_hash), expected=hex(expected_token_hash))
    if token_count == n_tokens and fnv1a64_tokens(prefix[:token_count]) != token_hash:
        return miss(path, "TOKEN_HASH_SELF_MISMATCH", stored=hex(token_hash))
    if n_tokens > token_count and expected_token_hash is None:
        return miss(path, "PROMPT_IDENTITY_UNVERIFIABLE", n_tokens=n_tokens, stored_tokens=token_count)
    return ok(
        path,
        LEGACY_VALID,
        "NO_PAYLOAD_INTEGRITY",
        eligible=False,
        format="cachyllama.system/v1-lp64le",
        token_hash=f"{token_hash:016x}",
        n_tokens=n_tokens,
        token_count=token_count,
        data_size=data_size,
        compatibility_hash=hex(compat_hash),
        created_at=created_at,
        last_used=last_used,
        access_count=access_count,
        payload_integrity_verified=False,
        keyed_authentication_verified=False,
    )


def _require_hex64(obj: dict[str, Any], key: str, path: Path) -> ValidationResult | None:
    value = obj.get(key)
    if not isinstance(value, str) or not HEX64_RE.fullmatch(value):
        return miss(path, "SCHEMA_FIELD_INVALID", field=key, expected="64 lowercase hex")
    return None


def validate_halofpx_object(
    path: Path,
    *,
    expected_compat: str | None = None,
    expected_cache_key: str | None = None,
    expected_prompt_root: str | None = None,
    max_object_bytes: int = DEFAULT_MAX_OBJECT_BYTES,
    max_metadata_bytes: int = DEFAULT_MAX_METADATA_BYTES,
    max_segments: int = DEFAULT_MAX_SEGMENTS,
) -> ValidationResult:
    st, err = _safe_regular_file(path)
    if err:
        return err
    assert st is not None
    if st.st_size > max_object_bytes:
        return miss(path, "OBJECT_TOO_LARGE", size=st.st_size, cap=max_object_bytes)
    header, err = _read_prefix(path, HALO_HEADER_SIZE)
    if err:
        return err
    assert header is not None
    try:
        magic, major, minor, header_len, metadata_len, payload_len, segment_count, flags, metadata_digest, payload_digest = struct.unpack(HALO_HEADER_FMT, header)
    except struct.error as exc:
        return miss(path, "HEADER_UNPACK_ERROR", error=str(exc))
    if magic != HALO_MAGIC:
        return miss(path, "BAD_MAGIC", actual=magic.hex(), expected=HALO_MAGIC.hex())
    if major != HALO_MAJOR or minor > HALO_MINOR_MAX:
        return miss(path, "UNSUPPORTED_VERSION", major=major, minor=minor, supported=f"{HALO_MAJOR}.0-{HALO_MINOR_MAX}")
    if header_len != HALO_HEADER_SIZE:
        return miss(path, "HEADER_LENGTH_INVALID", actual=header_len, expected=HALO_HEADER_SIZE)
    if flags & ~HALO_KNOWN_FLAGS:
        return miss(path, "UNKNOWN_CRITICAL_FLAGS", flags=flags)
    if metadata_len > max_metadata_bytes:
        return miss(path, "METADATA_TOO_LARGE", size=metadata_len, cap=max_metadata_bytes)
    if payload_len > max_object_bytes:
        return miss(path, "PAYLOAD_TOO_LARGE", size=payload_len, cap=max_object_bytes)
    if segment_count > max_segments:
        return miss(path, "SEGMENT_COUNT_EXCEEDED", count=segment_count, cap=max_segments)
    expected_size = HALO_HEADER_SIZE + metadata_len + payload_len
    if expected_size > max_object_bytes:
        return miss(path, "OBJECT_TOO_LARGE", expected_size=expected_size, cap=max_object_bytes)
    if st.st_size != expected_size:
        return miss(path, "LENGTH_MISMATCH", actual=st.st_size, expected=expected_size)
    try:
        with path.open("rb", buffering=0) as f:
            f.seek(HALO_HEADER_SIZE)
            metadata_bytes = f.read(metadata_len)
    except OSError as exc:
        return miss(path, "IO_READ_ERROR", error=str(exc))
    if len(metadata_bytes) != metadata_len:
        return miss(path, "SHORT_METADATA", actual=len(metadata_bytes), expected=metadata_len)
    if hashlib.sha256(metadata_bytes).digest() != metadata_digest:
        return miss(path, "METADATA_DIGEST_MISMATCH")
    try:
        metadata = json.loads(metadata_bytes.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        return miss(path, "METADATA_PARSE_ERROR", error=str(exc))
    if not isinstance(metadata, dict):
        return miss(path, "METADATA_SCHEMA_INVALID", error="top level must be object")
    if canonical_json_bytes(metadata) != metadata_bytes:
        return miss(path, "METADATA_NOT_CANONICAL")
    if metadata.get("schema") != "halofpx.kv.object/v1":
        return miss(path, "METADATA_SCHEMA_UNSUPPORTED", schema=metadata.get("schema"))
    required_metadata_fields = {
        "schema", "cache_key_sha256", "compatibility_fingerprint_sha256",
        "prompt_root_sha256", "boundary", "engine", "segments",
        "created_unix_ns", "writer_id", "policy",
    }
    missing_metadata_fields = sorted(required_metadata_fields - set(metadata))
    if missing_metadata_fields:
        return miss(path, "METADATA_SCHEMA_INVALID", missing=missing_metadata_fields)
    boundary = metadata["boundary"]
    if not isinstance(boundary, dict) or set(boundary) != {"kind", "start_token", "end_token"}:
        return miss(path, "BOUNDARY_SCHEMA_INVALID")
    if boundary.get("kind") not in {"token_page", "sequence_checkpoint", "recurrent_checkpoint"}:
        return miss(path, "BOUNDARY_KIND_UNSUPPORTED", kind=boundary.get("kind"))
    if not all(isinstance(boundary.get(k), int) and boundary[k] >= 0 for k in ("start_token", "end_token")):
        return miss(path, "BOUNDARY_RANGE_INVALID")
    if boundary["end_token"] < boundary["start_token"]:
        return miss(path, "BOUNDARY_RANGE_INVALID")
    engine = metadata["engine"]
    if not isinstance(engine, dict) or set(engine) != {"family", "state_abi"}:
        return miss(path, "ENGINE_SCHEMA_INVALID")
    if any(not isinstance(engine.get(k), str) or not engine[k] or len(engine[k]) > 256 for k in ("family", "state_abi")):
        return miss(path, "ENGINE_SCHEMA_INVALID")
    if not isinstance(metadata["created_unix_ns"], int) or metadata["created_unix_ns"] < 0:
        return miss(path, "CREATED_TIME_INVALID")
    if not isinstance(metadata["writer_id"], str) or not metadata["writer_id"] or len(metadata["writer_id"]) > 256:
        return miss(path, "WRITER_ID_INVALID")
    policy = metadata["policy"]
    if not isinstance(policy, dict) or policy.get("partial_reuse") not in {"complete_prefix_only", "all_or_nothing"}:
        return miss(path, "POLICY_SCHEMA_INVALID")
    if "optional_draft_catchup" in policy and not isinstance(policy["optional_draft_catchup"], bool):
        return miss(path, "POLICY_SCHEMA_INVALID")
    for field_name in ("cache_key_sha256", "compatibility_fingerprint_sha256", "prompt_root_sha256"):
        err = _require_hex64(metadata, field_name, path)
        if err:
            return err
    if expected_compat is not None and metadata["compatibility_fingerprint_sha256"] != expected_compat:
        return miss(path, "FINGERPRINT_MISMATCH", actual=metadata["compatibility_fingerprint_sha256"], expected=expected_compat)
    if expected_cache_key is not None and metadata["cache_key_sha256"] != expected_cache_key:
        return miss(path, "CACHE_KEY_MISMATCH", actual=metadata["cache_key_sha256"], expected=expected_cache_key)
    if expected_prompt_root is not None and metadata["prompt_root_sha256"] != expected_prompt_root:
        return miss(path, "PROMPT_ROOT_MISMATCH", actual=metadata["prompt_root_sha256"], expected=expected_prompt_root)
    segments = metadata.get("segments")
    if not isinstance(segments, list) or len(segments) != segment_count:
        return miss(path, "SEGMENT_TABLE_COUNT_MISMATCH", header_count=segment_count, metadata_count=len(segments) if isinstance(segments, list) else None)
    if not segments:
        return miss(path, "MISSING_SEGMENT_TABLE")

    payload_offset = HALO_HEADER_SIZE + metadata_len
    try:
        payload_actual = bytes.fromhex(sha256_file(path, offset=payload_offset, length=payload_len))
    except (OSError, EOFError) as exc:
        return miss(path, "PAYLOAD_READ_ERROR", error=str(exc))
    if payload_actual != payload_digest:
        return miss(path, "PAYLOAD_DIGEST_MISMATCH")

    expected_offset = 0
    names: set[str] = set()
    required_names: set[str] = set()
    segment_details: list[dict[str, Any]] = []
    for index, seg in enumerate(segments):
        if not isinstance(seg, dict):
            return miss(path, "SEGMENT_SCHEMA_INVALID", index=index)
        required_fields = {"name", "required", "offset", "stored_length", "logical_length", "stored_sha256", "codec", "encryption", "role"}
        missing_fields = sorted(required_fields - set(seg))
        if missing_fields:
            return miss(path, "SEGMENT_SCHEMA_INVALID", index=index, missing=missing_fields)
        name = seg["name"]
        if not isinstance(name, str) or not name or len(name) > 64 or name in names:
            return miss(path, "SEGMENT_NAME_INVALID", index=index, name=name)
        names.add(name)
        if seg["required"] is True:
            required_names.add(name)
        elif seg["required"] is not False:
            return miss(path, "SEGMENT_REQUIRED_FLAG_INVALID", segment=name)
        offset = seg["offset"]
        stored_length = seg["stored_length"]
        logical_length = seg["logical_length"]
        if not all(isinstance(x, int) and x >= 0 for x in (offset, stored_length, logical_length)):
            return miss(path, "SEGMENT_LENGTH_INVALID", segment=name)
        if offset != expected_offset:
            return miss(path, "SEGMENT_COVERAGE_INVALID", segment=name, actual_offset=offset, expected_offset=expected_offset)
        if stored_length > payload_len or offset + stored_length > payload_len:
            return miss(path, "SEGMENT_BOUNDS_INVALID", segment=name)
        if logical_length > max_object_bytes:
            return miss(path, "SEGMENT_LOGICAL_SIZE_EXCEEDED", segment=name, logical_length=logical_length)
        digest_hex = seg["stored_sha256"]
        if not isinstance(digest_hex, str) or not HEX64_RE.fullmatch(digest_hex):
            return miss(path, "SEGMENT_DIGEST_FIELD_INVALID", segment=name)
        role = seg["role"]
        if not isinstance(role, str) or not role or len(role) > 128:
            return miss(path, "SEGMENT_ROLE_INVALID", segment=name)
        codec = seg["codec"]
        encryption = seg["encryption"]
        if codec not in {"none", "zstd"}:
            return miss(path, "UNSUPPORTED_CODEC", segment=name, codec=codec)
        if encryption not in {"none", "aes-256-gcm", "xchacha20-poly1305"}:
            return miss(path, "UNSUPPORTED_ENCRYPTION", segment=name, encryption=encryption)
        try:
            actual_seg_digest = sha256_file(path, offset=payload_offset + offset, length=stored_length)
        except (OSError, EOFError) as exc:
            return miss(path, "SEGMENT_READ_ERROR", segment=name, error=str(exc))
        if actual_seg_digest != digest_hex:
            return miss(path, "SEGMENT_DIGEST_MISMATCH", segment=name)
        if encryption != "none":
            return miss(path, "KEY_UNAVAILABLE", segment=name, encryption=encryption, note="reference validator has no key provider")
        if codec != "none":
            return miss(path, "CODEC_UNAVAILABLE", segment=name, codec=codec, note="reference validator does not decompress")
        if codec == "none" and logical_length != stored_length:
            return miss(path, "SEGMENT_LOGICAL_LENGTH_MISMATCH", segment=name, logical_length=logical_length, stored_length=stored_length)
        expected_offset += stored_length
        segment_details.append({"name": name, "required": seg["required"], "offset": offset, "stored_length": stored_length, "role": seg["role"]})
    if expected_offset != payload_len:
        return miss(path, "SEGMENT_COVERAGE_INVALID", covered=expected_offset, payload_len=payload_len)
    if "target" not in required_names:
        return miss(path, "MISSING_REQUIRED_TARGET_SEGMENT", required=sorted(required_names))

    whole_digest = sha256_file(path)
    m = OBJECT_NAME_RE.match(path.name)
    if m and whole_digest != m.group(1):
        return miss(path, "OBJECT_FILENAME_DIGEST_MISMATCH", actual=whole_digest, filename=m.group(1))

    return ok(
        path,
        OBJECT_VALID,
        "OBJECT_INTEGRITY_VERIFIED",
        eligible=False,
        format="halofpx.kv.object/v1",
        object_sha256=whole_digest,
        object_size=st.st_size,
        metadata=metadata,
        segments=segment_details,
        required_segments=sorted(required_names),
        integrity_verified=True,
        keyed_authentication_verified=False,
        note="Object digest/segment integrity is valid; an authenticated manifest, authorized request, and isolated engine import are still required.",
    )


def _validate_manifest_shape(path: Path, manifest: Any) -> ValidationResult | None:
    if not isinstance(manifest, dict):
        return miss(path, "MANIFEST_SCHEMA_INVALID", error="top level must be object")
    required = {
        "schema", "generation", "namespace_id", "engine_family", "cache_key_sha256",
        "compatibility_fingerprint_sha256", "prompt_root_sha256", "object_sha256",
        "object_size", "created_unix_ns", "last_access_unix_ns", "verified_prefix_tokens",
        "required_segments", "encryption_policy", "state", "catalog_auth",
    }
    unknown = sorted(set(manifest) - required)
    missing_fields = sorted(required - set(manifest))
    if missing_fields or unknown:
        return miss(path, "MANIFEST_SCHEMA_INVALID", missing=missing_fields, unknown=unknown)
    if manifest["schema"] != "halofpx.kv.manifest/v1":
        return miss(path, "MANIFEST_SCHEMA_UNSUPPORTED", schema=manifest["schema"])
    if not isinstance(manifest["generation"], int) or manifest["generation"] < 1:
        return miss(path, "MANIFEST_GENERATION_INVALID")
    for key in ("namespace_id", "cache_key_sha256", "compatibility_fingerprint_sha256", "prompt_root_sha256", "object_sha256"):
        err = _require_hex64(manifest, key, path)
        if err:
            return err
    if not isinstance(manifest["engine_family"], str) or not manifest["engine_family"] or len(manifest["engine_family"]) > 128:
        return miss(path, "MANIFEST_ENGINE_INVALID")
    for key in ("object_size", "created_unix_ns", "last_access_unix_ns", "verified_prefix_tokens"):
        if not isinstance(manifest[key], int) or manifest[key] < 0:
            return miss(path, "MANIFEST_INTEGER_INVALID", field=key)
    if manifest["object_size"] < HALO_HEADER_SIZE:
        return miss(path, "MANIFEST_OBJECT_SIZE_INVALID")
    rs = manifest["required_segments"]
    if not isinstance(rs, list) or not rs or any(not isinstance(x, str) or not x or len(x) > 64 for x in rs) or len(set(rs)) != len(rs):
        return miss(path, "MANIFEST_REQUIRED_SEGMENTS_INVALID")
    if manifest["encryption_policy"] not in {"none", "optional-aead", "required-aead"}:
        return miss(path, "MANIFEST_ENCRYPTION_POLICY_INVALID")
    auth = manifest["catalog_auth"]
    if not isinstance(auth, dict) or set(auth) != {"mode", "key_id", "tag_hex"}:
        return miss(path, "MANIFEST_AUTH_SCHEMA_INVALID")
    if auth.get("mode") != "hmac-sha256":
        return miss(path, "MANIFEST_AUTH_MODE_UNSUPPORTED", mode=auth.get("mode"))
    if not isinstance(auth.get("key_id"), str) or not auth["key_id"] or len(auth["key_id"]) > 256:
        return miss(path, "MANIFEST_AUTH_KEY_ID_INVALID")
    if not isinstance(auth.get("tag_hex"), str) or not HEX64_RE.fullmatch(auth["tag_hex"]):
        return miss(path, "MANIFEST_AUTH_TAG_INVALID")
    if manifest["state"] != "committed":
        return miss(path, "MANIFEST_NOT_COMMITTED", state=manifest["state"])
    return None


def manifest_auth_input(manifest: dict[str, Any]) -> bytes:
    """Canonical bytes authenticated by catalog HMAC.

    The tag authenticates every manifest field and the auth mode/key identifier,
    but excludes the tag itself to avoid recursion.
    """
    unsigned = dict(manifest)
    auth = dict(unsigned.get("catalog_auth", {}))
    auth.pop("tag_hex", None)
    unsigned["catalog_auth"] = auth
    return MANIFEST_AUTH_DOMAIN + canonical_json_bytes(unsigned)


def compute_manifest_hmac(manifest: dict[str, Any], key: bytes) -> str:
    return hmac.new(key, manifest_auth_input(manifest), hashlib.sha256).hexdigest()


def load_hmac_key_file(path: Path | None) -> bytes | None:
    if path is None:
        return None
    raw = path.read_text(encoding="ascii").strip()
    try:
        key = bytes.fromhex(raw)
    except ValueError as exc:
        raise ValueError(f"manifest HMAC key file must contain hex: {path}") from exc
    if len(key) < 32:
        raise ValueError("manifest HMAC key must be at least 32 bytes")
    return key


def validate_halofpx_manifest(
    path: Path,
    *,
    object_root: Path,
    expected_namespace: str | None = None,
    expected_compat: str | None = None,
    expected_cache_key: str | None = None,
    expected_prompt_root: str | None = None,
    expected_engine_family: str | None = None,
    manifest_hmac_key: bytes | None = None,
    max_manifest_bytes: int = 1 << 20,
    max_object_bytes: int = DEFAULT_MAX_OBJECT_BYTES,
) -> ValidationResult:
    st, err = _safe_regular_file(path)
    if err:
        return err
    assert st is not None
    if st.st_size > max_manifest_bytes:
        return miss(path, "MANIFEST_TOO_LARGE", size=st.st_size, cap=max_manifest_bytes)
    try:
        raw = path.read_bytes()
        manifest = json.loads(raw.decode("utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        return miss(path, "MANIFEST_PARSE_ERROR", error=str(exc))
    if canonical_json_bytes(manifest) + b"\n" != raw and canonical_json_bytes(manifest) != raw:
        return miss(path, "MANIFEST_NOT_CANONICAL")
    shape_error = _validate_manifest_shape(path, manifest)
    if shape_error:
        return shape_error
    assert isinstance(manifest, dict)
    if manifest_hmac_key is None:
        return miss(path, "MANIFEST_AUTH_KEY_UNAVAILABLE", key_id=manifest["catalog_auth"]["key_id"])
    expected_tag = compute_manifest_hmac(manifest, manifest_hmac_key)
    actual_tag = manifest["catalog_auth"]["tag_hex"]
    if not hmac.compare_digest(actual_tag, expected_tag):
        return miss(path, "MANIFEST_AUTH_FAILURE", key_id=manifest["catalog_auth"]["key_id"])
    for value, expected, code in (
        (manifest["namespace_id"], expected_namespace, "NAMESPACE_MISMATCH"),
        (manifest["compatibility_fingerprint_sha256"], expected_compat, "FINGERPRINT_MISMATCH"),
        (manifest["cache_key_sha256"], expected_cache_key, "CACHE_KEY_MISMATCH"),
        (manifest["prompt_root_sha256"], expected_prompt_root, "PROMPT_ROOT_MISMATCH"),
        (manifest["engine_family"], expected_engine_family, "ENGINE_FAMILY_MISMATCH"),
    ):
        if expected is not None and value != expected:
            return miss(path, code, actual=value, expected=expected)
    digest = manifest["object_sha256"]
    obj_path = object_root / digest[:2] / f"{digest}.hkv"
    obj_result = validate_halofpx_object(
        obj_path,
        expected_compat=manifest["compatibility_fingerprint_sha256"],
        expected_cache_key=manifest["cache_key_sha256"],
        expected_prompt_root=manifest["prompt_root_sha256"],
        max_object_bytes=max_object_bytes,
    )
    if obj_result.is_miss:
        return miss(path, "REFERENCED_OBJECT_INVALID", object_path=str(obj_path), object_reason=obj_result.reason, object_details=obj_result.details)
    if obj_result.details.get("object_sha256") != digest:
        return miss(path, "OBJECT_DIGEST_BINDING_MISMATCH")
    if obj_result.details.get("object_size") != manifest["object_size"]:
        return miss(path, "OBJECT_SIZE_BINDING_MISMATCH", manifest=manifest["object_size"], object=obj_result.details.get("object_size"))
    metadata = obj_result.details["metadata"]
    for key in ("cache_key_sha256", "compatibility_fingerprint_sha256", "prompt_root_sha256"):
        if metadata.get(key) != manifest[key]:
            return miss(path, "MANIFEST_OBJECT_BINDING_MISMATCH", field=key)
    if metadata.get("engine", {}).get("family") != manifest["engine_family"]:
        return miss(path, "MANIFEST_OBJECT_BINDING_MISMATCH", field="engine_family")
    object_segment_names = {s["name"] for s in obj_result.details["segments"]}
    if not set(manifest["required_segments"]).issubset(object_segment_names):
        return miss(path, "MISSING_REQUIRED_SEGMENT", required=manifest["required_segments"], present=sorted(object_segment_names))
    object_required = set(obj_result.details["required_segments"])
    if set(manifest["required_segments"]) != object_required:
        return miss(path, "REQUIRED_SEGMENT_POLICY_MISMATCH", manifest=manifest["required_segments"], object=sorted(object_required))
    boundary = metadata["boundary"]
    if boundary["end_token"] != manifest["verified_prefix_tokens"]:
        return miss(path, "VERIFIED_PREFIX_BINDING_MISMATCH", manifest=manifest["verified_prefix_tokens"], boundary_end=boundary["end_token"])
    if manifest["encryption_policy"] == "required-aead":
        unencrypted = [s["name"] for s in metadata["segments"] if s.get("required") and s.get("encryption") == "none"]
        if unencrypted:
            return miss(path, "ENCRYPTION_POLICY_VIOLATION", unencrypted_required_segments=unencrypted)
    current_request_fields = {
        "namespace_id": expected_namespace,
        "compatibility_fingerprint_sha256": expected_compat,
        "cache_key_sha256": expected_cache_key,
        "prompt_root_sha256": expected_prompt_root,
        "engine_family": expected_engine_family,
    }
    missing_request_bindings = sorted(k for k, v in current_request_fields.items() if v is None)
    if missing_request_bindings:
        return ok(
            path,
            CATALOG_ENTRY_VALID,
            "AUTHENTICATED_CATALOG_ENTRY_VALID_BUT_UNBOUND",
            eligible=False,
            import_candidate=False,
            manifest=manifest,
            object_path=str(obj_path),
            object_sha256=digest,
            catalog_authentication="hmac-sha256",
            missing_current_request_bindings=missing_request_bindings,
            note="The catalog entry is internally valid but cannot be offered to the engine until every current-request binding is supplied and matched.",
        )
    return ok(
        path,
        IMPORT_CANDIDATE_VALID,
        "AUTHENTICATED_MANIFEST_OBJECT_AND_REQUEST_VERIFIED",
        eligible=False,
        import_candidate=True,
        manifest=manifest,
        object_path=str(obj_path),
        object_sha256=digest,
        verified_prefix_tokens=manifest["verified_prefix_tokens"],
        catalog_authentication="hmac-sha256",
        note="This is an import candidate, not a public hit. Transactional engine import must still succeed; any failure is MISS_RECOMPUTE.",
    )


def validate_scan(path: Path, args: argparse.Namespace) -> ValidationResult:
    if not path.exists():
        return miss(path, "NO_ENTRY")
    results: list[ValidationResult] = []
    paths = [path] if path.is_file() else sorted(p for p in path.rglob("*") if p.is_file())
    for p in paths:
        if CKPT_NAME_RE.match(p.name):
            results.append(validate_cachyllama_checkpoint(p, max_object_bytes=args.max_object_bytes))
        elif p.name == "index.bin":
            results.append(validate_cachyllama_index(p))
        elif SYS_NAME_RE.match(p.name):
            results.append(validate_cachyllama_system(p, max_object_bytes=args.max_object_bytes))
        elif p.suffix == ".hkv":
            results.append(validate_halofpx_object(p, max_object_bytes=args.max_object_bytes))
        elif p.suffix == ".json" and "manifests" in p.parts:
            obj_root = Path(args.object_root) if args.object_root else next((ancestor / "objects" / "sha256" for ancestor in [p.parent, *p.parents] if (ancestor / "objects" / "sha256").exists()), None)
            if obj_root is None:
                results.append(miss(p, "OBJECT_ROOT_REQUIRED"))
            else:
                results.append(validate_halofpx_manifest(
                    p,
                    object_root=obj_root,
                    manifest_hmac_key=load_hmac_key_file(args.manifest_hmac_key_file),
                    max_object_bytes=args.max_object_bytes,
                ))
    misses = [r for r in results if r.is_miss]
    return ValidationResult(
        status=MISS if misses else "SCAN_COMPLETE",
        reason="SCAN_HAS_INVALID_ENTRIES" if misses else "SCAN_VALID",
        path=str(path),
        eligible_for_hit=False,
        eligible_for_engine_import=False,
        details={"count": len(results), "misses": len(misses), "results": [r.to_dict() for r in results]},
    )


def _add_common_limits(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--max-object-bytes", type=parse_int, default=DEFAULT_MAX_OBJECT_BYTES)


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--pretty", action="store_true", help="Pretty-print JSON output")
    sub = p.add_subparsers(dest="command", required=True)

    c = sub.add_parser("cachyllama-checkpoint")
    c.add_argument("path", type=Path)
    c.add_argument("--expected-compat", type=parse_int)
    c.add_argument("--expected-token-hash", type=parse_int)
    c.add_argument("--max-tokens", type=int, default=DEFAULT_MAX_TOKENS)
    _add_common_limits(c)

    i = sub.add_parser("cachyllama-index")
    i.add_argument("path", type=Path)
    i.add_argument("--expected-compat", type=parse_int)

    s = sub.add_parser("cachyllama-system")
    s.add_argument("path", type=Path)
    s.add_argument("--expected-compat", type=parse_int)
    s.add_argument("--expected-token-hash", type=parse_int)
    s.add_argument("--max-tokens", type=int, default=DEFAULT_MAX_TOKENS)
    _add_common_limits(s)

    o = sub.add_parser("halofpx-object")
    o.add_argument("path", type=Path)
    o.add_argument("--expected-compat")
    o.add_argument("--expected-cache-key")
    o.add_argument("--expected-prompt-root")
    o.add_argument("--max-metadata-bytes", type=parse_int, default=DEFAULT_MAX_METADATA_BYTES)
    o.add_argument("--max-segments", type=int, default=DEFAULT_MAX_SEGMENTS)
    _add_common_limits(o)

    m = sub.add_parser("halofpx-manifest")
    m.add_argument("path", type=Path)
    m.add_argument("--object-root", type=Path, required=True)
    m.add_argument("--expected-namespace")
    m.add_argument("--expected-compat")
    m.add_argument("--expected-cache-key")
    m.add_argument("--expected-prompt-root")
    m.add_argument("--expected-engine-family")
    m.add_argument("--manifest-hmac-key-file", type=Path, required=True)
    _add_common_limits(m)

    sc = sub.add_parser("scan")
    sc.add_argument("path", type=Path)
    sc.add_argument("--object-root", type=Path)
    sc.add_argument("--manifest-hmac-key-file", type=Path)
    _add_common_limits(sc)
    return p


def run_command(args: argparse.Namespace) -> ValidationResult:
    if args.command == "cachyllama-checkpoint":
        return validate_cachyllama_checkpoint(args.path, expected_compat=args.expected_compat, expected_token_hash=args.expected_token_hash, max_object_bytes=args.max_object_bytes, max_tokens=args.max_tokens)
    if args.command == "cachyllama-index":
        return validate_cachyllama_index(args.path, expected_compat=args.expected_compat)
    if args.command == "cachyllama-system":
        return validate_cachyllama_system(args.path, expected_compat=args.expected_compat, expected_token_hash=args.expected_token_hash, max_object_bytes=args.max_object_bytes, max_tokens=args.max_tokens)
    if args.command == "halofpx-object":
        return validate_halofpx_object(args.path, expected_compat=args.expected_compat, expected_cache_key=args.expected_cache_key, expected_prompt_root=args.expected_prompt_root, max_object_bytes=args.max_object_bytes, max_metadata_bytes=args.max_metadata_bytes, max_segments=args.max_segments)
    if args.command == "halofpx-manifest":
        return validate_halofpx_manifest(
            args.path,
            object_root=args.object_root,
            expected_namespace=args.expected_namespace,
            expected_compat=args.expected_compat,
            expected_cache_key=args.expected_cache_key,
            expected_prompt_root=args.expected_prompt_root,
            expected_engine_family=args.expected_engine_family,
            manifest_hmac_key=load_hmac_key_file(args.manifest_hmac_key_file),
            max_object_bytes=args.max_object_bytes,
        )
    if args.command == "scan":
        return validate_scan(args.path, args)
    raise AssertionError(args.command)


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    try:
        args = parser.parse_args(argv)
        result = run_command(args)
    except SystemExit:
        raise
    except Exception as exc:  # validator bugs should be distinguishable from cache misses
        print(json.dumps({"status": "VALIDATOR_ERROR", "reason": type(exc).__name__, "error": str(exc)}), file=sys.stderr)
        return EXIT_ERROR
    print(json.dumps(result.to_dict(), indent=2 if args.pretty else None, sort_keys=True))
    return EXIT_MISS if result.is_miss else EXIT_VALID


if __name__ == "__main__":
    raise SystemExit(main())
