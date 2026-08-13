#!/usr/bin/env python3
"""Fail-closed, metadata-first HaloFPX fresh-PC recovery helper.

The default lane verifies prerequisites plus the four pinned GitHub release
records and attestations.  It deliberately does not download release assets or
claim that the full fresh-PC acceptance in issue #2 has passed.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.parse import quote


SCHEMA_VERSION = 1
MARKER_NAME = ".halofpx-recovery-root.json"
STATE_NAME = "state.json"
CANONICAL_REGISTRY_RELATIVE = Path("docs/publication/continuation-releases.json")
CANONICAL_REGISTRY_SHA256 = (
    "93d4874d8a836554c3da94442dff08b33ebb175a2ce4c16a8437546a7e015a96"
)
ALLOWED_ORIGIN_URLS = {
    "https://github.com/JCFrags/HaloFPX.git",
    "git@github.com:JCFrags/HaloFPX.git",
}
PUBLICATION_VERIFIER_SHA256 = (
    "2717886f637d96f644f0aafe393de46530875806f329ee33cc15749d4e1e250f"
)
TRACKED_PUBLICATION_MANIFEST_SHA256 = (
    "0b888309cb8318a651389e7b0020dadfb325959413f441adbdb9fd4c2de1c488"
)
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
GIT_ID_RE = re.compile(r"^[0-9a-f]{40}$")
TAG_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]*$")
UTC_RE = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")
STATEMENT_TYPE = "https://in-toto.io/Statement/v1"
PREDICATE_TYPE = "https://in-toto.io/attestation/release/v0.2"
VERIFICATION_RESULT_MEDIA_TYPE = (
    "application/vnd.dev.sigstore.verificationresult+json;version=0.1"
)
RELEASE_CERTIFICATE_ISSUER = "CN=Fulcio Intermediate l1,O=GitHub\\, Inc."
RELEASE_SUBJECT_ALTERNATIVE_NAME = "https://dotcom.releases.github.com"
VERIFIED_RELEASE_SAN_REGEXP = r"^https://dotcom\.releases\.github\.com$"
COMMAND_TIMEOUT_SECONDS = 300.0
# The original immutable release is about 23.3 GB (21.7 GiB).  Twelve hours
# admits an intentionally slow verifier averaging roughly 0.51 MiB/s while
# the 24-hour ceiling still prevents an accidentally unbounded command.
DEFAULT_VERIFIER_TIMEOUT_SECONDS = 12 * 60 * 60.0
MIN_VERIFIER_TIMEOUT_SECONDS = COMMAND_TIMEOUT_SECONDS
MAX_VERIFIER_TIMEOUT_SECONDS = 24 * 60 * 60.0
MAX_ERROR_MESSAGE_CHARACTERS = 1024


class RecoveryError(RuntimeError):
    exit_code = 1


class UsageSafetyError(RecoveryError):
    exit_code = 2


class BlockedError(RecoveryError):
    exit_code = 3


@dataclass(frozen=True)
class CommandResult:
    argv: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str


class CommandRunner:
    """Small injectable subprocess boundary; commands are never shell strings."""

    def run(
        self,
        argv: list[str],
        *,
        cwd: Path | None = None,
        timeout_seconds: float = COMMAND_TIMEOUT_SECONDS,
    ) -> CommandResult:
        if not argv or any(not isinstance(item, str) or not item for item in argv):
            raise UsageSafetyError("Command argv must contain non-empty strings.")
        if (
            isinstance(timeout_seconds, bool)
            or not isinstance(timeout_seconds, (int, float))
            or not math.isfinite(timeout_seconds)
            or timeout_seconds <= 0
            or timeout_seconds > MAX_VERIFIER_TIMEOUT_SECONDS
        ):
            raise UsageSafetyError("Command timeout is outside the supported bounded range.")
        environment = os.environ.copy()
        # Recovery commands must remain Unicode-safe even when the control PC's
        # legacy Windows locale is CP1252.  PYTHONIOENCODING also overrides a
        # hostile inherited value for any Python child process.
        environment["PYTHONUTF8"] = "1"
        environment["PYTHONIOENCODING"] = "utf-8"
        try:
            completed = subprocess.run(
                argv,
                cwd=cwd,
                check=False,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                shell=False,
                timeout=float(timeout_seconds),
                env=environment,
            )
        except subprocess.TimeoutExpired as error:
            raise BlockedError(
                f"{argv[0]} exceeded its {float(timeout_seconds):g}-second deadline."
            ) from error
        except OSError as error:
            raise BlockedError(
                f"Could not run {argv[0]} ({error.__class__.__name__})."
            ) from error
        return CommandResult(tuple(argv), completed.returncode, completed.stdout, completed.stderr)


def _utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def _expect_dict(value: Any, label: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise RecoveryError(f"{label} must be an object.")
    return value


def _expect_list(value: Any, label: str) -> list[Any]:
    if not isinstance(value, list):
        raise RecoveryError(f"{label} must be an array.")
    return value


def _exact_keys(value: dict[str, Any], expected: set[str], label: str) -> None:
    actual = set(value)
    if actual != expected:
        raise RecoveryError(
            f"{label} keys differ: missing={sorted(expected - actual)} "
            f"extra={sorted(actual - expected)}"
        )


def _read_json(path: Path, label: str) -> dict[str, Any]:
    try:
        return _expect_dict(json.loads(path.read_text(encoding="utf-8")), label)
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise RecoveryError(f"Could not read {label} at {path}: {error}") from error


def _safe_repo_relative(root: Path, raw: Any, label: str) -> Path:
    if not isinstance(raw, str) or not raw or "\\" in raw or ":" in raw:
        raise RecoveryError(f"{label} must be a safe repository-relative POSIX path.")
    relative = Path(raw)
    if relative.is_absolute() or any(part in ("", ".", "..") for part in relative.parts):
        raise RecoveryError(f"{label} must not escape the repository.")
    root_resolved = root.resolve()
    candidate = (root_resolved / relative).resolve()
    try:
        candidate.relative_to(root_resolved)
    except ValueError as error:
        raise RecoveryError(f"{label} escapes the repository.") from error
    return candidate


def _validate_asset(asset: Any, label: str) -> dict[str, Any]:
    item = _expect_dict(asset, label)
    _exact_keys(item, {"name", "size_bytes", "sha256"}, label)
    name = item["name"]
    if (
        not isinstance(name, str)
        or not name
        or name in {".", ".."}
        or Path(name).name != name
        or "/" in name
        or "\\" in name
        or ":" in name
        or "\x00" in name
    ):
        raise RecoveryError(f"{label}.name is not a safe leaf name.")
    size = item["size_bytes"]
    digest = item["sha256"]
    if not isinstance(size, int) or isinstance(size, bool) or size <= 0:
        raise RecoveryError(f"{label}.size_bytes must be a positive integer.")
    if not isinstance(digest, str) or not SHA256_RE.fullmatch(digest):
        raise RecoveryError(f"{label}.sha256 must be a lowercase SHA-256 digest.")
    return {"name": name, "size_bytes": size, "sha256": digest}


def _positive_integer(value: Any, label: str) -> int:
    if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
        raise RecoveryError(f"{label} must be a positive integer.")
    return value


def _safe_leaf_name(value: Any, label: str) -> str:
    if (
        not isinstance(value, str)
        or not value
        or value in {".", ".."}
        or Path(value).name != value
        or "/" in value
        or "\\" in value
        or ":" in value
        or "\x00" in value
    ):
        raise RecoveryError(f"{label} is not a safe leaf name.")
    return value


def _validate_split_payloads(
    raw_splits: Any, assets: list[dict[str, Any]], label: str
) -> list[dict[str, Any]]:
    splits = _expect_list(raw_splits, label)
    asset_sizes = {asset["name"]: asset["size_bytes"] for asset in assets}
    seen_logical: set[str] = set()
    seen_parts: set[str] = set()
    normalized: list[dict[str, Any]] = []
    for index, raw in enumerate(splits):
        split_label = f"{label}[{index}]"
        split = _expect_dict(raw, split_label)
        _exact_keys(
            split,
            {"logical_name", "original_size_bytes", "original_sha256", "reassembly_order"},
            split_label,
        )
        logical_name = _safe_leaf_name(split["logical_name"], f"{split_label}.logical_name")
        if logical_name in seen_logical or logical_name in asset_sizes:
            raise RecoveryError(f"{split_label}.logical_name is duplicated or collides with an asset.")
        seen_logical.add(logical_name)
        original_size = _positive_integer(
            split["original_size_bytes"], f"{split_label}.original_size_bytes"
        )
        original_sha = split["original_sha256"]
        if not isinstance(original_sha, str) or not SHA256_RE.fullmatch(original_sha):
            raise RecoveryError(f"{split_label}.original_sha256 is not a lowercase SHA-256 digest.")
        raw_order = _expect_list(split["reassembly_order"], f"{split_label}.reassembly_order")
        if not raw_order:
            raise RecoveryError(f"{split_label}.reassembly_order must not be empty.")
        order: list[str] = []
        split_seen: set[str] = set()
        for part_index, raw_name in enumerate(raw_order):
            part = _safe_leaf_name(raw_name, f"{split_label}.reassembly_order[{part_index}]")
            if part in split_seen or part in seen_parts:
                raise RecoveryError(f"{split_label} contains a duplicated or reused split part.")
            if part not in asset_sizes:
                raise RecoveryError(f"{split_label} references an asset that is not in the release.")
            split_seen.add(part)
            seen_parts.add(part)
            order.append(part)
        if sum(asset_sizes[part] for part in order) != original_size:
            raise RecoveryError(f"{split_label} part sizes do not equal original_size_bytes.")
        normalized.append(
            {
                "logical_name": logical_name,
                "original_size_bytes": original_size,
                "original_sha256": original_sha,
                "reassembly_order": order,
            }
        )
    return normalized


def _resolve_assets(
    authority: dict[str, Any],
    repo_root: Path,
    label: str,
    release: dict[str, Any],
    repository: dict[str, Any],
) -> list[dict[str, Any]]:
    kind = authority.get("kind")
    if kind == "direct":
        _exact_keys(authority, {"kind", "assets"}, label)
        raw_assets = _expect_list(authority["assets"], f"{label}.assets")
        return [_validate_asset(item, f"{label}.assets[{index}]") for index, item in enumerate(raw_assets)]

    if kind == "publication_manifest":
        _exact_keys(authority, {"kind", "path", "controls"}, label)
        source_path = _safe_repo_relative(repo_root, authority["path"], f"{label}.path")
        source = _read_json(source_path, f"{label} source")
        if set(source) != {"schema_version", "repository", "visibility", "release_tag", "generated_utc", "assets", "split_payloads"}:
            raise RecoveryError(f"{label} source has an unexpected schema.")
        if (
            source["schema_version"] != "1.0"
            or source["repository"] != repository["slug"]
            or source["visibility"] != repository["visibility"]
            or source["release_tag"] != release["tag"]
        ):
            raise RecoveryError(
                f"{label} source identity disagrees with its enclosing release."
            )
        assets = [
            _validate_asset(item, f"{label}.source.assets[{index}]")
            for index, item in enumerate(_expect_list(source["assets"], f"{label}.source.assets"))
        ]
        splits = _validate_split_payloads(
            source["split_payloads"], assets, f"{label}.source.split_payloads"
        )
        controls = [
            _validate_asset(item, f"{label}.controls[{index}]")
            for index, item in enumerate(_expect_list(authority["controls"], f"{label}.controls"))
        ]
        release["_split_payloads"] = splits
        return assets + controls

    if kind == "fixture_receipt":
        _exact_keys(authority, {"kind", "path"}, label)
        source_path = _safe_repo_relative(repo_root, authority["path"], f"{label}.path")
        source = _read_json(source_path, f"{label} source")
        if source.get("schema_version") != SCHEMA_VERSION:
            raise RecoveryError(f"{label} source has an unsupported schema_version.")
        source_release = _expect_dict(source.get("release"), f"{label}.source.release")
        expected_release = {
            "id": release["release_id"],
            "tag": release["tag"],
            "target_commit": release["peeled_commit"],
            "draft": release["draft"],
            "prerelease": release["prerelease"],
            "immutable": release["immutable"],
            "published_at_utc": release["published_at_utc"],
            "asset_count": release["asset_count"],
        }
        for field, expected in expected_release.items():
            if source_release.get(field) != expected:
                raise RecoveryError(
                    f"{label} source release field {field} disagrees with its enclosing release."
                )
        raw_assets = _expect_list(source.get("assets"), f"{label}.source.assets")
        assets: list[dict[str, Any]] = []
        for index, raw in enumerate(raw_assets):
            source_asset = _expect_dict(raw, f"{label}.source.assets[{index}]")
            if source_asset.get("state") != "uploaded":
                raise RecoveryError(f"{label}.source.assets[{index}] is not uploaded.")
            assets.append(
                _validate_asset(
                    {key: source_asset.get(key) for key in ("name", "size_bytes", "sha256")},
                    f"{label}.source.assets[{index}]",
                )
            )
        return assets

    raise RecoveryError(f"{label}.kind is unsupported.")


def load_registry(path: Path | str, repo_root: Path | str) -> dict[str, Any]:
    registry_path = Path(path)
    root = Path(repo_root).resolve()
    registry = _read_json(registry_path, "continuation registry")
    _exact_keys(registry, {"schema_version", "repository", "storage", "releases"}, "registry")
    if registry["schema_version"] != SCHEMA_VERSION:
        raise RecoveryError("Unsupported continuation registry schema_version.")

    repository = _expect_dict(registry["repository"], "registry.repository")
    _exact_keys(repository, {"slug", "id", "owner_id", "visibility"}, "registry.repository")
    if repository != {
        "slug": "JCFrags/HaloFPX",
        "id": 1332159679,
        "owner_id": 222912166,
        "visibility": "private",
    }:
        raise RecoveryError("Repository identity in the continuation registry is not the pinned private repository.")

    storage = _expect_dict(registry["storage"], "registry.storage")
    _exact_keys(
        storage,
        {
            "minimum_free_bytes",
            "recommended_free_bytes",
            "all_release_assets_bytes",
            "all_release_assets_plus_reconstructed_bytes",
        },
        "registry.storage",
    )
    for key, value in storage.items():
        _positive_integer(value, f"registry.storage.{key}")
    if storage["minimum_free_bytes"] != 53_687_091_200:
        raise RecoveryError("The minimum recovery capacity must remain exactly 50 GiB.")
    if storage["recommended_free_bytes"] < storage["minimum_free_bytes"]:
        raise RecoveryError("Recommended recovery capacity is below the minimum.")

    releases = _expect_list(registry["releases"], "registry.releases")
    if len(releases) != 4:
        raise RecoveryError("The continuation registry must contain exactly four releases.")
    seen_tags: set[str] = set()
    normalized_releases: list[dict[str, Any]] = []
    release_keys = {
        "tag",
        "release_id",
        "tag_object",
        "peeled_commit",
        "draft",
        "prerelease",
        "immutable",
        "published_at_utc",
        "asset_count",
        "asset_bytes",
        "asset_authority",
    }
    for index, raw_release in enumerate(releases):
        release = dict(_expect_dict(raw_release, f"registry.releases[{index}]"))
        _exact_keys(release, release_keys, f"registry.releases[{index}]")
        tag = release["tag"]
        if not isinstance(tag, str) or not TAG_RE.fullmatch(tag) or tag in seen_tags:
            raise RecoveryError(f"registry.releases[{index}].tag is unsafe or duplicated.")
        seen_tags.add(tag)
        if not isinstance(release["release_id"], int) or isinstance(release["release_id"], bool) or release["release_id"] <= 0:
            raise RecoveryError(f"registry.releases[{index}].release_id is invalid.")
        for key in ("tag_object", "peeled_commit"):
            if not isinstance(release[key], str) or not GIT_ID_RE.fullmatch(release[key]):
                raise RecoveryError(f"registry.releases[{index}].{key} is not a Git object ID.")
        for key in ("draft", "prerelease", "immutable"):
            if not isinstance(release[key], bool):
                raise RecoveryError(f"registry.releases[{index}].{key} must be boolean.")
        if release["draft"] or not release["immutable"]:
            raise RecoveryError(
                f"registry.releases[{index}] must be published (draft=false) and immutable."
            )
        if not isinstance(release["published_at_utc"], str) or not UTC_RE.fullmatch(release["published_at_utc"]):
            raise RecoveryError(f"registry.releases[{index}].published_at_utc is invalid.")
        _positive_integer(release["asset_count"], f"registry.releases[{index}].asset_count")
        _positive_integer(release["asset_bytes"], f"registry.releases[{index}].asset_bytes")
        authority = _expect_dict(release["asset_authority"], f"registry.releases[{index}].asset_authority")
        assets = _resolve_assets(
            authority,
            root,
            f"registry.releases[{index}].asset_authority",
            release,
            repository,
        )
        names = [asset["name"] for asset in assets]
        if len(names) != len(set(names)):
            raise RecoveryError(f"Release {tag} has duplicate asset names.")
        actual_bytes = sum(asset["size_bytes"] for asset in assets)
        if release["asset_count"] != len(assets) or release["asset_bytes"] != actual_bytes:
            raise RecoveryError(f"Release {tag} asset count or byte total disagrees with its authority.")
        release["_assets"] = assets
        release["_repository"] = repository
        normalized_releases.append(release)

    total_bytes = sum(release["asset_bytes"] for release in normalized_releases)
    if storage["all_release_assets_bytes"] != total_bytes:
        raise RecoveryError("Registry all-release byte total is inconsistent.")
    reconstructed_bytes = sum(
        split["original_size_bytes"]
        for release in normalized_releases
        for split in release.get("_split_payloads", [])
    )
    if storage["all_release_assets_plus_reconstructed_bytes"] != (
        total_bytes + reconstructed_bytes
    ):
        raise RecoveryError(
            "Registry release-plus-reconstructed byte total is inconsistent."
        )
    normalized = dict(registry)
    normalized["repository"] = repository
    normalized["storage"] = storage
    normalized["releases"] = normalized_releases
    return normalized


def _has_reparse_attribute(path: Path) -> bool:
    try:
        stat = path.lstat()
    except OSError:
        return False
    return bool(getattr(stat, "st_file_attributes", 0) & 0x400)


def _path_entry_exists(path: Path) -> bool:
    """Return true for ordinary entries and dangling links alike."""

    return os.path.lexists(os.fspath(path))


def _is_link_or_reparse(path: Path) -> bool:
    return _path_entry_exists(path) and (
        path.is_symlink() or _has_reparse_attribute(path)
    )


def _reject_linked_path(path: Path, stop: Path) -> None:
    """Reject links/reparse points from ``path`` through the inclusive stop."""

    cursor = path
    while True:
        if _is_link_or_reparse(cursor):
            raise UsageSafetyError(f"Path contains a link/reparse point: {cursor}")
        if cursor == stop:
            return
        if cursor.parent == cursor:
            raise UsageSafetyError(f"Path {path} is not contained by {stop}.")
        cursor = cursor.parent


def _reject_linked_descendants(root: Path) -> None:
    """Inspect a recovery tree without following any child link/reparse point."""

    pending = [root]
    while pending:
        directory = pending.pop()
        try:
            entries = list(os.scandir(directory))
        except OSError as error:
            raise UsageSafetyError(
                f"Could not inspect recovery directory {directory}: {error}"
            ) from error
        for entry in entries:
            path = Path(entry.path)
            try:
                attributes = getattr(entry.stat(follow_symlinks=False), "st_file_attributes", 0)
                linked = entry.is_symlink() or bool(attributes & 0x400)
            except OSError as error:
                raise UsageSafetyError(
                    f"Could not inspect recovery entry {path}: {error}"
                ) from error
            if linked:
                raise UsageSafetyError(
                    f"Recovery directory contains a link/reparse point: {path}"
                )
            try:
                if entry.is_dir(follow_symlinks=False):
                    pending.append(path)
            except OSError as error:
                raise UsageSafetyError(
                    f"Could not classify recovery entry {path}: {error}"
                ) from error


def _is_same_or_within(candidate: Path, root: Path) -> bool:
    try:
        candidate.relative_to(root)
        return True
    except ValueError:
        return False


def validate_work_root(work_root: Path | str, checkout_root: Path | str) -> Path:
    raw = Path(work_root)
    if not raw.is_absolute():
        raise UsageSafetyError("Recovery work root must be absolute.")
    raw_cursor = raw
    while True:
        if _is_link_or_reparse(raw_cursor):
            raise UsageSafetyError(f"Recovery path contains a link/reparse point: {raw_cursor}")
        if raw_cursor.parent == raw_cursor:
            break
        raw_cursor = raw_cursor.parent
    root = raw.resolve(strict=False)
    checkout = Path(checkout_root).resolve()
    if root.parent == root or root == Path.home().resolve():
        raise UsageSafetyError("Recovery work root cannot be a filesystem or home root.")
    if _is_same_or_within(root, checkout) or _is_same_or_within(checkout, root):
        raise UsageSafetyError("Recovery work root and checkout must not contain one another.")

    cursor = root
    while True:
        if _is_link_or_reparse(cursor):
            raise UsageSafetyError(f"Recovery path contains a link/reparse point: {cursor}")
        if cursor.parent == cursor:
            break
        cursor = cursor.parent

    if _path_entry_exists(root):
        if not root.is_dir():
            raise UsageSafetyError("Recovery work root exists but is not a directory.")
        _reject_linked_descendants(root)
        entries = list(root.iterdir())
        marker = root / MARKER_NAME
        marker_is_regular = False
        if _path_entry_exists(marker) and not _is_link_or_reparse(marker):
            try:
                marker_is_regular = stat.S_ISREG(marker.lstat().st_mode)
            except OSError:
                marker_is_regular = False
        if entries and not marker_is_regular:
            raise UsageSafetyError("Existing nonempty recovery root is not HaloFPX-marked.")
        if marker_is_regular:
            marker_data = _read_json(marker, "recovery marker")
            if marker_data != {"schema_version": SCHEMA_VERSION, "purpose": "halofpx-fresh-pc-recovery"}:
                raise UsageSafetyError("Recovery marker is invalid.")
    return root


def _asset_map(assets: list[dict[str, Any]], label: str) -> dict[str, tuple[int, str]]:
    result: dict[str, tuple[int, str]] = {}
    for asset in assets:
        name = asset["name"]
        if name in result:
            raise RecoveryError(f"{label} has duplicate asset {name}.")
        result[name] = (asset["size_bytes"], asset["sha256"])
    return result


def verify_release_api(
    release_spec: dict[str, Any], expected_assets: list[dict[str, Any]], payload: Any
) -> dict[str, Any]:
    release = _expect_dict(payload, "GitHub release API response")
    expected_fields = {
        "id": release_spec["release_id"],
        "tag_name": release_spec["tag"],
        "target_commitish": release_spec["peeled_commit"],
        "draft": release_spec["draft"],
        "prerelease": release_spec["prerelease"],
        "immutable": release_spec["immutable"],
        "published_at": release_spec["published_at_utc"],
    }
    for field, expected in expected_fields.items():
        observed_field = release.get(field)
        if type(observed_field) is not type(expected) or observed_field != expected:
            raise RecoveryError(f"Release {release_spec['tag']} field {field} disagrees with the registry.")
    raw_assets = _expect_list(release.get("assets"), "GitHub release API assets")
    observed: dict[str, tuple[int, str]] = {}
    for index, raw in enumerate(raw_assets):
        asset = _expect_dict(raw, f"GitHub release API asset[{index}]")
        name = asset.get("name")
        if not isinstance(name, str) or name in observed:
            raise RecoveryError("GitHub release API contains an invalid or duplicate asset name.")
        if asset.get("state") != "uploaded":
            raise RecoveryError(f"GitHub release asset {name} is not uploaded.")
        digest = asset.get("digest")
        size = asset.get("size")
        if not isinstance(size, int) or isinstance(size, bool) or size <= 0:
            raise RecoveryError(f"GitHub release asset {name} has an invalid size.")
        if not isinstance(digest, str) or not digest.startswith("sha256:"):
            raise RecoveryError(f"GitHub release asset {name} has no SHA-256 digest.")
        observed[name] = (size, digest[7:])
    expected = _asset_map(expected_assets, "registry release")
    if observed != expected:
        raise RecoveryError(f"Release {release_spec['tag']} asset inventory disagrees with the registry.")
    return {"tag": release_spec["tag"], "release_id": release_spec["release_id"], "asset_count": len(expected)}


def verify_attestation(
    release_spec: dict[str, Any], expected_assets: list[dict[str, Any]], payload: Any
) -> dict[str, Any]:
    document = _expect_dict(payload, "gh release verify response")
    attestation = document.get("attestation")
    if not isinstance(attestation, dict) or not attestation:
        raise RecoveryError("gh release verify response lacks a nonempty attestation object.")
    result = _expect_dict(document.get("verificationResult"), "verificationResult")
    if result.get("mediaType") != VERIFICATION_RESULT_MEDIA_TYPE:
        raise RecoveryError("Attestation verification result has an unexpected media type.")
    signature = result.get("signature")
    if not isinstance(signature, dict) or not signature:
        raise RecoveryError("Attestation verification result lacks a signature.")
    certificate = _expect_dict(signature.get("certificate"), "verified signature certificate")
    if certificate != {
        "certificateIssuer": RELEASE_CERTIFICATE_ISSUER,
        "subjectAlternativeName": RELEASE_SUBJECT_ALTERNATIVE_NAME,
    }:
        raise RecoveryError("Attestation certificate identity is not the pinned GitHub release signer.")
    verified_identity = result.get("verifiedIdentity")
    if not isinstance(verified_identity, dict) or not verified_identity:
        raise RecoveryError("Attestation verification result lacks a verified identity.")
    if verified_identity != {
        "issuer": {"issuer": "", "regexp": ".*"},
        "subjectAlternativeName": {
            "regexp": VERIFIED_RELEASE_SAN_REGEXP,
            "subjectAlternativeName": "",
        },
    }:
        raise RecoveryError("Attestation verified identity is not the pinned GitHub release policy.")
    timestamps = result.get("verifiedTimestamps")
    if (
        not isinstance(timestamps, list)
        or not timestamps
        or any(not isinstance(timestamp, dict) or not timestamp for timestamp in timestamps)
    ):
        raise RecoveryError("Attestation has no verified timestamp.")
    for timestamp in timestamps:
        if (
            timestamp.get("type") != "TimestampAuthority"
            or timestamp.get("uri") != "timestamp.githubapp.com"
        ):
            raise RecoveryError("Attestation timestamp authority is not the pinned GitHub authority.")
        raw_timestamp = timestamp.get("timestamp")
        if not isinstance(raw_timestamp, str) or not UTC_RE.fullmatch(raw_timestamp):
            raise RecoveryError("Attestation timestamp is not an exact UTC timestamp.")
        try:
            datetime.strptime(raw_timestamp, "%Y-%m-%dT%H:%M:%SZ")
        except ValueError as error:
            raise RecoveryError("Attestation timestamp is not a valid UTC date/time.") from error
    statement = _expect_dict(result.get("statement"), "verified statement")
    if statement.get("_type") != STATEMENT_TYPE or statement.get("predicateType") != PREDICATE_TYPE:
        raise RecoveryError("Attestation statement or predicate type is not the pinned release format.")
    predicate = _expect_dict(statement.get("predicate"), "release attestation predicate")
    repository = release_spec["_repository"]
    purl = f"pkg:github/{repository['slug']}@{release_spec['tag']}"
    expected_predicate = {
        "databaseId": str(release_spec["release_id"]),
        "ownerId": str(repository["owner_id"]),
        "packageId": str(repository["id"]),
        "purl": purl,
        "repository": repository["slug"],
        "repositoryId": str(repository["id"]),
        "tag": release_spec["tag"],
    }
    for field, expected in expected_predicate.items():
        if predicate.get(field) != expected:
            raise RecoveryError(f"Attestation predicate field {field} disagrees with the registry.")

    subjects = _expect_list(statement.get("subject"), "attestation subjects")
    package_subjects: list[dict[str, Any]] = []
    observed_assets: dict[str, str] = {}
    for index, raw in enumerate(subjects):
        subject = _expect_dict(raw, f"attestation subject[{index}]")
        if "uri" in subject:
            package_subjects.append(subject)
            continue
        name = subject.get("name")
        digest = subject.get("digest")
        if not isinstance(name, str) or name in observed_assets or not isinstance(digest, dict):
            raise RecoveryError("Attestation has an invalid or duplicate asset subject.")
        sha = digest.get("sha256")
        if not isinstance(sha, str):
            raise RecoveryError("Attestation asset subject lacks SHA-256.")
        observed_assets[name] = sha
    if len(package_subjects) != 1:
        raise RecoveryError("Attestation must have exactly one package subject.")
    package = package_subjects[0]
    digest = package.get("digest")
    if package.get("uri") != purl or not isinstance(digest, dict) or digest.get("sha1") != release_spec["tag_object"]:
        raise RecoveryError("Attestation package subject does not bind the exact tag object.")
    expected_digest_map = {asset["name"]: asset["sha256"] for asset in expected_assets}
    if observed_assets != expected_digest_map:
        raise RecoveryError("Attested asset set disagrees with the continuation registry.")
    return {"tag": release_spec["tag"], "release_id": release_spec["release_id"], "asset_count": len(expected_assets)}


def atomic_write_json(path: Path | str, payload: Any) -> None:
    destination = Path(path)
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        encoded = (json.dumps(payload, indent=2, sort_keys=True, ensure_ascii=False) + "\n").encode("utf-8")
    except (TypeError, ValueError):
        raise
    temporary: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="wb", dir=destination.parent, prefix=f".{destination.name}.", suffix=".tmp", delete=False
        ) as stream:
            temporary = Path(stream.name)
            stream.write(encoded)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, destination)
    except OSError as error:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
        raise RecoveryError(f"Could not atomically write {destination}: {error}") from error


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


class Recovery:
    def __init__(
        self,
        registry_path: Path,
        work_root: Path,
        expected_commit: str,
        *,
        repo_root: Path | None = None,
        runner: CommandRunner | None = None,
        verifier_timeout_seconds: float = DEFAULT_VERIFIER_TIMEOUT_SECONDS,
    ) -> None:
        self.repo_root = (repo_root or Path(__file__).resolve().parents[1]).resolve()
        canonical_registry_raw = self.repo_root / CANONICAL_REGISTRY_RELATIVE
        _reject_linked_path(canonical_registry_raw, self.repo_root)
        canonical_registry = canonical_registry_raw.resolve(strict=False)
        provided_registry = Path(registry_path)
        if not provided_registry.is_absolute():
            provided_registry = self.repo_root / provided_registry
        try:
            self.registry_path = provided_registry.resolve(strict=True)
        except OSError as error:
            raise UsageSafetyError(
                f"Could not resolve the canonical continuation registry: {error}"
            ) from error
        if self.registry_path != canonical_registry:
            raise UsageSafetyError(
                f"Recovery acceptance requires the canonical tracked registry at "
                f"{CANONICAL_REGISTRY_RELATIVE.as_posix()}."
            )
        self.registry_sha256 = _sha256_file(self.registry_path)
        if self.registry_sha256 != CANONICAL_REGISTRY_SHA256:
            raise UsageSafetyError(
                "The canonical continuation registry digest disagrees with the pinned runner authority."
            )
        self.registry = load_registry(self.registry_path, self.repo_root)
        if not GIT_ID_RE.fullmatch(expected_commit):
            raise UsageSafetyError("--expected-commit must be a lowercase 40-character Git object ID.")
        self.expected_commit = expected_commit
        self.work_root = validate_work_root(work_root, self.repo_root)
        self.runner = runner or CommandRunner()
        if (
            isinstance(verifier_timeout_seconds, bool)
            or not isinstance(verifier_timeout_seconds, (int, float))
            or not math.isfinite(verifier_timeout_seconds)
            or not (
                MIN_VERIFIER_TIMEOUT_SECONDS
                <= verifier_timeout_seconds
                <= MAX_VERIFIER_TIMEOUT_SECONDS
            )
        ):
            raise UsageSafetyError(
                "--verifier-timeout-seconds must be between "
                f"{MIN_VERIFIER_TIMEOUT_SECONDS:g} and "
                f"{MAX_VERIFIER_TIMEOUT_SECONDS:g}."
            )
        self.verifier_timeout_seconds = float(verifier_timeout_seconds)
        self.state_path = self.work_root / STATE_NAME

    def _initialize_root(self) -> None:
        self.work_root = validate_work_root(self.work_root, self.repo_root)
        self.work_root.mkdir(parents=True, exist_ok=True)
        self.work_root = validate_work_root(self.work_root, self.repo_root)
        marker = self.work_root / MARKER_NAME
        if not _path_entry_exists(marker):
            atomic_write_json(marker, {"schema_version": SCHEMA_VERSION, "purpose": "halofpx-fresh-pc-recovery"})
        self.work_root = validate_work_root(self.work_root, self.repo_root)
        for name in ("metadata", "receipts"):
            (self.work_root / name).mkdir(exist_ok=True)
        self.work_root = validate_work_root(self.work_root, self.repo_root)

    def _load_state(self) -> dict[str, Any]:
        if not self.state_path.exists():
            return {
                "schema_version": SCHEMA_VERSION,
                "overall": "OPEN",
                "claim_boundary": "metadata-only; full issue #2 fresh-PC recovery remains OPEN",
                "registry_sha256": self.registry_sha256,
                "expected_commit": self.expected_commit,
                "steps": {},
            }
        state = _read_json(self.state_path, "recovery state")
        required_keys = {
            "schema_version",
            "overall",
            "claim_boundary",
            "registry_sha256",
            "expected_commit",
            "steps",
        }
        actual_keys = set(state)
        if actual_keys not in (required_keys, required_keys | {"updated_at_utc"}):
            raise UsageSafetyError("Existing recovery state has unexpected or missing fields.")
        if state["schema_version"] != SCHEMA_VERSION:
            raise UsageSafetyError("Existing recovery state has an unsupported schema.")
        if state["claim_boundary"] != "metadata-only; full issue #2 fresh-PC recovery remains OPEN":
            raise UsageSafetyError("Existing recovery state has an invalid claim boundary.")
        if state["registry_sha256"] != self.registry_sha256 or state["expected_commit"] != self.expected_commit:
            raise UsageSafetyError("Existing recovery state belongs to another registry or candidate commit.")
        if state["overall"] != "OPEN":
            raise UsageSafetyError("Recovery state must retain overall=OPEN in this metadata-only runner.")
        if "updated_at_utc" in state:
            self._validate_utc(state["updated_at_utc"], "state.updated_at_utc")
        steps = _expect_dict(state["steps"], "state.steps")
        if not set(steps).issubset({"preflight", "metadata"}):
            raise UsageSafetyError("Recovery state contains an unknown step.")
        for name, raw_step in steps.items():
            step = _expect_dict(raw_step, f"state.steps.{name}")
            if set(step) != {"status", "recorded_at_utc", "details"}:
                raise UsageSafetyError(f"Recovery state step {name} has an invalid schema.")
            if step["status"] not in {"RUNNING", "PASS", "FAIL", "BLOCKED"}:
                raise UsageSafetyError(f"Recovery state step {name} has an invalid status.")
            self._validate_utc(step["recorded_at_utc"], f"state.steps.{name}.recorded_at_utc")
            if step["status"] == "RUNNING" and step["details"] is not None:
                raise UsageSafetyError(f"Recovery state running step {name} has unexpected details.")
            if step["status"] in {"FAIL", "BLOCKED"}:
                self._validate_error_details(step["details"], step["status"], name)
            if step["status"] == "PASS":
                if name == "preflight":
                    self._validate_preflight_details(step["details"])
                elif name == "metadata":
                    self._validate_metadata_details(step["details"])
        if (
            steps.get("metadata", {}).get("status") == "PASS"
            and steps.get("preflight", {}).get("status") != "PASS"
        ):
            raise UsageSafetyError(
                "Recovery metadata PASS requires a bound preflight PASS."
            )
        return state

    @staticmethod
    def _validate_utc(value: Any, label: str) -> None:
        if not isinstance(value, str) or not UTC_RE.fullmatch(value):
            raise UsageSafetyError(f"{label} is not an exact UTC timestamp.")
        try:
            datetime.strptime(value, "%Y-%m-%dT%H:%M:%SZ")
        except ValueError as error:
            raise UsageSafetyError(f"{label} is not a valid UTC date/time.") from error

    @staticmethod
    def _validate_error_details(raw: Any, status: str, step: str) -> None:
        details = _expect_dict(raw, f"state.steps.{step}.details")
        if set(details) != {"error_class", "exit_code", "message", "resume"}:
            raise UsageSafetyError(f"Recovery state step {step} has invalid error custody.")
        if (
            not isinstance(details["error_class"], str)
            or not details["error_class"]
            or type(details["exit_code"]) is not int
            or details["exit_code"] not in {1, 2, 3}
            or not isinstance(details["message"], str)
            or not details["message"]
            or len(details["message"]) > MAX_ERROR_MESSAGE_CHARACTERS
            or details["resume"] != "rerun-same-step-after-correcting-cause"
        ):
            raise UsageSafetyError(f"Recovery state step {step} has invalid error custody.")
        if (status == "BLOCKED") != (details["exit_code"] == BlockedError.exit_code):
            raise UsageSafetyError(f"Recovery state step {step} has inconsistent terminal status.")

    def _validate_preflight_details(self, raw: Any) -> None:
        details = _expect_dict(raw, "state.steps.preflight.details")
        expected_keys = {
            "python",
            "pwsh",
            "gh",
            "c_compiler_command",
            "cxx_compiler_command",
            "free_bytes",
            "minimum_free_bytes",
            "head",
            "origin_url",
            "tool_versions",
        }
        if set(details) != expected_keys:
            raise UsageSafetyError("Recovery preflight PASS details have an invalid schema.")
        for field in ("python", "pwsh", "gh", "c_compiler_command", "cxx_compiler_command"):
            if not isinstance(details[field], str) or not details[field]:
                raise UsageSafetyError(f"Recovery preflight PASS detail {field} is empty.")
        if details["head"] != self.expected_commit:
            raise UsageSafetyError("Recovery preflight PASS does not bind the expected commit.")
        if details["origin_url"] not in ALLOWED_ORIGIN_URLS:
            raise UsageSafetyError("Recovery preflight PASS does not bind an allowed origin.")
        minimum = self.registry["storage"]["minimum_free_bytes"]
        if (
            not isinstance(details["minimum_free_bytes"], int)
            or isinstance(details["minimum_free_bytes"], bool)
            or details["minimum_free_bytes"] != minimum
            or not isinstance(details["free_bytes"], int)
            or isinstance(details["free_bytes"], bool)
            or details["free_bytes"] < minimum
        ):
            raise UsageSafetyError("Recovery preflight PASS has invalid capacity evidence.")
        versions = _expect_dict(details["tool_versions"], "state preflight tool_versions")
        if set(versions) != {
            "git",
            "cmake",
            "ninja",
            "c_compiler",
            "cxx_compiler",
            "sha256sum",
            "tar",
        } or any(not isinstance(value, str) or not value for value in versions.values()):
            raise UsageSafetyError("Recovery preflight PASS has incomplete tool-version evidence.")

    def _validate_metadata_details(self, raw: Any) -> None:
        details = _expect_dict(raw, "state.steps.metadata.details")
        if set(details) != {
            "repository",
            "release_count",
            "asset_count",
            "asset_bytes",
            "releases",
            "bulk_payload_downloaded",
            "fresh_pc_acceptance",
        }:
            raise UsageSafetyError("Recovery metadata PASS details have an invalid schema.")
        expected_releases = self.registry["releases"]
        if (
            details["repository"] != self.registry["repository"]["slug"]
            or type(details["release_count"]) is not int
            or details["release_count"] != len(expected_releases)
            or type(details["asset_count"]) is not int
            or details["asset_count"] != sum(item["asset_count"] for item in expected_releases)
            or type(details["asset_bytes"]) is not int
            or details["asset_bytes"] != self.registry["storage"]["all_release_assets_bytes"]
            or details["bulk_payload_downloaded"] is not False
            or details["fresh_pc_acceptance"] != "OPEN"
        ):
            raise UsageSafetyError("Recovery metadata PASS summary disagrees with the registry.")
        releases = _expect_list(details["releases"], "state metadata releases")
        if len(releases) != len(expected_releases):
            raise UsageSafetyError("Recovery metadata PASS has the wrong release count.")
        for index, (raw_result, expected) in enumerate(zip(releases, expected_releases)):
            result = _expect_dict(raw_result, f"state metadata releases[{index}]")
            if set(result) != {"tag", "api", "attestation"} or result["tag"] != expected["tag"]:
                raise UsageSafetyError("Recovery metadata PASS has an invalid release identity.")
            expected_summary = {
                "tag": expected["tag"],
                "release_id": expected["release_id"],
                "asset_count": expected["asset_count"],
            }
            if result["api"] != expected_summary or result["attestation"] != expected_summary:
                raise UsageSafetyError("Recovery metadata PASS has an invalid release verification summary.")

    def _save_step(self, state: dict[str, Any], step: str, status: str, details: Any = None) -> None:
        state.setdefault("steps", {})[step] = {"status": status, "recorded_at_utc": _utc_now(), "details": details}
        state["updated_at_utc"] = _utc_now()
        atomic_write_json(self.state_path, state)

    def _save_terminal_error(
        self, state: dict[str, Any], step: str, error: RecoveryError
    ) -> None:
        message = " ".join(str(error).split())[:MAX_ERROR_MESSAGE_CHARACTERS]
        for path, replacement in (
            (self.registry_path, "<registry>"),
            (self.work_root, "<work-root>"),
            (self.repo_root, "<repo-root>"),
        ):
            message = message.replace(str(path), replacement)
        message = re.sub(r"https?://\S+", "<url>", message)
        message = re.sub(
            r"(?i)\b(token|password|secret|authorization)\s*[:=]\s*\S+",
            r"\1=<redacted>",
            message,
        )
        message = message[:MAX_ERROR_MESSAGE_CHARACTERS]
        if not message:
            message = "Recovery step failed without a diagnostic message."
        status = "BLOCKED" if isinstance(error, BlockedError) else "FAIL"
        self._save_step(
            state,
            step,
            status,
            {
                "error_class": error.__class__.__name__,
                "exit_code": error.exit_code,
                "message": message,
                "resume": "rerun-same-step-after-correcting-cause",
            },
        )

    def _command(
        self,
        argv: list[str],
        *,
        label: str,
        timeout_seconds: float = COMMAND_TIMEOUT_SECONDS,
    ) -> CommandResult:
        result = self.runner.run(
            argv, cwd=self.repo_root, timeout_seconds=timeout_seconds
        )
        if result.returncode != 0:
            raise BlockedError(f"{label} failed with exit code {result.returncode}.")
        return result

    def run_preflight(self) -> dict[str, Any]:
        self._initialize_root()
        state = self._load_state()
        # A fresh preflight invalidates every downstream result before any
        # fallible check runs.  Keeping a prior metadata PASS beside a new
        # preflight RUNNING state would create an impossible dependency state
        # after an interruption and make the resumable state fail closed on
        # its own next load.
        state.setdefault("steps", {}).pop("metadata", None)
        self._save_step(state, "preflight", "RUNNING")
        try:
            return self._run_preflight_checks(state)
        except RecoveryError as error:
            self._save_terminal_error(state, "preflight", error)
            raise

    def _run_preflight_checks(self, state: dict[str, Any]) -> dict[str, Any]:
        if sys.version_info[:2] != (3, 12):
            raise BlockedError(f"Python 3.12 is required; running {sys.version_info.major}.{sys.version_info.minor}.")
        try:
            import venv  # noqa: F401
        except ImportError as error:
            raise BlockedError("Python venv support is unavailable.") from error

        required = ("git", "gh", "pwsh", "cmake", "ninja", "sha256sum", "tar")
        missing = [name for name in required if shutil.which(name) is None]
        compiler_c = next((name for name in ("cc", "clang", "gcc") if shutil.which(name)), None)
        compiler_cxx = next((name for name in ("c++", "clang++", "g++") if shutil.which(name)), None)
        if missing or compiler_c is None or compiler_cxx is None:
            missing.extend([name for name, value in (("C compiler", compiler_c), ("C++ compiler", compiler_cxx)) if value is None])
            raise BlockedError(f"Missing required recovery tools: {', '.join(missing)}")
        free_bytes = shutil.disk_usage(self.work_root).free
        minimum = self.registry["storage"]["minimum_free_bytes"]
        if free_bytes < minimum:
            raise BlockedError(f"Recovery volume has {free_bytes} free bytes; {minimum} required.")

        gh_version = self._command(["gh", "--version"], label="GitHub CLI version")
        self._command(["gh", "auth", "status", "--hostname", "github.com"], label="GitHub authentication")
        self._command(["gh", "release", "verify", "--help"], label="GitHub release verification capability")
        pwsh_version = self._command(
            ["pwsh", "-NoLogo", "-NoProfile", "-NonInteractive", "-Command", "$PSVersionTable.PSVersion.ToString()"],
            label="PowerShell version",
        ).stdout.strip()
        try:
            pwsh_parts = tuple(int(item) for item in pwsh_version.split(".")[:2])
        except ValueError as error:
            raise BlockedError(f"Could not parse PowerShell version: {pwsh_version}") from error
        if pwsh_parts < (7, 2):
            raise BlockedError(f"PowerShell 7.2 or newer is required; found {pwsh_version}.")

        version_commands = {
            "git": ["git", "--version"],
            "cmake": ["cmake", "--version"],
            "ninja": ["ninja", "--version"],
            "c_compiler": [compiler_c, "--version"],
            "cxx_compiler": [compiler_cxx, "--version"],
            "sha256sum": ["sha256sum", "--version"],
            "tar": ["tar", "--version"],
        }
        tool_versions: dict[str, str] = {}
        for name, argv in version_commands.items():
            output = self._command(argv, label=f"{name} version").stdout.strip()
            if not output:
                raise BlockedError(f"{name} version command returned no output.")
            tool_versions[name] = output

        show_root = self._command(["git", "rev-parse", "--show-toplevel"], label="Git checkout identity").stdout.strip()
        if Path(show_root).resolve() != self.repo_root:
            raise RecoveryError("Git reports a different checkout root.")
        origin_url = self._command(
            ["git", "remote", "get-url", "origin"], label="Git origin identity"
        ).stdout.strip()
        if origin_url not in ALLOWED_ORIGIN_URLS:
            raise RecoveryError(
                "Git origin must be the pinned JCFrags/HaloFPX HTTPS or SSH repository."
            )
        shallow = self._command(["git", "rev-parse", "--is-shallow-repository"], label="Git shallow check").stdout.strip()
        if shallow != "false":
            raise RecoveryError("The recovery checkout is shallow.")
        head = self._command(["git", "rev-parse", "HEAD"], label="Git HEAD").stdout.strip()
        origin_main = self._command(["git", "rev-parse", "origin/main"], label="origin/main").stdout.strip()
        if head != self.expected_commit or origin_main != self.expected_commit:
            raise RecoveryError("HEAD and origin/main must both equal --expected-commit.")
        dirty = self._command(
            ["git", "status", "--porcelain=v1", "--untracked-files=all"], label="Git worktree status"
        ).stdout
        if dirty:
            raise RecoveryError("Recovery checkout is not clean.")
        self._command(["git", "fsck", "--full"], label="Git full object verification")
        for release in self.registry["releases"]:
            tag = release["tag"]
            raw = self._command(["git", "rev-parse", tag], label=f"tag {tag}").stdout.strip()
            peeled = self._command(["git", "rev-parse", f"{tag}^{{}}"], label=f"peeled tag {tag}").stdout.strip()
            if raw != release["tag_object"] or peeled != release["peeled_commit"]:
                raise RecoveryError(f"Local tag {tag} disagrees with the registry.")

        details = {
            "python": sys.version.split()[0],
            "pwsh": pwsh_version,
            "gh": gh_version.stdout.splitlines()[0] if gh_version.stdout else "",
            "c_compiler_command": compiler_c,
            "cxx_compiler_command": compiler_cxx,
            "free_bytes": free_bytes,
            "minimum_free_bytes": minimum,
            "head": head,
            "origin_url": origin_url,
            "tool_versions": tool_versions,
        }
        self._save_step(state, "preflight", "PASS", details)
        return details

    def run_metadata(self) -> dict[str, Any]:
        self.run_preflight()
        self._initialize_root()
        state = self._load_state()
        if state.get("steps", {}).get("preflight", {}).get("status") != "PASS":
            raise UsageSafetyError("Metadata verification requires a recorded passing preflight.")
        self._save_step(state, "metadata", "RUNNING")
        try:
            return self._run_metadata_checks(state)
        except RecoveryError as error:
            self._save_terminal_error(state, "metadata", error)
            raise

    def _run_metadata_checks(self, state: dict[str, Any]) -> dict[str, Any]:
        repository = self.registry["repository"]
        repo_payload_text = self._command(
            ["gh", "api", f"repos/{repository['slug']}"], label="GitHub repository metadata"
        ).stdout
        try:
            repo_payload = _expect_dict(json.loads(repo_payload_text), "GitHub repository metadata")
        except json.JSONDecodeError as error:
            raise RecoveryError("GitHub repository metadata was not JSON.") from error
        expected_repo = {
            "id": repository["id"],
            "full_name": repository["slug"],
            "visibility": repository["visibility"],
        }
        for field, expected in expected_repo.items():
            if repo_payload.get(field) != expected:
                raise RecoveryError(f"GitHub repository field {field} disagrees with the registry.")
        owner = _expect_dict(repo_payload.get("owner"), "GitHub repository owner")
        if owner.get("id") != repository["owner_id"]:
            raise RecoveryError("GitHub repository owner ID disagrees with the registry.")
        metadata_root = self.work_root / "metadata"
        atomic_write_json(metadata_root / "repository.json", repo_payload)

        release_results: list[dict[str, Any]] = []
        for release in self.registry["releases"]:
            tag = release["tag"]
            api_text = self._command(
                ["gh", "api", f"repos/{repository['slug']}/releases/tags/{quote(tag, safe='')}"],
                label=f"release metadata {tag}",
            ).stdout
            attestation_text = self._command(
                ["gh", "release", "verify", tag, "--repo", repository["slug"], "--format", "json"],
                label=f"release attestation {tag}",
            ).stdout
            try:
                api_payload = json.loads(api_text)
                attestation_payload = json.loads(attestation_text)
            except json.JSONDecodeError as error:
                raise RecoveryError(f"GitHub response for {tag} was not JSON.") from error
            api_summary = verify_release_api(release, release["_assets"], api_payload)
            attestation_summary = verify_attestation(release, release["_assets"], attestation_payload)
            tag_root = metadata_root / tag
            atomic_write_json(tag_root / "release-api.json", api_payload)
            atomic_write_json(tag_root / "release-attestation.json", attestation_payload)
            release_results.append({"tag": tag, "api": api_summary, "attestation": attestation_summary})
        details = {
            "repository": repository["slug"],
            "release_count": len(release_results),
            "asset_count": sum(item["asset_count"] for item in self.registry["releases"]),
            "asset_bytes": self.registry["storage"]["all_release_assets_bytes"],
            "releases": release_results,
            "bulk_payload_downloaded": False,
            "fresh_pc_acceptance": "OPEN",
        }
        self._save_step(state, "metadata", "PASS", details)
        return details

    def write_receipt(self) -> Path:
        self.run_metadata()
        self._initialize_root()
        state = self._load_state()
        receipt = {
            "schema_version": SCHEMA_VERSION,
            "generated_at_utc": _utc_now(),
            "overall_fresh_pc_recovery": "OPEN",
            "metadata_only_status": state.get("steps", {}).get("metadata", {}).get("status", "NOT RUN"),
            "expected_commit": self.expected_commit,
            "registry_sha256": self.registry_sha256,
            "bulk_payload_downloaded": False,
            "not_run": [
                "52-asset payload download and hashing",
                "split-payload reconstruction",
                "Git bundle recovery",
                "feature-off build",
                "fixture CPU smoke",
                "Strix Halo target, GPU, dual-node, quality, and performance qualification",
            ],
            "tracking": {
                "full_acceptance": "https://github.com/JCFrags/HaloFPX/issues/2",
                "bootstrap": "https://github.com/JCFrags/HaloFPX/issues/11",
            },
            "state_path": str(self.state_path),
            "template": str(self.repo_root / "docs" / "publication" / "fresh-pc-recovery-template.md"),
        }
        destination = self.work_root / "receipts" / "metadata-only-receipt.json"
        atomic_write_json(destination, receipt)
        return destination

    def verify_original_assets(self, asset_directory: Path) -> dict[str, Any]:
        self.run_preflight()
        directory = asset_directory.resolve()
        if not asset_directory.is_absolute() or not directory.is_dir():
            raise UsageSafetyError("--asset-directory must be an existing absolute directory.")
        verifier = self.repo_root / "scripts" / "verify-publication-assets.ps1"
        manifest = self.repo_root / "docs" / "publication" / "release-manifest.json"
        _reject_linked_path(verifier, self.repo_root)
        _reject_linked_path(manifest, self.repo_root)
        if _sha256_file(verifier) != PUBLICATION_VERIFIER_SHA256:
            raise RecoveryError("The publication verifier does not match its pinned SHA-256 authority.")
        if _sha256_file(manifest) != TRACKED_PUBLICATION_MANIFEST_SHA256:
            raise RecoveryError("The tracked publication manifest does not match its pinned SHA-256 authority.")
        result = self._command(
            [
                "pwsh",
                "-NoLogo",
                "-NoProfile",
                "-NonInteractive",
                "-File",
                str(verifier),
                "-AssetDirectory",
                str(directory),
                "-ManifestPath",
                str(manifest),
            ],
            label="original publication asset verifier",
            timeout_seconds=self.verifier_timeout_seconds,
        )
        return {"status": "PASS", "asset_directory": str(directory), "output": result.stdout.strip()}


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--registry", required=True, type=Path)
    parser.add_argument("--work-root", required=True, type=Path)
    parser.add_argument("--expected-commit", required=True)
    parser.add_argument(
        "--verifier-timeout-seconds",
        type=float,
        default=DEFAULT_VERIFIER_TIMEOUT_SECONDS,
        help=(
            "Deadline used only by verify-original-assets "
            f"({MIN_VERIFIER_TIMEOUT_SECONDS:g}..{MAX_VERIFIER_TIMEOUT_SECONDS:g}; "
            f"default {DEFAULT_VERIFIER_TIMEOUT_SECONDS:g})."
        ),
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    run = subparsers.add_parser("run", help="Run preflight and optional metadata-only verification.")
    run.add_argument("--through", choices=("preflight", "metadata"), default="metadata")
    subparsers.add_parser("status", help="Print the current checkpoint state.")
    subparsers.add_parser("receipt", help="Write an explicitly metadata-only OPEN receipt.")
    verify = subparsers.add_parser("verify-original-assets", help="Delegate the original release asset set to its exact verifier.")
    verify.add_argument("--asset-directory", required=True, type=Path)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)
    try:
        recovery = Recovery(
            args.registry,
            args.work_root,
            args.expected_commit,
            verifier_timeout_seconds=args.verifier_timeout_seconds,
        )
        if args.command == "run":
            if args.through == "metadata":
                metadata = recovery.run_metadata()
                state = recovery._load_state()
                output: Any = {
                    "preflight": state["steps"]["preflight"]["details"],
                    "metadata": metadata,
                    "overall_fresh_pc_recovery": "OPEN",
                }
            else:
                output = {
                    "preflight": recovery.run_preflight(),
                    "overall_fresh_pc_recovery": "OPEN",
                }
            print(json.dumps(output, indent=2, sort_keys=True))
        elif args.command == "status":
            recovery._initialize_root()
            print(json.dumps(recovery._load_state(), indent=2, sort_keys=True))
        elif args.command == "receipt":
            print(recovery.write_receipt())
        elif args.command == "verify-original-assets":
            print(json.dumps(recovery.verify_original_assets(args.asset_directory), indent=2, sort_keys=True))
        return 0
    except RecoveryError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return error.exit_code


if __name__ == "__main__":
    raise SystemExit(main())
