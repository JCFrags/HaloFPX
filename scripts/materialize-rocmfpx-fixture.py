#!/usr/bin/env python3
"""Materialize and validate the pinned small ROCmFPX GGUF fixture.

Large model bytes stay outside the Git checkout.  This tool treats the tracked
registry as an exact byte and provenance contract; it does not benchmark or
make model-quality claims.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.request
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REGISTRY = (
    REPO_ROOT / "docs" / "halofpx" / "fixtures" / "qwen3-0.6b-rocmfpx" / "registry.json"
)
SHA256_RE = re.compile(r"[0-9a-f]{64}")
COMMIT_RE = re.compile(r"[0-9a-f]{40}")
BUFFER_SIZE = 1024 * 1024


class FixtureError(RuntimeError):
    """A contract, provenance, or materialization check failed."""


def require_mapping(value: Any, where: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise FixtureError(f"{where} must be a JSON object")
    return value


def require_list(value: Any, where: str) -> list[Any]:
    if not isinstance(value, list):
        raise FixtureError(f"{where} must be a JSON array")
    return value


def expect_keys(
    value: dict[str, Any],
    required: set[str],
    where: str,
    optional: set[str] | None = None,
) -> None:
    optional = optional or set()
    missing = required - value.keys()
    extra = value.keys() - required - optional
    if missing or extra:
        details = []
        if missing:
            details.append(f"missing={sorted(missing)}")
        if extra:
            details.append(f"unknown={sorted(extra)}")
        raise FixtureError(f"{where} has invalid keys: {', '.join(details)}")


def require_string(value: Any, where: str) -> str:
    if not isinstance(value, str) or not value:
        raise FixtureError(f"{where} must be a non-empty string")
    return value


def require_positive_int(value: Any, where: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
        raise FixtureError(f"{where} must be a positive integer")
    return value


def require_sha256(value: Any, where: str) -> str:
    value = require_string(value, where)
    if SHA256_RE.fullmatch(value) is None:
        raise FixtureError(f"{where} must be lowercase SHA-256 hex")
    return value


def require_commit(value: Any, where: str) -> str:
    value = require_string(value, where)
    if COMMIT_RE.fullmatch(value) is None:
        raise FixtureError(f"{where} must be a full lowercase Git commit")
    return value


def require_relative_path(value: Any, where: str) -> str:
    value = require_string(value, where)
    path = PurePosixPath(value)
    if path.is_absolute() or ".." in path.parts or path.as_posix() != value:
        raise FixtureError(f"{where} must be a normalized relative POSIX path")
    return value


def require_https_huggingface_url(value: Any, where: str) -> str:
    value = require_string(value, where)
    if not value.startswith("https://huggingface.co/"):
        raise FixtureError(f"{where} must use https://huggingface.co/")
    return value


def validate_registry(registry: dict[str, Any], registry_path: Path) -> None:
    expect_keys(
        registry,
        {
            "schema_version",
            "fixture_id",
            "tracking_issue",
            "scope",
            "source",
            "source_sidecars",
            "tracked_source_captures",
            "producer",
            "smoke_consumer",
            "derived",
            "publication",
        },
        "registry",
    )
    if registry["schema_version"] != 1:
        raise FixtureError("registry.schema_version must be 1")
    require_string(registry["fixture_id"], "registry.fixture_id")
    if registry["tracking_issue"] != "https://github.com/JCFrags/HaloFPX/issues/43":
        raise FixtureError("registry.tracking_issue must identify issue #43")

    scope = require_mapping(registry["scope"], "registry.scope")
    expect_keys(
        scope,
        {"role", "environment", "claim_boundary", "target_qualification"},
        "registry.scope",
    )
    for key in scope:
        require_string(scope[key], f"registry.scope.{key}")

    source = require_mapping(registry["source"], "registry.source")
    expect_keys(
        source,
        {
            "provider",
            "repository",
            "revision",
            "filename",
            "relative_path",
            "url",
            "size_bytes",
            "sha256",
            "architecture",
            "publisher_file_type",
            "license",
        },
        "registry.source",
    )
    require_commit(source["revision"], "registry.source.revision")
    require_relative_path(source["relative_path"], "registry.source.relative_path")
    require_https_huggingface_url(source["url"], "registry.source.url")
    require_positive_int(source["size_bytes"], "registry.source.size_bytes")
    require_sha256(source["sha256"], "registry.source.sha256")
    require_positive_int(
        source["publisher_file_type"], "registry.source.publisher_file_type"
    )
    for key in ("provider", "repository", "filename", "architecture"):
        require_string(source[key], f"registry.source.{key}")

    license_info = require_mapping(source["license"], "registry.source.license")
    expect_keys(
        license_info,
        {
            "spdx",
            "declaration_authority",
            "distribution_card_url",
            "base_model",
            "base_model_revision_note",
            "bundled_license_path",
            "bundled_license_size_bytes",
            "bundled_license_sha256",
            "bundled_license_note",
        },
        "registry.source.license",
    )
    if license_info["spdx"] != "Apache-2.0":
        raise FixtureError("registry.source.license.spdx must be Apache-2.0")
    require_https_huggingface_url(
        license_info["distribution_card_url"],
        "registry.source.license.distribution_card_url",
    )
    bundled_path = require_relative_path(
        license_info["bundled_license_path"],
        "registry.source.license.bundled_license_path",
    )
    require_positive_int(
        license_info["bundled_license_size_bytes"],
        "registry.source.license.bundled_license_size_bytes",
    )
    require_sha256(
        license_info["bundled_license_sha256"],
        "registry.source.license.bundled_license_sha256",
    )
    for key in (
        "declaration_authority",
        "base_model",
        "base_model_revision_note",
        "bundled_license_note",
    ):
        require_string(license_info[key], f"registry.source.license.{key}")

    sidecars = require_list(registry["source_sidecars"], "registry.source_sidecars")
    if not sidecars:
        raise FixtureError("registry.source_sidecars must not be empty")
    for index, raw in enumerate(sidecars):
        sidecar = require_mapping(raw, f"registry.source_sidecars[{index}]")
        expect_keys(
            sidecar,
            {"role", "filename", "relative_path", "url", "size_bytes", "sha256"},
            f"registry.source_sidecars[{index}]",
            {"repository", "revision"},
        )
        for key in ("role", "filename"):
            require_string(sidecar[key], f"registry.source_sidecars[{index}].{key}")
        require_relative_path(
            sidecar["relative_path"],
            f"registry.source_sidecars[{index}].relative_path",
        )
        require_https_huggingface_url(
            sidecar["url"], f"registry.source_sidecars[{index}].url"
        )
        require_positive_int(
            sidecar["size_bytes"],
            f"registry.source_sidecars[{index}].size_bytes",
        )
        require_sha256(sidecar["sha256"], f"registry.source_sidecars[{index}].sha256")
        if "repository" in sidecar:
            require_string(
                sidecar["repository"],
                f"registry.source_sidecars[{index}].repository",
            )
        if "revision" in sidecar:
            require_commit(
                sidecar["revision"],
                f"registry.source_sidecars[{index}].revision",
            )

    captures = require_list(
        registry["tracked_source_captures"], "registry.tracked_source_captures"
    )
    if not captures:
        raise FixtureError("registry.tracked_source_captures must not be empty")
    for index, raw in enumerate(captures):
        capture = require_mapping(raw, f"registry.tracked_source_captures[{index}]")
        expect_keys(
            capture,
            {
                "role",
                "path",
                "source_size_bytes",
                "source_sha256",
                "tracked_size_bytes",
                "tracked_sha256",
                "normalization",
            },
            f"registry.tracked_source_captures[{index}]",
        )
        require_string(
            capture["role"], f"registry.tracked_source_captures[{index}].role"
        )
        path = require_relative_path(
            capture["path"], f"registry.tracked_source_captures[{index}].path"
        )
        for key in ("source_size_bytes", "tracked_size_bytes"):
            require_positive_int(
                capture[key], f"registry.tracked_source_captures[{index}].{key}"
            )
        for key in ("source_sha256", "tracked_sha256"):
            require_sha256(
                capture[key], f"registry.tracked_source_captures[{index}].{key}"
            )
        require_string(
            capture["normalization"],
            f"registry.tracked_source_captures[{index}].normalization",
        )
        verify_file(
            safe_repo_path(path),
            capture["tracked_size_bytes"],
            capture["tracked_sha256"],
            f"tracked source capture {index}",
        )

    producer = require_mapping(registry["producer"], "registry.producer")
    expect_keys(
        producer,
        {
            "repository",
            "requested_main_commit",
            "quantizer_commit",
            "quantizer_commit_role",
            "compatibility_reason",
            "observed_quantizer_binary",
            "arguments",
            "threads_used_for_recorded_artifacts",
        },
        "registry.producer",
    )
    require_commit(
        producer["requested_main_commit"], "registry.producer.requested_main_commit"
    )
    require_commit(producer["quantizer_commit"], "registry.producer.quantizer_commit")
    for key in ("repository", "quantizer_commit_role", "compatibility_reason"):
        require_string(producer[key], f"registry.producer.{key}")
    require_positive_int(
        producer["threads_used_for_recorded_artifacts"],
        "registry.producer.threads_used_for_recorded_artifacts",
    )
    validate_binary_record(
        producer["observed_quantizer_binary"],
        "registry.producer.observed_quantizer_binary",
    )
    validate_argv(producer["arguments"], "registry.producer.arguments")

    consumer = require_mapping(registry["smoke_consumer"], "registry.smoke_consumer")
    expect_keys(
        consumer,
        {"commit", "observed_completion_binary", "arguments", "acceptance"},
        "registry.smoke_consumer",
    )
    require_commit(consumer["commit"], "registry.smoke_consumer.commit")
    validate_binary_record(
        consumer["observed_completion_binary"],
        "registry.smoke_consumer.observed_completion_binary",
    )
    validate_argv(consumer["arguments"], "registry.smoke_consumer.arguments")
    require_string(consumer["acceptance"], "registry.smoke_consumer.acceptance")

    derived = require_list(registry["derived"], "registry.derived")
    if len(derived) != 3:
        raise FixtureError("registry.derived must contain exactly Q3, Q6, and Q8")
    expected_formats = {"Q3_0_ROCMFPX", "Q6_0_ROCMFPX", "Q8_0_ROCMFPX"}
    seen_formats: set[str] = set()
    for index, raw in enumerate(derived):
        item = require_mapping(raw, f"registry.derived[{index}]")
        expect_keys(
            item,
            {
                "id",
                "format",
                "filename",
                "relative_path",
                "size_bytes",
                "sha256",
                "gguf",
            },
            f"registry.derived[{index}]",
        )
        for key in ("id", "format", "filename"):
            require_string(item[key], f"registry.derived[{index}].{key}")
        seen_formats.add(item["format"])
        require_relative_path(
            item["relative_path"], f"registry.derived[{index}].relative_path"
        )
        require_positive_int(
            item["size_bytes"], f"registry.derived[{index}].size_bytes"
        )
        require_sha256(item["sha256"], f"registry.derived[{index}].sha256")
        gguf = require_mapping(item["gguf"], f"registry.derived[{index}].gguf")
        expect_keys(
            gguf,
            {"architecture", "file_type", "tensor_count", "tensor_types"},
            f"registry.derived[{index}].gguf",
        )
        require_string(
            gguf["architecture"], f"registry.derived[{index}].gguf.architecture"
        )
        require_positive_int(
            gguf["file_type"], f"registry.derived[{index}].gguf.file_type"
        )
        require_positive_int(
            gguf["tensor_count"], f"registry.derived[{index}].gguf.tensor_count"
        )
        tensor_types = require_mapping(
            gguf["tensor_types"], f"registry.derived[{index}].gguf.tensor_types"
        )
        if set(tensor_types) != {"F32", item["format"]}:
            raise FixtureError(
                f"registry.derived[{index}].gguf.tensor_types must contain F32 and {item['format']}"
            )
        for type_name, raw_census in tensor_types.items():
            census = require_mapping(
                raw_census,
                f"registry.derived[{index}].gguf.tensor_types.{type_name}",
            )
            expect_keys(
                census,
                {"count", "elements", "packed_bytes"},
                f"registry.derived[{index}].gguf.tensor_types.{type_name}",
            )
            for key in census:
                require_positive_int(
                    census[key],
                    f"registry.derived[{index}].gguf.tensor_types.{type_name}.{key}",
                )
    if seen_formats != expected_formats:
        raise FixtureError(
            f"registry.derived formats must be {sorted(expected_formats)}"
        )

    publication = require_mapping(registry["publication"], "registry.publication")
    expect_keys(
        publication,
        {
            "large_assets_published",
            "ordinary_git_limit",
            "ordinary_git_limit_source",
            "release_asset_limit",
            "release_asset_limit_source",
            "permission_observation",
            "required_before_publication",
        },
        "registry.publication",
    )
    if publication["large_assets_published"] is not False:
        raise FixtureError(
            "registry.publication.large_assets_published must remain false"
        )
    for key in publication:
        if key != "large_assets_published":
            require_string(publication[key], f"registry.publication.{key}")
    for key in ("ordinary_git_limit_source", "release_asset_limit_source"):
        if not publication[key].startswith("https://docs.github.com/"):
            raise FixtureError(
                f"registry.publication.{key} must use official GitHub Docs"
            )

    artifact_paths = [source["relative_path"]]
    artifact_paths.extend(sidecar["relative_path"] for sidecar in sidecars)
    artifact_paths.extend(item["relative_path"] for item in derived)
    if len(artifact_paths) != len(set(artifact_paths)):
        raise FixtureError("artifact relative paths must be unique")

    license_path = safe_repo_path(bundled_path)
    verify_file(
        license_path,
        license_info["bundled_license_size_bytes"],
        license_info["bundled_license_sha256"],
        "bundled license",
    )
    if registry_path.resolve() != DEFAULT_REGISTRY.resolve():
        print(
            f"warning: validating non-default registry {registry_path}", file=sys.stderr
        )


def validate_binary_record(value: Any, where: str) -> None:
    record = require_mapping(value, where)
    expect_keys(record, {"platform", "sha256"}, where)
    require_string(record["platform"], f"{where}.platform")
    require_sha256(record["sha256"], f"{where}.sha256")


def validate_argv(value: Any, where: str) -> None:
    argv = require_list(value, where)
    if not argv or not all(isinstance(item, str) and item for item in argv):
        raise FixtureError(f"{where} must contain non-empty strings")


def safe_repo_path(relative: str) -> Path:
    target = (REPO_ROOT / PurePosixPath(relative)).resolve()
    if not target.is_relative_to(REPO_ROOT.resolve()):
        raise FixtureError(f"repository path escapes checkout: {relative}")
    return target


def safe_artifact_path(root: Path, relative: str) -> Path:
    root = root.resolve()
    target = (root / PurePosixPath(relative)).resolve()
    if not target.is_relative_to(root):
        raise FixtureError(f"artifact path escapes root: {relative}")
    return target


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while chunk := handle.read(BUFFER_SIZE):
            digest.update(chunk)
    return digest.hexdigest()


def verify_file(
    path: Path, size: int, expected_sha256: str, role: str
) -> dict[str, Any]:
    if not path.is_file():
        raise FixtureError(f"missing {role}: {path}")
    actual_size = path.stat().st_size
    if actual_size != size:
        raise FixtureError(f"wrong size for {role}: {path}: {actual_size} != {size}")
    actual_sha256 = sha256_file(path)
    if actual_sha256 != expected_sha256:
        raise FixtureError(
            f"wrong SHA-256 for {role}: {path}: {actual_sha256} != {expected_sha256}"
        )
    return {"path": str(path), "size_bytes": actual_size, "sha256": actual_sha256}


def archive_partial(root: Path, partial: Path, label: str) -> Path:
    logs = safe_artifact_path(root, "logs")
    logs.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    destination = safe_artifact_path(root, f"logs/failed-{label}-{stamp}.partial")
    os.replace(partial, destination)
    return destination


def ensure_free_space(root: Path, required_bytes: int, action: str) -> None:
    root.mkdir(parents=True, exist_ok=True)
    free_bytes = shutil.disk_usage(root).free
    reserve_bytes = 64 * 1024 * 1024
    required_with_reserve = required_bytes + reserve_bytes
    if free_bytes < required_with_reserve:
        raise FixtureError(
            f"insufficient free space for {action} at {root}: "
            f"free={free_bytes}, required_with_reserve={required_with_reserve}"
        )


def download_one(root: Path, record: dict[str, Any], label: str) -> dict[str, Any]:
    final = safe_artifact_path(root, record["relative_path"])
    final.parent.mkdir(parents=True, exist_ok=True)
    if final.exists():
        return verify_file(final, record["size_bytes"], record["sha256"], label)

    partial = final.with_name(final.name + ".partial")
    if partial.exists():
        archived = archive_partial(root, partial, f"stale-download-{label}")
        print(f"archived stale partial as {archived}", file=sys.stderr)
    request = urllib.request.Request(
        record["url"], headers={"User-Agent": "HaloFPX-portable-fixture/1"}
    )
    digest = hashlib.sha256()
    count = 0
    print(f"downloading {label}: {record['url']}", file=sys.stderr)
    try:
        with (
            urllib.request.urlopen(request, timeout=60) as response,
            partial.open("xb") as output,
        ):
            while chunk := response.read(BUFFER_SIZE):
                output.write(chunk)
                digest.update(chunk)
                count += len(chunk)
            output.flush()
            os.fsync(output.fileno())
    except Exception:
        if partial.exists():
            archived = archive_partial(root, partial, f"download-{label}")
            print(f"retained failed download as {archived}", file=sys.stderr)
        raise
    if count != record["size_bytes"] or digest.hexdigest() != record["sha256"]:
        archived = archive_partial(root, partial, f"download-identity-{label}")
        raise FixtureError(
            f"download identity mismatch for {label}; retained {archived}: "
            f"bytes={count}, sha256={digest.hexdigest()}"
        )
    os.replace(partial, final)
    return verify_file(final, record["size_bytes"], record["sha256"], label)


def git_head(source: Path, role: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(source), "rev-parse", "HEAD"],
        check=False,
        capture_output=True,
        text=True,
        shell=False,
    )
    if result.returncode != 0:
        raise FixtureError(
            f"cannot resolve {role} Git HEAD at {source}: {result.stderr.strip()}"
        )
    return result.stdout.strip()


def require_exact_git_head(source: Path, expected: str, role: str) -> None:
    actual = git_head(source, role)
    if actual != expected:
        raise FixtureError(
            f"{role} must be exact commit {expected}; found {actual} at {source}"
        )
    result = subprocess.run(
        ["git", "-C", str(source), "status", "--porcelain", "--untracked-files=normal"],
        check=False,
        capture_output=True,
        text=True,
        shell=False,
    )
    if result.returncode != 0:
        raise FixtureError(
            f"cannot inspect {role} worktree at {source}: {result.stderr.strip()}"
        )
    if result.stdout.strip():
        raise FixtureError(f"{role} worktree must be clean: {source}")


def inspect_binary_identity(
    path: Path, expected_sha256: str, role: str, require_recorded: bool = False
) -> dict[str, Any]:
    if not path.is_file():
        raise FixtureError(f"{role} is not a file: {path}")
    actual = sha256_file(path)
    matches_recorded = actual == expected_sha256
    if require_recorded and not matches_recorded:
        raise FixtureError(
            f"{role} does not match the recorded evidence binary: "
            f"{actual} != {expected_sha256}: {path}"
        )
    return {
        "path": str(path),
        "sha256": actual,
        "matches_recorded_observation": matches_recorded,
    }


def require_reported_commit(text: str, expected: str, role: str) -> str:
    match = re.search(
        r"(?:build\s*=\s*\d+|version:\s*\d+)\s*\(([0-9a-f]{8,40})\)", text
    )
    if match is None:
        raise FixtureError(f"{role} did not report an embedded Git commit")
    reported = match.group(1)
    if len(reported) < 8 or not expected.startswith(reported):
        raise FixtureError(
            f"{role} reported commit {reported}, expected {expected}: source/binary mismatch"
        )
    return reported


def probe_version_commit(path: Path, expected: str, role: str) -> str:
    result = subprocess.run(
        [str(path), "--version"],
        check=False,
        capture_output=True,
        text=True,
        shell=False,
    )
    if result.returncode != 0:
        raise FixtureError(
            f"cannot query {role} version (exit {result.returncode}): "
            f"{result.stderr.strip()}"
        )
    return require_reported_commit(result.stdout + "\n" + result.stderr, expected, role)


def create_attempt_log(root: Path, operation: str, item_id: str) -> tuple[Path, Any]:
    attempts = safe_artifact_path(root, "logs/attempts")
    attempts.mkdir(parents=True, exist_ok=True)
    for _ in range(10):
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
        destination = safe_artifact_path(
            root, f"logs/attempts/{stamp}-{operation}-{item_id}.txt"
        )
        try:
            return destination, destination.open("xb")
        except FileExistsError:
            pass
        time.sleep(0.001)
    raise FixtureError(
        f"cannot allocate a unique retained log for {operation}/{item_id}"
    )


def materialize_downloads(root: Path, registry: dict[str, Any]) -> list[dict[str, Any]]:
    records = [("source GGUF", registry["source"])]
    records.extend(
        (f"source sidecar {item['filename']}", item)
        for item in registry["source_sidecars"]
    )
    required_bytes = sum(
        record["size_bytes"]
        for _, record in records
        if not safe_artifact_path(root, record["relative_path"]).exists()
    )
    ensure_free_space(root, required_bytes, "fixture download")
    return [download_one(root, record, label) for label, record in records]


def verify_source(root: Path, registry: dict[str, Any]) -> list[dict[str, Any]]:
    records = [("source GGUF", registry["source"])]
    records.extend(
        (f"source sidecar {item['filename']}", item)
        for item in registry["source_sidecars"]
    )
    return [
        verify_file(
            safe_artifact_path(root, record["relative_path"]),
            record["size_bytes"],
            record["sha256"],
            label,
        )
        for label, record in records
    ]


def verify_derived(root: Path, registry: dict[str, Any]) -> list[dict[str, Any]]:
    return [
        verify_file(
            safe_artifact_path(root, item["relative_path"]),
            item["size_bytes"],
            item["sha256"],
            f"derived artifact {item['id']}",
        )
        for item in registry["derived"]
    ]


def render_argv(
    template: list[str], replacements: dict[str, str], where: str
) -> list[str]:
    result = []
    for item in template:
        rendered = item
        for placeholder, replacement in replacements.items():
            rendered = rendered.replace("{" + placeholder + "}", replacement)
        if "{" in rendered or "}" in rendered:
            raise FixtureError(f"unresolved placeholder in {where}: {rendered}")
        result.append(rendered)
    return result


def quantize(
    root: Path,
    registry: dict[str, Any],
    quantizer: Path,
    quantizer_source: Path,
    threads: int,
    require_recorded_binary: bool = False,
) -> list[dict[str, Any]]:
    require_exact_git_head(
        quantizer_source,
        registry["producer"]["quantizer_commit"],
        "fixture quantizer source",
    )
    inspect_binary_identity(
        quantizer,
        registry["producer"]["observed_quantizer_binary"]["sha256"],
        "fixture quantizer binary",
        require_recorded_binary,
    )
    verify_file(
        safe_artifact_path(root, registry["source"]["relative_path"]),
        registry["source"]["size_bytes"],
        registry["source"]["sha256"],
        "source GGUF",
    )
    required_bytes = sum(
        item["size_bytes"]
        for item in registry["derived"]
        if not safe_artifact_path(root, item["relative_path"]).exists()
    )
    ensure_free_space(root, required_bytes, "fixture quantization")
    logs = safe_artifact_path(root, "logs")
    logs.mkdir(parents=True, exist_ok=True)
    results = []
    for item in registry["derived"]:
        final = safe_artifact_path(root, item["relative_path"])
        final.parent.mkdir(parents=True, exist_ok=True)
        if final.exists():
            results.append(
                verify_file(
                    final,
                    item["size_bytes"],
                    item["sha256"],
                    f"derived artifact {item['id']}",
                )
            )
            continue
        partial = final.with_name(final.name + ".partial")
        if partial.exists():
            archived = archive_partial(root, partial, f"stale-quantize-{item['id']}")
            print(f"archived stale partial as {archived}", file=sys.stderr)
        argv = [str(quantizer)] + render_argv(
            registry["producer"]["arguments"],
            {
                "source": str(
                    safe_artifact_path(root, registry["source"]["relative_path"])
                ),
                "output": str(partial),
                "format": item["format"],
                "threads": str(threads),
            },
            "registry.producer.arguments",
        )
        log_path, log = create_attempt_log(root, "quantize", item["id"])
        print(f"quantizing {item['id']}; log={log_path}", file=sys.stderr)
        with log:
            result = subprocess.run(
                argv,
                check=False,
                stdout=log,
                stderr=subprocess.STDOUT,
                shell=False,
            )
        if result.returncode != 0:
            retained = (
                archive_partial(root, partial, f"quantize-{item['id']}")
                if partial.exists()
                else None
            )
            raise FixtureError(
                f"quantizer failed for {item['id']} with exit {result.returncode}; "
                f"log={log_path}; partial={retained}"
            )
        log_text = log_path.read_text(encoding="utf-8", errors="replace")
        require_reported_commit(
            log_text,
            registry["producer"]["quantizer_commit"],
            "fixture quantizer binary",
        )
        try:
            verified = verify_file(
                partial,
                item["size_bytes"],
                item["sha256"],
                f"new partial artifact {item['id']}",
            )
        except FixtureError:
            retained = (
                archive_partial(root, partial, f"quantize-identity-{item['id']}")
                if partial.exists()
                else None
            )
            raise FixtureError(
                f"quantizer output identity mismatch for {item['id']}; retained {retained}; log={log_path}"
            )
        os.replace(partial, final)
        verified["path"] = str(final)
        results.append(verified)
    return results


def load_gguf_module() -> Any:
    gguf_path = REPO_ROOT / "gguf-py"
    sys.path.insert(0, str(gguf_path))
    try:
        import gguf  # type: ignore
    except (ImportError, ModuleNotFoundError) as exc:
        raise FixtureError(
            "census requires the repository gguf-py package and its numpy dependency"
        ) from exc
    expected_layouts = {
        "Q3_0_ROCMFPX": (104, (32, 14)),
        "Q6_0_ROCMFPX": (102, (32, 26)),
        "Q8_0_ROCMFPX": (103, (32, 33)),
    }
    for name, (enum_value, block) in expected_layouts.items():
        tensor_type = gguf.GGMLQuantizationType[name]
        if tensor_type.value != enum_value:
            raise FixtureError(
                f"repository gguf-py {name} enum changed: {tensor_type.value}"
            )
        actual_block = tuple(int(value) for value in gguf.GGML_QUANT_SIZES[tensor_type])
        if actual_block != block:
            raise FixtureError(
                f"repository gguf-py {name} block changed: {actual_block}"
            )
    return gguf


def census_one(path: Path, expected: dict[str, Any], gguf: Any) -> dict[str, Any]:
    reader = gguf.GGUFReader(path, "r")
    try:
        architecture_field = reader.get_field("general.architecture")
        file_type_field = reader.get_field("general.file_type")
        if architecture_field is None or file_type_field is None:
            raise FixtureError(f"missing required GGUF metadata in {path}")
        architecture = str(architecture_field.contents())
        file_type = int(file_type_field.contents())
        census: dict[str, dict[str, int]] = {}
        for tensor in reader.tensors:
            name = tensor.tensor_type.name
            entry = census.setdefault(
                name, {"count": 0, "elements": 0, "packed_bytes": 0}
            )
            entry["count"] += 1
            entry["elements"] += int(tensor.n_elements)
            entry["packed_bytes"] += int(tensor.n_bytes)
        actual = {
            "architecture": architecture,
            "file_type": file_type,
            "tensor_count": len(reader.tensors),
            "tensor_types": dict(sorted(census.items())),
        }
    finally:
        mmap = getattr(reader.data, "_mmap", None)
        if mmap is not None:
            mmap.close()
    if actual != expected:
        raise FixtureError(
            f"GGUF census mismatch for {path}:\nexpected={json.dumps(expected, sort_keys=True)}\n"
            f"actual={json.dumps(actual, sort_keys=True)}"
        )
    return {"path": str(path), **actual}


def census_all(root: Path, registry: dict[str, Any]) -> list[dict[str, Any]]:
    gguf = load_gguf_module()
    return [
        census_one(safe_artifact_path(root, item["relative_path"]), item["gguf"], gguf)
        for item in registry["derived"]
    ]


def smoke(
    root: Path,
    registry: dict[str, Any],
    completion: Path,
    completion_source: Path,
    require_recorded_binary: bool = False,
) -> list[dict[str, Any]]:
    require_exact_git_head(
        completion_source,
        registry["smoke_consumer"]["commit"],
        "fixture smoke-consumer source",
    )
    inspect_binary_identity(
        completion,
        registry["smoke_consumer"]["observed_completion_binary"]["sha256"],
        "fixture smoke-consumer binary",
        require_recorded_binary,
    )
    probe_version_commit(
        completion,
        registry["smoke_consumer"]["commit"],
        "fixture smoke-consumer binary",
    )
    verify_derived(root, registry)
    logs = safe_artifact_path(root, "logs")
    logs.mkdir(parents=True, exist_ok=True)
    results = []
    environment = os.environ.copy()
    environment["GGML_LOG_COLOR"] = "0"
    for item in registry["derived"]:
        model = safe_artifact_path(root, item["relative_path"])
        argv = [str(completion)] + render_argv(
            registry["smoke_consumer"]["arguments"],
            {"model": str(model)},
            "registry.smoke_consumer.arguments",
        )
        log_path, log = create_attempt_log(root, "smoke", item["id"])
        print(f"smoking {item['id']}; log={log_path}", file=sys.stderr)
        with log:
            result = subprocess.run(
                argv,
                check=False,
                stdout=log,
                stderr=subprocess.STDOUT,
                shell=False,
                cwd=completion_source,
                env=environment,
            )
        if result.returncode != 0:
            raise FixtureError(
                f"CPU smoke failed for {item['id']} with exit {result.returncode}; log={log_path}"
            )
        log_text = log_path.read_text(encoding="utf-8", errors="replace")
        required_markers = ("llama backend init", "generate: n_ctx", "n_predict = 4")
        missing_markers = [
            marker for marker in required_markers if marker not in log_text
        ]
        if missing_markers:
            raise FixtureError(
                f"CPU smoke evidence incomplete for {item['id']}: "
                f"missing={missing_markers}; log={log_path}"
            )
        generated_lines = [
            line
            for line in log_text.splitlines()
            if line.startswith(" ") and " I " not in line and " W " not in line
        ]
        if not generated_lines:
            raise FixtureError(
                f"CPU smoke produced no retained generated-text line for {item['id']}; "
                f"log={log_path}"
            )
        results.append(
            {"id": item["id"], "exit_code": result.returncode, "log": str(log_path)}
        )
    return results


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registry", type=Path, default=DEFAULT_REGISTRY)
    parser.add_argument("--artifact-root", type=Path)
    parser.add_argument("--validate-registry", action="store_true")
    parser.add_argument("--download", action="store_true")
    parser.add_argument("--quantize", action="store_true")
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--verify-source", action="store_true")
    parser.add_argument("--verify-derived", action="store_true")
    parser.add_argument("--census", action="store_true")
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument("--quantizer", type=Path)
    parser.add_argument("--quantizer-source", type=Path)
    parser.add_argument("--completion", type=Path)
    parser.add_argument("--completion-source", type=Path)
    parser.add_argument("--threads", type=int, default=8)
    parser.add_argument(
        "--require-recorded-binary",
        action="store_true",
        help="also require the exact observed evidence-binary SHA-256",
    )
    args = parser.parse_args()
    if not any(
        (
            args.validate_registry,
            args.download,
            args.quantize,
            args.verify,
            args.verify_source,
            args.verify_derived,
            args.census,
            args.smoke,
        )
    ):
        parser.error("select at least one action")
    if args.threads <= 0:
        parser.error("--threads must be positive")
    if args.quantize and (args.quantizer is None or args.quantizer_source is None):
        parser.error("--quantize requires --quantizer and --quantizer-source")
    if args.smoke and (args.completion is None or args.completion_source is None):
        parser.error("--smoke requires --completion and --completion-source")
    return args


def resolve_artifact_root(args: argparse.Namespace, registry: dict[str, Any]) -> Path:
    if args.artifact_root is not None:
        root = args.artifact_root
    elif os.environ.get("HALOFPX_FIXTURE_ROOT"):
        root = Path(os.environ["HALOFPX_FIXTURE_ROOT"])
    else:
        root = Path.home() / "halofpx-fixtures" / registry["fixture_id"]
    root = root.expanduser().resolve()
    repository = REPO_ROOT.resolve()
    if (
        root == repository
        or root.is_relative_to(repository)
        or repository.is_relative_to(root)
    ):
        raise FixtureError(
            f"artifact root must remain outside the Git checkout: {root}"
        )
    if root == Path(root.anchor):
        raise FixtureError(f"artifact root must not be a filesystem root: {root}")
    return root


def main() -> int:
    args = parse_args()
    try:
        registry_path = args.registry.expanduser().resolve()
        with registry_path.open("r", encoding="utf-8") as handle:
            registry = json.load(handle)
        registry = require_mapping(registry, "registry")
        validate_registry(registry, registry_path)
        root = resolve_artifact_root(args, registry)
        summary: dict[str, Any] = {
            "fixture_id": registry["fixture_id"],
            "registry": str(registry_path),
            "artifact_root": str(root),
            "registry_valid": True,
            "claim_boundary": registry["scope"]["claim_boundary"],
        }
        if args.download:
            summary["downloads"] = materialize_downloads(root, registry)
        if args.quantize:
            quantizer = args.quantizer.expanduser().resolve()
            summary["quantizer_binary"] = inspect_binary_identity(
                quantizer,
                registry["producer"]["observed_quantizer_binary"]["sha256"],
                "fixture quantizer binary",
                args.require_recorded_binary,
            )
            summary["quantization"] = quantize(
                root,
                registry,
                quantizer,
                args.quantizer_source.expanduser().resolve(),
                args.threads,
                args.require_recorded_binary,
            )
        if args.verify or args.verify_source:
            summary["source_verification"] = verify_source(root, registry)
        if args.verify or args.verify_derived:
            summary["derived_verification"] = verify_derived(root, registry)
        if args.census:
            summary["census"] = census_all(root, registry)
        if args.smoke:
            completion = args.completion.expanduser().resolve()
            summary["completion_binary"] = inspect_binary_identity(
                completion,
                registry["smoke_consumer"]["observed_completion_binary"]["sha256"],
                "fixture smoke-consumer binary",
                args.require_recorded_binary,
            )
            summary["completion_binary"]["reported_commit"] = probe_version_commit(
                completion,
                registry["smoke_consumer"]["commit"],
                "fixture smoke-consumer binary",
            )
            summary["smoke"] = smoke(
                root,
                registry,
                completion,
                args.completion_source.expanduser().resolve(),
                args.require_recorded_binary,
            )
        print(json.dumps(summary, indent=2, sort_keys=True))
        return 0
    except (FixtureError, OSError, json.JSONDecodeError) as exc:
        print(f"fixture error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
