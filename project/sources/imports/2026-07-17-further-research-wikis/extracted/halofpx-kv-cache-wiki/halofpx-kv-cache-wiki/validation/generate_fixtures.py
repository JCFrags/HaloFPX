#!/usr/bin/env python3
"""Generate deterministic valid CachyLLama-layout and HaloFPX fixtures."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path

from validate_cache import (
    CACHY_INDEX_FMT, CACHY_INDEX_MAGIC, CACHY_RECORD_FMT, CACHY_RECORD_MAGIC,
    CACHY_RECORD_VERSION, CACHY_SYSTEM_FMT, CACHY_SYSTEM_MAGIC,
    CACHY_SYSTEM_VERSION, CACHY_TOKEN_PREFIX_MAX, HALO_HEADER_FMT,
    HALO_HEADER_SIZE, HALO_MAGIC, canonical_json_bytes, compute_manifest_hmac,
    fnv1a64_tokens,
)

COMPAT_U64 = 0x1122334455667788


def deterministic_bytes(label: bytes, size: int) -> bytes:
    out = bytearray()
    counter = 0
    while len(out) < size:
        out.extend(hashlib.sha256(label + counter.to_bytes(8, "little")).digest())
        counter += 1
    return bytes(out[:size])


def sha256_hex(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def write_cachy(root: Path) -> dict[str, str | int]:
    valid = root / "cachyllama" / "valid"
    valid.mkdir(parents=True, exist_ok=True)
    tokens = [1, 3, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59]
    prefix = tokens + [0] * (CACHY_TOKEN_PREFIX_MAX - len(tokens))
    token_hash = fnv1a64_tokens(tokens)
    target = deterministic_bytes(b"legacy-target", 8192)
    draft = deterministic_bytes(b"legacy-draft", 1024)
    spec = deterministic_bytes(b"legacy-spec", 256)
    header = struct.pack(
        CACHY_RECORD_FMT,
        CACHY_RECORD_MAGIC, CACHY_RECORD_VERSION, 1, 4, 0, len(tokens) - 1,
        len(tokens), 9, len(target), token_hash, len(tokens), COMPAT_U64,
        *prefix, len(draft), len(spec),
    )
    ckpt = valid / "ckpt-1.bin"
    ckpt.write_bytes(header + target + draft + spec)

    index = struct.pack(CACHY_INDEX_FMT, CACHY_INDEX_MAGIC, CACHY_RECORD_VERSION, 2, COMPAT_U64, *([0] * 12))
    (valid / "index.bin").write_bytes(index)

    sys_tokens = [101, 102, 103, 104, 105, 106, 107, 108]
    sys_prefix = sys_tokens + [0] * (CACHY_TOKEN_PREFIX_MAX - len(sys_tokens))
    sys_hash = fnv1a64_tokens(sys_tokens)
    sys_payload = deterministic_bytes(b"legacy-system", 4096)
    sys_header = struct.pack(
        CACHY_SYSTEM_FMT,
        CACHY_SYSTEM_MAGIC, CACHY_SYSTEM_VERSION, sys_hash, len(sys_tokens), len(sys_payload),
        COMPAT_U64, 1700000000, 1700000100, 3, len(sys_tokens), *sys_prefix,
    )
    sys_path = valid / f"sys-{sys_hash:016x}.bin"
    sys_path.write_bytes(sys_header + sys_payload)
    return {
        "compat_u64": COMPAT_U64,
        "compat_u64_hex": hex(COMPAT_U64),
        "token_hash": token_hash,
        "token_hash_hex": hex(token_hash),
        "checkpoint": str(ckpt.relative_to(root)),
        "index": str((valid / "index.bin").relative_to(root)),
        "system": str(sys_path.relative_to(root)),
        "system_hash_hex": f"{sys_hash:016x}",
    }


def write_halofpx(root: Path) -> tuple[dict[str, str | int], dict]:
    base = root / "halofpx"
    objects = base / "objects" / "sha256"
    manifests = base / "manifests" / "private"
    keys = base / "keys"
    manifests.mkdir(parents=True, exist_ok=True)
    keys.mkdir(parents=True, exist_ok=True)
    manifest_hmac_key = hashlib.sha256(b"halofpx-fixture-manifest-hmac-key").digest()
    manifest_hmac_key_path = keys / "fixture-manifest-hmac.key"
    manifest_hmac_key_path.write_text(manifest_hmac_key.hex() + "\n", encoding="ascii")
    manifest_hmac_key_path.chmod(0o600)

    execution_manifest = {
        "schema": "halofpx.execution/v1",
        "engine": {"family": "llama.cpp", "state_abi": "GGSQ-v2+halofpx-adapter-v1"},
        "model_artifacts": [{"name": "model.gguf", "sha256": sha256_hex(b"model-artifact"), "size": 123456789}],
        "tokenizer_sha256": sha256_hex(b"tokenizer"),
        "chat_template_sha256": sha256_hex(b"template"),
        "kv": {"dtype": "f16", "layout": "layer-major", "layers": 32, "kv_heads": 8, "head_dim": 128},
        "runtime": {"n_ctx": 4096, "rope_type": "linear", "rope_scale": "1.0", "swa": False},
        "adapters": [],
    }
    compat = sha256_hex(b"halofpx.compatibility/v1\0" + canonical_json_bytes(execution_manifest))
    prompt_root = sha256_hex(b"halofpx.prompt-root/v1\0" + bytes.fromhex(compat) + b"fixture-prefix")
    cache_key = sha256_hex(b"halofpx.cache-key/v1\0" + bytes.fromhex(compat) + bytes.fromhex(prompt_root) + b"token_page:0:4096")
    namespace_id = sha256_hex(b"fixture-private-namespace")

    target = deterministic_bytes(b"halo-target", 65536)
    draft = deterministic_bytes(b"halo-draft", 8192)
    payload = target + draft
    segments = [
        {"name": "target", "required": True, "offset": 0, "stored_length": len(target), "logical_length": len(target), "stored_sha256": sha256_hex(target), "codec": "none", "encryption": "none", "role": "target_sequence_state"},
        {"name": "draft", "required": False, "offset": len(target), "stored_length": len(draft), "logical_length": len(draft), "stored_sha256": sha256_hex(draft), "codec": "none", "encryption": "none", "role": "draft_sequence_state"},
    ]
    metadata = {
        "schema": "halofpx.kv.object/v1",
        "cache_key_sha256": cache_key,
        "compatibility_fingerprint_sha256": compat,
        "prompt_root_sha256": prompt_root,
        "boundary": {"kind": "token_page", "start_token": 0, "end_token": 4096},
        "engine": {"family": "llama.cpp", "state_abi": "GGSQ-v2+halofpx-adapter-v1"},
        "segments": segments,
        "created_unix_ns": 1700000000000000000,
        "writer_id": "00000000-0000-4000-8000-000000000001",
        "policy": {"partial_reuse": "complete_prefix_only", "optional_draft_catchup": True},
    }
    meta_bytes = canonical_json_bytes(metadata)
    header = struct.pack(
        HALO_HEADER_FMT,
        HALO_MAGIC, 1, 0, HALO_HEADER_SIZE, len(meta_bytes), len(payload), len(segments), 0,
        hashlib.sha256(meta_bytes).digest(), hashlib.sha256(payload).digest(),
    )
    object_bytes = header + meta_bytes + payload
    object_digest = sha256_hex(object_bytes)
    object_dir = objects / object_digest[:2]
    object_dir.mkdir(parents=True, exist_ok=True)
    object_path = object_dir / f"{object_digest}.hkv"
    object_path.write_bytes(object_bytes)

    manifest = {
        "schema": "halofpx.kv.manifest/v1",
        "generation": 1,
        "namespace_id": namespace_id,
        "engine_family": "llama.cpp",
        "cache_key_sha256": cache_key,
        "compatibility_fingerprint_sha256": compat,
        "prompt_root_sha256": prompt_root,
        "object_sha256": object_digest,
        "object_size": len(object_bytes),
        "created_unix_ns": 1700000000000000000,
        "last_access_unix_ns": 1700000000000000000,
        "verified_prefix_tokens": 4096,
        "required_segments": ["target"],
        "encryption_policy": "none",
        "state": "committed",
        "catalog_auth": {
            "mode": "hmac-sha256",
            "key_id": "fixture/catalog-auth/1",
            "tag_hex": "0" * 64,
        },
    }
    manifest["catalog_auth"]["tag_hex"] = compute_manifest_hmac(manifest, manifest_hmac_key)
    manifest_path = manifests / "example.json"
    manifest_path.write_bytes(canonical_json_bytes(manifest) + b"\n")
    values = {
        "compatibility_fingerprint_sha256": compat,
        "prompt_root_sha256": prompt_root,
        "cache_key_sha256": cache_key,
        "namespace_id": namespace_id,
        "engine_family": manifest["engine_family"],
        "object_sha256": object_digest,
        "object": str(object_path.relative_to(root)),
        "object_root": str(objects.relative_to(root)),
        "manifest": str(manifest_path.relative_to(root)),
        "manifest_hmac_key": str(manifest_hmac_key_path.relative_to(root)),
        "manifest_hmac_key_id": manifest["catalog_auth"]["key_id"],
    }
    return values, manifest


def generate(output: Path, schema_sample: Path | None = None) -> dict:
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)
    cachy = write_cachy(output)
    halo, manifest = write_halofpx(output)
    values = {"cachyllama": cachy, "halofpx": halo}
    (output / "fixture-values.json").write_text(json.dumps(values, indent=2) + "\n", encoding="utf-8")
    if schema_sample:
        schema_sample.parent.mkdir(parents=True, exist_ok=True)
        schema_sample.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return values


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--output", type=Path, default=Path("fixtures"))
    p.add_argument("--schema-sample", type=Path)
    args = p.parse_args()
    values = generate(args.output, args.schema_sample)
    print(json.dumps(values, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
