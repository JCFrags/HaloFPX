#!/usr/bin/env python3
"""Validate the compact, focused L67 provenance manifest."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import subprocess
from pathlib import Path

import halofpx_l65_preexecute_verify as preexecute


REQUIRED_CASES = {
    "real_success",
    "admission_refusals",
    "lifecycle_refusal",
    "transport_refusal",
    "publication_collision",
    "feature_off",
    "compile_off",
    "structural_negatives",
    "reused_l65_handlers",
    "reused_l66_handlers",
}


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def git_blob_sha1(data: bytes) -> str:
    return hashlib.sha1(
        f"blob {len(data)}\0".encode("ascii") + data,
        usedforsecurity=False,
    ).hexdigest()


def verify(path: Path, key: bytes) -> dict[str, object]:
    root = path.resolve().parents[3]
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "halofpx.l67.focused-evidence.v1":
        raise ValueError("schema")
    if manifest.get("base_commit") != "755ba5f2ebd943e8a1204f31be4a80516dc06182":
        raise ValueError("base_commit")
    cases = manifest.get("cases", [])
    identities = {case.get("id") for case in cases}
    if identities != REQUIRED_CASES or len(cases) != len(REQUIRED_CASES):
        raise ValueError(f"case_completeness:{sorted(REQUIRED_CASES - identities)}")
    for case in cases:
        expected_status = "accepted_prior_evidence" if case["id"].startswith("reused_") else "pass"
        if case.get("status") != expected_status or not case.get("receipt"):
            raise ValueError(f"case_semantics:{case.get('id')}")
    validated = 0
    authority_by_case: dict[str, list[Path]] = {}
    for artifact in manifest.get("artifacts", []):
        source = root / artifact["path"]
        if not source.is_file() or digest(source) != artifact["sha256"]:
            raise ValueError(f"artifact:{artifact['path']}")
        if source.stat().st_size != artifact["bytes"]:
            raise ValueError(f"artifact_size:{artifact['path']}")
        case = artifact.get("case")
        if case not in REQUIRED_CASES:
            raise ValueError(f"artifact_case:{artifact['path']}")
        if source.suffix == ".authority":
            authority_by_case.setdefault(case, []).append(source)
        validated += 1
    expected_authority_counts = {
        "real_success": 3,
        "lifecycle_refusal": 1,
        "transport_refusal": 1,
    }
    for case, count in expected_authority_counts.items():
        sources = authority_by_case.get(case, [])
        if len(sources) != count:
            raise ValueError(f"authenticated_receipts:{case}")
        for source in sources:
            result = preexecute.verify(source, key)
            if result["status"] != "pass" or result["terminal_attempts"] != 1:
                raise ValueError(f"authenticated_receipt:{source.name}")
    required_provenance = {
        "source_diff_sha1", "binary_sha256", "protocol", "key_generation",
        "host", "kernel", "processes", "fixture", "receipt_hashes",
    }
    if set(manifest.get("provenance", {})) != required_provenance:
        raise ValueError("provenance")
    if manifest["provenance"]["protocol"] != {
        "scheduler_admission": "3.0",
        "mutable": "3.0",
        "preexecute_grammar": "1.0",
        "manifest": "1.0",
    }:
        raise ValueError("protocol")
    processes = manifest["provenance"]["processes"]
    if not processes or any(
            not isinstance(value, dict) or value.get("pid", 0) <= 0 or
            len(value.get("invocation_id", "")) != 32
            for value in processes.values()):
        raise ValueError("process_provenance")
    receipts = manifest["provenance"]["receipt_hashes"]
    for name, expected in receipts.items():
        artifact = next(
            (item for item in manifest["artifacts"]
             if Path(item["path"]).name == name), None)
        if artifact is None or artifact["sha256"] != expected:
            raise ValueError(f"receipt_hash:{name}")
    binary_receipt_path = root / "docs/halofpx/evidence/l67-binary-receipt.json"
    binary_receipt = json.loads(binary_receipt_path.read_text(encoding="utf-8"))
    binary_tag = bytes.fromhex(binary_receipt.pop("tag"))
    binary_canonical = json.dumps(
        binary_receipt, sort_keys=True, separators=(",", ":")).encode("utf-8")
    expected_binary_tag = hmac.new(
        key, b"halofpx.l67.binary-receipt.v1" + binary_canonical,
        hashlib.sha256).digest()
    if not hmac.compare_digest(binary_tag, expected_binary_tag) or \
       binary_receipt["binaries"] != manifest["provenance"]["binary_sha256"] or \
       binary_receipt["source_diff_sha1"] != manifest["provenance"]["source_diff_sha1"]:
        raise ValueError("binary_receipt")
    diff = subprocess.run(
        ["git", "diff", manifest["base_commit"], "--", "ggml", "src", "tests"],
        cwd=root, check=True, stdout=subprocess.PIPE,
    ).stdout
    if git_blob_sha1(diff) != manifest["provenance"]["source_diff_sha1"]:
        raise ValueError("source_diff_sha1")
    return {
        "schema": "halofpx.l67.focused-evidence-verification.v1",
        "status": "pass",
        "cases": len(cases),
        "artifacts": validated,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--key-file", type=Path, required=True)
    args = parser.parse_args()
    raw = args.key_file.read_bytes()
    key_digest = hashlib.sha256(raw).hexdigest()
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    if key_digest != manifest["provenance"]["fixture"]["key_file_sha256"]:
        raise ValueError("key_file_sha256")
    key_hex = raw.splitlines()[0].decode("ascii")
    key = bytes.fromhex(key_hex)
    if len(key) != 32 or not any(key):
        raise ValueError("key")
    print(json.dumps(verify(args.manifest, key), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
