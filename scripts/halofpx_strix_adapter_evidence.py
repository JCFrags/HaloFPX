#!/usr/bin/env python3
"""Closed-world verifier for a complete PR51 CachyOS adapter evidence tree.

This module is deliberately offline and success-only.  It rejects every
partial or failed adapter profile; controller failure custody and recovery
remain separately retainable and do not depend on this verifier.  It consumes
retained bytes, never contacts a target, and never grants target-execution or
performance authority.  The three retained HMM-admission inputs are passed to
ADR-0064's exact snapshot/policy-bound canonical recomputation; their presence
does not turn this structural verifier into an HMM collector, trusted-time
source, owner authorization, or target-execution mechanism.
"""

from __future__ import annotations

import datetime as dt
import hashlib
import importlib.util
import json
import math
import os
import re
import stat
import sys
import tempfile
import unicodedata
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any, Iterable


def _load_exact_sibling(name: str, filename: str) -> Any:
    path = Path(__file__).resolve().with_name(filename)
    existing = sys.modules.get(name)
    if existing is not None:
        origin = getattr(existing, "__file__", None)
        if not isinstance(origin, str) or Path(origin).resolve() != path:
            raise ImportError(
                f"refusing ambient {name}; expected exact sibling module {path}")
        return existing
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load exact sibling module {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    try:
        spec.loader.exec_module(module)
    except BaseException:
        sys.modules.pop(name, None)
        raise
    return module


def _load_optional_sibling(name: str, filename: str) -> Any | None:
    path = Path(__file__).resolve().with_name(filename)
    if not path.is_file():
        return None
    return _load_exact_sibling(name, filename)


core = _load_exact_sibling("halofpx_strix_ab", "halofpx_strix_ab.py")
adapter = _load_exact_sibling(
    "halofpx_strix_ab_cachyos", "halofpx_strix_ab_cachyos.py")
hmm_admission = _load_exact_sibling(
    "halofpx_strix_hmm_admission", "halofpx_strix_hmm_admission.py")
sampling_sync = _load_optional_sibling(
    "halofpx_strix_ab_sampling_sync", "halofpx_strix_ab_sampling_sync.py")


VERIFY_SCHEMA = "halofpx.strix-adapter-evidence-verification.v1"
MACHINE_AUTHORITY_SCHEMA = "halofpx.strix-ab-machine-authority.v2"
HMM_RESULT_SCHEMA = "halofpx.strix-hmm-admission-result.v1"
HMM_SNAPSHOT_FILENAME = "hmm-admission-snapshot.raw.json"
HMM_POLICY_FILENAME = "hmm-admission-policy.raw.json"
HMM_RESULT_FILENAME = "hmm-admission-result.raw.json"
SAMPLING_SYNC_CONTRACT = "sampling_output_sync_prometheus_v1"
SAMPLING_SYNC_ROOT_FILES = (
    "sampling-output-sync-plan.json", "sampling-output-sync-analysis.json")
SAMPLING_SYNC_DIRECTORY = "sampling-output-sync"
SAMPLING_SYNC_SAMPLE_FILES = (
    "before.prom", "after.prom", "capture.json", "summary.json")
_GENERATION_SETTINGS_KEYS = {
    "seed", "temperature", "dynatemp_range", "dynatemp_exponent", "top_k", "top_p",
    "min_p", "top_n_sigma", "xtc_probability", "xtc_threshold", "typical_p",
    "repeat_last_n", "repeat_penalty", "presence_penalty", "frequency_penalty",
    "dry_multiplier", "dry_base", "dry_allowed_length", "dry_penalty_last_n",
    "dry_sequence_breakers", "mirostat", "mirostat_tau", "mirostat_eta", "stop",
    "max_tokens", "n_predict", "n_keep", "n_discard", "ignore_eos", "stream",
    "logit_bias", "n_probs", "min_keep", "grammar", "grammar_lazy",
    "grammar_triggers", "preserved_tokens", "chat_format", "reasoning_format",
    "reasoning_in_content", "generation_prompt", "samplers", "speculative.types",
    "speculative.n_min", "speculative.n_max", "timings_per_token",
    "post_sampling_probs", "backend_sampling", "lora",
}

# The semantic maxima (128 schedule entries, 16 warmups, and the supported
# four-file sampling sidecar) expand to 15,638 files/directories.  Keep one
# finite ceiling above that exact supported maximum; do not advertise a plan
# cardinality which the custody scanner would refuse before semantic parsing.
_MAX_FILES = 16_384
_MAX_DEPTH = 8
_MAX_FILE_BYTES = 16 * 1024 * 1024
# Max N/W plus the supported sidecar exceeds 64 MiB even with small hosted
# artifacts.  256 MiB remains finite while admitting the exact declared
# schedule envelope and its bounded per-file evidence.
_MAX_TREE_BYTES = 256 * 1024 * 1024
_MAX_SCHEDULE_ENTRIES = 128
_MAX_WARMUPS_PER_ENTRY = 16
_MAX_JSON_DEPTH = 64
_MAX_JSON_TOKENS = 100_000
_MAX_JSON_INTEGER_DIGITS = 128
_REPARSE_ATTRIBUTE = 0x400
_HASH_RE = re.compile(r"^[0-9a-f]{64}$")
_INVOCATION_RE = re.compile(r"^[0-9a-f]{32}$")
_BOOT_ID_RE = re.compile(
    r"^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$")
_GPU_BUSY_PATH_RE = re.compile(
    r"^/sys/class/drm/card[0-9]+/device/gpu_busy_percent$")
_GPU_TEMP_PATH_RE = re.compile(
    r"^/sys/class/drm/card[0-9]+/device/hwmon/hwmon[0-9]+/temp[0-9]+_input$")
_LOADAVG_RE = re.compile(
    r"^([0-9]+(?:\.[0-9]+)?) ([0-9]+(?:\.[0-9]+)?) "
    r"([0-9]+(?:\.[0-9]+)?) ([0-9]+)/([0-9]+) ([0-9]+)\n?$")
_MEMINFO_LINE_RE = re.compile(
    r"^([A-Za-z0-9_()]+):[ \t]+([0-9]+)(?:[ \t]+(kB))?$")
_WINDOWS_RESERVED = {
    "con", "prn", "aux", "nul", "clock$",
    *(f"com{index}" for index in range(1, 10)),
    *(f"lpt{index}" for index in range(1, 10)),
}


class AdapterEvidenceError(ValueError):
    """Raised when retained adapter evidence is incomplete or ambiguous."""


@dataclass(frozen=True)
class _Node:
    kind: str
    identity: tuple[int, ...]
    content: bytes | None


@dataclass(frozen=True)
class _HmmDomain:
    trusted_now: dt.datetime
    snapshot_completed: dt.datetime
    expires: dt.datetime
    max_snapshot_age_seconds: int
    boot_ids: dict[str, str]
    capture_completed_monotonic_ns: dict[str, int]
    planned_increment_bytes: dict[str, int]


def _fail(detail: str) -> None:
    raise AdapterEvidenceError(detail)


def _sha(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def _strict_int(value: Any, where: str, minimum: int = 0, maximum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum or (
            maximum is not None and value > maximum):
        suffix = f" and <= {maximum}" if maximum is not None else ""
        _fail(f"{where} must be an integer >= {minimum}{suffix}")
    return value


def _number(value: Any, where: str, *, positive: bool = False) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        _fail(f"{where} must be a finite number")
    result = float(value)
    if not math.isfinite(result) or (positive and result <= 0):
        _fail(f"{where} must be {'positive and ' if positive else ''}finite")
    return result


def _string(value: Any, where: str, *, allow_empty: bool = False) -> str:
    if not isinstance(value, str) or (not allow_empty and not value) or "\x00" in value or any(
            ord(char) < 32 and char not in "\n\r\t" for char in value):
        _fail(f"{where} must be {'a' if allow_empty else 'a non-empty'} safe string")
    return value


def _hash(value: Any, where: str) -> str:
    if not isinstance(value, str) or _HASH_RE.fullmatch(value) is None:
        _fail(f"{where} must be a lowercase SHA-256 digest")
    return value


def _mapping(value: Any, where: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        _fail(f"{where} must be an object")
    return value


def _keys(value: Any, expected: Iterable[str], where: str) -> dict[str, Any]:
    result = _mapping(value, where)
    wanted = set(expected)
    if set(result) != wanted:
        missing = sorted(wanted - set(result))
        extra = sorted(set(result) - wanted)
        _fail(f"{where} has a non-closed field set (missing={missing}, extra={extra})")
    return result


def _list(value: Any, where: str) -> list[Any]:
    if not isinstance(value, list):
        _fail(f"{where} must be an array")
    return value


def _false(value: Any, where: str) -> None:
    if value is not False:
        _fail(f"{where} must be literal false")


def _true(value: Any, where: str) -> None:
    if value is not True:
        _fail(f"{where} must be literal true")


def _same(left: Any, right: Any) -> bool:
    """Deep equality that never aliases JSON booleans with 0/1 integers."""
    if isinstance(left, bool) or isinstance(right, bool):
        return type(left) is type(right) and left == right
    if isinstance(left, dict) or isinstance(right, dict):
        return isinstance(left, dict) and isinstance(right, dict) and \
            set(left) == set(right) and all(_same(left[key], right[key]) for key in left)
    if isinstance(left, list) or isinstance(right, list):
        return isinstance(left, list) and isinstance(right, list) and len(left) == len(right) and \
            all(_same(a, b) for a, b in zip(left, right))
    return left == right


def _timestamp(value: Any, where: str) -> dt.datetime:
    text = _string(value, where)
    try:
        parsed = dt.datetime.fromisoformat(text.replace("Z", "+00:00"))
    except ValueError as exc:
        raise AdapterEvidenceError(f"{where} is not an ISO-8601 timestamp") from exc
    if parsed.tzinfo is None or parsed.utcoffset() is None:
        _fail(f"{where} must include an offset")
    return parsed


def _parse_json(content: bytes, where: str) -> Any:
    try:
        if len(content) > _MAX_FILE_BYTES:
            _fail(f"{where} exceeds the bounded JSON byte budget")
        text = content.decode("utf-8", errors="strict")
        depth = 0
        tokens = 0
        digit_run = 0
        in_string = False
        escaped = False
        for char in text:
            if in_string:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    in_string = False
                continue
            if char == '"':
                in_string = True
                tokens += 1
                digit_run = 0
            elif char in "[{":
                depth += 1
                tokens += 1
                digit_run = 0
                if depth > _MAX_JSON_DEPTH:
                    _fail(f"{where} exceeds the bounded JSON nesting depth")
            elif char in "]}":
                depth -= 1
                digit_run = 0
                if depth < 0:
                    _fail(f"{where} has unbalanced JSON nesting")
            elif char == ",":
                tokens += 1
                digit_run = 0
            elif "0" <= char <= "9":
                digit_run += 1
                if digit_run > _MAX_JSON_INTEGER_DIGITS:
                    _fail(f"{where} exceeds the bounded JSON numeric token length")
            else:
                digit_run = 0
            if tokens > _MAX_JSON_TOKENS:
                _fail(f"{where} exceeds the bounded JSON node/token budget")
        if in_string or depth != 0:
            _fail(f"{where} has unterminated JSON string/nesting")
        return json.loads(
            text,
            object_pairs_hook=core.unique_json_object,
            parse_constant=core.reject_json_constant,
            parse_int=core.strict_json_int,
            parse_float=core.strict_json_float,
        )
    except AdapterEvidenceError:
        raise
    except (UnicodeError, json.JSONDecodeError, ValueError, RecursionError, MemoryError) as exc:
        raise AdapterEvidenceError(f"{where} is not strict duplicate-free UTF-8 JSON: {exc}") from exc


def _is_reparse(info: os.stat_result) -> bool:
    return bool(getattr(info, "st_file_attributes", 0) & _REPARSE_ATTRIBUTE)


def _identity(info: os.stat_result) -> tuple[int, ...]:
    return (
        int(info.st_dev), int(info.st_ino), int(info.st_mode), int(info.st_nlink),
        int(info.st_size), int(getattr(info, "st_mtime_ns", int(info.st_mtime * 1e9))),
        # Windows can materialize a different creation/change timestamp merely
        # by opening a freshly written file.  Device/file index, type, link
        # count, size, mtime, attributes, captured bytes, and the second full
        # pass remain the stable race-detection identity there.
        0 if os.name == "nt" else int(
            getattr(info, "st_ctime_ns", int(info.st_ctime * 1e9))),
        int(getattr(info, "st_file_attributes", 0)),
    )


def _path_component_identity(info: os.stat_result) -> tuple[int, ...]:
    """Return replacement-sensitive identity for an evidence-root ancestor.

    An ancestor's size, link count, and modification/change timestamps can
    legitimately move when an unrelated process creates or removes a sibling
    (notably in the shared Windows temporary directory).  Device/file index,
    type/mode, and reparse attributes instead identify the traversed directory
    itself.  The evidence root and every descendant retain the stricter full
    identity/content checks in ``_scan_once`` and the second capture pass.
    """
    return (
        int(info.st_dev), int(info.st_ino), int(info.st_mode),
        int(getattr(info, "st_file_attributes", 0)),
    )


def _direntry_matches_lstat(entry_info: os.stat_result, path_info: os.stat_result) -> bool:
    """Compare scandir/lstat while tolerating documented Windows zero fields.

    Windows ``DirEntry.stat`` may expose zero ``st_dev``, ``st_ino``, and
    ``st_nlink`` even when ``os.lstat`` returns the real values.  Its cached
    directory timestamp can also lag immediate ``lstat``. Directory traversal
    is independently anchored by visit's lstat before/after plus the second
    full pass, so this seam compares directory type/reparse and any reliable
    nonzero identity; regular files retain size/timestamp checks.
    """
    mandatory = [int(entry_info.st_mode), int(getattr(entry_info, "st_file_attributes", 0))]
    observed = [int(path_info.st_mode), int(getattr(path_info, "st_file_attributes", 0))]
    if os.name != "nt" or not stat.S_ISDIR(entry_info.st_mode):
        mandatory.append(int(getattr(entry_info, "st_mtime_ns", int(entry_info.st_mtime * 1e9))))
        observed.append(int(getattr(path_info, "st_mtime_ns", int(path_info.st_mtime * 1e9))))
    if os.name != "nt":
        mandatory.append(int(getattr(entry_info, "st_ctime_ns", int(entry_info.st_ctime * 1e9))))
        observed.append(int(getattr(path_info, "st_ctime_ns", int(path_info.st_ctime * 1e9))))
    if mandatory != observed:
        return False
    if stat.S_ISREG(entry_info.st_mode) and int(entry_info.st_size) != int(path_info.st_size):
        return False
    for name in ("st_dev", "st_ino", "st_nlink"):
        left, right = int(getattr(entry_info, name, 0)), int(getattr(path_info, name, 0))
        if left and right and left != right:
            return False
    return True


def _validate_name(name: str, where: str) -> None:
    if not name or name in {".", ".."} or len(name.encode("utf-8")) > 255:
        _fail(f"{where} has an invalid name")
    if not name.isascii() or unicodedata.normalize("NFC", name) != name:
        _fail(f"{where} must use canonical ASCII names")
    if any(char in name for char in ("/", "\\", "\x00", ":")) or any(ord(char) < 32 for char in name):
        _fail(f"{where} contains an unsafe name")
    stem = name.rstrip(". ").split(".", 1)[0].casefold()
    lowered = name.casefold()
    if stem in _WINDOWS_RESERVED or name != name.rstrip(". ") or name.startswith(".") or \
            lowered.endswith(("~", ".tmp", ".temp", ".part", ".partial", ".swp", ".bak")) or \
            lowered.startswith(("tmp-", "temp-", ".record-")):
        _fail(f"{where} uses a reserved or temporary name: {name}")


def _read_file(path: str, initial: os.stat_result, where: str) -> bytes:
    if initial.st_nlink != 1:
        _fail(f"{where} is hard-linked (st_nlink={initial.st_nlink})")
    if initial.st_size < 0 or initial.st_size > _MAX_FILE_BYTES:
        _fail(f"{where} exceeds the bounded file size")
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0) | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
    except OSError as exc:
        raise AdapterEvidenceError(f"cannot open {where}: {exc}") from exc
    try:
        before = os.fstat(descriptor)
        if _is_reparse(before) or not stat.S_ISREG(before.st_mode) or _identity(before) != _identity(initial):
            _fail(f"{where} changed type or identity before capture")
        chunks: list[bytes] = []
        count = 0
        while True:
            chunk = os.read(descriptor, min(1024 * 1024, _MAX_FILE_BYTES + 1 - count))
            if not chunk:
                break
            chunks.append(chunk)
            count += len(chunk)
            if count > _MAX_FILE_BYTES:
                _fail(f"{where} exceeded the bounded file size while read")
        after = os.fstat(descriptor)
        content = b"".join(chunks)
        if _identity(before) != _identity(after) or len(content) != after.st_size:
            _fail(f"{where} changed during capture")
        return content
    finally:
        os.close(descriptor)


def _scan_once(
    root: str, expected_identities: dict[str, tuple[int, ...]] | None = None,
    *, max_tree_bytes: int = _MAX_TREE_BYTES,
) -> dict[str, _Node]:
    nodes: dict[str, _Node] = {}
    total_bytes = 0
    enumerated_entries = 0

    def visit(path: str, relative: str, depth: int) -> None:
        nonlocal total_bytes, enumerated_entries
        if depth > _MAX_DEPTH:
            _fail("evidence tree exceeds the bounded depth")
        try:
            before = os.lstat(path)
        except OSError as exc:
            raise AdapterEvidenceError(f"cannot inspect {relative or 'root'}: {exc}") from exc
        where = relative or "root"
        if _is_reparse(before) or stat.S_ISLNK(before.st_mode):
            _fail(f"{where} is a symbolic link or reparse point")
        if not stat.S_ISDIR(before.st_mode):
            _fail(f"{where} is not a directory")
        if expected_identities is not None and relative in expected_identities and \
                _identity(before) != expected_identities[relative]:
            _fail(f"{where} differs from the initially captured directory identity")
        nodes[relative] = _Node("dir", _identity(before), None)
        try:
            with os.scandir(path) as iterator:
                entries = []
                for entry in iterator:
                    if enumerated_entries >= _MAX_FILES:
                        _fail("evidence tree exceeds the bounded entry count")
                    enumerated_entries += 1
                    entries.append(entry)
        except OSError as exc:
            raise AdapterEvidenceError(f"cannot enumerate {where}: {exc}") from exc
        names = [entry.name for entry in entries]
        if len({name.casefold() for name in names}) != len(names):
            _fail(f"{where} contains a case-colliding name")
        for name in names:
            _validate_name(name, f"{where}/{name}")
        for entry in sorted(entries, key=lambda item: item.name):
            child_rel = entry.name if not relative else f"{relative}/{entry.name}"
            child = os.path.join(path, entry.name)
            try:
                entry_info = entry.stat(follow_symlinks=False)
                path_info = os.lstat(child)
            except OSError as exc:
                raise AdapterEvidenceError(f"cannot inspect {child_rel}: {exc}") from exc
            if not _direntry_matches_lstat(entry_info, path_info):
                _fail(f"{child_rel} changed between directory and path inspection")
            if _is_reparse(path_info) or stat.S_ISLNK(path_info.st_mode):
                _fail(f"{child_rel} is a symbolic link or reparse point")
            if stat.S_ISDIR(path_info.st_mode):
                visit(child, child_rel, depth + 1)
            elif stat.S_ISREG(path_info.st_mode):
                content = _read_file(child, path_info, child_rel)
                total_bytes += len(content)
                if total_bytes > max_tree_bytes:
                    _fail("evidence tree exceeds the bounded byte budget")
                nodes[child_rel] = _Node("file", _identity(path_info), content)
            else:
                _fail(f"{child_rel} is neither a directory nor a regular file")
            if len(nodes) > _MAX_FILES:
                _fail("evidence tree exceeds the bounded entry count")
        try:
            after = os.lstat(path)
        except OSError as exc:
            raise AdapterEvidenceError(f"cannot reinspect {where}: {exc}") from exc
        if _identity(before) != _identity(after):
            _fail(f"{where} changed during enumeration")

    visit(root, "", 0)
    return nodes


def _capture_tree(
    root: Path, *, max_tree_bytes: int = _MAX_TREE_BYTES,
) -> tuple[dict[str, bytes], tuple[str, ...]]:
    if not isinstance(root, Path):
        _fail("root must be a pathlib.Path")
    absolute = os.path.abspath(os.fspath(root))
    component = Path(absolute)
    component_identities: dict[Path, tuple[int, ...]] = {}
    for candidate in (component, *component.parents):
        try:
            info = os.lstat(candidate)
        except OSError as exc:
            raise AdapterEvidenceError(
                f"cannot inspect evidence-root path component {candidate}: {exc}") from exc
        if _is_reparse(info) or stat.S_ISLNK(info.st_mode):
            _fail(f"evidence-root path component is a symbolic link or reparse point: {candidate}")
        if not stat.S_ISDIR(info.st_mode):
            _fail(f"evidence-root path component is not a directory: {candidate}")
        component_identities[candidate] = _path_component_identity(info)
    try:
        initial_root = os.lstat(absolute)
    except OSError as exc:
        raise AdapterEvidenceError(
            f"cannot bind initial evidence-root identity {absolute}: {exc}") from exc
    initial_root_identity = _identity(initial_root)
    if isinstance(max_tree_bytes, bool) or not isinstance(max_tree_bytes, int) or \
            max_tree_bytes < _MAX_FILE_BYTES:
        _fail("capture byte budget is invalid")
    first = _scan_once(
        absolute, {"": initial_root_identity}, max_tree_bytes=max_tree_bytes)
    expected_directories = {
        relative: node.identity
        for relative, node in first.items() if node.kind == "dir"
    }
    second = _scan_once(
        absolute, expected_directories, max_tree_bytes=max_tree_bytes)
    if set(first) != set(second):
        _fail("evidence tree inventory changed between capture passes")
    for relative in sorted(first):
        left, right = first[relative], second[relative]
        if left.kind != right.kind or left.identity != right.identity or left.content != right.content:
            _fail(f"{relative or 'root'} changed between capture passes")
    for candidate, identity in component_identities.items():
        try:
            info = os.lstat(candidate)
        except OSError as exc:
            raise AdapterEvidenceError(
                f"cannot reinspect evidence-root path component {candidate}: {exc}") from exc
        if _is_reparse(info) or stat.S_ISLNK(info.st_mode) or \
                not stat.S_ISDIR(info.st_mode) or _path_component_identity(info) != identity:
            _fail(f"evidence-root path component changed during capture: {candidate}")
    files = {
        relative: node.content
        for relative, node in first.items()
        if node.kind == "file" and node.content is not None
    }
    directories = tuple(sorted(
        relative for relative, node in first.items()
        if relative and node.kind == "dir"))
    return files, directories


def capture_closed_regular_tree(
    root: Path, *, max_tree_bytes: int = _MAX_TREE_BYTES,
) -> tuple[dict[str, bytes], tuple[str, ...]]:
    """Capture one bounded, observed-unchanged, contained regular tree.

    This generic custody primitive grants no semantic or promotion authority.
    Callers must still enforce a closed inventory and validate only the
    returned bytes; they must not reopen captured paths.  It is a trusted
    single-operator custody check, not a hostile-writer snapshot primitive:
    the two full passes detect changes which remain observable at an identity
    check, but synchronized nested A-to-B-to-A replacement can evade portable
    Python path traversal on platforms without held-directory/openat support.
    """
    return _capture_tree(root, max_tree_bytes=max_tree_bytes)


def _expect_inventory(files: dict[str, bytes], expected: set[str]) -> None:
    observed = set(files)
    if observed != expected:
        _fail(
            "evidence tree file inventory is not closed "
            f"(missing={sorted(expected - observed)}, extra={sorted(observed - expected)})")


def _canonical_ledger(files: dict[str, bytes]) -> bytes:
    return "".join(
        f"{_sha(files[name])}  {name}\n"
        for name in sorted(files) if name != "SHA256SUMS"
    ).encode("utf-8")


def _tree_digest(files: dict[str, bytes]) -> str:
    hasher = hashlib.sha256()
    for name in sorted(files):
        encoded = name.encode("ascii")
        content = files[name]
        hasher.update(len(encoded).to_bytes(4, "big"))
        hasher.update(encoded)
        hasher.update(len(content).to_bytes(8, "big"))
        hasher.update(content)
    return hasher.hexdigest()


def _sampling_profile_present(
    files: dict[str, bytes], directories: tuple[str, ...],
) -> bool:
    if any(name in files for name in SAMPLING_SYNC_ROOT_FILES):
        return True
    return any(
        PurePosixPath(relative).name == SAMPLING_SYNC_DIRECTORY
        for relative in directories
    )


def _validate_sampling_profile(
    files: dict[str, bytes], directories: tuple[str, ...], plan: dict[str, Any],
    policy: Any, hmm_domain: _HmmDomain,
) -> None:
    """Reparse a supported sidecar from captured bytes in an isolated replay.

    The authoritative sidecar API rewrites its derived analysis.  Calling it
    against the evidence root would violate this verifier's read-only capture
    boundary, so the exact captured tree is replayed under a private temporary
    root.  Every non-derived byte must remain unchanged and the regenerated
    report must equal the captured analysis document.
    """
    if sampling_sync is None:
        _fail(
            f"{SAMPLING_SYNC_CONTRACT} evidence is present but its exact sibling "
            "validator is unavailable")
    expected_constants = {
        "CONTRACT": SAMPLING_SYNC_CONTRACT,
        "EVIDENCE_VALIDATOR_ROOT_FILES": SAMPLING_SYNC_ROOT_FILES,
        "EVIDENCE_DIRECTORY": SAMPLING_SYNC_DIRECTORY,
        "EVIDENCE_VALIDATOR_SAMPLE_FILES": SAMPLING_SYNC_SAMPLE_FILES,
    }
    for name, expected in expected_constants.items():
        if getattr(sampling_sync, name, None) != expected:
            _fail(f"sampling-output-sync sibling exposes an unsupported {name} contract")

    plan_name, analysis_name = SAMPLING_SYNC_ROOT_FILES
    side_plan = _mapping(_parse_json(files[plan_name], plan_name), plan_name)
    captured_analysis = _mapping(
        _parse_json(files[analysis_name], analysis_name), analysis_name)
    if side_plan.get("schema") != sampling_sync.PLAN_SCHEMA or \
            side_plan.get("lane") != SAMPLING_SYNC_CONTRACT or \
            side_plan.get("enabled") is not True:
        _fail("sampling-output-sync profile is not the supported enabled v1 contract")
    schedule = core.make_schedule(plan)
    for schedule_index, entry in enumerate(schedule["entries"]):
        sample = _sample_directory(entry)
        sidecar = f"{sample}/{SAMPLING_SYNC_DIRECTORY}"
        capture_name = f"{sidecar}/capture.json"
        summary_name = f"{sidecar}/summary.json"
        capture = _mapping(_parse_json(files[capture_name], capture_name), capture_name)
        summary = _mapping(_parse_json(files[summary_name], summary_name), summary_name)
        receipt_name = f"execution/entry-{schedule_index:03d}/execution.json"
        receipt = _mapping(_parse_json(files[receipt_name], receipt_name), receipt_name)
        cycles = _list(receipt.get("cycles"), f"{receipt_name}.cycles")
        if not cycles:
            _fail(f"{capture_name} has no measured adapter identity to bind")
        measured = _mapping(cycles[-1], f"{receipt_name}.cycles[-1]")
        identities = _mapping(measured.get("identities"), f"{receipt_name}.cycles[-1].identities")
        coordinator = _mapping(
            identities.get("coordinator"),
            f"{receipt_name}.cycles[-1].identities.coordinator")
        expected_identity = {
            "pid": coordinator.get("pid"),
            "invocation_id": coordinator.get("invocation_id"),
            "process_start_ticks": coordinator.get("process_start_ticks"),
        }
        coordinator_start_ns = _strict_int(
            coordinator.get("start_monotonic_us"),
            f"{receipt_name}.cycles[-1].identities.coordinator.start_monotonic_us", 1) * 1000
        coordinator_host = plan["topology"]["coordinator"]["host"]
        sidecar_deadline_ns = min(
            coordinator_start_ns + policy.runtime_max_seconds * 1_000_000_000,
            hmm_domain.capture_completed_monotonic_ns[coordinator_host] +
            hmm_domain.max_snapshot_age_seconds * 1_000_000_000,
        )
        captured_times: dict[str, int] = {}
        for section in ("before", "after"):
            section_value = _mapping(capture.get(section), f"{capture_name}.{section}")
            identity = _mapping(
                section_value.get("identity"), f"{capture_name}.{section}.identity")
            if not _same(
                    {key: identity.get(key) for key in expected_identity}, expected_identity):
                _fail(
                    f"{capture_name}.{section}.identity differs from the measured "
                    "adapter coordinator")
            captured_ns = _strict_int(
                section_value.get("captured_monotonic_ns"),
                f"{capture_name}.{section}.captured_monotonic_ns", 1)
            captured_times[section] = captured_ns
            if captured_ns <= coordinator_start_ns or captured_ns >= sidecar_deadline_ns:
                _fail(
                    f"{capture_name}.{section} escapes the measured coordinator "
                    "freshness/lifetime window")
        request = _mapping(capture.get("request"), f"{capture_name}.request")
        request_identity = _mapping(
            request.get("identity"), f"{capture_name}.request.identity")
        if not _same(
                {key: request_identity.get(key) for key in expected_identity}, expected_identity):
            _fail(
                f"{capture_name}.request.identity differs from the measured adapter coordinator")
        summary_identity = _mapping(summary.get("identity"), f"{summary_name}.identity")
        if not _same(
                {key: summary_identity.get(key) for key in expected_identity}, expected_identity):
            _fail(f"{summary_name}.identity differs from the measured adapter coordinator")
        measured_request = _mapping(
            measured.get("request"), f"{receipt_name}.cycles[-1].request")
        measured_client_name = (
            f"execution/entry-{schedule_index:03d}/measured/measurement-0-client.json")
        measured_client = _mapping(
            _parse_json(files[measured_client_name], measured_client_name), measured_client_name)
        client_started_epoch = _timestamp(
            measured_client.get("started_at"), f"{measured_client_name}.started_at").timestamp()
        metrics_process_start = request_identity.get("metrics_process_start_time_unix")
        if isinstance(metrics_process_start, bool) or not isinstance(
                metrics_process_start, (int, float)) or \
                not math.isfinite(float(metrics_process_start)) or \
                float(metrics_process_start) <= 0 or \
                not 0 <= client_started_epoch - float(metrics_process_start) < \
                policy.runtime_max_seconds:
            _fail(
                f"{capture_name}.request metrics process wall time escapes RuntimeMaxSec")
        if request.get("started_monotonic_ns") != \
                measured_request.get("remote_started_monotonic_ns") or \
                request.get("ended_monotonic_ns") != \
                measured_request.get("remote_ended_monotonic_ns") or \
                request.get("request_sha256") != _sha(files["inputs/request.raw"]) or \
                request.get("response_sha256") != measured_request.get("response_sha256") or \
                request.get("client_sha256") != measured_request.get("client_sha256"):
            _fail(
                f"{capture_name}.request identity, interval, or hashes differ from the "
                "measured adapter request")
        request_started = _strict_int(
            request.get("started_monotonic_ns"),
            f"{capture_name}.request.started_monotonic_ns", 1)
        request_ended = _strict_int(
            request.get("ended_monotonic_ns"),
            f"{capture_name}.request.ended_monotonic_ns", 1)
        if not (
                coordinator_start_ns < captured_times["before"] < request_started <
                request_ended < captured_times["after"] < sidecar_deadline_ns):
            _fail(f"{capture_name} sequence escapes strict coordinator lifecycle order")
        cleanup = _mapping(measured.get("cleanup"), f"{receipt_name}.cycles[-1].cleanup")
        coordinator_cleanup = _mapping(
            cleanup.get("coordinator"),
            f"{receipt_name}.cycles[-1].cleanup.coordinator")
        cleanup_completed = _strict_int(
            coordinator_cleanup.get("completed_monotonic_ns"),
            f"{receipt_name}.cycles[-1].cleanup.coordinator.completed_monotonic_ns", 1)
        if captured_times["after"] >= cleanup_completed:
            _fail(f"{capture_name}.after does not precede measured coordinator cleanup")

    with tempfile.TemporaryDirectory(prefix="halofpx-adapter-evidence-replay-") as directory:
        replay_root = Path(directory) / "run"
        replay_root.mkdir()
        for relative in directories:
            replay_root.joinpath(*relative.split("/")).mkdir(parents=True, exist_ok=True)
        for relative, content in files.items():
            destination = replay_root.joinpath(*relative.split("/"))
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(content)
        try:
            report = sampling_sync.validate_frozen_run(replay_root, plan)
        except sampling_sync.PlanError as exc:
            raise AdapterEvidenceError(
                f"{SAMPLING_SYNC_CONTRACT} authoritative reparse failed: {exc}") from exc
        for relative, content in files.items():
            if relative == analysis_name:
                continue
            if replay_root.joinpath(*relative.split("/")).read_bytes() != content:
                _fail(
                    f"{SAMPLING_SYNC_CONTRACT} authoritative reparse unexpectedly "
                    f"modified {relative}")
    report = _mapping(report, "sampling-output-sync authoritative report")
    if not _same(report, captured_analysis) or \
            report.get("schema") != sampling_sync.ANALYSIS_SCHEMA or \
            report.get("contract") != SAMPLING_SYNC_CONTRACT or \
            report.get("enabled") is not True or report.get("evidence_complete") is not True:
        _fail(
            "sampling-output-sync captured analysis differs from the authoritative "
            "raw-evidence reparse")


def _validate_binding(value: Any, expected_path: str, content: bytes, where: str) -> None:
    binding = _keys(value, {"path", "size_bytes", "sha256"}, where)
    if binding["path"] != expected_path:
        _fail(f"{where}.path differs from the closed inventory")
    if _strict_int(binding["size_bytes"], f"{where}.size_bytes", 1) != len(content) or \
            _hash(binding["sha256"], f"{where}.sha256") != _sha(content):
        _fail(f"{where} differs from retained bytes")


def _validate_machine_authorities(
    files: dict[str, bytes], plan: dict[str, Any], policy: Any,
    expected_production_identity_sha256: dict[str, str],
) -> tuple[dict[str, bytes], dict[str, str], _HmmDomain]:
    snapshot_bytes = files[HMM_SNAPSHOT_FILENAME]
    policy_bytes = files[HMM_POLICY_FILENAME]
    result_bytes = files[HMM_RESULT_FILENAME]
    hmm_digests = {
        "snapshot": _sha(snapshot_bytes),
        "policy": _sha(policy_bytes),
        "result": _sha(result_bytes),
    }
    admission_error = getattr(hmm_admission, "AdmissionError", None)
    admission_validator = getattr(
        hmm_admission, "validate_bound_admission_result_bytes", None)
    if getattr(hmm_admission, "RESULT_SCHEMA", None) != HMM_RESULT_SCHEMA or \
            not isinstance(admission_error, type) or \
            not issubclass(admission_error, RuntimeError) or \
            not callable(admission_validator):
        _fail("exact sibling ADR0064 validator exposes an unsupported consumer contract")
    try:
        authoritative_hmm = admission_validator(
            result_bytes, snapshot_bytes, policy_bytes)
    except admission_error as exc:
        raise AdapterEvidenceError(
            f"{HMM_RESULT_FILENAME} fails snapshot/policy-bound ADR0064 "
            f"canonical recomputation: {exc}") from exc

    hmm = _keys(
        _parse_json(result_bytes, HMM_RESULT_FILENAME),
        {
            "schema", "issue", "snapshot_sha256", "policy_sha256", "trusted_now_utc",
            "roles", "decision", "reason_codes", "target_execution_authority",
            "performance_result",
        },
        HMM_RESULT_FILENAME,
    )
    if not _same(authoritative_hmm, hmm):
        _fail(f"{HMM_RESULT_FILENAME} differs after bound ADR0064 validation")
    hmm_name = HMM_RESULT_FILENAME
    if hmm["schema"] != HMM_RESULT_SCHEMA or hmm["issue"] != 41:
        _fail(f"{hmm_name} has the wrong schema/issue identity")
    if _hash(hmm["snapshot_sha256"], f"{hmm_name}.snapshot_sha256") != \
            hmm_digests["snapshot"] or \
            _hash(hmm["policy_sha256"], f"{hmm_name}.policy_sha256") != \
            hmm_digests["policy"]:
        _fail(f"{hmm_name} does not bind the exact retained snapshot/policy bytes")
    trusted_now = _timestamp(hmm["trusted_now_utc"], f"{hmm_name}.trusted_now_utc")
    roles = _keys(hmm["roles"], {"coordinator", "worker"}, f"{hmm_name}.roles")
    for hmm_role in ("coordinator", "worker"):
        record = _keys(
            roles[hmm_role],
            {
                "role", "host", "node_snapshot_sha256", "production_identity_sha256",
                "classification", "hmm_headroom_bytes", "reason_codes",
            },
            f"{hmm_name}.roles.{hmm_role}",
        )
        if record["role"] != hmm_role or record["host"] != plan["topology"][hmm_role]["host"] or \
                record["classification"] != "ADMIT" or record["reason_codes"] != []:
            _fail(f"{hmm_name}.roles.{hmm_role} is not the exact admitted planned role")
        _hash(record["node_snapshot_sha256"],
              f"{hmm_name}.roles.{hmm_role}.node_snapshot_sha256")
        _hash(record["production_identity_sha256"],
              f"{hmm_name}.roles.{hmm_role}.production_identity_sha256")
        if record["production_identity_sha256"] != expected_production_identity_sha256[hmm_role]:
            _fail(
                f"{hmm_name}.roles.{hmm_role}.production_identity_sha256 differs from "
                "the caller-bound maintenance authority")
        _strict_int(record["hmm_headroom_bytes"],
                    f"{hmm_name}.roles.{hmm_role}.hmm_headroom_bytes")
    if hmm["decision"] != "ADMIT" or hmm["reason_codes"] != []:
        _fail(f"{hmm_name} is not an overall ADMIT with no reason codes")
    _false(hmm["target_execution_authority"], f"{hmm_name}.target_execution_authority")
    _false(hmm["performance_result"], f"{hmm_name}.performance_result")

    snapshot = _mapping(_parse_json(snapshot_bytes, HMM_SNAPSHOT_FILENAME), HMM_SNAPSHOT_FILENAME)
    hmm_policy = _mapping(_parse_json(policy_bytes, HMM_POLICY_FILENAME), HMM_POLICY_FILENAME)
    capture = _mapping(snapshot.get("capture"), f"{HMM_SNAPSHOT_FILENAME}.capture")
    snapshot_completed = _timestamp(
        capture.get("completed_utc"), f"{HMM_SNAPSHOT_FILENAME}.capture.completed_utc")
    requirements = _mapping(
        hmm_policy.get("capture_requirements"),
        f"{HMM_POLICY_FILENAME}.capture_requirements")
    max_age = _strict_int(
        requirements.get("max_snapshot_age_seconds"),
        f"{HMM_POLICY_FILENAME}.capture_requirements.max_snapshot_age_seconds", 1)
    validity = _mapping(hmm_policy.get("validity"), f"{HMM_POLICY_FILENAME}.validity")
    expires = _timestamp(validity.get("expires_utc"), f"{HMM_POLICY_FILENAME}.validity.expires_utc")
    if trusted_now < snapshot_completed or trusted_now >= expires or \
            (trusted_now - snapshot_completed).total_seconds() >= max_age:
        _fail("bound HMM trusted time is outside its capture/validity interval")
    snapshot_roles = _mapping(snapshot.get("roles"), f"{HMM_SNAPSHOT_FILENAME}.roles")
    policy_roles = _mapping(hmm_policy.get("roles"), f"{HMM_POLICY_FILENAME}.roles")
    boot_ids: dict[str, str] = {}
    capture_monotonic: dict[str, int] = {}
    planned_increment: dict[str, int] = {}
    for role in ("coordinator", "worker"):
        host = plan["topology"][role]["host"]
        observation = _mapping(
            _mapping(snapshot_roles.get(role), f"{HMM_SNAPSHOT_FILENAME}.roles.{role}").get("observation"),
            f"{HMM_SNAPSHOT_FILENAME}.roles.{role}.observation")
        clock = _mapping(
            observation.get("capture_clock"),
            f"{HMM_SNAPSHOT_FILENAME}.roles.{role}.observation.capture_clock")
        policy_role = _mapping(policy_roles.get(role), f"{HMM_POLICY_FILENAME}.roles.{role}")
        observed_boot = _string(
            clock.get("boot_id"),
            f"{HMM_SNAPSHOT_FILENAME}.roles.{role}.observation.capture_clock.boot_id")
        expected_boot = _string(
            policy_role.get("expected_boot_id"),
            f"{HMM_POLICY_FILENAME}.roles.{role}.expected_boot_id")
        if observed_boot != expected_boot:
            _fail(f"bound HMM boot identity differs for {role}")
        boot_ids[host] = expected_boot
        capture_monotonic[host] = _strict_int(
            clock.get("completed_monotonic_ns"),
            f"{HMM_SNAPSHOT_FILENAME}.roles.{role}.observation.capture_clock.completed_monotonic_ns", 1)
        capacity = _mapping(
            policy_role.get("capacity"), f"{HMM_POLICY_FILENAME}.roles.{role}.capacity")
        planned_increment[role] = _strict_int(
            capacity.get("planned_increment_bytes"),
            f"{HMM_POLICY_FILENAME}.roles.{role}.capacity.planned_increment_bytes")

    parsed: dict[str, dict[str, Any]] = {}
    for role in ("coordinator", "worker"):
        path = f"inputs/authority-{role}.raw"
        authority = _keys(
            _parse_json(files[path], path),
            {
                "schema", "role", "host", "service", "process_baseline",
                "hmm_admission_result_sha256",
            },
            path,
        )
        if authority["schema"] != MACHINE_AUTHORITY_SCHEMA or authority["role"] != role or \
                authority["host"] != plan["topology"][role]["host"]:
            _fail(f"{path} machine identity differs from plan")
        service = _keys(
            authority["service"],
            {"unit", "port", "active", "pid", "control_group", "listener_pids"},
            f"{path}.service",
        )
        expected_protected = policy.protected[role]
        if service["unit"] != expected_protected["unit"] or \
                service["port"] != expected_protected["ports"][0]:
            _fail(f"{path}.service differs from protected policy authority")
        _false(service["active"], f"{path}.service.active")
        if _strict_int(service["pid"], f"{path}.service.pid") != 0 or \
                service["control_group"] is not None or service["listener_pids"] != []:
            _fail(f"{path}.service is not the required inactive/absent baseline")
        if authority["process_baseline"] is not None:
            _fail(f"{path}.process_baseline must be null for an inactive service")
        if _hash(
                authority["hmm_admission_result_sha256"],
                f"{path}.hmm_admission_result_sha256") != hmm_digests["result"]:
            _fail(f"{path} does not bind the exact retained HMM-admission result bytes")
        parsed[role] = authority
    return (
        {role: files[f"inputs/authority-{role}.raw"] for role in parsed},
        hmm_digests,
        _HmmDomain(
            trusted_now=trusted_now,
            snapshot_completed=snapshot_completed,
            expires=expires,
            max_snapshot_age_seconds=max_age,
            boot_ids=boot_ids,
            capture_completed_monotonic_ns=capture_monotonic,
            planned_increment_bytes=planned_increment,
        ),
    )


def _validate_production_snapshot(
    value: Any, policy: Any, where: str,
) -> dict[str, Any]:
    snapshot = _keys(value, {"coordinator", "worker"}, where)
    for role in ("coordinator", "worker"):
        item = _keys(
            snapshot[role],
            {
                "host", "unit", "active", "pid", "invocation_id", "nrestarts",
                "start_monotonic", "control_group", "process", "listeners", "health",
            },
            f"{where}.{role}",
        )
        protected = policy.protected[role]
        if item["host"] != protected["host"] or item["unit"] != protected["unit"]:
            _fail(f"{where}.{role} differs from protected policy identity")
        _false(item["active"], f"{where}.{role}.active")
        if _strict_int(item["pid"], f"{where}.{role}.pid") != 0:
            _fail(f"{where}.{role}.pid must be zero")
        invocation = _string(item["invocation_id"], f"{where}.{role}.invocation_id", allow_empty=True)
        if invocation and _INVOCATION_RE.fullmatch(invocation) is None:
            _fail(f"{where}.{role}.invocation_id is malformed")
        _strict_int(item["nrestarts"], f"{where}.{role}.nrestarts")
        start = _string(item["start_monotonic"], f"{where}.{role}.start_monotonic", allow_empty=True)
        if start and not start.isdecimal():
            _fail(f"{where}.{role}.start_monotonic is malformed")
        if item["control_group"] != "" or item["process"] is not None or item["health"] is not None:
            _fail(f"{where}.{role} does not prove an absent inactive process/cgroup")
        expected_listeners = {str(port): [] for port in protected["ports"]}
        if not _same(item["listeners"], expected_listeners):
            _fail(f"{where}.{role}.listeners is not the closed empty protected-port census")
    return snapshot


def _validate_gpu_role(
    value: Any, *, role: str, host: str, allowed: list[int], where: str,
) -> tuple[int, int]:
    row = _keys(
        value,
        {
            "host", "complete", "pids", "allowed_pids", "foreign_pids", "missing_pids",
            "errors", "controller_started_ns", "controller_ended_ns",
        },
        where,
    )
    if row["host"] != host:
        _fail(f"{where}.host differs from {role} host")
    _true(row["complete"], f"{where}.complete")
    for name in ("pids", "allowed_pids", "foreign_pids", "missing_pids"):
        values = _list(row[name], f"{where}.{name}")
        for index, pid in enumerate(values):
            _strict_int(pid, f"{where}.{name}[{index}]", 1)
        if len(values) != len(set(values)):
            _fail(f"{where}.{name} contains duplicate PIDs")
    if not _same(row["pids"], allowed) or not _same(row["allowed_pids"], allowed) or \
            row["foreign_pids"] != [] or row["missing_pids"] != [] or row["errors"] != []:
        _fail(f"{where} is not a complete exact allowed-process GPU census")
    if any(isinstance(pid, bool) or not isinstance(pid, int) or pid <= 0 for pid in allowed):
        _fail(f"{where}.allowed_pids is malformed")
    started = _strict_int(row["controller_started_ns"], f"{where}.controller_started_ns", 1)
    ended = _strict_int(row["controller_ended_ns"], f"{where}.controller_ended_ns", 1)
    if ended <= started:
        _fail(f"{where} controller interval is reversed or empty")
    return started, ended


def _validate_gpu_census(
    value: Any, plan: dict[str, Any], allowed_by_role: dict[str, list[int]], where: str,
) -> dict[str, tuple[int, int]]:
    census = _keys(value, {"coordinator", "worker"}, where)
    return {
        role: _validate_gpu_role(
            census[role], role=role, host=plan["topology"][role]["host"],
            allowed=allowed_by_role[role], where=f"{where}.{role}")
        for role in ("coordinator", "worker")
    }


def _validate_identity(
    value: Any, *, role: str, kind: str, ordinal: int, schedule_index: int,
    entry: dict[str, Any], plan: dict[str, Any], policy: Any,
    expected_boot_id: str, where: str,
) -> dict[str, Any]:
    identity = _keys(
        value,
        {
            "role", "host", "unit", "pid", "invocation_id", "process_start_ticks",
            "start_monotonic_us", "boot_id", "cursor_before", "argv", "environment",
            "executable_sha256", "port", "control_group",
        },
        where,
    )
    condition = entry["condition"]
    expected_unit = adapter.unit_name(
        policy, plan, schedule_index, kind, ordinal, condition, role)
    commands = core.commands_document(plan)["conditions"][condition]
    expected_port = policy.coordinator_port if role == "coordinator" else policy.worker_port
    expected_hash = adapter.binary_hash(plan, condition, role)
    if identity["role"] != role or identity["host"] != plan["topology"][role]["host"] or \
            identity["unit"] != expected_unit:
        _fail(f"{where} role/host/unit differs from the frozen entry")
    _strict_int(identity["pid"], f"{where}.pid", 1)
    invocation = _string(identity["invocation_id"], f"{where}.invocation_id")
    if _INVOCATION_RE.fullmatch(invocation) is None:
        _fail(f"{where}.invocation_id is malformed")
    _strict_int(identity["process_start_ticks"], f"{where}.process_start_ticks", 1)
    _strict_int(identity["start_monotonic_us"], f"{where}.start_monotonic_us", 1)
    boot_id = _string(identity["boot_id"], f"{where}.boot_id")
    if _BOOT_ID_RE.fullmatch(boot_id) is None or boot_id != expected_boot_id:
        _fail(f"{where}.boot_id differs from the bound HMM role boot")
    _string(identity["cursor_before"], f"{where}.cursor_before")
    if identity["argv"] != commands[role] or identity["environment"] != plan["runtime"]["common_environment"]:
        _fail(f"{where} argv/environment differs from the frozen command")
    if _hash(identity["executable_sha256"], f"{where}.executable_sha256") != expected_hash or \
            identity["port"] != expected_port:
        _fail(f"{where} binary/port differs from the frozen condition")
    control_group = _string(identity["control_group"], f"{where}.control_group")
    pure = PurePosixPath(control_group)
    if not pure.is_absolute() or pure.as_posix() != control_group or \
            any(part in {".", ".."} for part in pure.parts) or \
            not control_group.endswith(f"/{expected_unit}"):
        _fail(f"{where}.control_group is not the exact disposable unit cgroup")
    # Reuse the adapter's pure identity/intent boundary after the closed shape
    # above has prevented its dataclass construction from dropping fields.
    try:
        adapter.assert_identity(
            adapter.UnitIdentity(
                role=identity["role"], host=identity["host"], unit=identity["unit"],
                pid=identity["pid"], invocation_id=identity["invocation_id"],
                process_start_ticks=identity["process_start_ticks"],
                start_monotonic_us=identity["start_monotonic_us"],
                boot_id=identity["boot_id"],
                cursor_before=identity["cursor_before"], argv=tuple(identity["argv"]),
                environment=dict(identity["environment"]),
                executable_sha256=identity["executable_sha256"], port=identity["port"],
                control_group=identity["control_group"],
            ),
            {
                "role": role, "host": plan["topology"][role]["host"], "unit": expected_unit,
                "argv": commands[role], "environment": plan["runtime"]["common_environment"],
                "executable_sha256": expected_hash, "port": expected_port,
            },
            set(),
        )
    except adapter.AdapterError as exc:
        raise AdapterEvidenceError(f"{where} fails adapter identity validation: {exc}") from exc
    return identity


def _validate_live_proof(
    value: Any, identity: dict[str, Any], *, require_listener: bool, where: str,
    expected_fragment: str | None = None,
) -> tuple[str, int]:
    proof = _keys(
        value,
        {"systemd", "process", "listener_pids", "boot_id", "observed_monotonic_ns"},
        where)
    systemd = _keys(
        proof["systemd"],
        {"ExecMainPID", "InvocationID", "ActiveState", "SubState", "ControlGroup", "FragmentPath"},
        f"{where}.systemd",
    )
    expected_systemd = {
        "ExecMainPID": str(identity["pid"]),
        "InvocationID": identity["invocation_id"],
        "ActiveState": "active",
        "SubState": "running",
        "ControlGroup": identity["control_group"],
    }
    if not _same({key: systemd[key] for key in expected_systemd}, expected_systemd):
        _fail(f"{where}.systemd differs from the captured disposable identity")
    fragment = _string(systemd["FragmentPath"], f"{where}.systemd.FragmentPath")
    fragment_path = PurePosixPath(fragment)
    if not fragment_path.is_absolute() or fragment_path.as_posix() != fragment or \
            any(part in {".", ".."} for part in fragment_path.parts) or \
            not fragment.endswith(f"/transient/{identity['unit']}") or \
            (expected_fragment is not None and fragment != expected_fragment):
        _fail(f"{where}.systemd.FragmentPath differs from the exact transient unit")
    process = _keys(
        proof["process"],
        {"pid", "exe", "exe_sha256", "argv", "environment", "cgroup", "process_start_ticks"},
        f"{where}.process",
    )
    if _strict_int(process["pid"], f"{where}.process.pid", 1) != identity["pid"] or \
            _hash(process["exe_sha256"], f"{where}.process.exe_sha256") != identity["executable_sha256"] or \
            process["argv"] != identity["argv"] or process["environment"] != identity["environment"] or \
            process["cgroup"] != f"0::{identity['control_group']}\n" or \
            _strict_int(process["process_start_ticks"], f"{where}.process.process_start_ticks", 1) != \
            identity["process_start_ticks"]:
        _fail(f"{where}.process differs from the captured disposable identity")
    executable = _string(process["exe"], f"{where}.process.exe")
    if not core.is_canonical_absolute_path(executable) or executable != identity["argv"][0]:
        _fail(f"{where}.process.exe differs from the exact launched binary path")
    listeners = _list(proof["listener_pids"], f"{where}.listener_pids")
    for index, pid in enumerate(listeners):
        _strict_int(pid, f"{where}.listener_pids[{index}]", 1)
    expected = [identity["pid"]]
    if require_listener:
        if listeners != expected:
            _fail(f"{where}.listener_pids does not prove exclusive PID ownership")
    elif listeners not in ([], expected):
        _fail(f"{where}.listener_pids contains a foreign owner")
    if proof["boot_id"] != identity["boot_id"]:
        _fail(f"{where}.boot_id differs from the captured disposable identity")
    observed = _strict_int(
        proof["observed_monotonic_ns"], f"{where}.observed_monotonic_ns", 1)
    if observed <= identity["start_monotonic_us"] * 1000:
        _fail(f"{where}.observed_monotonic_ns does not follow process start")
    return fragment, observed


def _validate_readiness(value: Any, identity: dict[str, Any], where: str) -> int:
    common = {"identity", "boot_id", "observed_monotonic_ns"}
    if identity["role"] == "worker":
        ready = _keys(
            value, common | {"kind", "ready", "rpc_protocol", "connection_caps_sha256"}, where)
        if ready["kind"] != "pid-owned-rpc-hello" or ready["rpc_protocol"] != "4.0.1":
            _fail(f"{where} is not the exact RPC HELLO readiness proof")
        _hash(ready["connection_caps_sha256"], f"{where}.connection_caps_sha256")
    else:
        ready = _keys(
            value, common | {"kind", "ready", "body_sha256", "body_bytes"}, where)
        if ready["kind"] != "pid-owned-http-health":
            _fail(f"{where} is not the exact HTTP readiness proof")
        _hash(ready["body_sha256"], f"{where}.body_sha256")
        _strict_int(ready["body_bytes"], f"{where}.body_bytes", 1)
    _true(ready["ready"], f"{where}.ready")
    expected_identity = {
        key: identity[key] for key in (
            "role", "host", "unit", "pid", "invocation_id", "process_start_ticks")}
    if not _same(ready["identity"], expected_identity) or ready["boot_id"] != identity["boot_id"]:
        _fail(f"{where} differs from the exact process/boot identity")
    observed = _strict_int(
        ready["observed_monotonic_ns"], f"{where}.observed_monotonic_ns", 1)
    if observed <= identity["start_monotonic_us"] * 1000:
        _fail(f"{where}.observed_monotonic_ns does not follow process start")
    return observed


def _validate_terminal(
    value: Any, identity: dict[str, Any], journal_name: str, journal: bytes, where: str,
) -> None:
    terminal = _keys(value, {"properties", "journal"}, where)
    properties = _keys(
        terminal["properties"],
        {"ExecMainPID", "InvocationID", "ExecMainStatus", "Result", "ActiveState", "ControlGroup"},
        f"{where}.properties",
    )
    if properties["ExecMainPID"] != "0" or properties["InvocationID"] != identity["invocation_id"] or \
            properties["ExecMainStatus"] != "0" or properties["Result"] != "success" or \
            properties["ActiveState"] != "inactive" or \
            properties["ControlGroup"] not in {"", identity["control_group"]}:
        _fail(f"{where}.properties is not a successful terminal state for the captured identity")
    if not journal.strip():
        _fail(f"{where} journal is empty")
    _validate_binding(terminal["journal"], journal_name, journal, f"{where}.journal")


def _validate_cleanup(
    value: Any, identity: dict[str, Any], expected_fragment: str, where: str,
) -> int:
    cleanup = _keys(
        value,
        {
            "unit_absent", "port_closed", "stop_returncode", "reset_returncode",
            "captured_pid_absent", "captured_cgroup_absent", "captured_pid",
            "captured_control_group", "identity_source", "pre_state", "boot_id",
            "completed_monotonic_ns",
        },
        where,
    )
    for name in ("unit_absent", "port_closed", "captured_pid_absent", "captured_cgroup_absent"):
        _true(cleanup[name], f"{where}.{name}")
    if _strict_int(cleanup["stop_returncode"], f"{where}.stop_returncode") != 0 or \
            _strict_int(cleanup["reset_returncode"], f"{where}.reset_returncode") != 0 or \
            _strict_int(cleanup["captured_pid"], f"{where}.captured_pid", 1) != identity["pid"] or \
            cleanup["captured_control_group"] != identity["control_group"] or \
            cleanup["identity_source"] != "provided":
        _fail(f"{where} is not an exact identity-bound successful cleanup proof")
    pre = _keys(
        cleanup["pre_state"],
        {"LoadState", "ActiveState", "MainPID", "InvocationID", "ControlGroup", "FragmentPath"},
        f"{where}.pre_state",
    )
    loaded_terminal = (
        pre["LoadState"] == "loaded"
        and pre["ActiveState"] in {"inactive", "failed"}
        and pre["MainPID"] == "0"
        and pre["InvocationID"].lower() == identity["invocation_id"]
        and pre["ControlGroup"] in {"", identity["control_group"]}
        and pre["FragmentPath"] == expected_fragment
    )
    already_unloaded = pre == {
        "LoadState": "not-found",
        "ActiveState": "inactive",
        "MainPID": "0",
        "InvocationID": "",
        "ControlGroup": "",
        "FragmentPath": "",
    }
    if not loaded_terminal and not already_unloaded:
        _fail(f"{where}.pre_state differs from the captured terminal unit")
    if cleanup["boot_id"] != identity["boot_id"]:
        _fail(f"{where}.boot_id differs from the captured process boot")
    completed = _strict_int(
        cleanup["completed_monotonic_ns"], f"{where}.completed_monotonic_ns", 1)
    if completed <= identity["start_monotonic_us"] * 1000:
        _fail(f"{where}.completed_monotonic_ns does not follow process start")
    return completed


def _validate_response(value: Any, plan: dict[str, Any], where: str) -> dict[str, Any]:
    response = _keys(value, {"content", "timings"}, where)
    content = _string(response["content"], f"{where}.content", allow_empty=True)
    timings = _keys(
        response["timings"],
        {
            "cache_n", "prompt_n", "predicted_n", "prompt_ms", "predicted_ms",
            "prompt_per_token_ms", "prompt_per_second",
            "predicted_per_token_ms", "predicted_per_second",
        },
        f"{where}.timings",
    )
    if _strict_int(timings["cache_n"], f"{where}.timings.cache_n") != 0 or \
            _strict_int(timings["prompt_n"], f"{where}.timings.prompt_n", 1) != plan["request"]["prompt_tokens"] or \
            _strict_int(timings["predicted_n"], f"{where}.timings.predicted_n", 1) != plan["request"]["output_tokens"]:
        _fail(f"{where}.timings token counts differ from the cold-cache-off plan")
    prompt_ms = _number(timings["prompt_ms"], f"{where}.timings.prompt_ms", positive=True)
    predicted_ms = _number(timings["predicted_ms"], f"{where}.timings.predicted_ms", positive=True)
    prompt_per_token_ms = _number(
        timings["prompt_per_token_ms"], f"{where}.timings.prompt_per_token_ms", positive=True)
    predicted_per_token_ms = _number(
        timings["predicted_per_token_ms"],
        f"{where}.timings.predicted_per_token_ms", positive=True)
    prompt_rate = _number(
        timings["prompt_per_second"], f"{where}.timings.prompt_per_second", positive=True)
    predicted_rate = _number(
        timings["predicted_per_second"], f"{where}.timings.predicted_per_second", positive=True)
    # Match llama-server's operation order exactly (server-context.cpp): the
    # JSON value is formed as 1e3 / elapsed_ms * token_count.  Reassociating
    # the floating-point operations can differ by an ulp for real timings.
    expected_prompt_rate = 1_000.0 / prompt_ms * timings["prompt_n"]
    expected_predicted_rate = 1_000.0 / predicted_ms * timings["predicted_n"]
    expected_prompt_per_token_ms = prompt_ms / timings["prompt_n"]
    expected_predicted_per_token_ms = predicted_ms / timings["predicted_n"]
    if not math.isclose(prompt_rate, expected_prompt_rate, rel_tol=1e-12, abs_tol=1e-9) or \
            not math.isclose(predicted_rate, expected_predicted_rate, rel_tol=1e-12, abs_tol=1e-9) or \
            not math.isclose(
                prompt_per_token_ms, expected_prompt_per_token_ms,
                rel_tol=1e-12, abs_tol=1e-12) or \
            not math.isclose(
                predicted_per_token_ms, expected_predicted_per_token_ms,
                rel_tol=1e-12, abs_tol=1e-12):
        _fail(f"{where}.timings rates are inconsistent with counts and durations")
    content_sha = _sha(content.encode("utf-8"))
    expected_content = plan["request"].get("expected_content_sha256")
    if expected_content is not None and content_sha != expected_content:
        _fail(f"{where}.content differs from the frozen golden digest")
    return {
        "cache_n": 0,
        "prompt_n": timings["prompt_n"],
        "predicted_n": timings["predicted_n"],
        "prompt_ms": prompt_ms,
        "generation_ms": predicted_ms,
        # Normalize derived metrics to the retained counts/durations; the
        # server-provided rates are validation inputs, never analysis inputs.
        "prompt_tokens_per_second": expected_prompt_rate,
        "generation_tokens_per_second": expected_predicted_rate,
        "content_sha256": content_sha,
    }


def _validate_client(
    value: Any, output_tokens: int, expected_remote: tuple[int, int], where: str,
) -> dict[str, Any]:
    client = _keys(
        value,
        {
            "schema", "started_at", "ended_at", "http_status", "wall_ms", "ttft_ms",
            "itl_ms", "remote_started_monotonic_ns", "remote_ended_monotonic_ns",
            "event_monotonic_ns",
        },
        where,
    )
    if client["schema"] != core.CLIENT_SCHEMA_V2:
        _fail(f"{where}.schema is not the emitted-event-bound client v2 contract")
    started = _timestamp(client["started_at"], f"{where}.started_at")
    ended = _timestamp(client["ended_at"], f"{where}.ended_at")
    if ended <= started or _strict_int(client["http_status"], f"{where}.http_status", 100, 599) != 200:
        _fail(f"{where} does not describe a successful positive-duration HTTP request")
    wall = _number(client["wall_ms"], f"{where}.wall_ms", positive=True)
    ttft = _number(client["ttft_ms"], f"{where}.ttft_ms", positive=True)
    remote_start = _strict_int(
        client["remote_started_monotonic_ns"],
        f"{where}.remote_started_monotonic_ns", 1)
    remote_end = _strict_int(
        client["remote_ended_monotonic_ns"], f"{where}.remote_ended_monotonic_ns", 1)
    if (remote_start, remote_end) != expected_remote or remote_end <= remote_start:
        _fail(f"{where} remote interval differs from its execution request")
    events = _list(client["event_monotonic_ns"], f"{where}.event_monotonic_ns")
    if len(events) != output_tokens:
        _fail(f"{where}.event_monotonic_ns does not retain exactly one event per output token")
    observed_events = [
        _strict_int(item, f"{where}.event_monotonic_ns[{index}]", 1)
        for index, item in enumerate(events)]
    previous = remote_start
    for event in observed_events:
        if event <= previous or event >= remote_end:
            _fail(f"{where}.event_monotonic_ns is not strictly inside the remote request")
        previous = event
    intervals = _list(client["itl_ms"], f"{where}.itl_ms")
    if len(intervals) != output_tokens - 1:
        _fail(f"{where}.itl_ms does not cover every generated token after the first")
    itl = [_number(item, f"{where}.itl_ms[{index}]", positive=True) for index, item in enumerate(intervals)]
    expected_ttft = (observed_events[0] - remote_start) / 1_000_000
    expected_itl = [
        (right - left) / 1_000_000
        for left, right in zip(observed_events, observed_events[1:])]
    expected_wall = (remote_end - remote_start) / 1_000_000
    if not math.isclose(ttft, expected_ttft, rel_tol=1e-12, abs_tol=1e-9) or \
            not math.isclose(wall, expected_wall, rel_tol=1e-12, abs_tol=1e-9) or \
            any(not math.isclose(observed, expected, rel_tol=1e-12, abs_tol=1e-9)
                for observed, expected in zip(itl, expected_itl)):
        _fail(f"{where} timing summaries differ from retained remote emitted-event stamps")
    if ttft > wall or ttft + sum(itl) > wall + max(5.0, wall * 0.01):
        _fail(f"{where} token-event timing exceeds wall time")
    timestamp_ms = (ended - started).total_seconds() * 1000.0
    if not math.isclose(timestamp_ms, wall, rel_tol=0.001, abs_tol=50.0):
        _fail(f"{where} wall_ms differs from its retained UTC timestamp interval")
    return dict(client)


def _validate_sse(
    raw: bytes, response: dict[str, Any], request: dict[str, Any], where: str,
) -> int:
    output_tokens = request["n_predict"]
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeError as exc:
        raise AdapterEvidenceError(f"{where} is not UTF-8 SSE") from exc
    contents: list[str] = []
    terminal: dict[str, Any] | None = None
    previous_predicted = 0
    for line_number, line in enumerate(text.splitlines(), 1):
        stripped = line.strip()
        if not stripped:
            continue
        if not stripped.startswith("data:"):
            _fail(f"{where}:{line_number} is not an SSE data line")
        payload = stripped[5:].strip()
        # The frozen request uses llama-server's non-OAI /completion response
        # type. server-context.cpp emits EOF after the final response for that
        # type; [DONE] belongs to other response types and is therefore not an
        # admissible terminal marker here.
        if payload == "[DONE]":
            _fail(f"{where} contains an unsupported [DONE] marker")
        if terminal is not None:
            _fail(f"{where} contains an event after the terminal response")
        event = _mapping(
            _parse_json(payload.encode("utf-8"), f"{where}:{line_number}"),
            f"{where}:{line_number}",
        )
        if "error" in event:
            _fail(f"{where}:{line_number} is an error event, not successful completion evidence")
        stop = event.get("stop")
        if stop is False:
            partial = _keys(
                event,
                {"index", "content", "tokens", "stop", "id_slot", "tokens_predicted", "tokens_evaluated"},
                f"{where}:{line_number}",
            )
            tokens = _list(partial["tokens"], f"{where}:{line_number}.tokens")
            predicted = _strict_int(
                partial["tokens_predicted"],
                f"{where}:{line_number}.tokens_predicted", 1, output_tokens)
            if _strict_int(partial["index"], f"{where}:{line_number}.index") != 0 or \
                    _strict_int(partial["id_slot"], f"{where}:{line_number}.id_slot", -1) != -1 or \
                    len(tokens) != 1 or \
                    _strict_int(tokens[0], f"{where}:{line_number}.tokens[0]") < 0 or \
                    predicted <= previous_predicted or \
                    _strict_int(partial["tokens_evaluated"], f"{where}:{line_number}.tokens_evaluated", 1) != response["timings"]["prompt_n"]:
                _fail(f"{where}:{line_number} partial event counters are not exact")
            content = _string(
                partial["content"], f"{where}:{line_number}.content", allow_empty=True)
            contents.append(content)
            previous_predicted = predicted
            continue
        if stop is not True:
            _fail(f"{where}:{line_number} has no exact boolean stop state")
        terminal = _keys(
            event,
            {
                "index", "content", "tokens", "id_slot", "stop", "model",
                "tokens_predicted", "tokens_evaluated", "generation_settings", "prompt",
                "has_new_line", "truncated", "stop_type", "stopping_word",
                "tokens_cached", "timings",
            },
            f"{where}:{line_number}",
        )
        if _strict_int(terminal["index"], f"{where}:{line_number}.index") != 0 or \
                _strict_int(terminal["id_slot"], f"{where}:{line_number}.id_slot") < 0 or \
                terminal["content"] != "" or terminal["tokens"] != [] or \
                _strict_int(terminal["tokens_predicted"], f"{where}:{line_number}.tokens_predicted", 1) != output_tokens or \
                _strict_int(terminal["tokens_evaluated"], f"{where}:{line_number}.tokens_evaluated", 1) != response["timings"]["prompt_n"] or \
                terminal["truncated"] is not False or terminal["stop_type"] != "limit" or \
                terminal["stopping_word"] != "" or \
                _strict_int(terminal["tokens_cached"], f"{where}:{line_number}.tokens_cached") != \
                response["timings"]["prompt_n"] + output_tokens - 1 or \
                not isinstance(terminal["has_new_line"], bool) or \
                not isinstance(terminal["generation_settings"], dict) or \
                not isinstance(terminal["prompt"], str) or not isinstance(terminal["model"], str) or \
                not _same(terminal["timings"], response["timings"]):
            _fail(f"{where}:{line_number} terminal event is not the exact successful fixed-limit response")
        settings = _keys(
            terminal["generation_settings"], _GENERATION_SETTINGS_KEYS,
            f"{where}:{line_number}.generation_settings")
        if any(not _same(settings.get(name), expected) for name, expected in {
                "n_predict": output_tokens, "max_tokens": output_tokens,
                "stream": True, "ignore_eos": True,
                "seed": request["seed"], "temperature": request["temperature"],
                "stop": [], "n_probs": 0, "timings_per_token": False,
        }.items()):
            _fail(f"{where}:{line_number} terminal generation settings differ from the frozen request")
    content = "".join(contents)
    if terminal is None or len(contents) != output_tokens or previous_predicted != output_tokens or \
            content != response["content"] or terminal["has_new_line"] is not ("\n" in content):
        _fail(f"{where} SSE token/content/timing evidence differs from retained response")
    return len(contents)


def _validate_telemetry(
    value: Any, plan: dict[str, Any], request_start: int, request_end: int,
    process_started_ns: dict[str, int], expected_boot_ids: dict[str, str],
    role_deadlines_ns: dict[str, int], cleanup_completed_ns: dict[str, int],
    remote_request_start: int,
    remote_request_end: int, where: str,
) -> None:
    telemetry = _keys(
        value,
        {plan["topology"]["coordinator"]["host"], plan["topology"]["worker"]["host"]},
        where,
    )
    for host, raw_rows in telemetry.items():
        rows = _list(raw_rows, f"{where}.{host}")
        if not rows:
            _fail(f"{where}.{host} is empty")
        boot_id: str | None = None
        previous_remote = 0
        previous_controller_start = 0
        previous_controller_end = 0
        contained_in_request = False
        remote_overlaps = False
        for index, raw_row in enumerate(rows):
            row_where = f"{where}.{host}[{index}]"
            row = _keys(raw_row, {"controller_started_ns", "controller_ended_ns", "sample"}, row_where)
            controller_start = _strict_int(row["controller_started_ns"], f"{row_where}.controller_started_ns", 1)
            controller_end = _strict_int(row["controller_ended_ns"], f"{row_where}.controller_ended_ns", 1)
            if controller_end <= controller_start:
                _fail(f"{row_where} controller interval is reversed or empty")
            if controller_start <= previous_controller_start or \
                    controller_end <= previous_controller_end:
                _fail(f"{where}.{host} controller sample order is not strictly increasing")
            previous_controller_start = controller_start
            previous_controller_end = controller_end
            contained_in_request |= (
                controller_start >= request_start and controller_end <= request_end)
            sample = _keys(
                row["sample"], {"monotonic_ns", "boot_id", "loadavg", "meminfo", "gpu"},
                f"{row_where}.sample")
            remote = _strict_int(sample["monotonic_ns"], f"{row_where}.sample.monotonic_ns", 1)
            if remote <= process_started_ns[host]:
                _fail(f"{row_where}.sample.monotonic_ns does not follow process start")
            if remote >= role_deadlines_ns[host]:
                _fail(f"{row_where}.sample.monotonic_ns escapes the disposable freshness/lifetime window")
            if remote >= cleanup_completed_ns[host]:
                _fail(f"{row_where}.sample.monotonic_ns does not precede same-host cleanup")
            observed_boot = _string(sample["boot_id"], f"{row_where}.sample.boot_id")
            if _BOOT_ID_RE.fullmatch(observed_boot) is None:
                _fail(f"{row_where}.sample.boot_id is malformed")
            if observed_boot != expected_boot_ids[host]:
                _fail(f"{row_where}.sample.boot_id differs from bound HMM admission boot")
            if boot_id is None:
                boot_id = observed_boot
            if observed_boot != boot_id or remote <= previous_remote:
                _fail(f"{where}.{host} boot identity or remote monotonic order changed")
            previous_remote = remote
            if host == plan["topology"]["coordinator"]["host"]:
                remote_overlaps |= remote_request_start <= remote < remote_request_end
            loadavg = _string(sample["loadavg"], f"{row_where}.sample.loadavg")
            match = _LOADAVG_RE.fullmatch(loadavg)
            if match is None or any(not math.isfinite(float(value)) for value in match.groups()[:3]) or \
                    int(match.group(4)) > int(match.group(5)) or int(match.group(5)) <= 0 or \
                    int(match.group(6)) <= 0:
                _fail(f"{row_where}.sample.loadavg is not canonical /proc/loadavg telemetry")
            meminfo = _string(sample["meminfo"], f"{row_where}.sample.meminfo")
            memory: dict[str, tuple[int, str | None]] = {}
            for line in meminfo.splitlines():
                match = _MEMINFO_LINE_RE.fullmatch(line)
                if match is None or match.group(1) in memory:
                    _fail(f"{row_where}.sample.meminfo is malformed or has duplicate keys")
                memory[match.group(1)] = (int(match.group(2)), match.group(3))
            for field in ("MemTotal", "MemAvailable"):
                if field not in memory or memory[field][0] <= 0 or memory[field][1] != "kB":
                    _fail(f"{row_where}.sample.meminfo lacks positive {field} kB telemetry")
            if memory["MemAvailable"][0] > memory["MemTotal"][0]:
                _fail(f"{row_where}.sample.meminfo available memory exceeds total memory")
            gpu = _mapping(sample["gpu"], f"{row_where}.sample.gpu")
            if not gpu:
                _fail(f"{row_where}.sample.gpu is empty")
            for gpu_path, reading in gpu.items():
                if not isinstance(gpu_path, str) or (
                        _GPU_BUSY_PATH_RE.fullmatch(gpu_path) is None and
                        _GPU_TEMP_PATH_RE.fullmatch(gpu_path) is None):
                    _fail(f"{row_where}.sample.gpu contains an unrecognized sensor path")
                reading_text = _string(reading, f"{row_where}.sample.gpu[{gpu_path!r}]")
                try:
                    reading_value = int(reading_text)
                except ValueError as exc:
                    raise AdapterEvidenceError(
                        f"{row_where}.sample.gpu[{gpu_path!r}] is not an integer reading") from exc
                if _GPU_BUSY_PATH_RE.fullmatch(gpu_path) is not None:
                    if not 0 <= reading_value <= 100:
                        _fail(f"{row_where}.sample.gpu[{gpu_path!r}] is outside [0, 100]")
                elif not 0 <= reading_value <= 200_000:
                    _fail(f"{row_where}.sample.gpu[{gpu_path!r}] is outside [0, 200000]")
        if not contained_in_request:
            _fail(f"{where}.{host} has no sample wholly contained in the request interval")
        if host == plan["topology"]["coordinator"]["host"] and not remote_overlaps:
            _fail(f"{where}.{host} has no remote sample overlapping the same-host request")


def _validate_gpu_monitor(
    value: Any, plan: dict[str, Any], identities: dict[str, dict[str, Any]],
    request_start: int, request_end: int, where: str,
) -> None:
    monitor = _keys(value, {"samples", "errors"}, where)
    if monitor["errors"] != []:
        _fail(f"{where}.errors is not empty")
    rows = _list(monitor["samples"], f"{where}.samples")
    if len(rows) < 3:
        _fail(f"{where}.samples lacks pre/overlap/post request censuses")
    coverage = {role: {"pre": False, "overlap": False, "post": False} for role in ("coordinator", "worker")}
    common_contained_witness = False
    previous_outer_start = 0
    previous_outer_end = 0
    for index, value_row in enumerate(rows):
        row_where = f"{where}.samples[{index}]"
        row = _keys(
            value_row, {"controller_started_ns", "controller_ended_ns", "admission", "error"},
            row_where)
        if row["error"] is not None:
            _fail(f"{row_where}.error is not null")
        outer_start = _strict_int(row["controller_started_ns"], f"{row_where}.controller_started_ns", 1)
        outer_end = _strict_int(row["controller_ended_ns"], f"{row_where}.controller_ended_ns", 1)
        if outer_end <= outer_start:
            _fail(f"{row_where} outer interval is reversed or empty")
        if outer_start <= previous_outer_start or outer_end <= previous_outer_end:
            _fail(f"{where}.samples controller order is not strictly increasing")
        previous_outer_start = outer_start
        previous_outer_end = outer_end
        windows = _validate_gpu_census(
            row["admission"], plan,
            {role: [identities[role]["pid"]] for role in ("coordinator", "worker")},
            f"{row_where}.admission")
        for role, (started, ended) in windows.items():
            if started < outer_start or ended > outer_end:
                _fail(f"{row_where}.admission.{role} escapes its outer census interval")
            coverage[role]["pre"] |= ended <= request_start
            coverage[role]["overlap"] |= started >= request_start and ended <= request_end
            coverage[role]["post"] |= started >= request_end
        common_contained_witness |= all(
            started >= request_start and ended <= request_end
            for started, ended in windows.values())
    if any(not all(flags.values()) for flags in coverage.values()):
        _fail(f"{where} does not cover pre-request, contained-request, and post-request ownership")
    if not common_contained_witness:
        _fail(f"{where} has no common two-role census wholly contained in the request")


def _validate_cycle(
    value: Any, *, files: dict[str, bytes], directory: str, schedule_index: int,
    entry: dict[str, Any], plan: dict[str, Any], policy: Any, kind: str, ordinal: int,
    intent_time: dt.datetime, hmm_domain: _HmmDomain,
) -> tuple[
    dict[str, Any], dict[str, Any], dict[str, dict[str, Any]],
    tuple[int, int], tuple[int, int], dict[str, tuple[int, int]],
]:
    where = f"execution entry {schedule_index} {kind}-{ordinal}"
    cycle = _keys(
        value,
        {
            "kind", "ordinal", "status", "identities", "live_proofs", "readiness",
            "request", "telemetry", "gpu_admission", "terminal", "cleanup", "errors",
        },
        where,
    )
    if cycle["kind"] != kind or \
            _strict_int(cycle["ordinal"], f"{where}.ordinal") != ordinal or \
            cycle["status"] != "success" or \
            cycle["errors"] != []:
        _fail(f"{where} is not the required successful cycle")
    identities_raw = _keys(cycle["identities"], {"coordinator", "worker"}, f"{where}.identities")
    identities = {
        role: _validate_identity(
            identities_raw[role], role=role, kind=kind, ordinal=ordinal,
            schedule_index=schedule_index, entry=entry, plan=plan, policy=policy,
            expected_boot_id=hmm_domain.boot_ids[plan["topology"][role]["host"]],
            where=f"{where}.identities.{role}")
        for role in ("coordinator", "worker")
    }
    if identities["coordinator"]["pid"] == identities["worker"]["pid"] and \
            identities["coordinator"]["host"] == identities["worker"]["host"]:
        _fail(f"{where} roles collide on one host/PID")
    for role in ("coordinator", "worker"):
        host = identities[role]["host"]
        if identities[role]["start_monotonic_us"] * 1000 <= \
                hmm_domain.capture_completed_monotonic_ns[host]:
            _fail(f"{where}.{role} same-boot process start does not follow bound HMM capture")
        if identities[role]["start_monotonic_us"] * 1000 >= \
                hmm_domain.capture_completed_monotonic_ns[host] + \
                hmm_domain.max_snapshot_age_seconds * 1_000_000_000:
            _fail(f"{where}.{role} process start is outside the bound HMM monotonic freshness window")

    live = _keys(
        cycle["live_proofs"],
        {
            "worker_process", "worker_ready", "coordinator_process", "coordinator_ready",
            "worker_after_coordinator_ready", "worker_after_request",
        },
        f"{where}.live_proofs",
    )
    worker_fragment, worker_process_observed = _validate_live_proof(
        live["worker_process"], identities["worker"], require_listener=False,
        where=f"{where}.live_proofs.worker_process")
    _, worker_ready_observed = _validate_live_proof(
        live["worker_ready"], identities["worker"], require_listener=True,
        where=f"{where}.live_proofs.worker_ready", expected_fragment=worker_fragment)
    _, worker_after_coordinator_observed = _validate_live_proof(
        live["worker_after_coordinator_ready"], identities["worker"], require_listener=True,
        where=f"{where}.live_proofs.worker_after_coordinator_ready",
        expected_fragment=worker_fragment)
    coordinator_fragment, coordinator_process_observed = _validate_live_proof(
        live["coordinator_process"], identities["coordinator"], require_listener=False,
        where=f"{where}.live_proofs.coordinator_process")
    _, coordinator_ready_observed = _validate_live_proof(
        live["coordinator_ready"], identities["coordinator"], require_listener=True,
        where=f"{where}.live_proofs.coordinator_ready", expected_fragment=coordinator_fragment)
    readiness = _keys(cycle["readiness"], {"coordinator", "worker"}, f"{where}.readiness")
    readiness_observed = {
        role: _validate_readiness(
            readiness[role], identities[role], f"{where}.readiness.{role}")
        for role in ("coordinator", "worker")}
    if not (
            worker_process_observed < readiness_observed["worker"] < worker_ready_observed <
            worker_after_coordinator_observed) or not (
            coordinator_process_observed < readiness_observed["coordinator"] <
            coordinator_ready_observed):
        _fail(f"{where} live/readiness host-local order is not strict")

    runtime_limit_ns = policy.runtime_max_seconds * 1_000_000_000
    role_deadlines_ns = {
        identities[role]["host"]: min(
            identities[role]["start_monotonic_us"] * 1000 + runtime_limit_ns,
            hmm_domain.capture_completed_monotonic_ns[identities[role]["host"]] +
            hmm_domain.max_snapshot_age_seconds * 1_000_000_000)
        for role in ("coordinator", "worker")
    }
    role_observations = {
        "coordinator": (
            coordinator_process_observed, readiness_observed["coordinator"],
            coordinator_ready_observed),
        "worker": (
            worker_process_observed, readiness_observed["worker"],
            worker_ready_observed, worker_after_coordinator_observed),
    }
    for role, observed_values in role_observations.items():
        deadline = role_deadlines_ns[identities[role]["host"]]
        if any(observed >= deadline for observed in observed_values):
            _fail(f"{where}.{role} live/readiness evidence escapes freshness/lifetime")

    request = _keys(
        cycle["request"],
        {
            "body_sha256", "remote_host", "remote_started_monotonic_ns",
            "remote_ended_monotonic_ns", "controller_started_monotonic_ns",
            "controller_ended_monotonic_ns", "response_sha256", "client_sha256",
            "raw_http_sha256",
        },
        f"{where}.request",
    )
    request_bytes = files["inputs/request.raw"]
    if _hash(request["body_sha256"], f"{where}.request.body_sha256") != _sha(request_bytes) or \
            request["remote_host"] != plan["topology"]["coordinator"]["host"]:
        _fail(f"{where}.request differs from retained request/host")
    remote_start = _strict_int(
        request["remote_started_monotonic_ns"], f"{where}.request.remote_started_monotonic_ns", 1)
    remote_end = _strict_int(
        request["remote_ended_monotonic_ns"], f"{where}.request.remote_ended_monotonic_ns", 1)
    controller_start = _strict_int(
        request["controller_started_monotonic_ns"], f"{where}.request.controller_started_monotonic_ns", 1)
    controller_end = _strict_int(
        request["controller_ended_monotonic_ns"], f"{where}.request.controller_ended_monotonic_ns", 1)
    if remote_end <= remote_start or controller_end <= controller_start:
        _fail(f"{where}.request has a reversed or zero monotonic interval")
    coordinator_start_ns = identities["coordinator"]["start_monotonic_us"] * 1000
    if coordinator_start_ns >= remote_start:
        _fail(f"{where}.request does not follow coordinator process start")
    remote_duration = remote_end - remote_start
    controller_duration = controller_end - controller_start
    request_limit_ns = policy.request_timeout_seconds * 1_000_000_000
    if remote_end >= role_deadlines_ns[identities["coordinator"]["host"]]:
        _fail(f"{where}.request escapes coordinator disposable freshness/lifetime")
    if coordinator_ready_observed >= remote_start:
        _fail(f"{where}.request does not follow coordinator readiness")
    if remote_duration > request_limit_ns or controller_duration > request_limit_ns:
        _fail(f"{where}.request exceeds the frozen policy request timeout")
    if remote_duration > controller_duration + max(5_000_000, int(controller_duration * 0.01)):
        _fail(f"{where}.request remote duration escapes the controller request envelope")

    evidence_dir = f"{directory}/{'measured' if kind == 'measurement' else f'warmup-{ordinal}'}"
    stem = f"{kind}-{ordinal}"
    response_name = f"{evidence_dir}/{stem}-response.json"
    client_name = f"{evidence_dir}/{stem}-client.json"
    raw_name = f"{evidence_dir}/{stem}-response.raw"
    response_value = _mapping(_parse_json(files[response_name], response_name), response_name)
    client_value = _mapping(_parse_json(files[client_name], client_name), client_name)
    result = _validate_response(response_value, plan, response_name)
    client = _validate_client(
        client_value, plan["request"]["output_tokens"],
        (remote_start, remote_end), client_name)
    client_started = _timestamp(client["started_at"], f"{client_name}.started_at")
    client_ended = _timestamp(client["ended_at"], f"{client_name}.ended_at")
    if client_started < intent_time or client_ended >= hmm_domain.expires or \
            (client_ended - hmm_domain.snapshot_completed).total_seconds() >= \
            hmm_domain.max_snapshot_age_seconds:
        _fail(f"{where}.client escapes the bound HMM capture/validity window")
    server_duration_ms = result["prompt_ms"] + result["generation_ms"]
    client_wall_ms = _number(client["wall_ms"], f"{client_name}.wall_ms", positive=True)
    if result["prompt_ms"] > _number(
            client["ttft_ms"], f"{client_name}.ttft_ms", positive=True) + \
            max(5.0, client_wall_ms * 0.01):
        _fail(f"{where}.response prompt phase exceeds same request TTFT")
    if server_duration_ms > client_wall_ms + max(5.0, client_wall_ms * 0.01):
        _fail(f"{where}.response server timings exceed the same request wall time")
    wall_duration = client_wall_ms * 1_000_000
    # The remote script derives both fields from the same monotonic start/end
    # pair; only decimal JSON round-off is admissible here.
    if not math.isclose(wall_duration, remote_duration, rel_tol=1e-9, abs_tol=1_000.0):
        _fail(f"{where}.request remote monotonic duration differs from client wall_ms")
    retained_request = _mapping(
        _parse_json(files["inputs/request.raw"], "inputs/request.raw"),
        "inputs/request.raw")
    emitted_events = _validate_sse(
        files[raw_name], response_value, retained_request, raw_name)
    if emitted_events != len(client["event_monotonic_ns"]):
        _fail(f"{where} SSE event count differs from retained client event stamps")
    for field, content in (
        ("response_sha256", files[response_name]),
        ("client_sha256", files[client_name]),
        ("raw_http_sha256", files[raw_name]),
    ):
        if _hash(request[field], f"{where}.request.{field}") != _sha(content):
            _fail(f"{where}.request.{field} differs from retained evidence")

    terminal = _keys(cycle["terminal"], {"coordinator", "worker"}, f"{where}.terminal")
    cleanup = _keys(cycle["cleanup"], {"coordinator", "worker"}, f"{where}.cleanup")
    lifecycle: dict[str, tuple[int, int]] = {}
    for role in ("coordinator", "worker"):
        journal_name = f"{evidence_dir}/{stem}-{role}.journal"
        _validate_terminal(
            terminal[role], identities[role], Path(journal_name).name, files[journal_name],
            f"{where}.terminal.{role}")
        completed = _validate_cleanup(
            cleanup[role], identities[role],
            coordinator_fragment if role == "coordinator" else worker_fragment,
            f"{where}.cleanup.{role}")
        role_start = identities[role]["start_monotonic_us"] * 1000
        role_last_live = remote_end if role == "coordinator" else None
        if role == "worker":
            post_request = _keys(
                live["worker_after_request"],
                {"controller_started_monotonic_ns", "controller_ended_monotonic_ns", "proof"},
                f"{where}.live_proofs.worker_after_request",
            )
            post_controller_start = _strict_int(
                post_request["controller_started_monotonic_ns"],
                f"{where}.live_proofs.worker_after_request.controller_started_monotonic_ns", 1)
            post_controller_end = _strict_int(
                post_request["controller_ended_monotonic_ns"],
                f"{where}.live_proofs.worker_after_request.controller_ended_monotonic_ns", 1)
            if post_controller_start <= controller_end or post_controller_end <= post_controller_start:
                _fail(f"{where}.worker post-request proof is outside the strict controller order")
            _, role_last_live = _validate_live_proof(
                post_request["proof"], identities["worker"], require_listener=True,
                where=f"{where}.live_proofs.worker_after_request.proof",
                expected_fragment=worker_fragment)
            if role_last_live <= worker_after_coordinator_observed:
                _fail(f"{where}.worker post-request proof replays earlier host-local liveness")
        deadline = role_deadlines_ns[identities[role]["host"]]
        if role_last_live is None or role_last_live >= completed or completed >= deadline:
            _fail(f"{where}.{role} terminal cleanup is outside strict host-local lifecycle bounds")
        lifecycle[role] = (role_start, completed)

    if kind == "measurement":
        telemetry_name = f"{evidence_dir}/telemetry.json"
        admission_name = f"{evidence_dir}/gpu-admission.json"
        telemetry = _mapping(_parse_json(files[telemetry_name], telemetry_name), telemetry_name)
        admission = _mapping(_parse_json(files[admission_name], admission_name), admission_name)
        if not _same(cycle["telemetry"], telemetry) or not _same(cycle["gpu_admission"], admission):
            _fail(f"{where} inline telemetry/GPU admission differs from retained JSON")
        process_started_ns = {
            identities[role]["host"]: identities[role]["start_monotonic_us"] * 1000
            for role in ("coordinator", "worker")
        }
        _validate_telemetry(
            telemetry, plan, controller_start, controller_end,
            process_started_ns, hmm_domain.boot_ids, role_deadlines_ns,
            {
                identities[role]["host"]: lifecycle[role][1]
                for role in ("coordinator", "worker")
            },
            remote_start, remote_end, telemetry_name)
        _validate_gpu_monitor(
            admission, plan, identities, controller_start, controller_end, admission_name)
    elif cycle["telemetry"] is not None or cycle["gpu_admission"] is not None:
        _fail(f"{where} warmup must not claim measured telemetry or GPU admission")
    return (
        result, client, identities,
        (remote_start, remote_end), (controller_start, controller_end), lifecycle,
    )


def _expected_inventory(
    schedule: dict[str, Any], plan: dict[str, Any], *, sampling_profile: bool,
) -> tuple[set[str], set[str]]:
    files = {
        "plan.json", "schedule.json", "commands.json", "status.json", "analysis.json",
        "samples.jsonl", "SHA256SUMS", "incident.raw",
        HMM_SNAPSHOT_FILENAME, HMM_POLICY_FILENAME, HMM_RESULT_FILENAME,
        "preflight/coordinator.json", "preflight/worker.json",
        "inputs/authority-coordinator.raw", "inputs/authority-worker.raw", "inputs/request.raw",
    }
    directories = {"preflight", "inputs", "execution", "raw"}
    warmups = plan["execution"]["warmups_per_condition"]
    for index, entry in enumerate(schedule["entries"]):
        execution = f"execution/entry-{index:03d}"
        directories.add(execution)
        files.update({f"{execution}/intent.json", f"{execution}/policy.raw", f"{execution}/execution.json"})
        for ordinal in range(warmups):
            cycle = f"{execution}/warmup-{ordinal}"
            directories.add(cycle)
            stem = f"warmup-{ordinal}"
            files.update({
                f"{cycle}/{stem}-response.json", f"{cycle}/{stem}-client.json",
                f"{cycle}/{stem}-response.raw", f"{cycle}/{stem}-coordinator.journal",
                f"{cycle}/{stem}-worker.journal",
            })
        measured = f"{execution}/measured"
        directories.add(measured)
        files.update({
            f"{measured}/measurement-0-response.json",
            f"{measured}/measurement-0-client.json",
            f"{measured}/measurement-0-response.raw",
            f"{measured}/measurement-0-coordinator.journal",
            f"{measured}/measurement-0-worker.journal",
            f"{measured}/telemetry.json", f"{measured}/gpu-admission.json",
        })
        sample = (
            f"raw/pair-{entry['pair_id']:03d}-order-{entry['order_index']}-{entry['condition']}")
        directories.add(sample)
        files.update({
            f"{sample}/sample.json", f"{sample}/response.json", f"{sample}/client.json",
            f"{sample}/extra-00-execution.json", f"{sample}/extra-01-policy.raw",
            f"{sample}/extra-02-measurement-0-response.raw",
            f"{sample}/extra-03-telemetry.json", f"{sample}/extra-04-gpu-admission.json",
        })
        if sampling_profile:
            sidecar = f"{sample}/{SAMPLING_SYNC_DIRECTORY}"
            directories.add(sidecar)
            files.update(
                f"{sidecar}/{filename}" for filename in SAMPLING_SYNC_SAMPLE_FILES)
    if sampling_profile:
        files.update(SAMPLING_SYNC_ROOT_FILES)
    return files, directories


def _expected_inventory_entry_count(schedule_entries: int, warmups: int, sampling: bool) -> int:
    """Exact file+directory count for the closed supported profile."""
    base = 20
    per_entry = 21 + 6 * warmups
    if sampling:
        base += 2
        per_entry += 5
    return base + schedule_entries * per_entry


def _validate_preflights(
    files: dict[str, bytes], plan: dict[str, Any], authority_bytes: dict[str, bytes],
) -> dict[str, str]:
    hashes: dict[str, str] = {}
    for role in ("coordinator", "worker"):
        name = f"preflight/{role}.json"
        receipt = _mapping(_parse_json(files[name], name), name)
        try:
            observed_role = core.validate_preflight_receipt(plan, receipt)
        except core.PlanError as exc:
            raise AdapterEvidenceError(f"{name} fails core validation: {exc}") from exc
        if observed_role != role:
            _fail(f"{name} role differs from its retained path")
        system = _keys(
            receipt["system"], {"platform", "uname", "python", "os_release"},
            f"{name}.system")
        _string(system["platform"], f"{name}.system.platform")
        _string(system["python"], f"{name}.system.python")
        _string(system["os_release"], f"{name}.system.os_release", allow_empty=True)
        uname = _list(system["uname"], f"{name}.system.uname")
        if len(uname) != 6:
            _fail(f"{name}.system.uname must retain the exact six platform fields")
        for index, field in enumerate(uname):
            _string(field, f"{name}.system.uname[{index}]", allow_empty=True)
        if any(not uname[index] for index in (0, 1, 2, 4)):
            _fail(f"{name}.system.uname lacks system/node/release/machine identity")
        retained_authority = core.decode_retained_artifact(
            receipt["artifacts"]["authority_receipt"], f"{name} authority_receipt")
        if retained_authority != authority_bytes[role]:
            _fail(f"{name} retained authority differs from inputs/authority-{role}.raw")
        if role == "coordinator":
            retained_request = core.decode_retained_artifact(
                receipt["artifacts"]["request"], f"{name} request")
            if retained_request != files["inputs/request.raw"]:
                _fail(f"{name} retained request differs from inputs/request.raw")
        hashes[role] = _sha(files[name])
    return hashes


def _validate_execution_entry(
    *, files: dict[str, bytes], schedule_index: int, entry: dict[str, Any],
    plan: dict[str, Any], policy: Any, policy_bytes: bytes, preflight_hashes: dict[str, str],
    hmm_domain: _HmmDomain,
) -> tuple[
    dict[str, Any], dict[str, Any], dict[str, Any], bytes,
    tuple[dt.datetime, dt.datetime, dt.datetime], tuple[tuple[int, int], ...],
    tuple[tuple[int, int], ...], dict[str, tuple[int, int]], str,
]:
    directory = f"execution/entry-{schedule_index:03d}"
    intent_name = f"{directory}/intent.json"
    receipt_name = f"{directory}/execution.json"
    retained_policy_name = f"{directory}/policy.raw"
    if files[retained_policy_name] != policy_bytes:
        _fail(f"{retained_policy_name} differs from expected policy bytes")
    policy_binding = {
        "path": "policy.raw", "size_bytes": len(policy_bytes), "sha256": _sha(policy_bytes)}
    intent = _keys(
        _parse_json(files[intent_name], intent_name),
        {
            "schema", "issue", "experiment_id", "plan_sha256", "policy_sha256",
            "policy_binding", "schedule_index", "entry", "created_at", "no_retry_after_intent",
        },
        intent_name,
    )
    if intent["schema"] != adapter.INTENT_SCHEMA or \
            _strict_int(intent["issue"], f"{intent_name}.issue") != 37 or \
            intent["experiment_id"] != plan["experiment_id"] or \
            intent["plan_sha256"] != core.plan_digest(plan) or \
            intent["policy_sha256"] != _sha(policy_bytes) or not _same(intent["policy_binding"], policy_binding) or \
            _strict_int(intent["schedule_index"], f"{intent_name}.schedule_index") != schedule_index or \
            not _same(intent["entry"], entry):
        _fail(f"{intent_name} differs from the frozen plan/schedule/policy")
    intent_time = _timestamp(intent["created_at"], f"{intent_name}.created_at")
    if intent_time < hmm_domain.trusted_now or intent_time >= hmm_domain.expires or \
            (intent_time - hmm_domain.snapshot_completed).total_seconds() >= \
            hmm_domain.max_snapshot_age_seconds:
        _fail(f"{intent_name}.created_at escapes the bound HMM capture/validity window")
    _true(intent["no_retry_after_intent"], f"{intent_name}.no_retry_after_intent")

    receipt = _keys(
        _parse_json(files[receipt_name], receipt_name),
        {
            "schema", "issue", "experiment_id", "plan_sha256", "policy_sha256",
            "policy_binding", "schedule_index", "entry", "input_bindings", "model_binding",
            "preflight_sha256", "production_before", "production_after",
            "gpu_admission_before_intent", "model_binding_after", "cycles", "errors", "outcome",
            "execution_qualified", "measurement_ready", "performance_claim",
        },
        receipt_name,
    )
    if receipt["schema"] != adapter.RECEIPT_SCHEMA or \
            _strict_int(receipt["issue"], f"{receipt_name}.issue") != 37 or \
            receipt["experiment_id"] != plan["experiment_id"] or \
            receipt["plan_sha256"] != core.plan_digest(plan) or \
            receipt["policy_sha256"] != _sha(policy_bytes) or \
            not _same(receipt["policy_binding"], policy_binding) or \
            _strict_int(receipt["schedule_index"], f"{receipt_name}.schedule_index") != schedule_index or \
            not _same(receipt["entry"], entry):
        _fail(f"{receipt_name} differs from the frozen plan/schedule/policy")
    expected_inputs = {
        "coordinator_authority_receipt": {
            "path": "inputs/authority-coordinator.raw",
            "size_bytes": len(files["inputs/authority-coordinator.raw"]),
            "sha256": _sha(files["inputs/authority-coordinator.raw"]),
        },
        "worker_authority_receipt": {
            "path": "inputs/authority-worker.raw",
            "size_bytes": len(files["inputs/authority-worker.raw"]),
            "sha256": _sha(files["inputs/authority-worker.raw"]),
        },
        "coordinator_request": {
            "path": "inputs/request.raw", "size_bytes": len(files["inputs/request.raw"]),
            "sha256": _sha(files["inputs/request.raw"]),
        },
    }
    if not _same(receipt["input_bindings"], expected_inputs):
        _fail(f"{receipt_name}.input_bindings differs from captured retained inputs")
    expected_model = {
        "path": plan["model"]["path"], "size_bytes": plan["model"]["size_bytes"],
        "sha256": plan["model"]["sha256"],
    }
    if not _same(receipt["model_binding"], expected_model) or not _same(receipt["model_binding_after"], expected_model):
        _fail(f"{receipt_name} model bindings differ from the frozen model")
    if receipt["preflight_sha256"] != preflight_hashes:
        _fail(f"{receipt_name}.preflight_sha256 differs from captured preflights")
    before = _validate_production_snapshot(
        receipt["production_before"], policy, f"{receipt_name}.production_before")
    after = _validate_production_snapshot(
        receipt["production_after"], policy, f"{receipt_name}.production_after")
    if not _same(before, after):
        _fail(f"{receipt_name} protected production snapshots differ")
    _validate_gpu_census(
        receipt["gpu_admission_before_intent"], plan,
        {"coordinator": [], "worker": []}, f"{receipt_name}.gpu_admission_before_intent")
    if receipt["errors"] != []:
        _fail(f"{receipt_name}.errors is not empty")
    outcome = _keys(receipt["outcome"], {"status", "failure_code"}, f"{receipt_name}.outcome")
    if not _same(outcome, {"status": "success", "failure_code": None}):
        _fail(f"{receipt_name}.outcome is not successful")
    for field in ("execution_qualified", "measurement_ready", "performance_claim"):
        _false(receipt[field], f"{receipt_name}.{field}")

    cycles = _list(receipt["cycles"], f"{receipt_name}.cycles")
    warmup_count = plan["execution"]["warmups_per_condition"]
    if len(cycles) != warmup_count + 1:
        _fail(f"{receipt_name}.cycles does not contain exact warmups plus one measurement")
    identity_history: dict[str, list[dict[str, Any]]] = {"coordinator": [], "worker": []}
    cycle_wall_windows: list[tuple[dt.datetime, dt.datetime]] = []
    remote_windows: list[tuple[int, int]] = []
    controller_windows: list[tuple[int, int]] = []
    lifecycle_windows: list[dict[str, tuple[int, int]]] = []
    content_hashes: list[str] = []
    for ordinal in range(warmup_count):
        cycle_result, cycle_client, identities, remote_window, controller_window, lifecycle = _validate_cycle(
            cycles[ordinal], files=files, directory=directory, schedule_index=schedule_index,
            entry=entry, plan=plan, policy=policy, kind="warmup", ordinal=ordinal,
            intent_time=intent_time, hmm_domain=hmm_domain)
        cycle_wall_windows.append((
            _timestamp(cycle_client["started_at"], f"{receipt_name}.cycles[{ordinal}].client.started_at"),
            _timestamp(cycle_client["ended_at"], f"{receipt_name}.cycles[{ordinal}].client.ended_at"),
        ))
        remote_windows.append(remote_window)
        controller_windows.append(controller_window)
        lifecycle_windows.append(lifecycle)
        content_hashes.append(cycle_result["content_sha256"])
        for role in identity_history:
            identity_history[role].append(identities[role])
    measured_result, measured_client, measured_identities, remote_window, controller_window, lifecycle = _validate_cycle(
        cycles[-1], files=files, directory=directory, schedule_index=schedule_index,
        entry=entry, plan=plan, policy=policy, kind="measurement", ordinal=0,
        intent_time=intent_time, hmm_domain=hmm_domain)
    cycle_wall_windows.append((
        _timestamp(measured_client["started_at"], f"{receipt_name}.cycles[-1].client.started_at"),
        _timestamp(measured_client["ended_at"], f"{receipt_name}.cycles[-1].client.ended_at"),
    ))
    remote_windows.append(remote_window)
    controller_windows.append(controller_window)
    lifecycle_windows.append(lifecycle)
    content_hashes.append(measured_result["content_sha256"])
    if any(later[0] <= earlier[1]
           for earlier, later in zip(cycle_wall_windows, cycle_wall_windows[1:])):
        _fail(f"{receipt_name} cycle request wall intervals overlap or replay")
    for name, windows in (("remote", remote_windows), ("controller", controller_windows)):
        if any(later[0] <= earlier[1]
               for earlier, later in zip(windows, windows[1:])):
            _fail(f"{receipt_name} cycle {name} monotonic intervals overlap or replay")
    for role in identity_history:
        identity_history[role].append(measured_identities[role])
        seen_invocations: set[str] = set()
        seen_processes: set[tuple[int, int]] = set()
        prior_start = 0
        for identity in identity_history[role]:
            invocation = identity["invocation_id"]
            process_key = (identity["pid"], identity["process_start_ticks"])
            if invocation in seen_invocations or process_key in seen_processes or \
                    identity["start_monotonic_us"] <= prior_start:
                _fail(f"{receipt_name} {role} process identities are not fresh and increasing")
            seen_invocations.add(invocation)
            seen_processes.add(process_key)
            prior_start = identity["start_monotonic_us"]
        if any(
                later[role][0] <= earlier[role][1]
                for earlier, later in zip(lifecycle_windows, lifecycle_windows[1:])):
            _fail(f"{receipt_name} {role} process starts before predecessor cleanup")
    if len(set(content_hashes)) != 1:
        _fail(f"{receipt_name} warmup/measurement deterministic content differs")
    return (
        receipt, measured_result, measured_client, files[receipt_name],
        (intent_time, cycle_wall_windows[0][0], cycle_wall_windows[-1][1]),
        tuple(remote_windows), tuple(controller_windows),
        {
            role: (lifecycle_windows[0][role][0], lifecycle_windows[-1][role][1])
            for role in ("coordinator", "worker")
        },
        content_hashes[0],
    )


def _sample_directory(entry: dict[str, Any]) -> str:
    return f"raw/pair-{entry['pair_id']:03d}-order-{entry['order_index']}-{entry['condition']}"


def _validate_sample(
    *, files: dict[str, bytes], entry: dict[str, Any], schedule_index: int,
    plan: dict[str, Any], policy_bytes: bytes, receipt_bytes: bytes,
    measured_result: dict[str, Any], measured_client: dict[str, Any],
) -> tuple[dict[str, Any], str]:
    directory = _sample_directory(entry)
    name = f"{directory}/sample.json"
    sample = _keys(
        _parse_json(files[name], name),
        {
            "schema", "experiment_id", "plan_sha256", "pair_id", "order_index",
            "condition", "status", "failure_code", "identity", "client", "result", "raw",
        },
        name,
    )
    if sample["schema"] != core.SAMPLE_SCHEMA or sample["experiment_id"] != plan["experiment_id"] or \
            sample["plan_sha256"] != core.plan_digest(plan) or \
            _strict_int(sample["pair_id"], f"{name}.pair_id", 1) != entry["pair_id"] or \
            _strict_int(sample["order_index"], f"{name}.order_index") != entry["order_index"] or \
            sample["condition"] != entry["condition"] or \
            sample["status"] != "success" or sample["failure_code"] is not None:
        _fail(f"{name} differs from the successful frozen schedule entry")
    condition = entry["condition"]
    expected_identity = {
        "source_commit": plan["conditions"][condition]["source_commit"],
        "coordinator_binary_sha256": plan["conditions"][condition]["coordinator_binary"]["sha256"],
        "worker_binary_sha256": plan["conditions"][condition]["worker_binary"]["sha256"],
        "model_sha256": plan["model"]["sha256"],
        "request_sha256": plan["request"]["sha256"],
        "commands_sha256": _sha(core.canonical_bytes(core.condition_commands(plan, condition))),
    }
    if not _same(sample["identity"], expected_identity) or not _same(sample["result"], measured_result) or \
            not _same(sample["client"], measured_client):
        _fail(f"{name} summaries or artifact identity differ from measured evidence")
    raw = _keys(
        sample["raw"],
        {"response", "client", "extra_0", "extra_1", "extra_2", "extra_3", "extra_4"},
        f"{name}.raw",
    )
    expected_copies = {
        "response": ("response.json", f"execution/entry-{schedule_index:03d}/measured/measurement-0-response.json"),
        "client": ("client.json", f"execution/entry-{schedule_index:03d}/measured/measurement-0-client.json"),
        "extra_0": ("extra-00-execution.json", f"execution/entry-{schedule_index:03d}/execution.json"),
        "extra_1": ("extra-01-policy.raw", f"execution/entry-{schedule_index:03d}/policy.raw"),
        "extra_2": ("extra-02-measurement-0-response.raw", f"execution/entry-{schedule_index:03d}/measured/measurement-0-response.raw"),
        "extra_3": ("extra-03-telemetry.json", f"execution/entry-{schedule_index:03d}/measured/telemetry.json"),
        "extra_4": ("extra-04-gpu-admission.json", f"execution/entry-{schedule_index:03d}/measured/gpu-admission.json"),
    }
    for key, (filename, source_name) in expected_copies.items():
        copy_name = f"{directory}/{filename}"
        _validate_binding(raw[key], filename, files[copy_name], f"{name}.raw.{key}")
        if files[copy_name] != files[source_name]:
            _fail(f"{copy_name} differs from its immutable execution source")
    if files[f"{directory}/extra-00-execution.json"] != receipt_bytes or \
            files[f"{directory}/extra-01-policy.raw"] != policy_bytes:
        _fail(f"{directory} receipt/policy extra copies differ from selected captured bytes")
    response = _mapping(_parse_json(files[f"{directory}/response.json"], f"{directory}/response.json"),
                        f"{directory}/response.json")
    client = _mapping(_parse_json(files[f"{directory}/client.json"], f"{directory}/client.json"),
                      f"{directory}/client.json")
    if not _same(_validate_response(response, plan, f"{directory}/response.json"), measured_result) or \
            not _same(client, measured_client):
        _fail(f"{directory} reparsed raw response/client differ from sample summaries")
    return sample, _sha(files[name])


def _expected_analysis(
    plan: dict[str, Any], preflight_hashes: dict[str, str], samples: list[dict[str, Any]],
) -> dict[str, Any]:
    report: dict[str, Any] = {
        "schema": core.ANALYSIS_SCHEMA_V2 if plan["schema"] == core.PLAN_SCHEMA_V2 else core.ANALYSIS_SCHEMA_V1,
        "experiment_id": plan["experiment_id"],
        "plan_sha256": core.plan_digest(plan),
        "lane": plan["runtime"]["lane"],
        "cache_class": plan["runtime"]["cache_class"],
        "preflight_sha256": preflight_hashes,
        "scheduled_samples": len(samples),
        "retained_samples": len(samples),
        "missing": [],
        "failures": [],
        "evidence_core_complete": True,
        "execution_qualified": False,
        "measurement_ready": False,
        "minimum_five_pairs_met": plan["execution"]["pairs"] >= 5,
        "performance_claim": False,
        "metrics": {},
    }
    if plan["schema"] == core.PLAN_SCHEMA_V2:
        report["comparison"] = {
            "kind": plan["comparison"]["kind"],
            "control": plan["comparison"]["control"],
            "candidate": plan["comparison"]["candidate"],
            "batch_by_condition": dict(plan["runtime"]["batch_by_condition"]),
            "ubatch": plan["runtime"]["ubatch"],
            "condition_commands_sha256": {
                condition: _sha(core.canonical_bytes(core.condition_commands(plan, condition)))
                for condition in ("off", "on")
            },
        }
    by_pair: dict[int, dict[str, list[dict[str, Any]]]] = {}
    content_hashes: set[str] = set()
    for sample in samples:
        by_pair.setdefault(sample["pair_id"], {"off": [], "on": []})[sample["condition"]].append(sample)
        content_hashes.add(sample["result"]["content_sha256"])
    if len(by_pair) != plan["execution"]["pairs"] or len(content_hashes) != 1:
        _fail("complete samples do not provide every pair with deterministic content parity")
    expected_per_cell = plan["execution"]["retained_per_condition_per_pair"]
    for pair_id in range(1, plan["execution"]["pairs"] + 1):
        if pair_id not in by_pair or any(
                len(by_pair[pair_id][condition]) != expected_per_cell for condition in ("off", "on")):
            _fail(f"pair {pair_id} does not contain the exact OFF/ON retained sample count")
    fields = {
        "prompt_tokens_per_second": (lambda sample: sample["result"]["prompt_tokens_per_second"], False),
        "generation_tokens_per_second": (lambda sample: sample["result"]["generation_tokens_per_second"], False),
        "client_wall_ms": (lambda sample: float(sample["client"]["wall_ms"]), True),
        "ttft_ms": (lambda sample: float(sample["client"]["ttft_ms"]), True),
        "mean_itl_ms": (
            lambda sample: core.mean([float(value) for value in sample["client"]["itl_ms"]]), True),
    }
    for metric, (extract, lower_is_better) in fields.items():
        values = [
            {
                condition: core.mean([extract(sample) for sample in by_pair[pair_id][condition]])
                for condition in ("off", "on")
            }
            for pair_id in sorted(by_pair)
        ]
        report["metrics"][metric] = core.metric_summary(values, metric, lower_is_better)
    report["content_sha256"] = next(iter(content_hashes))
    return report


def _verify_captured_impl(
    *, files: dict[str, bytes], directories: tuple[str, ...],
    expected_plan_bytes: bytes, expected_policy_bytes: bytes,
    expected_incident_bytes: bytes, expected_schedule_index: int,
    expected_production_identity_sha256: dict[str, str],
) -> dict[str, Any]:
    for value, where in (
        (expected_plan_bytes, "expected_plan_bytes"),
        (expected_policy_bytes, "expected_policy_bytes"),
        (expected_incident_bytes, "expected_incident_bytes"),
    ):
        if not isinstance(value, bytes) or not value or len(value) > _MAX_FILE_BYTES:
            _fail(f"{where} must be non-empty bounded bytes")
    selected_index = _strict_int(
        expected_schedule_index, "expected_schedule_index", 0, _MAX_SCHEDULE_ENTRIES - 1)
    identities = _keys(
        expected_production_identity_sha256,
        {"coordinator", "worker"},
        "expected_production_identity_sha256",
    )
    expected_identities = {
        role: _hash(identities[role], f"expected_production_identity_sha256.{role}")
        for role in ("coordinator", "worker")
    }
    if files.get("plan.json") != expected_plan_bytes:
        _fail("plan.json differs byte-for-byte from expected_plan_bytes")
    if files.get("incident.raw") != expected_incident_bytes:
        _fail("incident.raw differs byte-for-byte from expected_incident_bytes")
    if _sha(expected_incident_bytes) != adapter.ISSUE41_MANIFEST_SHA256:
        _fail("expected incident bytes do not match the pinned PR44 issue #41 manifest")

    plan_value = _mapping(_parse_json(expected_plan_bytes, "expected_plan_bytes"), "plan")
    try:
        plan = core.validate_plan(plan_value)
    except core.PlanError as exc:
        raise AdapterEvidenceError(f"plan fails core validation: {exc}") from exc
    pairs = _strict_int(
        plan["execution"]["pairs"], "plan.execution.pairs", 1,
        _MAX_SCHEDULE_ENTRIES // 2)
    warmups = _strict_int(
        plan["execution"]["warmups_per_condition"],
        "plan.execution.warmups_per_condition", 1, _MAX_WARMUPS_PER_ENTRY)
    retained = _strict_int(
        plan["execution"]["retained_per_condition_per_pair"],
        "plan.execution.retained_per_condition_per_pair", 1, 1)
    schedule_entries = pairs * 2 * retained
    if schedule_entries > _MAX_SCHEDULE_ENTRIES:
        _fail("plan schedule exceeds the closed entry bound")
    schedule = _mapping(_parse_json(files["schedule.json"], "schedule.json"), "schedule.json")
    expected_schedule = core.make_schedule(plan)
    if not _same(schedule, expected_schedule):
        _fail("schedule.json differs from the deterministic plan schedule")
    entries = _list(schedule.get("entries"), "schedule.json.entries")
    if not entries or len(entries) > _MAX_SCHEDULE_ENTRIES:
        _fail("schedule entry count is empty or exceeds the closed bound")
    if selected_index >= len(entries):
        _fail("expected_schedule_index is outside the frozen schedule")
    commands = _mapping(_parse_json(files["commands.json"], "commands.json"), "commands.json")
    if not _same(commands, core.commands_document(plan)):
        _fail("commands.json differs from the deterministic plan commands")
    try:
        policy = adapter.load_policy_bytes(expected_policy_bytes, plan)
    except adapter.AdapterError as exc:
        raise AdapterEvidenceError(f"expected policy fails adapter validation: {exc}") from exc

    sampling_profile = _sampling_profile_present(files, directories)
    expected_files, expected_directories = _expected_inventory(
        schedule, plan, sampling_profile=sampling_profile)
    expected_count = _expected_inventory_entry_count(
        len(entries), plan["execution"]["warmups_per_condition"], sampling_profile)
    if len(expected_files) + len(expected_directories) != expected_count or \
            expected_count > _MAX_FILES:
        _fail("plan expands beyond the bounded exact evidence inventory")
    _expect_inventory(files, expected_files)
    if set(directories) != expected_directories:
        _fail(
            "evidence tree directory inventory is not closed "
            f"(missing={sorted(expected_directories - set(directories))}, "
            f"extra={sorted(set(directories) - expected_directories)})")

    incident = _keys(
        _parse_json(files["incident.raw"], "incident.raw"),
        {
            "schema", "incident_id", "classification", "benchmark_valid",
            "performance_result", "repository_base", "candidate_commit", "tracker",
            "timezone", "kernel_window", "production_before", "production_recovered",
            "interrupted_build", "safety_invariants", "evidence",
        },
        "incident.raw",
    )
    if incident["schema"] != "halofpx.target-safety-incident.v1" or \
            incident["incident_id"] != "2026-08-12-nimo-2-hmm-global-oom" or \
            incident["classification"] != "production-safety-incident" or \
            incident["benchmark_valid"] is not False or \
            incident["performance_result"] is not None or \
            incident["tracker"] != adapter.ISSUE41_TRACKER or \
            incident["timezone"] != "America/Los_Angeles":
        _fail("incident.raw does not retain the issue #41 non-benchmark incident identity")
    for field in ("repository_base", "candidate_commit"):
        if not isinstance(incident[field], str) or re.fullmatch(r"[0-9a-f]{40}", incident[field]) is None:
            _fail(f"incident.raw.{field} is not a commit identity")
    for field in (
        "kernel_window", "production_before", "production_recovered", "interrupted_build"):
        _mapping(incident[field], f"incident.raw.{field}")
    evidence_rows = _list(incident["evidence"], "incident.raw.evidence")
    if not evidence_rows:
        _fail("incident.raw.evidence is empty")
    invariants = incident["safety_invariants"]
    if not isinstance(invariants, list) or any(not isinstance(item, str) for item in invariants) or \
            not adapter.ISSUE41_REQUIRED_INVARIANTS.issubset(invariants):
        _fail("incident.raw lacks the issue #41 required safety invariants")

    authority_bytes, hmm_digests, hmm_domain = _validate_machine_authorities(
        files, plan, policy, expected_identities)
    try:
        core.validate_completion_request(files["inputs/request.raw"], plan)
    except core.PlanError as exc:
        raise AdapterEvidenceError(f"inputs/request.raw fails core validation: {exc}") from exc
    if _sha(files["inputs/request.raw"]) != plan["request"]["sha256"]:
        _fail("inputs/request.raw differs from the frozen request digest")
    for role in ("coordinator", "worker"):
        if _sha(authority_bytes[role]) != plan["topology"][role]["authority_receipt"]["sha256"]:
            _fail(f"inputs/authority-{role}.raw differs from the frozen plan digest")
    preflight_hashes = _validate_preflights(files, plan, authority_bytes)

    samples: list[dict[str, Any]] = []
    sample_summaries: list[dict[str, Any]] = []
    receipt_bytes_by_index: list[bytes] = []
    hosts = {
        plan["topology"]["coordinator"]["host"],
        plan["topology"]["worker"]["host"],
    }
    invocation_seen: set[str] = set()
    process_seen: dict[str, set[tuple[int, int]]] = {host: set() for host in hosts}
    last_start: dict[str, int] = {host: 0 for host in hosts}
    prior_entry_client_end: dt.datetime | None = None
    prior_remote_end: int | None = None
    prior_controller_end: int | None = None
    prior_role_cleanup: dict[str, int] = {"coordinator": 0, "worker": 0}
    deterministic_content_sha256: str | None = None
    for index, entry in enumerate(entries):
        (
            receipt, result, client, receipt_bytes, entry_wall_window,
            entry_remote_windows, entry_controller_windows, entry_lifecycle,
            entry_content_sha256,
        ) = _validate_execution_entry(
            files=files, schedule_index=index, entry=entry, plan=plan, policy=policy,
            policy_bytes=expected_policy_bytes, preflight_hashes=preflight_hashes,
            hmm_domain=hmm_domain)
        intent_time, first_client_start, last_client_end = entry_wall_window
        if intent_time > first_client_start:
            _fail(f"execution entry {index} intent follows its first client request")
        if prior_entry_client_end is not None and intent_time <= prior_entry_client_end:
            _fail(f"execution entry {index} intent overlaps or replays its predecessor")
        prior_entry_client_end = last_client_end
        if prior_remote_end is not None and entry_remote_windows[0][0] <= prior_remote_end:
            _fail(f"execution entry {index} remote monotonic interval overlaps or replays")
        if prior_controller_end is not None and \
                entry_controller_windows[0][0] <= prior_controller_end:
            _fail(f"execution entry {index} controller monotonic interval overlaps or replays")
        prior_remote_end = entry_remote_windows[-1][1]
        prior_controller_end = entry_controller_windows[-1][1]
        for role in ("coordinator", "worker"):
            if entry_lifecycle[role][0] <= prior_role_cleanup[role]:
                _fail(f"execution entry {index} {role} process starts before predecessor cleanup")
            prior_role_cleanup[role] = entry_lifecycle[role][1]
        if deterministic_content_sha256 is None:
            deterministic_content_sha256 = entry_content_sha256
        elif entry_content_sha256 != deterministic_content_sha256:
            _fail("warmup/measurement deterministic content differs across schedule entries")
        for cycle in receipt["cycles"]:
            for role in ("coordinator", "worker"):
                identity = cycle["identities"][role]
                host = identity["host"]
                invocation = identity["invocation_id"]
                process_key = (identity["pid"], identity["process_start_ticks"])
                if invocation in invocation_seen or process_key in process_seen[host] or \
                        identity["start_monotonic_us"] <= last_start[host]:
                    _fail(f"execution entry {index} reuses or reverses a disposable identity on {host}")
                invocation_seen.add(invocation)
                process_seen[host].add(process_key)
                last_start[host] = identity["start_monotonic_us"]
        sample, sample_sha = _validate_sample(
            files=files, entry=entry, schedule_index=index, plan=plan,
            policy_bytes=expected_policy_bytes, receipt_bytes=receipt_bytes,
            measured_result=result, measured_client=client)
        samples.append(sample)
        sample_summaries.append({
            "schedule_index": index, "pair_id": entry["pair_id"],
            "order_index": entry["order_index"], "condition": entry["condition"],
            "sample_sha256": sample_sha,
        })
        receipt_bytes_by_index.append(receipt_bytes)

    sorted_samples = sorted(samples, key=lambda sample: (sample["pair_id"], sample["order_index"]))
    expected_jsonl = b"".join(
        json.dumps(sample, sort_keys=True, separators=(",", ":")).encode("utf-8") + b"\n"
        for sample in sorted_samples)
    if files["samples.jsonl"] != expected_jsonl:
        _fail("samples.jsonl is not the exact canonical complete-sample sequence")
    # Parse each retained line independently as well, so a coincidental byte
    # comparison cannot mask duplicate-key or non-finite JSON.
    parsed_lines = [
        _parse_json(line, f"samples.jsonl line {index + 1}")
        for index, line in enumerate(files["samples.jsonl"].splitlines()) if line
    ]
    if not _same(parsed_lines, sorted_samples):
        _fail("samples.jsonl parsed identities differ from raw sample records")

    analysis = _mapping(_parse_json(files["analysis.json"], "analysis.json"), "analysis.json")
    expected_analysis = _expected_analysis(plan, preflight_hashes, samples)
    if not _same(analysis, expected_analysis):
        _fail("analysis.json is not the exact complete core analysis")
    status = _keys(
        _parse_json(files["status.json"], "status.json"),
        {
            "schema", "experiment_id", "state", "execution_qualified",
            "measurement_ready", "performance_claim",
        },
        "status.json",
    )
    if not _same(status, {
        "schema": "halofpx.strix-ab-status.v1",
        "experiment_id": plan["experiment_id"],
        "state": "evidence-core-complete",
        "execution_qualified": False,
        "measurement_ready": False,
        "performance_claim": False,
    }):
        _fail("status.json is not the exact complete non-promotional status")
    expected_ledger = _canonical_ledger(files)
    if files["SHA256SUMS"] != expected_ledger:
        _fail("SHA256SUMS is not the canonical exact inventory/digest ledger")
    if sampling_profile:
        _validate_sampling_profile(files, directories, plan, policy, hmm_domain)

    selected_receipt = receipt_bytes_by_index[selected_index]
    selected_sample = sample_summaries[selected_index]
    return {
        "schema": VERIFY_SCHEMA,
        "experiment_id": plan["experiment_id"],
        "plan_sha256": core.plan_digest(plan),
        "policy_sha256": _sha(expected_policy_bytes),
        "incident_sha256": _sha(expected_incident_bytes),
        "hmm_admission_snapshot_sha256": hmm_digests["snapshot"],
        "hmm_admission_policy_sha256": hmm_digests["policy"],
        "hmm_admission_result_sha256": hmm_digests["result"],
        "hmm_boot_ids": dict(sorted(hmm_domain.boot_ids.items())),
        "hmm_measured_cycle_boot_bound": True,
        "hmm_warmup_cycle_boot_bound": True,
        "hmm_planned_increment_bytes": dict(hmm_domain.planned_increment_bytes),
        "hmm_planned_increment_bound_to_adapter": False,
        "workload_allocation_authority": False,
        "schedule_sha256": _sha(files["schedule.json"]),
        "samples_jsonl_sha256": _sha(files["samples.jsonl"]),
        "observability_profile": (
            SAMPLING_SYNC_CONTRACT if sampling_profile else None),
        "schedule_entries": len(entries),
        "retained_samples": len(samples),
        "selected_schedule_index": selected_index,
        "selected_entry": dict(entries[selected_index]),
        "selected_receipt_sha256": _sha(selected_receipt),
        "receipt_sha256": _sha(selected_receipt),
        "selected_sample_sha256": selected_sample["sample_sha256"],
        "sample_identities": tuple(sample_summaries),
        "tree_sha256": _tree_digest(files),
        "target_execution_authority": False,
        "performance_result": False,
        "selected_receipt_bytes": selected_receipt,
        "files": tuple(sorted(files)),
        "file_sha256": {name: _sha(files[name]) for name in sorted(files)},
        "directories": directories,
    }


def verify_captured_adapter_evidence_tree(
    files: dict[str, bytes], directories: tuple[str, ...], *,
    expected_plan_bytes: bytes, expected_policy_bytes: bytes,
    expected_incident_bytes: bytes, expected_schedule_index: int,
    expected_production_identity_sha256: dict[str, str],
) -> dict[str, Any]:
    """Semantically verify bytes from ``capture_closed_regular_tree``.

    This avoids reopening paths when a containing custody verifier already
    captured the adapter subtree. It remains success-only and non-promotional.
    """
    try:
        if not isinstance(files, dict) or any(
                not isinstance(name, str) or not isinstance(content, bytes)
                for name, content in files.items()) or \
                not isinstance(directories, tuple) or any(
                    not isinstance(relative, str) for relative in directories):
            _fail("captured adapter evidence has invalid container types")
        if len(files) + len(directories) > _MAX_FILES or \
                any(len(content) > _MAX_FILE_BYTES for content in files.values()) or \
                sum(len(content) for content in files.values()) > _MAX_TREE_BYTES:
            _fail("captured adapter evidence exceeds bounded count/byte limits")
        all_names = [*files, *directories]
        if len(all_names) != len(set(all_names)) or \
                len({name.casefold() for name in all_names}) != len(all_names):
            _fail("captured adapter evidence has duplicate/case-colliding paths")
        directory_set = set(directories)
        for relative in all_names:
            pure = PurePosixPath(relative)
            if not relative or pure.as_posix() != relative or pure.is_absolute() or \
                    any(part in {"", ".", ".."} for part in pure.parts) or \
                    len(pure.parts) > _MAX_DEPTH:
                _fail(f"captured adapter evidence has unsafe path: {relative!r}")
            for part in pure.parts:
                _validate_name(part, relative)
            for depth in range(1, len(pure.parts)):
                parent = PurePosixPath(*pure.parts[:depth]).as_posix()
                if parent not in directory_set:
                    _fail(f"captured adapter evidence omits parent directory: {parent}")
        if set(files) & directory_set:
            _fail("captured adapter evidence aliases a file and directory")
        return _verify_captured_impl(
            files=files, directories=directories,
            expected_plan_bytes=expected_plan_bytes,
            expected_policy_bytes=expected_policy_bytes,
            expected_incident_bytes=expected_incident_bytes,
            expected_schedule_index=expected_schedule_index,
            expected_production_identity_sha256=expected_production_identity_sha256,
        )
    except AdapterEvidenceError:
        raise
    except (
        core.PlanError, adapter.AdapterError, KeyError, IndexError, TypeError,
        OSError, ValueError, RecursionError, MemoryError,
    ) as exc:
        raise AdapterEvidenceError(f"captured adapter evidence validation failed closed: {exc}") from exc


def _verify_impl(
    root: Path, *, expected_plan_bytes: bytes, expected_policy_bytes: bytes,
    expected_incident_bytes: bytes, expected_schedule_index: int,
    expected_production_identity_sha256: dict[str, str],
) -> dict[str, Any]:
    files, directories = capture_closed_regular_tree(root)
    return verify_captured_adapter_evidence_tree(
        files, directories,
        expected_plan_bytes=expected_plan_bytes,
        expected_policy_bytes=expected_policy_bytes,
        expected_incident_bytes=expected_incident_bytes,
        expected_schedule_index=expected_schedule_index,
        expected_production_identity_sha256=expected_production_identity_sha256,
    )


def verify_adapter_evidence_tree(
    root: Path, *, expected_plan_bytes: bytes, expected_policy_bytes: bytes,
    expected_incident_bytes: bytes, expected_schedule_index: int,
    expected_production_identity_sha256: dict[str, str],
) -> dict[str, Any]:
    """Verify and capture one complete, successful PR51 adapter evidence tree.

    Only a complete successful schedule is supported; partial and failure
    custody belong to the controller's independent recovery record.  Returned
    ``selected_receipt_bytes`` and normalized inventories come from
    the immutable two-pass capture and let a caller compose outer custody
    without reopening any attacker-controlled path.  The result is not a
    target authorization or a performance result.
    """
    try:
        return _verify_impl(
            root,
            expected_plan_bytes=expected_plan_bytes,
            expected_policy_bytes=expected_policy_bytes,
            expected_incident_bytes=expected_incident_bytes,
            expected_schedule_index=expected_schedule_index,
            expected_production_identity_sha256=expected_production_identity_sha256,
        )
    except AdapterEvidenceError:
        raise
    except (
        core.PlanError, adapter.AdapterError, KeyError, IndexError, TypeError,
        OSError, ValueError, RecursionError, MemoryError,
    ) as exc:
        raise AdapterEvidenceError(f"adapter evidence validation failed closed: {exc}") from exc
