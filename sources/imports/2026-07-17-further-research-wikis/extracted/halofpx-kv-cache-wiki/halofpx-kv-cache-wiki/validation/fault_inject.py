#!/usr/bin/env python3
"""Inject deterministic cache faults and assert fail-closed outcomes."""
from __future__ import annotations

import argparse
import json
import shutil
import struct
from pathlib import Path
from typing import Callable

from generate_fixtures import generate
from validate_cache import (
    CACHY_RECORD_SIZE, HALO_HEADER_SIZE, LEGACY_VALID, MISS,
    canonical_json_bytes, compute_manifest_hmac, validate_cachyllama_checkpoint,
    validate_halofpx_manifest, validate_halofpx_object,
)


def copy_mutate(src: Path, dst: Path, mutator: Callable[[bytearray], None]) -> Path:
    data = bytearray(src.read_bytes())
    mutator(data)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(data)
    return dst


def set_u32(data: bytearray, offset: int, value: int) -> None:
    data[offset:offset + 4] = struct.pack("<I", value)


def set_u64(data: bytearray, offset: int, value: int) -> None:
    data[offset:offset + 8] = struct.pack("<Q", value)


def run(fixtures: Path, results: Path) -> dict:
    values_file = fixtures / "fixture-values.json"
    if not values_file.exists():
        generate(fixtures)
    values = json.loads(values_file.read_text())
    work = results / "fault-work"
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)

    compat_u64 = int(values["cachyllama"]["compat_u64_hex"], 0)
    legacy_src = fixtures / values["cachyllama"]["checkpoint"]
    halo_src = fixtures / values["halofpx"]["object"]
    manifest_src = fixtures / values["halofpx"]["manifest"]
    object_root = fixtures / values["halofpx"]["object_root"]
    manifest_hmac_key = bytes.fromhex((fixtures / values["halofpx"]["manifest_hmac_key"]).read_text().strip())

    cases: list[dict] = []

    def record(name: str, expected: str, result, passed: bool, note: str = "") -> None:
        cases.append({
            "case": name,
            "expected": expected,
            "actual_status": result.status,
            "actual_reason": result.reason,
            "eligible_for_hit": result.eligible_for_hit,
            "eligible_for_engine_import": result.eligible_for_engine_import,
            "passed": passed,
            "note": note,
        })

    # Legacy structural failures.
    p = copy_mutate(legacy_src, work / "legacy-bad-magic.bin", lambda d: set_u32(d, 0, 0))
    r = validate_cachyllama_checkpoint(p, expected_compat=compat_u64)
    record("legacy_bad_magic", "MISS_RECOMPUTE", r, r.status == MISS)

    p = copy_mutate(legacy_src, work / "legacy-bad-version.bin", lambda d: set_u32(d, 4, 99))
    r = validate_cachyllama_checkpoint(p, expected_compat=compat_u64)
    record("legacy_bad_version", "MISS_RECOMPUTE", r, r.status == MISS)

    truncated = work / "legacy-truncated.bin"
    truncated.write_bytes(legacy_src.read_bytes()[:-1])
    r = validate_cachyllama_checkpoint(truncated, expected_compat=compat_u64)
    record("legacy_truncated_payload", "MISS_RECOMPUTE", r, r.status == MISS)

    p = copy_mutate(legacy_src, work / "legacy-token-overflow.bin", lambda d: set_u32(d, 64, 5000))
    r = validate_cachyllama_checkpoint(p, expected_compat=compat_u64)
    record("legacy_token_count_overflow", "MISS_RECOMPUTE", r, r.status == MISS)

    p = copy_mutate(legacy_src, work / "legacy-length-inflated.bin", lambda d: set_u64(d, 48, (1 << 40)))
    r = validate_cachyllama_checkpoint(p, expected_compat=compat_u64)
    record("legacy_length_inflated", "MISS_RECOMPUTE", r, r.status == MISS)

    r = validate_cachyllama_checkpoint(legacy_src, expected_compat=compat_u64 ^ 1)
    record("legacy_compatibility_mismatch", "MISS_RECOMPUTE", r, r.status == MISS)

    # The critical expected blind spot: same length and header stay valid.
    def flip_legacy_payload(d: bytearray) -> None:
        d[CACHY_RECORD_SIZE + 123] ^= 0x40
    p = copy_mutate(legacy_src, work / "legacy-payload-bitflip.bin", flip_legacy_payload)
    r = validate_cachyllama_checkpoint(p, expected_compat=compat_u64)
    record(
        "legacy_same_length_payload_bitflip",
        "LEGACY_STRUCTURALLY_VALID_UNAUTHENTICATED (not hit eligible)",
        r,
        r.status == LEGACY_VALID and not r.eligible_for_hit,
        "Expected demonstration: the observed outer format has no payload digest.",
    )

    # Halo object corruption must always be a miss.
    p = copy_mutate(halo_src, work / halo_src.name, lambda d: d.__setitem__(0, d[0] ^ 0x01))
    r = validate_halofpx_object(p)
    record("halo_bad_magic", "MISS_RECOMPUTE", r, r.status == MISS)

    p = copy_mutate(halo_src, work / "halo-bad-version.hkv", lambda d: d.__setitem__(8, 2))
    r = validate_halofpx_object(p)
    record("halo_bad_version", "MISS_RECOMPUTE", r, r.status == MISS)

    p = work / "halo-truncated.hkv"
    p.write_bytes(halo_src.read_bytes()[:-1])
    r = validate_halofpx_object(p)
    record("halo_truncated_payload", "MISS_RECOMPUTE", r, r.status == MISS)

    def flip_metadata(d: bytearray) -> None:
        d[HALO_HEADER_SIZE + 5] ^= 0x01
    p = copy_mutate(halo_src, work / "halo-metadata-bitflip.hkv", flip_metadata)
    r = validate_halofpx_object(p)
    record("halo_metadata_bitflip", "MISS_RECOMPUTE", r, r.status == MISS)

    # Read metadata length from fixed header and mutate one payload byte.
    header = struct.unpack("<8sHHIQQII32s32s", halo_src.read_bytes()[:HALO_HEADER_SIZE])
    metadata_len = header[4]
    def flip_payload(d: bytearray) -> None:
        d[HALO_HEADER_SIZE + metadata_len + 123] ^= 0x40
    p = copy_mutate(halo_src, work / "halo-payload-bitflip.hkv", flip_payload)
    r = validate_halofpx_object(p)
    record("halo_same_length_payload_bitflip", "MISS_RECOMPUTE", r, r.status == MISS)

    def inflate_payload_len(d: bytearray) -> None:
        set_u64(d, 24, header[5] + 1)
    p = copy_mutate(halo_src, work / "halo-length-inflated.hkv", inflate_payload_len)
    r = validate_halofpx_object(p)
    record("halo_length_inflated", "MISS_RECOMPUTE", r, r.status == MISS)

    # Manifest authentication and semantic binding.
    manifest = json.loads(manifest_src.read_text())

    r = validate_halofpx_manifest(manifest_src, object_root=object_root)
    record("manifest_auth_key_unavailable", "MISS_RECOMPUTE", r, r.status == MISS and r.reason == "MANIFEST_AUTH_KEY_UNAVAILABLE")

    bad = json.loads(json.dumps(manifest))
    tag = bad["catalog_auth"]["tag_hex"]
    bad["catalog_auth"]["tag_hex"] = ("0" if tag[0] != "0" else "1") + tag[1:]
    p = work / "manifest-bad-hmac.json"
    p.write_bytes(canonical_json_bytes(bad) + b"\n")
    r = validate_halofpx_manifest(p, object_root=object_root, manifest_hmac_key=manifest_hmac_key)
    record("manifest_hmac_failure", "MISS_RECOMPUTE", r, r.status == MISS and r.reason == "MANIFEST_AUTH_FAILURE")

    def sign(bad: dict) -> dict:
        bad["catalog_auth"]["tag_hex"] = compute_manifest_hmac(bad, manifest_hmac_key)
        return bad

    bad = json.loads(json.dumps(manifest))
    bad["object_sha256"] = "f" * 64
    sign(bad)
    p = work / "manifest-object-alias.json"
    p.write_bytes(canonical_json_bytes(bad) + b"\n")
    r = validate_halofpx_manifest(p, object_root=object_root, manifest_hmac_key=manifest_hmac_key)
    record("manifest_object_alias", "MISS_RECOMPUTE", r, r.status == MISS)

    bad = json.loads(json.dumps(manifest))
    bad["required_segments"] = ["target", "recurrent"]
    sign(bad)
    p = work / "manifest-missing-required-segment.json"
    p.write_bytes(canonical_json_bytes(bad) + b"\n")
    r = validate_halofpx_manifest(p, object_root=object_root, manifest_hmac_key=manifest_hmac_key)
    record("manifest_missing_required_segment", "MISS_RECOMPUTE", r, r.status == MISS)

    bad = json.loads(json.dumps(manifest))
    bad["cache_key_sha256"] = "a" * 64
    sign(bad)
    p = work / "manifest-cache-key-alias.json"
    p.write_bytes(canonical_json_bytes(bad) + b"\n")
    r = validate_halofpx_manifest(p, object_root=object_root, manifest_hmac_key=manifest_hmac_key)
    record("manifest_cache_key_alias", "MISS_RECOMPUTE", r, r.status == MISS)

    all_passed = all(c["passed"] for c in cases)
    report = {"all_passed": all_passed, "case_count": len(cases), "cases": cases}
    results.mkdir(parents=True, exist_ok=True)
    (results / "fault-injection.json").write_text(json.dumps(report, indent=2) + "\n")
    md = [
        "# Fault-injection results",
        "",
        f"**All expectations passed:** `{all_passed}`",
        "",
        "| Case | Expected | Actual | Import candidate | Hit eligible | Pass | Note |",
        "|---|---|---|---:|---:|---:|---|",
    ]
    for c in cases:
        md.append(
            f"| `{c['case']}` | {c['expected']} | `{c['actual_status']} / {c['actual_reason']}` | "
            f"{c['eligible_for_engine_import']} | {c['eligible_for_hit']} | {c['passed']} | {c['note']} |"
        )
    (results / "fault-injection.md").write_text("\n".join(md) + "\n")
    return report


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--fixtures", type=Path, default=Path("fixtures"))
    p.add_argument("--results", type=Path, default=Path("results"))
    args = p.parse_args()
    report = run(args.fixtures, args.results)
    print(json.dumps(report, indent=2))
    return 0 if report["all_passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
