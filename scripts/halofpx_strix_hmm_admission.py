#!/usr/bin/env python3
"""Offline-only issue-#41 Strix HMM admission snapshot validator.

This module consumes already-collected JSON bytes.  It contains no SSH,
privilege escalation, service control, subprocess, or target Runner path.
Supplying a trusted timestamp is mandatory; the local system clock is never
used as admission authority.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import importlib.util
import json
import os
import re
import stat
import sys
from pathlib import Path
from types import MappingProxyType
from typing import Any, Iterable


def _load_exact_sibling(name: str, filename: str) -> Any:
    path = Path(__file__).resolve().with_name(filename)
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise ImportError(f"cannot load exact sibling module {path}")
    module = importlib.util.module_from_spec(spec)
    previous = sys.modules.get(name)
    sys.modules[name] = module
    try:
        spec.loader.exec_module(module)
    finally:
        if previous is None:
            sys.modules.pop(name, None)
        else:
            sys.modules[name] = previous
    return module


production_identity_contract = _load_exact_sibling(
    "halofpx_strix_production_identity", "halofpx_strix_production_identity.py")


SNAPSHOT_SCHEMA = "halofpx.strix-hmm-admission-snapshot.v1"
POLICY_SCHEMA = "halofpx.strix-hmm-admission-policy.v1"
RESULT_SCHEMA = "halofpx.strix-hmm-admission-result.v1"
ISSUE_NUMBER = 41
TARGET_EXECUTION_AUTHORITY = False
MAX_INPUT_BYTES = 1024 * 1024
MAX_U64 = (1 << 64) - 1

ROLES = ("coordinator", "worker")
ROLE_HOSTS = {"coordinator": "nimo-1", "worker": "nimo-2"}
ROLE_UNITS = {
    "coordinator": "minimax-m27-q6-server.service",
    "worker": "minimax-m27-rpc-worker.service",
}
ROLE_PORTS = {"coordinator": 8081, "worker": 50052}
COLLECTION_STATES = frozenset({"complete", "unreadable", "refused"})
SOURCE_KINDS = frozenset({
    "synthetic-offline-fixture",
    "retained-incident-evidence",
    "future-elevated-collector",
})
CLASSIFICATIONS = frozenset({"ADMIT", "REFUSE"})

# A source name is not evidence of exact accounting.  Every accepted name is
# reviewed in code and bound to the capture domain whose collector semantics it
# describes.  The retained incident profile is listed only so historical input
# can produce a typed refusal.  No live elevated source is admitted in v1 until
# its exact per-owner interface is implemented and reviewed.
# value: (capture source kind, exact per-owner bytes, admissible)
HMM_ACCOUNTING_SOURCE_PROFILES = MappingProxyType({
    "synthetic-exact-hmm-accounting-v1": (
        "synthetic-offline-fixture", True, True),
    "retained-incident-aggregate-gpuactive-v1": (
        "retained-incident-evidence", False, False),
    "future-elevated-collector-unqualified-v1": (
        "future-elevated-collector", False, False),
})

REASON_CODES = frozenset({
    "CAPTURE_BEFORE_POLICY_WINDOW",
    "CAPTURE_CLOCK_ORDER_INVALID",
    "CAPTURE_ERROR",
    "CAPTURE_NOT_CLOSED_WORLD",
    "CAPTURE_NOT_ELEVATED",
    "CAPTURE_SOURCE_MISMATCH",
    "CGROUP_MEMBERSHIP_MISMATCH",
    "COLLECTION_REFUSED",
    "COLLECTION_UNREADABLE",
    "DEVICE_CENSUS_ERROR",
    "DEVICE_SET_MISMATCH",
    "FOREIGN_DEVICE_OWNER",
    "HMM_ACCOUNTING_RECONCILIATION_FAILED",
    "HMM_ACCOUNTING_SOURCE_NOT_ADMISSIBLE",
    "HMM_ACCOUNTING_SOURCE_MISMATCH",
    "HMM_CAPACITY_EXCEEDED",
    "HMM_HEADROOM_INSUFFICIENT",
    "KERNEL_BASELINE_NOT_CLEAN",
    "KERNEL_BASELINE_COVERAGE_INVALID",
    "KERNEL_BOOT_ID_MISMATCH",
    "LISTENER_OWNERSHIP_MISMATCH",
    "NODE_ERROR",
    "PHYSICAL_CAPACITY_MISMATCH",
    "REQUIRED_DEVICE_OWNER_MISSING",
    "RETAINED_INCIDENT_NON_ADMISSIBLE",
    "SERVICE_IDENTITY_MISMATCH",
    "SNAPSHOT_AFTER_POLICY_WINDOW",
    "SNAPSHOT_ERROR",
    "SNAPSHOT_STALE",
    "TRUSTED_TIME_BEFORE_CAPTURE",
    "TRUSTED_TIME_OUTSIDE_POLICY_WINDOW",
})

SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
INVOCATION_RE = re.compile(r"^[0-9a-f]{32}$")
BOOT_ID_RE = re.compile(
    r"^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$")
CAPTURE_ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{7,127}$")
ERROR_CODE_RE = re.compile(r"^[A-Z][A-Z0-9_]{2,63}$")
RENDER_RE = re.compile(r"^/dev/dri/renderD[0-9]+$")
UNIT_RE = re.compile(r"^[A-Za-z0-9_.@-]+\.service$")


class AdmissionError(RuntimeError):
    """An input is structurally invalid and cannot produce authority."""


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, sort_keys=True, separators=(",", ":"), ensure_ascii=False,
        allow_nan=False,
    ).encode("utf-8")


def pretty_bytes(value: Any) -> bytes:
    return json.dumps(
        value, sort_keys=True, indent=2, ensure_ascii=False, allow_nan=False,
    ).encode("utf-8") + b"\n"


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def digest_value(value: Any) -> str:
    return sha256_bytes(canonical_bytes(value))


def unique_json_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_json_constant(value: str) -> None:
    raise ValueError(f"non-finite JSON number: {value}")


def parse_closed_json(content: bytes, where: str) -> dict[str, Any]:
    if len(content) > MAX_INPUT_BYTES:
        raise AdmissionError(f"{where} exceeds {MAX_INPUT_BYTES} bytes")
    try:
        value = json.loads(
            content.decode("utf-8", errors="strict"),
            object_pairs_hook=unique_json_object,
            parse_constant=reject_json_constant,
        )
    except (UnicodeError, json.JSONDecodeError, ValueError, RecursionError) as exc:
        raise AdmissionError(f"{where} is unreadable: {exc}") from exc
    if not isinstance(value, dict):
        raise AdmissionError(f"{where} must be a JSON object")
    return value


def require_exact(value: Any, keys: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != keys:
        raise AdmissionError(f"{where} has the wrong closed field set")
    return value


def require_string(value: Any, where: str) -> str:
    if not isinstance(value, str) or not value or \
            any(ord(character) < 0x20 or 0x7f <= ord(character) <= 0x9f
                for character in value):
        raise AdmissionError(f"{where} must be a nonempty string without control characters")
    try:
        value.encode("utf-8", errors="strict")
    except UnicodeError as exc:
        raise AdmissionError(f"{where} must contain Unicode scalar values") from exc
    return value


def require_bool(value: Any, where: str) -> bool:
    if type(value) is not bool:
        raise AdmissionError(f"{where} must be a boolean")
    return value


def require_int(
    value: Any, where: str, minimum: int = 0, maximum: int = MAX_U64,
) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or \
            not minimum <= value <= maximum:
        raise AdmissionError(f"{where} must be an integer in [{minimum}, {maximum}]")
    return value


def checked_add(left: int, right: int, where: str) -> int:
    if left > MAX_U64 - right:
        raise AdmissionError(f"{where} overflows unsigned 64-bit accounting")
    return left + right


def checked_sum(values: Iterable[int], where: str) -> int:
    result = 0
    for value in values:
        result = checked_add(result, value, where)
    return result


def require_hash(value: Any, where: str) -> str:
    text = require_string(value, where)
    if SHA256_RE.fullmatch(text) is None:
        raise AdmissionError(f"{where} must be a lowercase SHA-256 digest")
    return text


def require_sorted_unique_strings(
    value: Any, where: str, *, allow_empty: bool = True,
) -> list[str]:
    if not isinstance(value, list) or any(not isinstance(item, str) or not item for item in value):
        raise AdmissionError(f"{where} must be a list of nonempty strings")
    for index, item in enumerate(value):
        require_string(item, f"{where}[{index}]")
    if not allow_empty and not value:
        raise AdmissionError(f"{where} may not be empty")
    if value != sorted(set(value)):
        raise AdmissionError(f"{where} must be sorted and duplicate-free")
    return value


def require_error_list(value: Any, where: str) -> list[dict[str, str]]:
    if not isinstance(value, list):
        raise AdmissionError(f"{where} must be a list")
    observed: list[tuple[str, str]] = []
    for index, item in enumerate(value):
        row = require_exact(item, {"code", "detail"}, f"{where}[{index}]")
        code = require_string(row["code"], f"{where}[{index}].code")
        detail = require_string(row["detail"], f"{where}[{index}].detail")
        if ERROR_CODE_RE.fullmatch(code) is None:
            raise AdmissionError(f"{where}[{index}].code is not canonical")
        observed.append((code, detail))
    if observed != sorted(set(observed)):
        raise AdmissionError(f"{where} must be sorted and duplicate-free")
    return value


def parse_utc(value: Any, where: str) -> dt.datetime:
    text = require_string(value, where)
    if not text.endswith("Z"):
        raise AdmissionError(f"{where} must be canonical whole-second UTC")
    try:
        parsed = dt.datetime.fromisoformat(text[:-1] + "+00:00")
    except ValueError as exc:
        raise AdmissionError(f"{where} is not a UTC timestamp") from exc
    if parsed.tzinfo != dt.timezone.utc or parsed.microsecond != 0 or \
            parsed.strftime("%Y-%m-%dT%H:%M:%SZ") != text:
        raise AdmissionError(f"{where} must be canonical whole-second UTC")
    return parsed


def read_regular_bytes(path: Path, where: str) -> bytes:
    absolute = path.absolute()
    for component in reversed((absolute, *absolute.parents)):
        try:
            if component.is_symlink() or \
                    (hasattr(component, "is_junction") and component.is_junction()):
                raise AdmissionError(
                    f"{where} has a symbolic-link or junction path component: {component}")
        except OSError as exc:
            raise AdmissionError(
                f"{where} path component cannot be inspected: {component}: {exc}") from exc
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0) | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
    except OSError as exc:
        raise AdmissionError(f"{where} cannot be opened safely: {exc}") from exc
    try:
        before = os.fstat(descriptor)
        if not stat.S_ISREG(before.st_mode):
            raise AdmissionError(f"{where} must be a regular non-symlink file")
        if before.st_size > MAX_INPUT_BYTES:
            raise AdmissionError(f"{where} exceeds {MAX_INPUT_BYTES} bytes")
        with os.fdopen(descriptor, "rb", closefd=False) as stream:
            content = stream.read(MAX_INPUT_BYTES + 1)
        after = os.fstat(descriptor)
        if len(content) > MAX_INPUT_BYTES:
            raise AdmissionError(f"{where} exceeds {MAX_INPUT_BYTES} bytes")
        if (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) != \
                (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns) or \
                len(content) != after.st_size:
            raise AdmissionError(f"{where} changed while it was read")
        return content
    finally:
        os.close(descriptor)


def kib_to_bytes(value: Any, where: str) -> int:
    """Convert retained Linux ``kB``/KiB accounting with checked arithmetic."""
    kib = require_int(value, where)
    if kib > MAX_U64 // 1024:
        raise AdmissionError(f"{where} cannot be multiplied by 1024 safely")
    return kib * 1024


def extract_meminfo_kib_as_bytes(content: bytes, field: str) -> int:
    """Extract one exact Linux meminfo-style ``<field>: <n> kB`` record."""
    if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*", field):
        raise AdmissionError("meminfo field name is malformed")
    try:
        text = content.decode("utf-8", errors="strict")
    except UnicodeError as exc:
        raise AdmissionError("meminfo evidence is not UTF-8") from exc
    matches = re.findall(
        rf"(?m)^{re.escape(field)}:[ \t]+([0-9]+)[ \t]+kB[ \t]*$", text)
    if len(matches) != 1:
        raise AdmissionError(f"meminfo evidence must contain exactly one {field} row")
    return kib_to_bytes(int(matches[0]), f"{field} KiB")


def extract_kernel_gpu_active_kib_as_bytes(content: bytes) -> int:
    """Extract one consistent ``gpu_active:<n>kB`` value from an OOM journal."""
    try:
        text = content.decode("utf-8", errors="strict")
    except UnicodeError as exc:
        raise AdmissionError("kernel evidence is not UTF-8") from exc
    matches = re.findall(r"\bgpu_active:([0-9]+)kB\b", text)
    if not matches or len(set(matches)) != 1:
        raise AdmissionError(
            "kernel evidence must contain one consistent gpu_active KiB value")
    return kib_to_bytes(int(matches[0]), "kernel gpu_active KiB")


def production_identity(
    role: str, host: str, service: dict[str, Any], process: dict[str, Any],
    listener: dict[str, Any],
) -> dict[str, Any]:
    return {
        "role": role,
        "host": host,
        "unit": service["unit"],
        "pid": service["main_pid"],
        "invocation_id": service["invocation_id"],
        "nrestarts": service["nrestarts"],
        "process_start_ticks": process["start_ticks"],
        "start_monotonic_us": service["start_monotonic_us"],
        "executable_sha256": process["executable_sha256"],
        "argv_sha256": process["argv_sha256"],
        "control_group": service["control_group"],
        "listener_port": listener["port"],
        "listener_pid": listener["owner_pids"][0],
        "health_sha256": listener["health_sha256"],
    }


def device_owner_identity(host: str, owner: dict[str, Any]) -> dict[str, Any]:
    return {
        "host": host,
        "pid": owner["pid"],
        "process_start_ticks": owner["process_start_ticks"],
        "unit": owner["unit"],
        "control_group": owner["control_group"],
        "executable_sha256": owner["executable_sha256"],
        "argv_sha256": owner["argv_sha256"],
        "device_paths": owner["device_paths"],
    }


def _parse_service(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "unit", "active_state", "sub_state", "invocation_id", "main_pid",
        "nrestarts", "start_monotonic_us", "control_group",
    }, where)
    require_string(row["unit"], f"{where}.unit")
    if UNIT_RE.fullmatch(row["unit"]) is None:
        raise AdmissionError(f"{where}.unit is malformed")
    require_string(row["active_state"], f"{where}.active_state")
    require_string(row["sub_state"], f"{where}.sub_state")
    invocation = require_string(row["invocation_id"], f"{where}.invocation_id")
    if INVOCATION_RE.fullmatch(invocation) is None:
        raise AdmissionError(f"{where}.invocation_id is malformed")
    require_int(row["main_pid"], f"{where}.main_pid", 1)
    require_int(row["nrestarts"], f"{where}.nrestarts")
    require_int(
        row["start_monotonic_us"], f"{where}.start_monotonic_us", 1,
        MAX_U64 // 1000,
    )
    cgroup = require_string(row["control_group"], f"{where}.control_group")
    if not cgroup.startswith("/") or ".." in cgroup.split("/"):
        raise AdmissionError(f"{where}.control_group is not canonical")
    return row


def _parse_process(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "pid", "start_ticks", "executable_sha256", "argv_sha256", "cgroup",
        "cgroup_member_pids",
    }, where)
    require_int(row["pid"], f"{where}.pid", 1)
    require_int(row["start_ticks"], f"{where}.start_ticks", 1)
    require_hash(row["executable_sha256"], f"{where}.executable_sha256")
    require_hash(row["argv_sha256"], f"{where}.argv_sha256")
    require_string(row["cgroup"], f"{where}.cgroup")
    members = row["cgroup_member_pids"]
    if not isinstance(members, list):
        raise AdmissionError(f"{where}.cgroup_member_pids must be a list")
    for index, pid in enumerate(members):
        require_int(pid, f"{where}.cgroup_member_pids[{index}]", 1)
    if members != sorted(set(members)):
        raise AdmissionError(f"{where}.cgroup_member_pids must be sorted and duplicate-free")
    return row


def _parse_listener(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {"protocol", "port", "owner_pids", "health_sha256"}, where)
    if row["protocol"] != "tcp":
        raise AdmissionError(f"{where}.protocol must be tcp")
    require_int(row["port"], f"{where}.port", 1, 65535)
    owners = row["owner_pids"]
    if not isinstance(owners, list):
        raise AdmissionError(f"{where}.owner_pids must be a list")
    for index, pid in enumerate(owners):
        require_int(pid, f"{where}.owner_pids[{index}]", 1)
    if owners != sorted(set(owners)):
        raise AdmissionError(f"{where}.owner_pids must be sorted and duplicate-free")
    health = row["health_sha256"]
    if health is not None:
        require_hash(health, f"{where}.health_sha256")
    return row


def _parse_device_census(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "devices", "owners", "hmm_accounting_source", "hmm_aggregate_bytes",
        "errors",
    }, where)
    devices = require_sorted_unique_strings(row["devices"], f"{where}.devices", allow_empty=False)
    if "/dev/kfd" not in devices or not any(RENDER_RE.fullmatch(path) for path in devices) or \
            any(path != "/dev/kfd" and RENDER_RE.fullmatch(path) is None for path in devices):
        raise AdmissionError(f"{where}.devices must include /dev/kfd and a render node")
    source = require_string(row["hmm_accounting_source"], f"{where}.hmm_accounting_source")
    if source not in HMM_ACCOUNTING_SOURCE_PROFILES:
        raise AdmissionError(
            f"{where}.hmm_accounting_source is not in the reviewed v1 registry")
    require_int(row["hmm_aggregate_bytes"], f"{where}.hmm_aggregate_bytes")
    require_error_list(row["errors"], f"{where}.errors")
    if not isinstance(row["owners"], list):
        raise AdmissionError(f"{where}.owners must be a list")
    pids: list[int] = []
    for index, value_owner in enumerate(row["owners"]):
        owner_where = f"{where}.owners[{index}]"
        owner = require_exact(value_owner, {
            "pid", "process_start_ticks", "unit", "control_group",
            "executable_sha256", "argv_sha256", "device_paths",
            "hmm_allocated_bytes",
        }, owner_where)
        pid = require_int(owner["pid"], f"{owner_where}.pid", 1)
        pids.append(pid)
        require_int(owner["process_start_ticks"], f"{owner_where}.process_start_ticks", 1)
        unit = require_string(owner["unit"], f"{owner_where}.unit")
        if UNIT_RE.fullmatch(unit) is None:
            raise AdmissionError(f"{owner_where}.unit is malformed")
        cgroup = require_string(owner["control_group"], f"{owner_where}.control_group")
        if not cgroup.startswith("/") or ".." in cgroup.split("/") or \
                not cgroup.endswith("/" + unit):
            raise AdmissionError(f"{owner_where}.control_group is not the owner unit cgroup")
        require_hash(owner["executable_sha256"], f"{owner_where}.executable_sha256")
        require_hash(owner["argv_sha256"], f"{owner_where}.argv_sha256")
        paths = require_sorted_unique_strings(
            owner["device_paths"], f"{owner_where}.device_paths", allow_empty=False)
        if not set(paths).issubset(devices):
            raise AdmissionError(f"{owner_where}.device_paths are outside the closed census")
        require_int(owner["hmm_allocated_bytes"], f"{owner_where}.hmm_allocated_bytes")
    if pids != sorted(set(pids)):
        raise AdmissionError(f"{where}.owners must be PID-sorted and duplicate-free")
    return row


def _parse_memory(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "physical_capacity_bytes", "gpu_active_bytes", "gpu_reclaim_bytes",
    }, where)
    require_int(row["physical_capacity_bytes"], f"{where}.physical_capacity_bytes", 1)
    require_int(row["gpu_active_bytes"], f"{where}.gpu_active_bytes")
    require_int(row["gpu_reclaim_bytes"], f"{where}.gpu_reclaim_bytes")
    return row


def _parse_capture_clock(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "boot_id", "started_monotonic_ns", "completed_monotonic_ns",
    }, where)
    boot_id = require_string(row["boot_id"], f"{where}.boot_id")
    if BOOT_ID_RE.fullmatch(boot_id) is None:
        raise AdmissionError(f"{where}.boot_id is malformed")
    started = require_int(
        row["started_monotonic_ns"], f"{where}.started_monotonic_ns", 1)
    completed = require_int(
        row["completed_monotonic_ns"], f"{where}.completed_monotonic_ns", 1)
    if completed <= started:
        raise AdmissionError(f"{where} interval is empty or runs backward")
    return row


KERNEL_COUNTERS = (
    "global_oom_count", "oom_kill_count", "amdgpu_fault_count",
    "kfd_fault_count", "gpu_reset_count",
)


def _parse_kernel(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "boot_id", "window_start_monotonic_ns", "observed_monotonic_ns",
        "journal_cursor_start", "journal_cursor_end", *KERNEL_COUNTERS,
        "errors",
    }, where)
    boot_id = require_string(row["boot_id"], f"{where}.boot_id")
    if BOOT_ID_RE.fullmatch(boot_id) is None:
        raise AdmissionError(f"{where}.boot_id is malformed")
    start = require_int(row["window_start_monotonic_ns"], f"{where}.window_start_monotonic_ns", 1)
    observed = require_int(row["observed_monotonic_ns"], f"{where}.observed_monotonic_ns", 1)
    if observed <= start:
        raise AdmissionError(f"{where} monotonic window is empty or runs backward")
    cursor_start = require_string(row["journal_cursor_start"], f"{where}.journal_cursor_start")
    cursor_end = require_string(row["journal_cursor_end"], f"{where}.journal_cursor_end")
    if cursor_end == cursor_start:
        raise AdmissionError(f"{where} journal cursor window is empty")
    for name in KERNEL_COUNTERS:
        require_int(row[name], f"{where}.{name}")
    require_error_list(row["errors"], f"{where}.errors")
    return row


def _parse_observation(value: Any, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "capture_clock", "service", "process", "listener", "device_census",
        "memory", "kernel",
    }, where)
    _parse_capture_clock(row["capture_clock"], f"{where}.capture_clock")
    _parse_service(row["service"], f"{where}.service")
    _parse_process(row["process"], f"{where}.process")
    _parse_listener(row["listener"], f"{where}.listener")
    _parse_device_census(row["device_census"], f"{where}.device_census")
    _parse_memory(row["memory"], f"{where}.memory")
    _parse_kernel(row["kernel"], f"{where}.kernel")
    return row


def parse_snapshot_bytes(content: bytes) -> dict[str, Any]:
    root = parse_closed_json(content, "HMM admission snapshot")
    require_exact(root, {"schema", "issue", "capture", "roles", "errors"}, "snapshot")
    if root["schema"] != SNAPSHOT_SCHEMA or root["issue"] != ISSUE_NUMBER:
        raise AdmissionError("snapshot schema/issue differs from v1/#41")
    capture = require_exact(root["capture"], {
        "capture_id", "source_kind", "started_utc", "completed_utc",
        "elevated", "closed_world", "errors",
    }, "snapshot.capture")
    capture_id = require_string(capture["capture_id"], "snapshot.capture.capture_id")
    if CAPTURE_ID_RE.fullmatch(capture_id) is None:
        raise AdmissionError("snapshot.capture.capture_id is malformed")
    source_kind = require_string(capture["source_kind"], "snapshot.capture.source_kind")
    if source_kind not in SOURCE_KINDS:
        raise AdmissionError("snapshot.capture.source_kind is unsupported")
    parse_utc(capture["started_utc"], "snapshot.capture.started_utc")
    parse_utc(capture["completed_utc"], "snapshot.capture.completed_utc")
    require_bool(capture["elevated"], "snapshot.capture.elevated")
    require_bool(capture["closed_world"], "snapshot.capture.closed_world")
    require_error_list(capture["errors"], "snapshot.capture.errors")
    require_error_list(root["errors"], "snapshot.errors")
    roles = require_exact(root["roles"], set(ROLES), "snapshot.roles")
    for role in ROLES:
        where = f"snapshot.roles.{role}"
        node = require_exact(roles[role], {
            "role", "host", "collection_state", "observation", "errors",
        }, where)
        if node["role"] != role or node["host"] != ROLE_HOSTS[role]:
            raise AdmissionError(f"{where} differs from the fixed two-role topology")
        state = require_string(node["collection_state"], f"{where}.collection_state")
        if state not in COLLECTION_STATES:
            raise AdmissionError(f"{where}.collection_state is unsupported")
        errors = require_error_list(node["errors"], f"{where}.errors")
        if state == "complete":
            if errors:
                raise AdmissionError(f"{where} complete state may not contain errors")
            observation = _parse_observation(
                node["observation"], f"{where}.observation")
            accounting_source = observation["device_census"]["hmm_accounting_source"]
            if HMM_ACCOUNTING_SOURCE_PROFILES[accounting_source][0] != source_kind:
                raise AdmissionError(
                    f"{where} HMM accounting profile is outside the capture source domain")
        elif node["observation"] is not None or not errors:
            raise AdmissionError(f"{where} {state} state requires null observation and errors")
    return root


def _parse_expected_identity(value: Any, role: str, where: str) -> dict[str, Any]:
    row = require_exact(
        value, set(production_identity_contract.PRODUCTION_IDENTITY_FIELDS), where)
    if row["role"] != role or row["host"] != ROLE_HOSTS[role] or \
            row["unit"] != ROLE_UNITS[role]:
        raise AdmissionError(f"{where} differs from the fixed protected topology")
    require_int(row["pid"], f"{where}.pid", 1)
    invocation = require_string(row["invocation_id"], f"{where}.invocation_id")
    if INVOCATION_RE.fullmatch(invocation) is None:
        raise AdmissionError(f"{where}.invocation_id is malformed")
    require_int(row["nrestarts"], f"{where}.nrestarts")
    require_int(row["process_start_ticks"], f"{where}.process_start_ticks", 1)
    require_int(
        row["start_monotonic_us"], f"{where}.start_monotonic_us", 1,
        MAX_U64 // 1000,
    )
    require_hash(row["executable_sha256"], f"{where}.executable_sha256")
    require_hash(row["argv_sha256"], f"{where}.argv_sha256")
    cgroup = require_string(row["control_group"], f"{where}.control_group")
    if not cgroup.startswith("/") or ".." in cgroup.split("/") or \
            not cgroup.endswith("/" + row["unit"]):
        raise AdmissionError(f"{where}.control_group is not the protected unit cgroup")
    port = require_int(row["listener_port"], f"{where}.listener_port", 1, 65535)
    listener_pid = require_int(row["listener_pid"], f"{where}.listener_pid", 1)
    if port != ROLE_PORTS[role] or listener_pid != row["pid"]:
        raise AdmissionError(f"{where} listener identity differs from the protected PID/port")
    health = row["health_sha256"]
    if role == "coordinator":
        require_hash(health, f"{where}.health_sha256")
    elif health is not None:
        raise AdmissionError(f"{where}.health_sha256 must be null for worker")
    return row


def parse_policy_bytes(content: bytes) -> dict[str, Any]:
    root = parse_closed_json(content, "HMM admission policy")
    require_exact(root, {
        "schema", "issue", "validity", "capture_requirements", "roles", "errors",
    }, "policy")
    if root["schema"] != POLICY_SCHEMA or root["issue"] != ISSUE_NUMBER:
        raise AdmissionError("policy schema/issue differs from v1/#41")
    if require_error_list(root["errors"], "policy.errors"):
        raise AdmissionError("policy contains unresolved errors")
    validity = require_exact(root["validity"], {
        "not_before_utc", "expires_utc",
    }, "policy.validity")
    not_before = parse_utc(validity["not_before_utc"], "policy.validity.not_before_utc")
    expires = parse_utc(validity["expires_utc"], "policy.validity.expires_utc")
    if expires <= not_before:
        raise AdmissionError("policy validity window is empty or reversed")
    requirements = require_exact(root["capture_requirements"], {
        "source_kind", "elevated", "closed_world", "max_snapshot_age_seconds",
        "require_zero_kernel_counts",
    }, "policy.capture_requirements")
    source_kind = require_string(
        requirements["source_kind"], "policy.capture_requirements.source_kind")
    if source_kind not in SOURCE_KINDS:
        raise AdmissionError("policy capture source kind is unsupported")
    if requirements["elevated"] is not True or requirements["closed_world"] is not True or \
            requirements["require_zero_kernel_counts"] is not True:
        raise AdmissionError("v1 policy must require elevated closed-world zero-kernel-event capture")
    require_int(requirements["max_snapshot_age_seconds"],
                "policy.capture_requirements.max_snapshot_age_seconds", 1)
    roles = require_exact(root["roles"], set(ROLES), "policy.roles")
    for role in ROLES:
        where = f"policy.roles.{role}"
        row = require_exact(roles[role], {
            "role", "host", "expected_boot_id", "expected_identity", "device_paths",
            "allowed_device_owner_identity_sha256s",
            "required_device_owner_identity_sha256s", "hmm_accounting_source",
            "capacity",
        }, where)
        if row["role"] != role or row["host"] != ROLE_HOSTS[role]:
            raise AdmissionError(f"{where} differs from the fixed two-role topology")
        expected_boot_id = require_string(
            row["expected_boot_id"], f"{where}.expected_boot_id")
        if BOOT_ID_RE.fullmatch(expected_boot_id) is None:
            raise AdmissionError(f"{where}.expected_boot_id is malformed")
        _parse_expected_identity(row["expected_identity"], role, f"{where}.expected_identity")
        devices = require_sorted_unique_strings(
            row["device_paths"], f"{where}.device_paths", allow_empty=False)
        if "/dev/kfd" not in devices or not any(RENDER_RE.fullmatch(path) for path in devices) or \
                any(path != "/dev/kfd" and RENDER_RE.fullmatch(path) is None for path in devices):
            raise AdmissionError(f"{where}.device_paths lacks KFD/render authority")
        allowed = require_sorted_unique_strings(
            row["allowed_device_owner_identity_sha256s"],
            f"{where}.allowed_device_owner_identity_sha256s", allow_empty=False)
        required = require_sorted_unique_strings(
            row["required_device_owner_identity_sha256s"],
            f"{where}.required_device_owner_identity_sha256s", allow_empty=False)
        for index, digest in enumerate((*allowed, *required)):
            require_hash(digest, f"{where}.owner_digest[{index}]")
        if not set(required).issubset(allowed):
            raise AdmissionError(f"{where} required device owners are not allowed")
        expected = row["expected_identity"]
        expected_owner = {
            "host": expected["host"],
            "pid": expected["pid"],
            "process_start_ticks": expected["process_start_ticks"],
            "unit": expected["unit"],
            "control_group": expected["control_group"],
            "executable_sha256": expected["executable_sha256"],
            "argv_sha256": expected["argv_sha256"],
            "device_paths": devices,
        }
        if digest_value(expected_owner) not in required:
            raise AdmissionError(
                f"{where} does not require the protected production device owner")
        source = require_string(row["hmm_accounting_source"], f"{where}.hmm_accounting_source")
        if source not in HMM_ACCOUNTING_SOURCE_PROFILES:
            raise AdmissionError(
                f"{where}.hmm_accounting_source is not in the reviewed v1 registry")
        if HMM_ACCOUNTING_SOURCE_PROFILES[source][0] != source_kind:
            raise AdmissionError(
                f"{where}.hmm_accounting_source is outside the policy capture domain")
        capacity = require_exact(row["capacity"], {
            "physical_capacity_bytes", "admitted_hmm_capacity_bytes",
            "planned_increment_bytes", "required_reserve_bytes",
        }, f"{where}.capacity")
        physical = require_int(
            capacity["physical_capacity_bytes"], f"{where}.capacity.physical_capacity_bytes", 1)
        admitted = require_int(
            capacity["admitted_hmm_capacity_bytes"],
            f"{where}.capacity.admitted_hmm_capacity_bytes", 1)
        planned = require_int(
            capacity["planned_increment_bytes"], f"{where}.capacity.planned_increment_bytes")
        reserve = require_int(
            capacity["required_reserve_bytes"],
            f"{where}.capacity.required_reserve_bytes", 1)
        if admitted > physical:
            raise AdmissionError(f"{where} admitted HMM capacity exceeds physical capacity")
        if checked_add(planned, reserve, f"{where} planned increment plus reserve") > admitted:
            raise AdmissionError(f"{where} plan and reserve exceed admitted HMM capacity")
    return root


def _global_reasons(
    snapshot: dict[str, Any], policy: dict[str, Any], trusted_now: dt.datetime,
) -> set[str]:
    reasons: set[str] = set()
    capture = snapshot["capture"]
    requirements = policy["capture_requirements"]
    started = parse_utc(capture["started_utc"], "snapshot.capture.started_utc")
    completed = parse_utc(capture["completed_utc"], "snapshot.capture.completed_utc")
    not_before = parse_utc(policy["validity"]["not_before_utc"], "policy.validity.not_before_utc")
    expires = parse_utc(policy["validity"]["expires_utc"], "policy.validity.expires_utc")
    if started >= completed:
        reasons.add("CAPTURE_CLOCK_ORDER_INVALID")
    if capture["source_kind"] != requirements["source_kind"]:
        reasons.add("CAPTURE_SOURCE_MISMATCH")
    if capture["source_kind"] == "retained-incident-evidence" or \
            requirements["source_kind"] == "retained-incident-evidence":
        reasons.add("RETAINED_INCIDENT_NON_ADMISSIBLE")
    if capture["elevated"] is not True:
        reasons.add("CAPTURE_NOT_ELEVATED")
    if capture["closed_world"] is not True:
        reasons.add("CAPTURE_NOT_CLOSED_WORLD")
    if capture["errors"]:
        reasons.add("CAPTURE_ERROR")
    if snapshot["errors"]:
        reasons.add("SNAPSHOT_ERROR")
    if started < not_before:
        reasons.add("CAPTURE_BEFORE_POLICY_WINDOW")
    if completed >= expires:
        reasons.add("SNAPSHOT_AFTER_POLICY_WINDOW")
    if trusted_now < completed:
        reasons.add("TRUSTED_TIME_BEFORE_CAPTURE")
    if trusted_now < not_before or trusted_now >= expires:
        reasons.add("TRUSTED_TIME_OUTSIDE_POLICY_WINDOW")
    age_seconds = (trusted_now - completed).total_seconds()
    if age_seconds > requirements["max_snapshot_age_seconds"]:
        reasons.add("SNAPSHOT_STALE")
    return reasons


def _role_result(
    role: str, snapshot_node: dict[str, Any], policy_role: dict[str, Any],
) -> dict[str, Any]:
    reasons: set[str] = set()
    node_digest = digest_value(snapshot_node)
    state = snapshot_node["collection_state"]
    if state != "complete":
        reasons.add("COLLECTION_UNREADABLE" if state == "unreadable" else "COLLECTION_REFUSED")
        if snapshot_node["errors"]:
            reasons.add("NODE_ERROR")
        return {
            "role": role,
            "host": ROLE_HOSTS[role],
            "node_snapshot_sha256": node_digest,
            "production_identity_sha256": None,
            "classification": "REFUSE",
            "hmm_headroom_bytes": None,
            "reason_codes": sorted(reasons),
        }

    observation = snapshot_node["observation"]
    capture_clock = observation["capture_clock"]
    service = observation["service"]
    process = observation["process"]
    listener = observation["listener"]
    census = observation["device_census"]
    memory = observation["memory"]
    kernel = observation["kernel"]
    observed_identity = None
    identity_digest = None
    if len(listener["owner_pids"]) == 1:
        observed_identity = production_identity(
            role, snapshot_node["host"], service, process, listener)
        identity_digest = production_identity_contract.production_identity_digest(
            observed_identity)

    expected_identity = policy_role["expected_identity"]
    if observed_identity != expected_identity or \
            service["active_state"] != "active" or service["sub_state"] != "running" or \
            service["main_pid"] != process["pid"] or \
            service["control_group"] != process["cgroup"]:
        reasons.add("SERVICE_IDENTITY_MISMATCH")
    if process["cgroup_member_pids"] != [process["pid"]]:
        reasons.add("CGROUP_MEMBERSHIP_MISMATCH")
    if listener["owner_pids"] != [process["pid"]]:
        reasons.add("LISTENER_OWNERSHIP_MISMATCH")
    if (role == "coordinator" and listener["health_sha256"] is None) or \
            (role == "worker" and listener["health_sha256"] is not None):
        reasons.add("SERVICE_IDENTITY_MISMATCH")
    if census["devices"] != policy_role["device_paths"]:
        reasons.add("DEVICE_SET_MISMATCH")
    if census["errors"]:
        reasons.add("DEVICE_CENSUS_ERROR")
    if census["hmm_accounting_source"] != policy_role["hmm_accounting_source"]:
        reasons.add("HMM_ACCOUNTING_SOURCE_MISMATCH")
    accounting_profile = HMM_ACCOUNTING_SOURCE_PROFILES[
        census["hmm_accounting_source"]]
    if not accounting_profile[1] or not accounting_profile[2]:
        reasons.add("HMM_ACCOUNTING_SOURCE_NOT_ADMISSIBLE")

    try:
        owner_bytes = checked_sum(
            (owner["hmm_allocated_bytes"] for owner in census["owners"]),
            f"{role} per-owner HMM sum")
    except AdmissionError:
        owner_bytes = None
        reasons.add("HMM_ACCOUNTING_RECONCILIATION_FAILED")
    aggregate = census["hmm_aggregate_bytes"]
    if owner_bytes is None or owner_bytes != aggregate or \
            memory["gpu_active_bytes"] != aggregate:
        reasons.add("HMM_ACCOUNTING_RECONCILIATION_FAILED")
    observed_owner_digests = {
        digest_value(device_owner_identity(snapshot_node["host"], owner))
        for owner in census["owners"]
    }
    allowed = set(policy_role["allowed_device_owner_identity_sha256s"])
    required = set(policy_role["required_device_owner_identity_sha256s"])
    if not observed_owner_digests.issubset(allowed):
        reasons.add("FOREIGN_DEVICE_OWNER")
    if not required.issubset(observed_owner_digests):
        reasons.add("REQUIRED_DEVICE_OWNER_MISSING")

    capacity = policy_role["capacity"]
    if memory["physical_capacity_bytes"] != capacity["physical_capacity_bytes"]:
        reasons.add("PHYSICAL_CAPACITY_MISMATCH")
    admitted = capacity["admitted_hmm_capacity_bytes"]
    headroom: int | None
    if aggregate > admitted:
        reasons.add("HMM_CAPACITY_EXCEEDED")
        headroom = None
    else:
        raw_headroom = admitted - aggregate
        planned = capacity["planned_increment_bytes"]
        if planned > raw_headroom:
            reasons.add("HMM_CAPACITY_EXCEEDED")
            headroom = None
        else:
            # The result reports residual admitted HMM headroom after the exact
            # planned increment.  Required reserve is a separate lower bound.
            headroom = raw_headroom - planned
            if headroom < capacity["required_reserve_bytes"]:
                reasons.add("HMM_HEADROOM_INSUFFICIENT")
    expected_boot_id = policy_role["expected_boot_id"]
    if capture_clock["boot_id"] != expected_boot_id or \
            kernel["boot_id"] != expected_boot_id:
        reasons.add("KERNEL_BOOT_ID_MISMATCH")
    service_start_ns = service["start_monotonic_us"] * 1000
    if not (
        kernel["window_start_monotonic_ns"] <= service_start_ns <=
        capture_clock["started_monotonic_ns"] <
        capture_clock["completed_monotonic_ns"] <=
        kernel["observed_monotonic_ns"]
    ):
        reasons.add("KERNEL_BASELINE_COVERAGE_INVALID")
    if kernel["errors"] or any(kernel[name] != 0 for name in KERNEL_COUNTERS):
        reasons.add("KERNEL_BASELINE_NOT_CLEAN")
    if snapshot_node["errors"]:
        reasons.add("NODE_ERROR")

    return {
        "role": role,
        "host": ROLE_HOSTS[role],
        "node_snapshot_sha256": node_digest,
        "production_identity_sha256": identity_digest,
        "classification": "REFUSE" if reasons else "ADMIT",
        "hmm_headroom_bytes": headroom,
        "reason_codes": sorted(reasons),
    }


def evaluate_bytes(
    snapshot_content: bytes, policy_content: bytes, *, trusted_now_utc: str,
) -> dict[str, Any]:
    """Evaluate exact offline bytes with a mandatory caller-supplied timestamp."""
    trusted_now = parse_utc(trusted_now_utc, "trusted_now_utc")
    snapshot = parse_snapshot_bytes(snapshot_content)
    policy = parse_policy_bytes(policy_content)
    global_reasons = _global_reasons(snapshot, policy, trusted_now)
    roles = {
        role: _role_result(role, snapshot["roles"][role], policy["roles"][role])
        for role in ROLES
    }
    all_reasons = set(global_reasons)
    for role in ROLES:
        all_reasons.update(roles[role]["reason_codes"])
    decision = "ADMIT" if not all_reasons and all(
        roles[role]["classification"] == "ADMIT" for role in ROLES) else "REFUSE"
    result = {
        "schema": RESULT_SCHEMA,
        "issue": ISSUE_NUMBER,
        "snapshot_sha256": sha256_bytes(snapshot_content),
        "policy_sha256": sha256_bytes(policy_content),
        "trusted_now_utc": trusted_now_utc,
        "roles": roles,
        "decision": decision,
        "reason_codes": sorted(all_reasons),
        "target_execution_authority": TARGET_EXECUTION_AUTHORITY,
        "performance_result": False,
    }
    _validate_admission_result_bytes(pretty_bytes(result), allow_admit=True)
    return result


def _validate_reason_codes(value: Any, where: str) -> list[str]:
    reasons = require_sorted_unique_strings(value, where)
    unknown = set(reasons) - REASON_CODES
    if unknown:
        raise AdmissionError(f"{where} contains unsupported reason codes: {sorted(unknown)}")
    return reasons


def _validate_admission_result_bytes(
    content: bytes, *, allow_admit: bool,
) -> dict[str, Any]:
    root = parse_closed_json(content, "HMM admission result")
    require_exact(root, {
        "schema", "issue", "snapshot_sha256", "policy_sha256", "trusted_now_utc",
        "roles", "decision", "reason_codes", "target_execution_authority",
        "performance_result",
    }, "result")
    if root["schema"] != RESULT_SCHEMA or root["issue"] != ISSUE_NUMBER:
        raise AdmissionError("result schema/issue differs from v1/#41")
    require_hash(root["snapshot_sha256"], "result.snapshot_sha256")
    require_hash(root["policy_sha256"], "result.policy_sha256")
    parse_utc(root["trusted_now_utc"], "result.trusted_now_utc")
    decision = require_string(root["decision"], "result.decision")
    if decision not in CLASSIFICATIONS:
        raise AdmissionError("result.decision is unsupported")
    reasons = _validate_reason_codes(root["reason_codes"], "result.reason_codes")
    if root["target_execution_authority"] is not False or root["performance_result"] is not False:
        raise AdmissionError("result attempts to claim target authority or performance")
    roles = require_exact(root["roles"], set(ROLES), "result.roles")
    role_reasons: set[str] = set()
    for role in ROLES:
        where = f"result.roles.{role}"
        row = require_exact(roles[role], {
            "role", "host", "node_snapshot_sha256", "production_identity_sha256",
            "classification", "hmm_headroom_bytes", "reason_codes",
        }, where)
        if row["role"] != role or row["host"] != ROLE_HOSTS[role]:
            raise AdmissionError(f"{where} differs from the fixed two-role topology")
        classification = require_string(row["classification"], f"{where}.classification")
        if classification not in CLASSIFICATIONS:
            raise AdmissionError(f"{where}.classification is unsupported")
        local_reasons = _validate_reason_codes(row["reason_codes"], f"{where}.reason_codes")
        role_reasons.update(local_reasons)
        if row["node_snapshot_sha256"] is not None:
            require_hash(row["node_snapshot_sha256"], f"{where}.node_snapshot_sha256")
        if row["production_identity_sha256"] is not None:
            require_hash(row["production_identity_sha256"], f"{where}.production_identity_sha256")
        if row["hmm_headroom_bytes"] is not None:
            require_int(row["hmm_headroom_bytes"], f"{where}.hmm_headroom_bytes")
        if classification == "ADMIT":
            if local_reasons or row["node_snapshot_sha256"] is None or \
                    row["production_identity_sha256"] is None or \
                    row["hmm_headroom_bytes"] is None:
                raise AdmissionError(f"{where} ADMIT is incomplete")
        elif not local_reasons:
            raise AdmissionError(f"{where} REFUSE lacks a reason")
    if not role_reasons.issubset(set(reasons)):
        raise AdmissionError("result aggregate reasons omit a role refusal")
    admitted = all(roles[role]["classification"] == "ADMIT" for role in ROLES)
    if decision == "ADMIT":
        if reasons or not admitted:
            raise AdmissionError("result ADMIT is inconsistent")
    elif not reasons or admitted and not reasons:
        raise AdmissionError("result REFUSE lacks a reason")
    if not allow_admit and (
        decision == "ADMIT" or
        any(roles[role]["classification"] == "ADMIT" for role in ROLES)
    ):
        raise AdmissionError(
            "positive admission requires snapshot/policy-bound canonical recomputation")
    return root


def validate_admission_result_bytes(content: bytes) -> dict[str, Any]:
    """Validate only a non-positive result envelope.

    Result bytes alone can be coherently forged.  Therefore this API rejects
    overall or per-role ``ADMIT`` and cannot establish positive admission.
    Positive consumers must retain the snapshot and policy and call
    :func:`validate_bound_admission_result_bytes`.
    """
    return _validate_admission_result_bytes(content, allow_admit=False)


def validate_bound_admission_result_bytes(
    result_content: bytes, snapshot_content: bytes, policy_content: bytes,
) -> dict[str, Any]:
    """Recompute and validate one result against its exact snapshot and policy."""
    result = _validate_admission_result_bytes(result_content, allow_admit=True)
    if sha256_bytes(snapshot_content) != result["snapshot_sha256"] or \
            sha256_bytes(policy_content) != result["policy_sha256"]:
        raise AdmissionError("result snapshot/policy digest binding is invalid")
    expected = evaluate_bytes(
        snapshot_content, policy_content,
        trusted_now_utc=result["trusted_now_utc"],
    )
    if result != expected or result_content != pretty_bytes(expected):
        raise AdmissionError("result differs from canonical recomputation")
    return result


def _write_new(path: Path, content: bytes) -> None:
    try:
        descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    except OSError as exc:
        raise AdmissionError(f"output must be a new file: {exc}") from exc
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
    except BaseException:
        try:
            path.unlink()
        except OSError:
            pass
        raise


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--policy", type=Path, required=True)
    parser.add_argument(
        "--trusted-now-utc", required=True,
        help="trusted canonical whole-second UTC input; there is no system-clock fallback",
    )
    parser.add_argument("--output", type=Path, help="new local result path; stdout if omitted")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        snapshot_content = read_regular_bytes(args.snapshot, "snapshot file")
        policy_content = read_regular_bytes(args.policy, "policy file")
        result = evaluate_bytes(
            snapshot_content, policy_content, trusted_now_utc=args.trusted_now_utc)
        content = pretty_bytes(result)
        if args.output is None:
            sys.stdout.buffer.write(content)
        else:
            _write_new(args.output, content)
        return 0 if result["decision"] == "ADMIT" else 3
    except AdmissionError as exc:
        print(f"REFUSE: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
