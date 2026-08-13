#!/usr/bin/env python3
"""Fail-closed CachyOS process adapter for the frozen HaloFPX Strix A/B run.

The adapter executes only the next schedule entry.  It never stops or restarts
the protected system services, never retries an ambiguous measured request,
and never emits a performance claim.  A domain Runner keeps the lifecycle
offline-testable; SshCachyRunner is the bounded real implementation intended to
run from the coordinator CachyOS node during an authorized maintenance window.
"""

from __future__ import annotations

import argparse
import base64
import dataclasses
import datetime as dt
import hashlib
import importlib.util
import json
import os
import re
import shlex
import signal
import socket
import struct
import subprocess
import sys
import threading
import time
from pathlib import Path, PurePosixPath
from typing import Any, Protocol, Sequence


try:
    import halofpx_strix_ab as core
except ModuleNotFoundError:
    _CORE_PATH = Path(__file__).with_name("halofpx_strix_ab.py")
    _SPEC = importlib.util.spec_from_file_location("halofpx_strix_ab", _CORE_PATH)
    if _SPEC is None or _SPEC.loader is None:
        raise
    core = importlib.util.module_from_spec(_SPEC)
    _SPEC.loader.exec_module(core)


POLICY_SCHEMA = "halofpx.strix-ab-cachyos-policy.v1"
RECEIPT_SCHEMA = "halofpx.strix-ab-cachyos-execution.v1"
INTENT_SCHEMA = "halofpx.strix-ab-cachyos-intent.v1"
ISSUE41_MANIFEST_RELATIVE = Path(
    "docs/halofpx/evidence/2026-08-12-target-hmm-oom-incident/manifest.json")
ISSUE41_MANIFEST_SHA256 = "331634016681b57183aedbea3550f95d86486ce21d1baf8e7e3e3e5c6f35d815"
ISSUE41_TRACKER = "https://github.com/JCFrags/HaloFPX/issues/41"
ISSUE41_REQUIRED_INVARIANTS = {
    "MemAvailable and conventional RSS are insufficient admission predicates while gpu_active or other HMM ownership is high.",
    "Do not build, quantize, or run disposable inference while production or unaccounted KFD/render/HMM owners are active.",
    "A worker restart invalidates coordinator RPC readiness until exact identities and a real minimal inference request pass.",
    "This incident and its recovery probe are not benchmark evidence.",
}
ISSUE41_UNRESOLVED_CUSTODY = (
    "authorized-maintenance-window-receipt",
    "exact-before-state-and-clean-kernel-oom-baseline",
    "complete-empty-kfd-render-hmm-owner-census",
    "reviewed-disposable-process-and-cleanup-custody",
    "two-rank-recovery-and-real-minimal-inference-contract",
)
PROTECTED_UNITS = {
    "coordinator": "minimax-m27-q6-server.service",
    "worker": "minimax-m27-rpc-worker.service",
}
PROTECTED_PORTS = {8081, 50052}
INVOCATION_RE = re.compile(r"^[0-9a-f]{32}$")
UNIT_RE = re.compile(r"^[a-z0-9][a-z0-9_.@-]{2,127}\.service$")
RPC_CMD_HELLO = 14
RPC_PROTOCOL = (4, 0, 1)
RPC_HELLO_RESPONSE = struct.Struct("<BBBB24s")
# Target mutation remains deliberately unavailable in this draft.  Offline
# qualification can exercise execute_next with a domain fake; the real SSH
# runner must not cross this gate until issues #37 and #41 are reviewed.
TARGET_EXECUTION_ENABLED = False


class AdapterError(RuntimeError):
    pass


class CycleExecutionError(AdapterError):
    """Carries the complete failed-cycle record into the durable entry receipt."""

    def __init__(self, record: dict[str, Any]):
        self.record = record
        errors = record.get("errors", [])
        detail = "; ".join(
            f"{item.get('stage', 'unknown')}: {item.get('detail', 'unknown failure')}"
            for item in errors if isinstance(item, dict)
        ) or "cycle execution failed"
        super().__init__(detail)


class GpuAdmissionError(AdapterError):
    def __init__(self, detail: str, record: dict[str, Any]):
        self.record = record
        super().__init__(detail)


class _CycleAbort(Exception):
    pass


def load_issue41_authority(repository_root: Path | None = None) -> dict[str, Any]:
    """Bind this draft to the immutable incident authority without recollection.

    The incident bundle owns its own closed-world validator and optional
    read-only collector.  The adapter deliberately does not copy either one.
    This check only pins the already-validated manifest and carries its safety
    boundary into local ``validate`` output.
    """
    root = repository_root if repository_root is not None else Path(__file__).resolve().parents[1]
    path = root / ISSUE41_MANIFEST_RELATIVE
    content = core.read_regular_bytes(path, "issue #41 incident manifest")
    observed_sha256 = sha256_bytes(content)
    if observed_sha256 != ISSUE41_MANIFEST_SHA256:
        raise AdapterError(
            "issue #41 incident manifest differs from the reviewed immutable authority")
    manifest = core.require_mapping(
        core.parse_json_bytes(content, "issue #41 incident manifest"),
        "issue #41 incident manifest",
    )
    if manifest.get("schema") != "halofpx.target-safety-incident.v1" or \
            manifest.get("incident_id") != "2026-08-12-nimo-2-hmm-global-oom" or \
            manifest.get("classification") != "production-safety-incident" or \
            manifest.get("benchmark_valid") is not False or \
            manifest.get("performance_result") is not None or \
            manifest.get("tracker") != ISSUE41_TRACKER:
        raise AdapterError("issue #41 incident manifest safety identity is malformed")
    invariants = manifest.get("safety_invariants")
    if not isinstance(invariants, list) or any(not isinstance(item, str) for item in invariants) or \
            not ISSUE41_REQUIRED_INVARIANTS.issubset(invariants):
        raise AdapterError("issue #41 incident manifest lacks the required safety invariants")
    return {
        "manifest_path": ISSUE41_MANIFEST_RELATIVE.as_posix(),
        "manifest_sha256": observed_sha256,
        "schema": manifest["schema"],
        "incident_id": manifest["incident_id"],
        "tracker": manifest["tracker"],
        "target_execution_state": "blocked",
        "target_execution_enabled": False,
        "unresolved_custody": list(ISSUE41_UNRESOLVED_CUSTODY),
    }


def canonical_bytes(value: Any) -> bytes:
    return core.canonical_bytes(value)


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def write_json_durable(path: Path, value: Any, *, exclusive: bool = False) -> None:
    data = json.dumps(value, indent=2, sort_keys=True).encode("utf-8") + b"\n"
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    mode = "xb" if exclusive else "wb"
    with path.open(mode) as handle:
        handle.write(data)
        handle.flush()
        os.fsync(handle.fileno())
    if os.name == "posix":
        descriptor = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)


def write_bytes_durable(path: Path, value: bytes, *, exclusive: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    mode = "xb" if exclusive else "wb"
    with path.open(mode) as handle:
        handle.write(value)
        handle.flush()
        os.fsync(handle.fileno())
    if os.name == "posix":
        descriptor = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)


def require_exact_keys(value: Any, keys: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != keys:
        raise AdapterError(f"{where} has the wrong closed field set")
    return value


def require_port(value: Any, where: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not 1024 <= value <= 65535:
        raise AdapterError(f"{where} must be an unprivileged TCP port")
    return value


def require_target_path(value: Any, where: str) -> str:
    path = core.require_string(value, where)
    pure = PurePosixPath(path)
    if not pure.is_absolute() or pure.as_posix() != path or any(
            part in {".", ".."} for part in pure.parts):
        raise AdapterError(f"{where} must be a canonical absolute POSIX path")
    return path


def validate_target_plan_paths(plan: dict[str, Any]) -> None:
    """Require target-visible POSIX paths before real process execution.

    Offline/fake-runner qualification may build its plan on Windows. Real SSH
    execution is narrower and refuses those host-local control paths.
    """
    require_target_path(plan["model"]["path"], "plan.model.path")
    require_target_path(plan["request"]["path"], "plan.request.path")
    for condition in ("off", "on"):
        for name in ("coordinator_binary", "worker_binary"):
            require_target_path(
                plan["conditions"][condition][name]["path"],
                f"plan.conditions.{condition}.{name}.path",
            )
    for role in ("coordinator", "worker"):
        require_target_path(
            plan["topology"][role]["authority_receipt"]["path"],
            f"plan.topology.{role}.authority_receipt.path",
        )


def argv_option(argv: Sequence[str], flag: str) -> str:
    values: list[str] = []
    for index, item in enumerate(argv):
        if item == flag:
            if index + 1 >= len(argv):
                raise AdapterError(f"argv ends after {flag}")
            values.append(argv[index + 1])
        elif item.startswith(flag + "="):
            values.append(item.split("=", 1)[1])
    if len(values) != 1:
        raise AdapterError(f"argv must contain exactly one {flag}")
    return values[0]


@dataclasses.dataclass(frozen=True)
class Policy:
    controller_host: str
    protected: dict[str, dict[str, Any]]
    coordinator_port: int
    worker_port: int
    unit_prefix: str
    runtime_max_seconds: int
    identity_timeout_seconds: int
    readiness_timeout_seconds: int
    request_timeout_seconds: int
    stop_timeout_seconds: int
    telemetry_interval_seconds: float
    require_no_foreign_gpu_clients: bool


@dataclasses.dataclass(frozen=True)
class UnitIdentity:
    role: str
    host: str
    unit: str
    pid: int
    invocation_id: str
    process_start_ticks: int
    start_monotonic_us: int
    cursor_before: str
    argv: tuple[str, ...]
    environment: dict[str, str]
    executable_sha256: str
    port: int
    control_group: str


@dataclasses.dataclass(frozen=True)
class RequestCapture:
    response: dict[str, Any]
    client: dict[str, Any]
    raw_http: bytes
    sent_body_sha256: str
    started_monotonic_ns: int
    ended_monotonic_ns: int


class Runner(Protocol):
    def snapshot_production(self, protected: dict[str, dict[str, Any]]) -> dict[str, Any]: ...
    def artifact(self, host: str, path: str) -> dict[str, Any]: ...
    def gpu_clients(self, host: str) -> dict[str, Any]: ...
    def port_owners(self, host: str, port: int) -> list[int]: ...
    def ensure_unit_absent(self, host: str, unit: str) -> None: ...
    def start_unit(
        self, role: str, host: str, unit: str, argv: Sequence[str], environment: dict[str, str],
        executable_sha256: str, port: int, runtime_max_seconds: int, identity_timeout_seconds: int,
        stop_timeout_seconds: int,
    ) -> UnitIdentity: ...
    def prove_live(self, identity: UnitIdentity, require_listener: bool = True) -> dict[str, Any]: ...
    def wait_ready(self, identity: UnitIdentity, timeout_seconds: int) -> dict[str, Any]: ...
    def request(
        self, host: str, port: int, body: bytes, output_tokens: int, timeout_seconds: int,
    ) -> RequestCapture: ...
    def telemetry(self, host: str) -> dict[str, Any]: ...
    def stop_unit(self, identity: UnitIdentity, timeout_seconds: int) -> tuple[dict[str, Any], bytes]: ...
    def cleanup_unit(
        self, host: str, unit: str, port: int, timeout_seconds: int,
        identity: UnitIdentity | None = None,
    ) -> dict[str, Any]: ...


def load_policy_bytes(content: bytes, plan: dict[str, Any]) -> Policy:
    if plan.get("topology", {}).get("world_size") != 2:
        raise AdapterError(
            "issue #37 adapter currently supports only the frozen dual-node topology; "
            "single-node execution is not implemented and fails closed")
    try:
        core.validate_plan(plan)
    except core.PlanError as exc:
        raise AdapterError(f"adapter plan is invalid: {exc}") from exc
    try:
        raw = json.loads(
            content.decode("utf-8", errors="strict"),
            object_pairs_hook=core.unique_json_object,
            parse_constant=core.reject_json_constant,
        )
    except (UnicodeError, json.JSONDecodeError, ValueError) as exc:
        raise AdapterError(f"policy is unreadable: {exc}") from exc
    require_exact_keys(raw, {
        "schema", "issue", "controller_host", "protected", "disposable", "timeouts",
        "require_no_foreign_gpu_clients",
    }, "policy")
    if raw["schema"] != POLICY_SCHEMA or raw["issue"] != 37:
        raise AdapterError("policy must be the issue #37 v1 policy")
    controller_host = core.require_string(raw["controller_host"], "policy.controller_host")
    protected = require_exact_keys(raw["protected"], {"coordinator", "worker"}, "policy.protected")
    checked_protected: dict[str, dict[str, Any]] = {}
    for role in ("coordinator", "worker"):
        item = require_exact_keys(protected[role], {"host", "unit", "ports", "health_url"}, f"policy.protected.{role}")
        host = core.require_string(item["host"], f"policy.protected.{role}.host")
        unit = core.require_string(item["unit"], f"policy.protected.{role}.unit")
        if unit != PROTECTED_UNITS[role] or not UNIT_RE.fullmatch(unit):
            raise AdapterError("protected production unit authority differs from the bounded allowlist")
        if host != plan["topology"][role]["host"]:
            raise AdapterError("protected host differs from frozen plan topology")
        ports = item["ports"]
        expected_ports = [8081] if role == "coordinator" else [50052]
        if not isinstance(ports, list) or ports != expected_ports or any(
                require_port(port, "protected port") not in PROTECTED_PORTS for port in ports):
            raise AdapterError("protected production port authority differs from the bounded allowlist")
        health = item["health_url"]
        if role == "coordinator":
            if not isinstance(health, str) or health != "http://127.0.0.1:8081/health":
                raise AdapterError("protected coordinator health URL differs from authority")
        elif health is not None:
            raise AdapterError("protected worker health_url must be null")
        checked_protected[role] = {"host": host, "unit": unit, "ports": ports, "health_url": health}
    if controller_host != plan["topology"]["coordinator"]["host"]:
        raise AdapterError("adapter must run from the frozen coordinator host")
    disposable = require_exact_keys(raw["disposable"], {
        "unit_prefix", "coordinator_port", "worker_port", "runtime_max_seconds"}, "policy.disposable")
    prefix = core.require_string(disposable["unit_prefix"], "policy.disposable.unit_prefix")
    if prefix != "halofpx-ab-":
        raise AdapterError("disposable unit prefix must be halofpx-ab-")
    coordinator_port = require_port(disposable["coordinator_port"], "coordinator_port")
    worker_port = require_port(disposable["worker_port"], "worker_port")
    if coordinator_port == worker_port or {coordinator_port, worker_port} & PROTECTED_PORTS:
        raise AdapterError("disposable ports collide with each other or production")
    commands = core.commands_document(plan)["conditions"]
    comparison_kind = (
        plan.get("comparison", {}).get("kind")
        if plan.get("schema") == core.PLAN_SCHEMA_V2
        else "feature_build"
    )
    if comparison_kind == "runtime_n_batch":
        if plan["source"]["off_commit"] != plan["source"]["on_commit"]:
            raise AdapterError("runtime_n_batch OFF and ON source commits differ")
        for role, key in (
                ("coordinator", "coordinator_binary"), ("worker", "worker_binary")):
            if plan["conditions"]["off"][key] != plan["conditions"]["on"][key]:
                raise AdapterError(
                    f"runtime_n_batch OFF and ON {role} binary path/SHA-256 differ")
    elif comparison_kind == "feature_build":
        if all(
            binary_hash(plan, "off", role) == binary_hash(plan, "on", role)
            for role in ("coordinator", "worker")
        ):
            raise AdapterError("OFF and ON execute identical binary hashes")
    else:
        raise AdapterError("adapter received an unknown comparison kind")
    for condition in ("off", "on"):
        expected_batch = (
            plan["runtime"]["batch_by_condition"][condition]
            if plan.get("schema") == core.PLAN_SCHEMA_V2
            else plan["runtime"]["batch"]
        )
        if argv_option(commands[condition]["coordinator"], "--batch-size") != str(expected_batch):
            raise AdapterError("coordinator generated batch differs from the typed plan")
        if int(argv_option(commands[condition]["coordinator"], "--port")) != coordinator_port:
            raise AdapterError("coordinator disposable port differs from frozen argv")
        if int(argv_option(commands[condition]["worker"], "--port")) != worker_port:
            raise AdapterError("worker disposable port differs from frozen argv")
    endpoint = plan["topology"]["rpc_endpoint"]
    try:
        endpoint_host, endpoint_port_text = endpoint.rsplit(":", 1)
        endpoint_port = int(endpoint_port_text)
    except (IndexError, ValueError) as exc:
        raise AdapterError("RPC endpoint has no exact TCP port") from exc
    worker_host = plan["topology"]["worker"]["host"]
    if endpoint_host.lower().split(".", 1)[0] != worker_host.lower().split(".", 1)[0]:
        raise AdapterError("RPC endpoint host differs from the frozen worker")
    if endpoint_port != worker_port:
        raise AdapterError("RPC endpoint differs from disposable worker port")
    runtime_max = disposable["runtime_max_seconds"]
    if isinstance(runtime_max, bool) or not isinstance(runtime_max, int) or not 60 <= runtime_max <= 7200:
        raise AdapterError("runtime_max_seconds is outside [60, 7200]")
    timeouts = require_exact_keys(raw["timeouts"], {
        "identity_seconds", "readiness_seconds", "request_seconds", "stop_seconds",
        "telemetry_interval_seconds"}, "policy.timeouts")
    integer_timeouts = []
    for name in ("identity_seconds", "readiness_seconds", "request_seconds", "stop_seconds"):
        value = timeouts[name]
        if isinstance(value, bool) or not isinstance(value, int) or not 1 <= value <= 3600:
            raise AdapterError(f"policy.timeouts.{name} is outside [1, 3600]")
        integer_timeouts.append(value)
    telemetry_interval = timeouts["telemetry_interval_seconds"]
    if isinstance(telemetry_interval, bool) or not isinstance(telemetry_interval, (int, float)) or not 0.1 <= telemetry_interval <= 30:
        raise AdapterError("telemetry interval is outside [0.1, 30]")
    if raw["require_no_foreign_gpu_clients"] is not True:
        raise AdapterError("issue #37 policy must require exclusive GPU admission")
    return Policy(
        controller_host, checked_protected, coordinator_port, worker_port, prefix, runtime_max,
        *integer_timeouts, float(telemetry_interval), True,
    )


def read_policy(path: Path, plan: dict[str, Any]) -> tuple[bytes, Policy, str]:
    if not path.is_file() or path.is_symlink():
        raise AdapterError("policy must be a regular non-symlink file")
    try:
        content = path.read_bytes()
    except OSError as exc:
        raise AdapterError(f"policy is unreadable: {exc}") from exc
    if not content:
        raise AdapterError("policy is empty")
    return content, load_policy_bytes(content, plan), sha256_bytes(content)


def load_policy(path: Path, plan: dict[str, Any]) -> Policy:
    return read_policy(path, plan)[1]


def policy_digest(path: Path) -> str:
    if not path.is_file() or path.is_symlink():
        raise AdapterError("policy must be a regular non-symlink file")
    return sha256_bytes(path.read_bytes())


def next_schedule_entry(
    root: Path, schedule: dict[str, Any], expected_policy_sha256: str | None = None,
) -> tuple[int, dict[str, Any], Path]:
    execution_root = root / "execution"
    execution_root.mkdir(mode=0o700, exist_ok=True)
    entries = schedule["entries"]
    for index, entry in enumerate(entries):
        directory = execution_root / f"entry-{index:03d}"
        if not directory.exists():
            return index, entry, directory
        intent = directory / "intent.json"
        receipt = directory / "execution.json"
        if not intent.is_file() or not receipt.is_file():
            raise AdapterError(f"schedule entry {index} is consumed but has no complete receipt; never retry it")
        observed = json.loads(intent.read_text(encoding="utf-8"))
        if observed.get("entry") != entry or observed.get("schedule_index") != index:
            raise AdapterError("execution prefix differs from the frozen schedule")
        policy_copy = directory / "policy.raw"
        policy_binding = observed.get("policy_binding")
        if not isinstance(policy_binding, dict) or set(policy_binding) != {"path", "size_bytes", "sha256"} or \
                policy_binding.get("path") != "policy.raw" or not policy_copy.is_file() or policy_copy.is_symlink() or \
                policy_copy.stat().st_size != policy_binding.get("size_bytes") or \
                core.digest_file(policy_copy) != policy_binding.get("sha256"):
            raise AdapterError("completed execution policy bytes differ from its intent")
        execution = json.loads(receipt.read_text(encoding="utf-8"))
        plan = core.load_plan(root / "plan.json")
        if execution.get("schema") != RECEIPT_SCHEMA or execution.get("issue") != 37 or \
                execution.get("experiment_id") != plan["experiment_id"] or \
                execution.get("plan_sha256") != core.plan_digest(plan) or \
                execution.get("schedule_index") != index or execution.get("entry") != entry:
            raise AdapterError("completed execution receipt differs from the frozen run")
        if execution.get("policy_binding") != policy_binding:
            raise AdapterError("completed execution receipt differs from retained policy bytes")
        if expected_policy_sha256 is not None and execution.get("policy_sha256") != expected_policy_sha256:
            raise AdapterError("completed execution receipt used a different adapter policy")
        _, current_bindings = input_bindings(root, plan)
        if execution.get("input_bindings") != current_bindings:
            raise AdapterError("completed execution receipt input bytes differ from the current run")
        outcome = execution.get("outcome")
        if not isinstance(outcome, dict) or outcome.get("status") not in {"success", "failure"}:
            raise AdapterError("completed execution receipt outcome is malformed")
        sample_path = root / "raw" / f"pair-{entry['pair_id']:03d}-order-{entry['order_index']}-{entry['condition']}" / "sample.json"
        if not sample_path.is_file():
            raise AdapterError("completed execution has no evidence-core sample")
        sample = core.require_mapping(core.read_json(sample_path), "completed sample")
        if sample.get("pair_id") != entry["pair_id"] or sample.get("order_index") != entry["order_index"] or \
                sample.get("condition") != entry["condition"] or sample.get("status") != outcome["status"]:
            raise AdapterError("completed evidence-core sample differs from execution outcome")
        core.validate_raw_evidence(sample_path, sample, plan)
        raw = core.require_mapping(sample.get("raw"), "completed sample.raw")
        extra = core.require_mapping(raw.get("extra_0"), "completed execution extra")
        retained_receipt = sample_path.parent / core.require_string(
            extra.get("path"), "completed execution extra.path")
        if not retained_receipt.is_file() or core.digest_file(retained_receipt) != core.digest_file(receipt) or \
                retained_receipt.read_bytes() != receipt.read_bytes():
            raise AdapterError("execution receipt differs from the evidence-core retained copy")
    raise AdapterError("frozen schedule is complete")


def input_bindings(root: Path, plan: dict[str, Any]) -> tuple[bytes, dict[str, Any]]:
    core.validate_preflights(root, plan)
    records: dict[str, Any] = {}
    request = b""
    for role, name in (("coordinator", "authority_receipt"), ("worker", "authority_receipt"), ("coordinator", "request")):
        path = core.retained_input_path(root, role, name)
        if not path.is_file() or path.is_symlink():
            raise AdapterError(f"retained {role}/{name} is missing or not a regular file")
        content = path.read_bytes()
        records[f"{role}_{name}"] = {
            "path": path.relative_to(root).as_posix(), "size_bytes": len(content), "sha256": sha256_bytes(content)}
        if name == "request":
            request = content
    core.validate_completion_request(request, plan)
    return request, records


def unit_name(policy: Policy, plan: dict[str, Any], schedule_index: int, kind: str, ordinal: int, condition: str, role: str) -> str:
    experiment = plan["experiment_id"][:32]
    value = f"{policy.unit_prefix}{experiment}-e{schedule_index:03d}-{kind}{ordinal}-{condition}-{role}.service"
    if not UNIT_RE.fullmatch(value) or value in PROTECTED_UNITS.values():
        raise AdapterError("generated disposable unit is unsafe")
    return value


def binary_hash(plan: dict[str, Any], condition: str, role: str) -> str:
    key = "coordinator_binary" if role == "coordinator" else "worker_binary"
    return plan["conditions"][condition][key]["sha256"]


def assert_identity(identity: UnitIdentity, expected: dict[str, Any], protected_pids: set[int]) -> None:
    if identity.role != expected["role"] or identity.host != expected["host"] or identity.unit != expected["unit"]:
        raise AdapterError("live unit role/host/name differs from intent")
    if identity.pid <= 0 or identity.pid in protected_pids:
        raise AdapterError("disposable PID is zero or collides with production")
    if not INVOCATION_RE.fullmatch(identity.invocation_id):
        raise AdapterError("disposable InvocationID is malformed")
    if identity.process_start_ticks <= 0 or identity.start_monotonic_us <= 0 or not identity.cursor_before:
        raise AdapterError("disposable process freshness evidence is incomplete")
    if list(identity.argv) != expected["argv"] or identity.environment != expected["environment"]:
        raise AdapterError("live argv or allowlisted environment differs from frozen controls")
    if identity.executable_sha256 != expected["executable_sha256"] or identity.port != expected["port"]:
        raise AdapterError("live executable hash or listener port differs from the condition")
    pure_cgroup = PurePosixPath(identity.control_group)
    if not pure_cgroup.is_absolute() or pure_cgroup.as_posix() != identity.control_group or \
            any(part in {".", ".."} for part in pure_cgroup.parts) or \
            not identity.control_group.endswith(f"/{identity.unit}"):
        raise AdapterError("disposable control-group identity is malformed")


def validate_gpu_admission(
    runner: Runner, role_hosts: dict[str, str], allowed_pids: dict[str, set[int]],
) -> dict[str, Any]:
    """Return an exact census and reject incomplete or foreign GPU ownership."""
    result: dict[str, Any] = {}
    for role in ("coordinator", "worker"):
        host = role_hosts[role]
        started_ns = time.monotonic_ns()
        try:
            admission = runner.gpu_clients(host)
        finally:
            ended_ns = time.monotonic_ns()
        if not isinstance(admission, dict) or admission.get("complete") is not True or admission.get("errors"):
            result[role] = {
                "host": host, "controller_started_ns": started_ns,
                "controller_ended_ns": ended_ns, "observed": admission,
            }
            raise GpuAdmissionError(f"GPU-client admission is incomplete on {role}", result)
        pids = admission.get("pids")
        if not isinstance(pids, list) or any(
                isinstance(pid, bool) or not isinstance(pid, int) or pid <= 0 for pid in pids) or \
                len(pids) != len(set(pids)):
            result[role] = {"host": host, "observed": admission}
            raise GpuAdmissionError(f"GPU-client admission returned malformed PIDs on {role}", result)
        allowed = allowed_pids.get(host, set())
        foreign = sorted(set(pids) - allowed)
        missing = sorted(allowed - set(pids))
        result[role] = {
            "host": host, "complete": True, "pids": pids, "allowed_pids": sorted(allowed),
            "foreign_pids": foreign, "missing_pids": missing, "errors": [],
            "controller_started_ns": started_ns, "controller_ended_ns": ended_ns,
        }
        if foreign:
            raise GpuAdmissionError(
                f"foreign GPU clients make an isolated measurement invalid on {role}: {foreign}", result)
        if missing:
            raise GpuAdmissionError(
                f"captured disposable GPU PIDs are absent on {role}: {missing}", result)
    return result


class TelemetryThread:
    def __init__(self, runner: Runner, hosts: Sequence[str], interval: float):
        self.runner = runner
        self.hosts = tuple(hosts)
        self.interval = interval
        self.records: dict[str, list[dict[str, Any]]] = {host: [] for host in hosts}
        self.errors: list[str] = []
        self.stop_event = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)

    def _run(self) -> None:
        while not self.stop_event.is_set():
            for host in self.hosts:
                started = time.monotonic_ns()
                try:
                    value = self.runner.telemetry(host)
                    ended = time.monotonic_ns()
                    self.records[host].append({"controller_started_ns": started, "controller_ended_ns": ended, "sample": value})
                except Exception as exc:
                    self.errors.append(f"{host}: {exc}")
                    self.stop_event.set()
                    return
            self.stop_event.wait(self.interval)

    def start(self) -> None:
        self.thread.start()

    def stop(self) -> None:
        self.stop_event.set()
        self.thread.join(timeout=max(5.0, self.interval * 2))
        if self.thread.is_alive():
            self.errors.append("telemetry thread did not stop")

    def validate(self, started_ns: int, ended_ns: int) -> dict[str, list[dict[str, Any]]]:
        if self.errors:
            raise AdapterError(f"telemetry collection failed: {self.errors}")
        for host, rows in self.records.items():
            if not rows:
                raise AdapterError(f"telemetry is empty for {host}")
            boot_ids: set[str] = set()
            remote_monotonic: list[int] = []
            for row in rows:
                sample = row.get("sample")
                if not isinstance(sample, dict) or not isinstance(sample.get("boot_id"), str) or \
                        not sample["boot_id"] or isinstance(sample.get("monotonic_ns"), bool) or \
                        not isinstance(sample.get("monotonic_ns"), int) or sample["monotonic_ns"] <= 0:
                    raise AdapterError(f"telemetry identity is malformed for {host}")
                boot_ids.add(sample["boot_id"])
                remote_monotonic.append(sample["monotonic_ns"])
            if len(boot_ids) != 1 or any(
                    later <= earlier for earlier, later in zip(remote_monotonic, remote_monotonic[1:])):
                raise AdapterError(f"telemetry boot identity or monotonic order changed for {host}")
            if not any(row["controller_ended_ns"] >= started_ns and row["controller_started_ns"] <= ended_ns for row in rows):
                raise AdapterError(f"telemetry does not overlap the request for {host}")
        return self.records


class GpuAdmissionMonitor:
    """Continuously retain exact exclusive GPU ownership around one request."""

    def __init__(
        self, runner: Runner, role_hosts: dict[str, str], allowed_pids: dict[str, set[int]],
        interval: float,
    ):
        self.runner = runner
        self.role_hosts = role_hosts
        self.allowed_pids = allowed_pids
        self.interval = min(interval, 0.1)
        self.records: list[dict[str, Any]] = []
        self.errors: list[dict[str, Any]] = []
        self.stop_event = threading.Event()
        self.thread = threading.Thread(target=self._run, daemon=True)

    def _sample(self) -> None:
        started = time.monotonic_ns()
        try:
            admission = validate_gpu_admission(self.runner, self.role_hosts, self.allowed_pids)
            error = None
        except GpuAdmissionError as exc:
            admission = exc.record
            error = {"type": type(exc).__name__, "detail": str(exc)}
        except Exception as exc:
            admission = None
            error = {"type": type(exc).__name__, "detail": str(exc)}
        ended = time.monotonic_ns()
        row = {
            "controller_started_ns": started,
            "controller_ended_ns": ended,
            "admission": admission,
            "error": error,
        }
        self.records.append(row)
        if error is not None:
            self.errors.append(row)

    def _run(self) -> None:
        while not self.stop_event.wait(self.interval):
            self._sample()

    def start(self) -> None:
        self._sample()
        if self.errors:
            detail = self.errors[0].get("error", {}).get("detail", "unknown GPU admission failure")
            raise GpuAdmissionError(
                f"initial measured GPU admission failed: {detail}", {"samples": self.records})
        self.thread.start()

    def stop(self) -> None:
        self.stop_event.set()
        self.thread.join(timeout=max(5.0, self.interval * 2))
        if self.thread.is_alive():
            self.errors.append({"error": {"type": "AdapterError", "detail": "GPU monitor did not stop"}})
        self._sample()

    def validate(self, request_started_ns: int, request_ended_ns: int) -> list[dict[str, Any]]:
        if self.errors:
            raise AdapterError(f"measured GPU admission monitor failed: {self.errors}")
        for role in ("coordinator", "worker"):
            windows = [
                row["admission"][role] for row in self.records
                if isinstance(row.get("admission"), dict) and
                isinstance(row["admission"].get(role), dict)
            ]
            if not any(row["controller_ended_ns"] <= request_started_ns for row in windows):
                raise AdapterError(f"GPU admission monitor has no completed pre-request census for {role}")
            if not any(row["controller_started_ns"] >= request_ended_ns for row in windows):
                raise AdapterError(f"GPU admission monitor has no post-request re-census for {role}")
            if not any(
                    row["controller_ended_ns"] >= request_started_ns and
                    row["controller_started_ns"] <= request_ended_ns for row in windows):
                raise AdapterError(f"GPU admission monitor has no request-overlapping census for {role}")
        return self.records


def run_cycle(
    runner: Runner, policy: Policy, plan: dict[str, Any], schedule_index: int, entry: dict[str, Any],
    kind: str, ordinal: int, request_bytes: bytes, protected_pids: set[int], evidence_dir: Path,
) -> dict[str, Any]:
    condition = entry["condition"]
    commands = core.commands_document(plan)["conditions"][condition]
    environment = dict(plan["runtime"]["common_environment"])
    identities: dict[str, UnitIdentity] = {}
    proofs: dict[str, Any] = {}
    readiness: dict[str, Any] = {}
    terminal: dict[str, Any] = {}
    cleanup: dict[str, Any] = {}
    telemetry_records: dict[str, Any] | None = None
    gpu_admission: dict[str, Any] = {"samples": [], "errors": []}
    capture: RequestCapture | None = None
    errors: list[dict[str, str]] = []
    cleanup_authorized: set[str] = set()
    stage = "cycle initialization"
    units = {
        role: unit_name(policy, plan, schedule_index, kind, ordinal, condition, role)
        for role in ("worker", "coordinator")}
    ports = {"worker": policy.worker_port, "coordinator": policy.coordinator_port}
    hosts = {role: plan["topology"][role]["host"] for role in ("worker", "coordinator")}

    def record_error(error_stage: str, exc: Exception) -> None:
        errors.append({"stage": error_stage, "type": type(exc).__name__, "detail": str(exc)})

    try:
        for role in ("worker", "coordinator"):
            stage = f"{role} pre-start absence"
            runner.ensure_unit_absent(hosts[role], units[role])
            owners = runner.port_owners(hosts[role], ports[role])
            if owners:
                raise AdapterError(f"{role} disposable port is already owned by PIDs {owners}")
            cleanup_authorized.add(role)
        for role in ("worker", "coordinator"):
            stage = f"{role} start and identity"
            expected = {
                "role": role, "host": hosts[role], "unit": units[role], "argv": commands[role],
                "environment": environment, "executable_sha256": binary_hash(plan, condition, role),
                "port": ports[role],
            }
            identity = runner.start_unit(
                role, hosts[role], units[role], commands[role], environment,
                expected["executable_sha256"], ports[role], policy.runtime_max_seconds,
                policy.identity_timeout_seconds, policy.stop_timeout_seconds)
            assert_identity(identity, expected, protected_pids)
            identities[role] = identity
            stage = f"{role} initial live proof"
            proofs[f"{role}_process"] = runner.prove_live(identity, require_listener=False)
            stage = f"{role} readiness"
            readiness[role] = runner.wait_ready(identity, policy.readiness_timeout_seconds)
            assert_identity(identity, expected, protected_pids)
            stage = f"{role} ready live proof"
            proofs[f"{role}_ready"] = runner.prove_live(identity)
            if role == "coordinator":
                stage = "worker post-coordinator live proof"
                worker_proof = runner.prove_live(identities["worker"])
                proofs["worker_after_coordinator_ready"] = worker_proof

        allowed_pids: dict[str, set[int]] = {}
        for identity in identities.values():
            allowed_pids.setdefault(identity.host, set()).add(identity.pid)
        if kind == "measurement":
            stage = "measured pre-request GPU admission"
            gpu_monitor = GpuAdmissionMonitor(
                runner, hosts, allowed_pids, policy.telemetry_interval_seconds)
            try:
                gpu_monitor.start()
            except Exception:
                gpu_admission["samples"] = gpu_monitor.records
                gpu_admission["errors"] = gpu_monitor.errors
                write_json_durable(evidence_dir / "gpu-admission.json", gpu_admission, exclusive=True)
                raise
            telemetry = TelemetryThread(runner, (hosts["coordinator"], hosts["worker"]), policy.telemetry_interval_seconds)
            try:
                telemetry.start()
            except Exception as exc:
                record_error("telemetry start", exc)
                try:
                    gpu_monitor.stop()
                    gpu_admission["samples"] = gpu_monitor.records
                    gpu_admission["errors"] = gpu_monitor.errors
                except Exception as stop_exc:
                    record_error("GPU monitor stop after telemetry start failure", stop_exc)
                try:
                    write_json_durable(
                        evidence_dir / "gpu-admission.json", gpu_admission, exclusive=True)
                except Exception as retention_exc:
                    record_error("GPU admission retention after telemetry start failure", retention_exc)
                raise _CycleAbort()
        else:
            gpu_monitor = None
            telemetry = None

        request_failed = False
        request_controller_started_ns = time.monotonic_ns()
        try:
            stage = "request transport"
            capture = runner.request(
                hosts["coordinator"], policy.coordinator_port, request_bytes,
                plan["request"]["output_tokens"], policy.request_timeout_seconds)
        except Exception as exc:
            request_failed = True
            record_error(stage, exc)
        finally:
            request_controller_ended_ns = time.monotonic_ns()
            if telemetry is not None:
                try:
                    telemetry.stop()
                except Exception as exc:
                    record_error("telemetry stop", exc)
                telemetry_records = telemetry.records
                try:
                    telemetry_records = telemetry.validate(
                        request_controller_started_ns, request_controller_ended_ns)
                except Exception as exc:
                    record_error("telemetry validation", exc)
                try:
                    write_json_durable(evidence_dir / "telemetry.json", telemetry_records, exclusive=True)
                except Exception as exc:
                    record_error("telemetry retention", exc)
            if gpu_monitor is not None:
                try:
                    gpu_monitor.stop()
                    gpu_admission["samples"] = gpu_monitor.records
                    gpu_admission["errors"] = gpu_monitor.errors
                    gpu_monitor.validate(request_controller_started_ns, request_controller_ended_ns)
                except Exception as exc:
                    gpu_admission["samples"] = gpu_monitor.records
                    gpu_admission["errors"] = gpu_monitor.errors
                    record_error("measured GPU admission monitor", exc)
                try:
                    write_json_durable(
                        evidence_dir / "gpu-admission.json", gpu_admission, exclusive=True)
                except Exception as exc:
                    record_error("GPU admission retention", exc)
        if request_failed:
            raise _CycleAbort()

        stage = "request byte binding"
        assert capture is not None
        if capture.sent_body_sha256 != sha256_bytes(request_bytes):
            raise AdapterError("request transport did not send the retained request bytes")
        response_path = evidence_dir / f"{kind}-{ordinal}-response.json"
        client_path = evidence_dir / f"{kind}-{ordinal}-client.json"
        raw_path = evidence_dir / f"{kind}-{ordinal}-response.raw"
        stage = "request evidence retention"
        write_json_durable(response_path, capture.response, exclusive=True)
        write_json_durable(client_path, capture.client, exclusive=True)
        write_bytes_durable(raw_path, capture.raw_http, exclusive=True)
        stage = "request evidence validation"
        result = core.parse_response(response_path, plan)
        core.parse_client(client_path, plan["request"]["output_tokens"])
        if result["cache_n"] != 0:
            raise AdapterError("cache-off process reported reused prompt tokens")
        if errors:
            raise _CycleAbort()
    except _CycleAbort:
        pass
    except Exception as exc:
        record_error(stage, exc)
    finally:
        for role in ("coordinator", "worker"):
            if role in identities:
                try:
                    terminal_record, journal = runner.stop_unit(
                        identities[role], policy.stop_timeout_seconds)
                    journal_path = evidence_dir / f"{kind}-{ordinal}-{role}.journal"
                    write_bytes_durable(journal_path, journal, exclusive=True)
                    terminal_record = dict(terminal_record)
                    terminal_record["journal"] = {
                        "path": journal_path.name,
                        "size_bytes": len(journal),
                        "sha256": sha256_bytes(journal),
                    }
                    terminal[role] = terminal_record
                except Exception as exc:
                    record_error(f"{role} terminal evidence and stop", exc)
        for role in ("coordinator", "worker"):
            if role not in cleanup_authorized:
                continue
            try:
                cleanup[role] = runner.cleanup_unit(
                    hosts[role], units[role], ports[role], policy.stop_timeout_seconds,
                    identities.get(role))
            except Exception as exc:
                record_error(f"{role} cleanup proof", exc)

    if not errors and (
            set(identities) != {"worker", "coordinator"} or capture is None or
            set(terminal) != {"worker", "coordinator"} or
            set(cleanup) != {"worker", "coordinator"}):
        record_error("cycle completeness", AdapterError(
            "cycle did not retain both identities, terminal records, cleanup proofs, and one request"))

    request_record: dict[str, Any] | None = None
    if capture is not None:
        request_record = {
            "body_sha256": capture.sent_body_sha256,
            "remote_host": hosts["coordinator"],
            "remote_started_monotonic_ns": capture.started_monotonic_ns,
            "remote_ended_monotonic_ns": capture.ended_monotonic_ns,
            "controller_started_monotonic_ns": request_controller_started_ns,
            "controller_ended_monotonic_ns": request_controller_ended_ns,
        }
        for label, path in (
            ("response_sha256", evidence_dir / f"{kind}-{ordinal}-response.json"),
            ("client_sha256", evidence_dir / f"{kind}-{ordinal}-client.json"),
            ("raw_http_sha256", evidence_dir / f"{kind}-{ordinal}-response.raw"),
        ):
            if path.is_file():
                request_record[label] = core.digest_file(path)

    record = {
        "kind": kind,
        "ordinal": ordinal,
        "status": "failure" if errors else "success",
        "identities": {role: dataclasses.asdict(identity) for role, identity in identities.items()},
        "live_proofs": proofs,
        "readiness": readiness,
        "request": request_record,
        "telemetry": telemetry_records,
        "gpu_admission": gpu_admission if kind == "measurement" else None,
        "terminal": terminal,
        "cleanup": cleanup,
        "errors": errors,
    }
    if errors:
        raise CycleExecutionError(record)
    return record


def execute_next(root: Path, policy_path: Path, runner: Runner) -> Path:
    plan = core.load_plan(root / "plan.json")
    schedule = core.validate_run_contract(root, plan)
    policy_bytes, policy, fixed_policy_sha256 = read_policy(policy_path, plan)
    if isinstance(runner, SshCachyRunner):
        issue41 = load_issue41_authority()
        if not TARGET_EXECUTION_ENABLED:
            raise AdapterError(
                "target execute-next is disabled in this draft pending issue #37 lifecycle review "
                "and issue #41 protected GPU/HMM/OOM admission and recovery custody; "
                f"unresolved={issue41['unresolved_custody']}; validate remains available")
        validate_target_plan_paths(plan)
    request_bytes, bindings = input_bindings(root, plan)
    index, entry, directory = next_schedule_entry(root, schedule, fixed_policy_sha256)
    coordinator_host = plan["topology"]["coordinator"]["host"]
    model_before = runner.artifact(coordinator_host, plan["model"]["path"])
    if model_before != {
        "path": plan["model"]["path"],
        "size_bytes": plan["model"]["size_bytes"],
        "sha256": plan["model"]["sha256"],
    }:
        raise AdapterError("execution-time model bytes differ from the frozen plan")
    before = runner.snapshot_production(policy.protected)
    active_production = [role for role, value in before.items() if value.get("active") is True]
    if active_production:
        raise AdapterError(
            "protected production is active; this adapter never stops it and requires an already-idle authorized window: "
            + ", ".join(active_production))
    protected_pids = {
        value.get("pid", 0) for value in before.values()
        if isinstance(value, dict) and isinstance(value.get("pid"), int) and value.get("pid", 0) > 0}
    role_hosts = {role: plan["topology"][role]["host"] for role in ("coordinator", "worker")}
    admission_before_intent = validate_gpu_admission(runner, role_hosts, {})
    directory.mkdir(mode=0o700)
    policy_copy = directory / "policy.raw"
    write_bytes_durable(policy_copy, policy_bytes, exclusive=True)
    policy_binding = {
        "path": policy_copy.name,
        "size_bytes": len(policy_bytes),
        "sha256": fixed_policy_sha256,
    }
    intent = {
        "schema": INTENT_SCHEMA, "issue": 37, "experiment_id": plan["experiment_id"],
        "plan_sha256": core.plan_digest(plan), "policy_sha256": fixed_policy_sha256,
        "policy_binding": policy_binding,
        "schedule_index": index, "entry": entry,
        "created_at": dt.datetime.now(dt.timezone.utc).isoformat(),
        "no_retry_after_intent": True,
    }
    write_json_durable(directory / "intent.json", intent, exclusive=True)
    cycles: list[dict[str, Any]] = []
    entry_errors: list[dict[str, str]] = []
    measured_dir = directory / "measured"
    measured_dir.mkdir(mode=0o700)
    measured: dict[str, Any] | None = None

    def record_entry_error(error_stage: str, exc: Exception) -> None:
        if isinstance(exc, CycleExecutionError) and exc.record.get("errors"):
            primary = exc.record["errors"][0]
            entry_errors.append({
                "stage": error_stage,
                "type": str(primary.get("type", type(exc).__name__)),
                "detail": str(exc),
            })
        else:
            entry_errors.append({"stage": error_stage, "type": type(exc).__name__, "detail": str(exc)})

    try:
        warmup_identities: list[dict[str, Any]] = []
        for ordinal in range(plan["execution"]["warmups_per_condition"]):
            evidence_dir = directory / f"warmup-{ordinal}"
            evidence_dir.mkdir(mode=0o700)
            try:
                cycle = run_cycle(
                    runner, policy, plan, index, entry, "warmup", ordinal, request_bytes,
                    protected_pids, evidence_dir)
            except CycleExecutionError as exc:
                cycles.append(exc.record)
                raise
            cycles.append(cycle)
            warmup_identities.append(cycle["identities"])
        try:
            measured = run_cycle(
                runner, policy, plan, index, entry, "measurement", 0, request_bytes,
                protected_pids, measured_dir)
        except CycleExecutionError as exc:
            cycles.append(exc.record)
            raise
        for warmup in warmup_identities:
            for role in ("coordinator", "worker"):
                old = warmup[role]
                new = measured["identities"][role]
                if old["pid"] == new["pid"] or old["invocation_id"] == new["invocation_id"]:
                    raise AdapterError("warmup and measured process identities are not fresh")
        cycles.append(measured)
    except Exception as exc:
        record_entry_error("scheduled cycle execution", exc)

    model_after: dict[str, Any] | None = None
    try:
        model_after = runner.artifact(coordinator_host, plan["model"]["path"])
        if model_after != model_before:
            raise AdapterError("model bytes changed during the scheduled entry")
    except Exception as exc:
        record_entry_error("post-entry model binding", exc)

    try:
        after = runner.snapshot_production(policy.protected)
        if after != before:
            raise AdapterError("protected production authority changed during the adapter entry")
    except Exception as exc:
        after = {"snapshot_error": str(exc)}
        record_entry_error("production reconciliation", exc)

    if entry_errors:
        primary_code = (
            "PRODUCTION_RECONCILIATION"
            if entry_errors[0]["stage"] == "production reconciliation"
            else entry_errors[0]["type"]
        )
        outcome = {
            "status": "failure",
            "failure_code": primary_code,
            "detail": "; ".join(f"{item['stage']}: {item['detail']}" for item in entry_errors),
        }
    else:
        outcome = {"status": "success", "failure_code": None}
    receipt = {
        "schema": RECEIPT_SCHEMA,
        "issue": 37,
        "experiment_id": plan["experiment_id"],
        "plan_sha256": core.plan_digest(plan),
        "policy_sha256": fixed_policy_sha256,
        "policy_binding": policy_binding,
        "schedule_index": index,
        "entry": entry,
        "input_bindings": bindings,
        "model_binding": model_before,
        "preflight_sha256": {
            role: core.digest_file(root / "preflight" / f"{role}.json") for role in ("coordinator", "worker")},
        "production_before": before,
        "production_after": after,
        "gpu_admission_before_intent": admission_before_intent,
        "model_binding_after": model_after,
        "cycles": cycles,
        "errors": entry_errors,
        "outcome": outcome,
        "execution_qualified": False,
        "measurement_ready": False,
        "performance_claim": False,
    }
    receipt_path = directory / "execution.json"
    write_json_durable(receipt_path, receipt, exclusive=True)
    if outcome["status"] == "success":
        assert measured is not None
        response = measured_dir / "measurement-0-response.json"
        client = measured_dir / "measurement-0-client.json"
        extras = [
            receipt_path, policy_copy, measured_dir / "measurement-0-response.raw",
            measured_dir / "telemetry.json", measured_dir / "gpu-admission.json",
        ]
        core.record_sample(
            root, entry["pair_id"], entry["condition"], entry["order_index"],
            response, client, "success", None, extras)
    else:
        core.record_sample(
            root, entry["pair_id"], entry["condition"], entry["order_index"],
            None, None, "failure", outcome["failure_code"], [receipt_path, policy_copy])
    if outcome["status"] != "success":
        raise AdapterError(outcome.get("detail", outcome["failure_code"]))
    return receipt_path


@dataclasses.dataclass(frozen=True)
class CommandResult:
    returncode: int
    stdout: bytes
    stderr: bytes


def parse_properties(raw: bytes) -> dict[str, str]:
    try:
        text = raw.decode("utf-8", errors="strict")
    except UnicodeError as exc:
        raise AdapterError("systemd properties are not UTF-8") from exc
    result: dict[str, str] = {}
    for line in text.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key in result:
            raise AdapterError(f"duplicate systemd property {key}")
        result[key] = value
    return result


def parse_unified_cgroup(raw: Any, where: str) -> str:
    """Return one canonical cgroup-v2 path from /proc/<pid>/cgroup."""
    if not isinstance(raw, str):
        raise AdapterError(f"{where} is not text")
    lines = raw.splitlines()
    if len(lines) != 1:
        raise AdapterError(f"{where} must contain exactly one unified cgroup entry")
    match = re.fullmatch(r"0::(/[^\x00\r\n]*)", lines[0])
    if match is None:
        raise AdapterError(f"{where} is not a unified cgroup-v2 entry")
    path = match.group(1)
    pure = PurePosixPath(path)
    if not pure.is_absolute() or pure.as_posix() != path or any(
            part in {".", ".."} for part in pure.parts):
        raise AdapterError(f"{where} has a noncanonical cgroup path")
    return path


def require_properties(value: dict[str, str], keys: set[str], where: str) -> dict[str, str]:
    if set(value) != keys:
        raise AdapterError(f"{where} has an incomplete or unexpected systemd property set")
    return value


REMOTE_PROCESS_PROGRAM = r'''
import hashlib,json,os,pathlib,sys
pid=int(sys.argv[1]); retain_env=sys.argv[2]=='1'; root=pathlib.Path('/proc')/str(pid)
def split0(name):
    raw=(root/name).read_bytes()
    return [item.decode('utf-8','strict') for item in raw.split(b'\0') if item]
env={}
if retain_env:
    for item in split0('environ'):
        if '=' not in item: raise RuntimeError('malformed environment')
        key,value=item.split('=',1)
        if key in env: raise RuntimeError('duplicate environment')
        env[key]=value
exe=os.readlink(root/'exe')
h=hashlib.sha256()
with open(root/'exe','rb') as f:
    for chunk in iter(lambda:f.read(1048576),b''): h.update(chunk)
stat=(root/'stat').read_text(); close=stat.rfind(')')
if close<0: raise RuntimeError('malformed process stat')
tail=stat[close+2:].split()
print(json.dumps({'pid':pid,'exe':exe,'exe_sha256':h.hexdigest(),'argv':split0('cmdline'),
                  'environment':env if retain_env else None,'cgroup':(root/'cgroup').read_text(),
                  'process_start_ticks':int(tail[19])},sort_keys=True))
'''


REMOTE_TELEMETRY_PROGRAM = r'''
import glob,json,pathlib,time
def read(path):
    try:return pathlib.Path(path).read_text().strip()
    except OSError:return None
gpu={}
for path in sorted(glob.glob('/sys/class/drm/card*/device/gpu_busy_percent')):gpu[path]=read(path)
for path in sorted(glob.glob('/sys/class/drm/card*/device/hwmon/hwmon*/temp*_input')):gpu[path]=read(path)
print(json.dumps({'monotonic_ns':time.monotonic_ns(),'boot_id':read('/proc/sys/kernel/random/boot_id'),
 'loadavg':read('/proc/loadavg'),'meminfo':read('/proc/meminfo'),'gpu':gpu},sort_keys=True))
'''


REMOTE_REQUEST_PROGRAM = r'''
import base64,datetime,hashlib,json,sys,time,urllib.request
url=sys.argv[1]; expected=int(sys.argv[2]); timeout=float(sys.argv[3]); body=sys.stdin.buffer.read()
wall_start=datetime.datetime.now(datetime.timezone.utc); start=time.monotonic_ns(); raw=bytearray(); contents=[]; stamps=[]; timings=None
request=urllib.request.Request(url,data=body,headers={'Content-Type':'application/json'},method='POST')
with urllib.request.urlopen(request,timeout=timeout) as response:
    status=response.status
    while True:
        line=response.readline()
        if not line:break
        raw.extend(line)
        stripped=line.strip()
        if not stripped.startswith(b'data:'):continue
        payload=stripped[5:].strip()
        if payload==b'[DONE]':continue
        event=json.loads(payload.decode('utf-8','strict'))
        content=event.get('content')
        if isinstance(content,str) and content:
            contents.append(content); stamps.append(time.monotonic_ns())
        if isinstance(event.get('timings'),dict):timings=event['timings']
end=time.monotonic_ns(); wall_end=datetime.datetime.now(datetime.timezone.utc)
if status!=200 or timings is None or len(stamps)!=expected:
    raise RuntimeError(f'stream status/timing/token-event mismatch status={status} events={len(stamps)} expected={expected}')
ttft=(stamps[0]-start)/1e6; itl=[(b-a)/1e6 for a,b in zip(stamps,stamps[1:])]
print(json.dumps({'response':{'content':''.join(contents),'timings':timings},
 'client':{'schema':'halofpx.client-timing.v1','started_at':wall_start.isoformat(),
 'ended_at':wall_end.isoformat(),'http_status':status,'wall_ms':(end-start)/1e6,'ttft_ms':ttft,'itl_ms':itl},
 'raw_base64':base64.b64encode(raw).decode('ascii'),'sent_body_sha256':hashlib.sha256(body).hexdigest(),
 'started_monotonic_ns':start,'ended_monotonic_ns':end},sort_keys=True))
'''


class SshCachyRunner:
    """Array-oriented SSH/systemd implementation; it never mutates system units."""

    def __init__(self, ssh_binary: str = "ssh"):
        self.ssh_binary = ssh_binary

    def _run(self, host: str, argv: Sequence[str], *, stdin: bytes | None = None, timeout: int = 60) -> CommandResult:
        if not argv or any(not isinstance(item, str) or "\x00" in item for item in argv):
            raise AdapterError("remote argv is empty or malformed")
        if isinstance(timeout, bool) or not isinstance(timeout, int) or timeout <= 0:
            raise AdapterError("remote command timeout is malformed")
        # The remote coreutils watchdog survives a lost SSH connection and owns
        # the remote child; the local process group bounds the SSH client itself.
        remote = [
            "/usr/bin/timeout", "--signal=TERM", "--kill-after=5s", f"{timeout}s", "--", *argv,
        ]
        command = [
            self.ssh_binary, "-o", "BatchMode=yes", "-o", "StrictHostKeyChecking=yes",
            "-o", "UpdateHostKeys=no", "-o", "ConnectTimeout=10",
            "-o", "ConnectionAttempts=1", "-o", "ServerAliveInterval=5",
            "-o", "ServerAliveCountMax=2", "--", host, shlex.join(remote),
        ]
        popen_kwargs: dict[str, Any] = {
            "stdin": subprocess.PIPE if stdin is not None else subprocess.DEVNULL,
            "stdout": subprocess.PIPE,
            "stderr": subprocess.PIPE,
        }
        if os.name == "posix":
            popen_kwargs["start_new_session"] = True
        elif hasattr(subprocess, "CREATE_NEW_PROCESS_GROUP"):
            popen_kwargs["creationflags"] = subprocess.CREATE_NEW_PROCESS_GROUP
        try:
            process = subprocess.Popen(command, **popen_kwargs)
            stdout, stderr = process.communicate(input=stdin, timeout=timeout + 15)
        except OSError as exc:
            raise AdapterError(f"remote command failed on {host}: {argv[0]}: {exc}") from exc
        except subprocess.TimeoutExpired as exc:
            if os.name == "posix":
                try:
                    os.killpg(process.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
            else:
                process.terminate()
            try:
                process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                if os.name == "posix":
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                else:
                    process.kill()
                try:
                    process.communicate(timeout=5)
                except subprocess.TimeoutExpired as reap_exc:
                    raise AdapterError(
                        f"local SSH process group could not be reaped on {host}: {argv[0]}; "
                        "remote /usr/bin/timeout remains the child-process watchdog") from reap_exc
            raise AdapterError(
                f"local SSH custody timed out on {host}: {argv[0]}; "
                "remote /usr/bin/timeout remains the child-process watchdog") from exc
        return CommandResult(process.returncode, stdout, stderr)

    def _required(self, host: str, argv: Sequence[str], *, stdin: bytes | None = None, timeout: int = 60) -> bytes:
        result = self._run(host, argv, stdin=stdin, timeout=timeout)
        if result.returncode != 0:
            detail = result.stderr.decode("utf-8", errors="replace").strip()
            raise AdapterError(f"required command failed on {host}: {argv[0]}: {detail}")
        return result.stdout

    def _process(self, host: str, pid: int, *, retain_environment: bool = True) -> dict[str, Any]:
        raw = self._required(host, [
            "python3", "-c", REMOTE_PROCESS_PROGRAM, str(pid),
            "1" if retain_environment else "0"])
        try:
            value = json.loads(raw.decode("utf-8", errors="strict"))
        except (UnicodeError, json.JSONDecodeError) as exc:
            raise AdapterError("process identity output is malformed") from exc
        require_exact_keys(value, {"pid", "exe", "exe_sha256", "argv", "environment", "cgroup", "process_start_ticks"}, "process proof")
        return value

    def artifact(self, host: str, path: str) -> dict[str, Any]:
        program = r'''
import hashlib,json,os,pathlib,stat,sys
p=pathlib.Path(sys.argv[1]); s=os.lstat(p)
if stat.S_ISLNK(s.st_mode) or not stat.S_ISREG(s.st_mode): raise RuntimeError('not a regular non-symlink file')
h=hashlib.sha256()
with p.open('rb') as f:
    for chunk in iter(lambda:f.read(1048576),b''): h.update(chunk)
print(json.dumps({'path':sys.argv[1],'size_bytes':s.st_size,'sha256':h.hexdigest()},sort_keys=True))
'''
        raw = self._required(host, ["python3", "-c", program, path], timeout=3600)
        try:
            value = json.loads(raw.decode("utf-8", errors="strict"))
        except (UnicodeError, json.JSONDecodeError) as exc:
            raise AdapterError("artifact proof is malformed") from exc
        require_exact_keys(value, {"path", "size_bytes", "sha256"}, "artifact proof")
        return value

    def port_owners(self, host: str, port: int) -> list[int]:
        output = self._required(host, ["ss", "-H", "-ltnp"])
        owners: set[int] = set()
        for raw_line in output.decode("utf-8", errors="strict").splitlines():
            fields = raw_line.split()
            if len(fields) < 4:
                continue
            address = fields[3]
            if not address.rsplit(":", 1)[-1] == str(port):
                continue
            matches = {int(value) for value in re.findall(r"pid=(\d+)", raw_line)}
            if not matches:
                raise AdapterError(f"listener on {host}:{port} has no attributable PID")
            owners.update(matches)
        return sorted(owners)

    def snapshot_production(self, protected: dict[str, dict[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for role in ("coordinator", "worker"):
            spec = protected[role]
            host, unit = spec["host"], spec["unit"]
            props = parse_properties(self._required(host, [
                "systemctl", "--system", "show", unit, "-p", "Id", "-p", "LoadState",
                "-p", "ActiveState", "-p", "SubState", "-p", "MainPID", "-p", "InvocationID",
                "-p", "NRestarts", "-p", "ExecMainStartTimestampMonotonic", "-p", "ControlGroup",
            ]))
            require_properties(props, {
                "Id", "LoadState", "ActiveState", "SubState", "MainPID", "InvocationID",
                "NRestarts", "ExecMainStartTimestampMonotonic", "ControlGroup",
            }, "protected production snapshot")
            if props.get("Id") != unit or props.get("LoadState") != "loaded":
                raise AdapterError("protected production unit authority is unavailable")
            try:
                pid = int(props.get("MainPID", "0"))
                restarts = int(props.get("NRestarts", "-1"))
            except ValueError as exc:
                raise AdapterError("protected production numeric authority is malformed") from exc
            active = props.get("ActiveState") == "active" and props.get("SubState") == "running"
            if active:
                invocation = props.get("InvocationID", "").lower()
                if pid <= 0 or not INVOCATION_RE.fullmatch(invocation) or restarts < 0:
                    raise AdapterError("active production identity is incomplete")
                raw_process = self._process(host, pid, retain_environment=False)
                process = {key: value for key, value in raw_process.items() if key != "environment"}
                listeners = {str(port): self.port_owners(host, port) for port in spec["ports"]}
                if any(owners != [pid] for owners in listeners.values()):
                    raise AdapterError("protected production listener is not owned by its PID")
                health: dict[str, Any] | None = None
                if spec["health_url"]:
                    raw = self._required(host, ["curl", "-fsS", "--max-time", "5", spec["health_url"]])
                    health = {"sha256": sha256_bytes(raw), "bytes": len(raw)}
            else:
                if pid != 0:
                    raise AdapterError("inactive protected unit retains a PID")
                invocation = props.get("InvocationID", "").lower()
                process = None
                listeners = {str(port): self.port_owners(host, port) for port in spec["ports"]}
                if any(listeners.values()):
                    raise AdapterError("protected port is occupied while its unit is inactive")
                health = None
            result[role] = {
                "host": host, "unit": unit, "active": active, "pid": pid,
                "invocation_id": invocation, "nrestarts": restarts,
                "start_monotonic": props.get("ExecMainStartTimestampMonotonic", ""),
                "control_group": props.get("ControlGroup", ""), "process": process,
                "listeners": listeners, "health": health,
            }
        return result

    def gpu_clients(self, host: str) -> dict[str, Any]:
        devices_raw = self._run(host, [
            "find", "/dev", "-maxdepth", "3", "-type", "c", "(",
            "-path", "/dev/kfd", "-o", "-path", "/dev/dri/renderD*", ")", "-print",
        ])
        if devices_raw.returncode != 0:
            return {"complete": False, "pids": [], "errors": ["GPU device enumeration failed"]}
        try:
            devices = sorted({line for line in devices_raw.stdout.decode("utf-8", errors="strict").splitlines() if line})
        except UnicodeError:
            return {"complete": False, "pids": [], "errors": ["GPU device enumeration is malformed"]}
        if "/dev/kfd" not in devices or not any(path.startswith("/dev/dri/renderD") for path in devices):
            return {"complete": False, "pids": [], "errors": ["GPU device set is incomplete"]}
        # Root visibility is required to call this a complete machine census.
        # A host without non-interactive authority is ineligible, not empty.
        result = self._run(host, ["sudo", "-n", "fuser", *devices])
        if result.returncode not in {0, 1}:
            return {"complete": False, "pids": [], "errors": [result.stderr.decode("utf-8", errors="replace")]}
        stderr_text = result.stderr.decode("utf-8", errors="replace")
        residual = stderr_text
        for device in devices:
            residual = re.sub(re.escape(device) + r":\s*", "", residual)
        if residual.strip():
            return {"complete": False, "pids": [], "errors": [stderr_text]}
        values = re.findall(rb"\b\d+\b", result.stdout)
        # `/proc` must expose every reported client; this rejects hidden/vanished
        # ownership instead of accepting an incomplete fuser census.
        pids = sorted({int(value) for value in values})
        for pid in pids:
            probe = self._run(host, ["test", "-r", f"/proc/{pid}/cmdline"])
            if probe.returncode != 0:
                return {"complete": False, "pids": pids, "errors": [f"GPU client PID {pid} is not inspectable"]}
        return {"complete": True, "pids": pids, "errors": []}

    def ensure_unit_absent(self, host: str, unit: str) -> None:
        output = self._run(host, ["systemctl", "--user", "show", unit, "-p", "LoadState", "-p", "ActiveState", "-p", "MainPID"])
        if output.returncode != 0:
            raise AdapterError(f"cannot prove disposable unit absent: {host}/{unit}")
        props = parse_properties(output.stdout)
        require_properties(props, {"LoadState", "ActiveState", "MainPID"}, "disposable absence proof")
        if props["LoadState"] != "not-found" or props["ActiveState"] != "inactive" or props["MainPID"] != "0":
            raise AdapterError(f"disposable unit already exists: {host}/{unit}")

    def start_unit(
        self, role: str, host: str, unit: str, argv: Sequence[str], environment: dict[str, str],
        executable_sha256: str, port: int, runtime_max_seconds: int, identity_timeout_seconds: int,
        stop_timeout_seconds: int,
    ) -> UnitIdentity:
        self.ensure_unit_absent(host, unit)
        cursor_raw = self._required(host, ["journalctl", "--user", "-n", "0", "--show-cursor", "--no-pager"])
        matches = re.findall(rb"^-- cursor: (\S+)$", cursor_raw, re.MULTILINE)
        if len(matches) != 1:
            raise AdapterError("pre-start journal cursor is malformed or ambiguous")
        cursor = matches[0].decode("ascii", errors="strict")
        env_argv = [f"{key}={value}" for key, value in sorted(environment.items())]
        start_argv = [
            "systemd-run", "--user", f"--unit={unit.removesuffix('.service')}",
            "--property=Type=exec", "--property=RemainAfterExit=yes",
            "--property=KillMode=control-group", "--property=KillSignal=SIGTERM",
            "--property=SendSIGKILL=yes", "--property=Restart=no",
            f"--property=TimeoutStartSec={identity_timeout_seconds}",
            f"--property=TimeoutStopSec={stop_timeout_seconds}",
            f"--property=RuntimeMaxSec={runtime_max_seconds}", "--",
            "/usr/bin/env", "-i", *env_argv, *argv,
        ]
        self._required(host, start_argv)
        deadline = time.monotonic() + identity_timeout_seconds
        last: dict[str, str] = {}
        while time.monotonic() < deadline:
            last = parse_properties(self._required(host, [
                "systemctl", "--user", "show", unit, "-p", "ExecMainPID", "-p", "InvocationID",
                "-p", "ExecMainStartTimestampMonotonic", "-p", "ActiveState", "-p", "ControlGroup",
                "-p", "FragmentPath",
            ]))
            require_properties(last, {
                "ExecMainPID", "InvocationID", "ExecMainStartTimestampMonotonic",
                "ActiveState", "ControlGroup", "FragmentPath",
            }, "disposable start identity")
            try:
                pid = int(last.get("ExecMainPID", "0"))
                start_mono = int(last.get("ExecMainStartTimestampMonotonic", "0"))
            except ValueError:
                pid = start_mono = 0
            invocation = last.get("InvocationID", "").lower()
            if last["ActiveState"] == "active" and pid > 0 and start_mono > 0 and INVOCATION_RE.fullmatch(invocation):
                process = self._process(host, pid)
                control_group = last.get("ControlGroup", "")
                process_control_group = parse_unified_cgroup(
                    process["cgroup"], "new process /proc cgroup proof")
                if not last.get("FragmentPath", "").endswith(f"/transient/{unit}") or \
                        not control_group.endswith(f"/{unit}") or \
                        process_control_group != control_group:
                    raise AdapterError("new process is outside the expected user transient unit/cgroup")
                identity = UnitIdentity(
                    role, host, unit, pid, invocation, process["process_start_ticks"], start_mono,
                    cursor, tuple(process["argv"]), dict(process["environment"]),
                    process["exe_sha256"], port, control_group)
                expected = {
                    "role": role, "host": host, "unit": unit, "argv": list(argv),
                    "environment": environment, "executable_sha256": executable_sha256, "port": port}
                assert_identity(identity, expected, set())
                return identity
            time.sleep(0.05)
        raise AdapterError(f"unit identity was not captured before timeout: {last}")

    def prove_live(self, identity: UnitIdentity, require_listener: bool = True) -> dict[str, Any]:
        props = parse_properties(self._required(identity.host, [
            "systemctl", "--user", "show", identity.unit, "-p", "ExecMainPID", "-p", "InvocationID",
            "-p", "ActiveState", "-p", "SubState", "-p", "ControlGroup", "-p", "FragmentPath",
        ]))
        require_properties(props, {
            "ExecMainPID", "InvocationID", "ActiveState", "SubState", "ControlGroup", "FragmentPath",
        }, "disposable live proof")
        if props.get("ActiveState") != "active" or int(props.get("ExecMainPID", "0")) != identity.pid or \
                props.get("InvocationID", "").lower() != identity.invocation_id:
            raise AdapterError("disposable unit identity changed or is not active")
        process = self._process(identity.host, identity.pid)
        if process["process_start_ticks"] != identity.process_start_ticks or process["argv"] != list(identity.argv) or \
                process["environment"] != identity.environment or process["exe_sha256"] != identity.executable_sha256:
            raise AdapterError("disposable process identity changed")
        fragment = props.get("FragmentPath", "")
        control_group = props.get("ControlGroup", "")
        process_control_group = parse_unified_cgroup(
            process["cgroup"], "live process /proc cgroup proof")
        if not fragment.endswith(f"/transient/{identity.unit}") or \
                control_group != identity.control_group or \
                process_control_group != control_group:
            raise AdapterError("disposable process is outside the expected user transient unit/cgroup")
        owners = self.port_owners(identity.host, identity.port)
        if require_listener and owners != [identity.pid]:
            raise AdapterError("disposable listener is not exclusively PID-owned")
        if not require_listener and owners not in {[], [identity.pid]}:
            raise AdapterError("disposable port is owned by another PID during startup")
        return {"systemd": props, "process": process, "listener_pids": owners}

    def wait_ready(self, identity: UnitIdentity, timeout_seconds: int) -> dict[str, Any]:
        deadline = time.monotonic() + timeout_seconds
        last = ""
        while time.monotonic() < deadline:
            try:
                self.prove_live(identity, require_listener=False)
                owners = self.port_owners(identity.host, identity.port)
                if owners != [identity.pid]:
                    last = f"listener owners are {owners}"
                    time.sleep(0.1)
                    continue
                if identity.role == "worker":
                    program = r'''
import json,socket,struct,sys
s=socket.create_connection(('127.0.0.1',int(sys.argv[1])),timeout=2);s.settimeout(2)
with s:
 s.sendall(struct.pack('<BQ',14,24)+bytes(24))
 raw=b''
 while len(raw)<8:
  part=s.recv(8-len(raw))
  if not part:raise RuntimeError('early hello size EOF')
  raw+=part
 size=struct.unpack('<Q',raw)[0]
 body=b''
 while len(body)<size:
  part=s.recv(size-len(body))
  if not part:raise RuntimeError('early hello body EOF')
  body+=part
 if size!=28:raise RuntimeError('wrong hello size')
 major,minor,patch,padding,caps=struct.unpack('<BBBB24s',body)
 if (major,minor,patch)!=(4,0,1) or padding!=0:raise RuntimeError('wrong RPC protocol')
 print(json.dumps({'rpc_protocol':'4.0.1','connection_caps_sha256':__import__('hashlib').sha256(caps).hexdigest()}))
'''
                    raw = self._required(identity.host, [
                        "python3", "-c", program, str(identity.port)], timeout=5)
                    try:
                        hello = json.loads(raw.decode("utf-8", errors="strict"))
                    except (UnicodeError, json.JSONDecodeError) as exc:
                        raise AdapterError("RPC HELLO readiness output is malformed") from exc
                    return {"kind": "pid-owned-rpc-hello", "ready": True, **hello}
                result = self._run(identity.host, [
                    "curl", "-fsS", "--max-time", "2", f"http://127.0.0.1:{identity.port}/health"])
                if result.returncode == 0:
                    return {"kind": "pid-owned-http-health", "ready": True,
                            "body_sha256": sha256_bytes(result.stdout), "body_bytes": len(result.stdout)}
                last = result.stderr.decode("utf-8", errors="replace")
            except AdapterError as exc:
                last = str(exc)
            time.sleep(0.1)
        raise AdapterError(f"{identity.role} readiness timed out: {last}")

    def request(self, host: str, port: int, body: bytes, output_tokens: int, timeout_seconds: int) -> RequestCapture:
        raw = self._required(
            host, ["python3", "-c", REMOTE_REQUEST_PROGRAM, f"http://127.0.0.1:{port}/completion",
                   str(output_tokens), str(timeout_seconds)], stdin=body, timeout=timeout_seconds + 10)
        try:
            value = json.loads(raw.decode("utf-8", errors="strict"))
            raw_http = base64.b64decode(value["raw_base64"], validate=True)
            return RequestCapture(
                value["response"], value["client"], raw_http, value["sent_body_sha256"],
                int(value["started_monotonic_ns"]), int(value["ended_monotonic_ns"]))
        except (KeyError, TypeError, ValueError, UnicodeError, json.JSONDecodeError) as exc:
            raise AdapterError("remote request capture is malformed") from exc

    def telemetry(self, host: str) -> dict[str, Any]:
        raw = self._required(host, ["python3", "-c", REMOTE_TELEMETRY_PROGRAM])
        try:
            value = json.loads(raw.decode("utf-8", errors="strict"))
        except (UnicodeError, json.JSONDecodeError) as exc:
            raise AdapterError("telemetry sample is malformed") from exc
        if not isinstance(value, dict) or not value.get("boot_id") or not isinstance(value.get("monotonic_ns"), int):
            raise AdapterError("telemetry sample lacks boot/monotonic authority")
        return value

    def stop_unit(self, identity: UnitIdentity, timeout_seconds: int) -> tuple[dict[str, Any], bytes]:
        before = parse_properties(self._required(identity.host, [
            "systemctl", "--user", "show", identity.unit, "-p", "ExecMainPID", "-p", "InvocationID",
            "-p", "ActiveState", "-p", "SubState", "-p", "ControlGroup", "-p", "FragmentPath",
        ]))
        require_properties(before, {
            "ExecMainPID", "InvocationID", "ActiveState", "SubState", "ControlGroup", "FragmentPath",
        }, "disposable terminal pre-state")
        if before.get("InvocationID", "").lower() != identity.invocation_id or \
                int(before.get("ExecMainPID", "0") or "0") != identity.pid:
            raise AdapterError("unit PID/InvocationID changed before terminal evidence")
        if before.get("ActiveState") == "active":
            self.prove_live(identity)
            self._required(identity.host, ["systemctl", "--user", "stop", identity.unit], timeout=timeout_seconds)
        elif before.get("ActiveState") not in {"inactive", "failed"}:
            raise AdapterError("unit entered an unsupported state before terminal evidence")
        props = parse_properties(self._required(identity.host, [
            "systemctl", "--user", "show", identity.unit, "-p", "ExecMainPID", "-p", "InvocationID",
            "-p", "ExecMainStatus", "-p", "Result", "-p", "ActiveState", "-p", "ControlGroup",
        ]))
        require_properties(props, {
            "ExecMainPID", "InvocationID", "ExecMainStatus", "Result", "ActiveState", "ControlGroup",
        }, "disposable terminal state")
        if props.get("InvocationID", "").lower() != identity.invocation_id:
            raise AdapterError("InvocationID changed before terminal evidence capture")
        journal = self._required(identity.host, [
            "journalctl", "--user", "-u", identity.unit,
            f"_SYSTEMD_INVOCATION_ID={identity.invocation_id}", "--after-cursor", identity.cursor_before,
            "--no-pager", "-o", "short-monotonic",
        ])
        if not journal.strip():
            raise AdapterError("InvocationID/cursor-bound journal is empty")
        if props.get("ActiveState") not in {"inactive", "failed"}:
            raise AdapterError("unit did not enter a terminal state")
        return ({"properties": props}, journal)

    def _path_absent(self, host: str, path: str, where: str) -> bool:
        """Distinguish a present path from a failed/ambiguous remote probe."""
        result = self._run(host, ["test", "!", "-e", path])
        if result.returncode == 0:
            return True
        if result.returncode == 1:
            return False
        detail = result.stderr.decode("utf-8", errors="replace").strip()
        raise AdapterError(
            f"{where} absence probe failed on {host} with rc={result.returncode}: {detail}")

    def cleanup_unit(
        self, host: str, unit: str, port: int, timeout_seconds: int,
        identity: UnitIdentity | None = None,
    ) -> dict[str, Any]:
        pre_result = self._run(host, [
            "systemctl", "--user", "show", unit, "-p", "LoadState", "-p", "ActiveState",
            "-p", "MainPID", "-p", "InvocationID", "-p", "ControlGroup", "-p", "FragmentPath",
        ])
        if pre_result.returncode != 0:
            raise AdapterError(f"cleanup pre-state probe failed: {host}/{unit}")
        pre = parse_properties(pre_result.stdout)
        require_properties(pre, {
            "LoadState", "ActiveState", "MainPID", "InvocationID", "ControlGroup", "FragmentPath",
        }, "cleanup pre-state proof")
        try:
            observed_pid = int(pre["MainPID"])
        except ValueError as exc:
            raise AdapterError("cleanup pre-state PID is malformed") from exc
        if observed_pid < 0:
            raise AdapterError("cleanup pre-state PID is malformed")

        captured_pid = identity.pid if identity is not None else None
        captured_control_group = identity.control_group if identity is not None else None
        identity_source = "provided" if identity is not None else "none"
        unit_was_loaded = pre["LoadState"] == "loaded"
        if pre["LoadState"] == "not-found":
            if pre["ActiveState"] != "inactive" or observed_pid != 0:
                raise AdapterError("not-found cleanup unit has live state")
        elif unit_was_loaded:
            observed_invocation = pre["InvocationID"].lower()
            observed_cgroup = pre["ControlGroup"]
            if observed_pid > 0:
                if not INVOCATION_RE.fullmatch(observed_invocation):
                    raise AdapterError("cleanup pre-state InvocationID is malformed")
                pure = PurePosixPath(observed_cgroup)
                if not pure.is_absolute() or pure.as_posix() != observed_cgroup or \
                        any(part in {".", ".."} for part in pure.parts) or \
                        not observed_cgroup.endswith(f"/{unit}") or \
                        not pre["FragmentPath"].endswith(f"/transient/{unit}"):
                    raise AdapterError("cleanup refuses an unowned unit/cgroup identity")
                process = self._process(host, observed_pid, retain_environment=False)
                if parse_unified_cgroup(
                        process["cgroup"], "cleanup process /proc cgroup proof") != observed_cgroup:
                    raise AdapterError("cleanup PID is outside the systemd control group")
                if identity is not None and (
                        observed_pid != identity.pid or
                        observed_invocation != identity.invocation_id or
                        observed_cgroup != identity.control_group or
                        process["process_start_ticks"] != identity.process_start_ticks):
                    raise AdapterError("cleanup refuses a changed disposable process identity")
                if identity is None:
                    captured_pid = observed_pid
                    captured_control_group = observed_cgroup
                    identity_source = "cleanup-pre-state"
            elif identity is not None:
                if observed_invocation and observed_invocation != identity.invocation_id:
                    raise AdapterError("cleanup terminal InvocationID differs from captured identity")
                if observed_cgroup and observed_cgroup != identity.control_group:
                    raise AdapterError("cleanup terminal control group differs from captured identity")
            elif observed_cgroup:
                pure = PurePosixPath(observed_cgroup)
                if not pure.is_absolute() or pure.as_posix() != observed_cgroup or \
                        any(part in {".", ".."} for part in pure.parts) or \
                        not observed_cgroup.endswith(f"/{unit}"):
                    raise AdapterError("cleanup terminal control group is malformed")
                captured_control_group = observed_cgroup
                identity_source = "cleanup-pre-state"
        else:
            raise AdapterError(f"cleanup unit has unsupported LoadState={pre['LoadState']}")

        if unit_was_loaded:
            stop = self._run(host, ["systemctl", "--user", "stop", unit], timeout=timeout_seconds)
            reset = self._run(host, ["systemctl", "--user", "reset-failed", unit], timeout=timeout_seconds)
        else:
            stop = CommandResult(0, b"", b"")
            reset = CommandResult(0, b"", b"")
        deadline = time.monotonic() + timeout_seconds
        last: dict[str, str] = {}
        while time.monotonic() < deadline:
            result = self._run(host, ["systemctl", "--user", "show", unit, "-p", "LoadState", "-p", "ActiveState", "-p", "MainPID"])
            if result.returncode != 0:
                raise AdapterError(f"cleanup unit probe failed: {host}/{unit}")
            last = parse_properties(result.stdout)
            require_properties(last, {"LoadState", "ActiveState", "MainPID"}, "cleanup unit proof")
            pid_absent = None if captured_pid is None else self._path_absent(
                host, f"/proc/{captured_pid}", "captured PID")
            cgroup_absent = None if captured_control_group is None else self._path_absent(
                host, f"/sys/fs/cgroup{captured_control_group}", "captured cgroup")
            if last["LoadState"] == "not-found" and last["ActiveState"] == "inactive" and \
                    last["MainPID"] == "0" and not self.port_owners(host, port) and \
                    pid_absent is not False and cgroup_absent is not False:
                if stop.returncode != 0 or reset.returncode != 0:
                    raise AdapterError(f"cleanup stop/reset failed despite eventual absence: {host}/{unit}")
                return {"unit_absent": True, "port_closed": True, "stop_returncode": stop.returncode,
                        "reset_returncode": reset.returncode,
                        "captured_pid_absent": pid_absent,
                        "captured_cgroup_absent": cgroup_absent,
                        "captured_pid": captured_pid,
                        "captured_control_group": captured_control_group,
                        "identity_source": identity_source,
                        "pre_state": pre}
            time.sleep(0.05)
        raise AdapterError(f"cleanup did not unload unit/close port: {host}/{unit}: {last}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate = subparsers.add_parser("validate", help="validate frozen run and issue #37 policy without target mutation")
    validate.add_argument("run_root", type=Path)
    validate.add_argument("policy", type=Path)
    execute = subparsers.add_parser(
        "execute-next",
        help="disabled draft command; issue #41 admission/recovery custody is unresolved",
    )
    execute.add_argument("run_root", type=Path)
    execute.add_argument("policy", type=Path)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        plan = core.load_plan(args.run_root / "plan.json")
        schedule = core.validate_run_contract(args.run_root, plan)
        policy = load_policy(args.policy, plan)
        input_bindings(args.run_root, plan)
        issue41 = load_issue41_authority()
        index, entry, _ = next_schedule_entry(
            args.run_root, schedule, policy_digest(args.policy))
        if args.command == "validate":
            print(json.dumps({
                "schema": POLICY_SCHEMA, "issue": 37, "experiment_id": plan["experiment_id"],
                "plan_sha256": core.plan_digest(plan), "policy_sha256": policy_digest(args.policy),
                "next_schedule_index": index, "next_entry": entry,
                "controller_host": policy.controller_host,
                "issue_41_authority": issue41,
                "execution_qualified": False,
                "measurement_ready": False,
                "performance_claim": False,
            }, sort_keys=True))
        else:
            if not TARGET_EXECUTION_ENABLED:
                raise AdapterError(
                    "target execute-next is disabled; local validation reports the exact "
                    "unresolved issue #41 admission/recovery custody")
            observed = socket.gethostname().split(".", 1)[0].lower()
            expected = policy.controller_host.split(".", 1)[0].lower()
            if observed != expected:
                raise AdapterError(f"execute-next must run on {policy.controller_host}, observed {observed}")
            receipt = execute_next(args.run_root, args.policy, SshCachyRunner())
            print(receipt)
    except (AdapterError, core.PlanError, OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
