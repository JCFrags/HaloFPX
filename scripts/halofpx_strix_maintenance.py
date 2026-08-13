#!/usr/bin/env python3
"""Offline-only fail-closed Strix Halo maintenance admission controller.

The module defines and tests the complete issue-#41 maintenance transaction,
but deliberately defines and constructs no target Runner.  The public CLI
validates frozen authorization/policy inputs only.  ``execute_offline_domain`` exists solely so
deterministic fakes can prove ordering, cleanup, recovery, and evidence custody.
"""

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
import hashlib
import importlib.util
import json
import os
import re
import sys
from pathlib import Path
from typing import Any, Protocol, Sequence


def _load_sibling(name: str, filename: str) -> Any:
    try:
        return __import__(name)
    except ModuleNotFoundError:
        path = Path(__file__).with_name(filename)
        spec = importlib.util.spec_from_file_location(name, path)
        if spec is None or spec.loader is None:
            raise
        module = importlib.util.module_from_spec(spec)
        sys.modules[name] = module
        try:
            spec.loader.exec_module(module)
        except BaseException:
            sys.modules.pop(name, None)
            raise
        return module


core = _load_sibling("halofpx_strix_ab", "halofpx_strix_ab.py")
adapter = _load_sibling("halofpx_strix_ab_cachyos", "halofpx_strix_ab_cachyos.py")


AUTHORIZATION_SCHEMA = "halofpx.strix-maintenance-authorization.v1"
POLICY_SCHEMA = "halofpx.strix-maintenance-policy.v1"
TERMINAL_SCHEMA = "halofpx.strix-maintenance-terminal.v1"
INTENT_SCHEMA = "halofpx.strix-maintenance-intent.v1"
EVENT_SCHEMA = "halofpx.strix-maintenance-event.v1"
GPU_CENSUS_SCHEMA = "halofpx.strix-maintenance-gpu-census.v1"
KERNEL_BASELINE_SCHEMA = "halofpx.strix-maintenance-kernel-baseline.v1"
PROBE_SCHEMA = "halofpx.strix-maintenance-two-rank-probe.v1"
ISSUE41_TRACKER = "https://github.com/JCFrags/HaloFPX/issues/41"
ISSUE41_MANIFEST_RELATIVE = adapter.ISSUE41_MANIFEST_RELATIVE
ISSUE41_MANIFEST_SHA256 = adapter.ISSUE41_MANIFEST_SHA256
APPROVAL_STATEMENT = (
    "I authorize only the exact bounded HaloFPX maintenance transaction described "
    "by this receipt; any identity, digest, time-window, or scope mismatch refuses."
)
OFFLINE_EXAMPLE_STATEMENT = (
    "This tracked receipt authorizes offline domain simulation only and does not "
    "authorize access to or mutation of nimo-1 or nimo-2."
)
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")
INVOCATION_RE = adapter.INVOCATION_RE
NONCE_RE = re.compile(r"^[A-Za-z0-9_-]{32,128}$")
GITHUB_NODE_RE = re.compile(r"^(IC|I|MDEy)_[A-Za-z0-9_-]{8,128}$")
UNIT_RE = adapter.UNIT_RE
PROTECTED_UNITS = dict(adapter.PROTECTED_UNITS)
PROTECTED_PORTS = {"coordinator": 8081, "worker": 50052}
TARGET_EXECUTION_ENABLED = False
MAX_WINDOW = dt.timedelta(hours=8)


class MaintenanceError(RuntimeError):
    pass


class MaintenanceRunFailed(MaintenanceError):
    def __init__(self, terminal_path: Path, errors: list[dict[str, str]]):
        self.terminal_path = terminal_path
        self.errors = errors
        detail = "; ".join(f"{item['stage']}: {item['detail']}" for item in errors)
        super().__init__(detail or "maintenance transaction failed")


def canonical_bytes(value: Any) -> bytes:
    return core.canonical_bytes(value)


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def require_exact(value: Any, keys: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != keys:
        raise MaintenanceError(f"{where} has the wrong closed field set")
    return value


def require_string(value: Any, where: str) -> str:
    try:
        return core.require_string(value, where)
    except core.PlanError as exc:
        raise MaintenanceError(str(exc)) from exc


def require_int(value: Any, where: str, minimum: int = 0) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum:
        raise MaintenanceError(f"{where} must be an integer >= {minimum}")
    return value


def require_hash(value: Any, where: str, pattern: re.Pattern[str] = SHA256_RE) -> str:
    text = require_string(value, where)
    if pattern.fullmatch(text) is None:
        raise MaintenanceError(f"{where} has the wrong digest/identity format")
    return text


def parse_closed_json(content: bytes, where: str) -> dict[str, Any]:
    try:
        value = json.loads(
            content.decode("utf-8", errors="strict"),
            object_pairs_hook=core.unique_json_object,
            parse_constant=core.reject_json_constant,
        )
    except (UnicodeError, json.JSONDecodeError, ValueError) as exc:
        raise MaintenanceError(f"{where} is unreadable: {exc}") from exc
    if not isinstance(value, dict):
        raise MaintenanceError(f"{where} must be an object")
    return value


def read_regular(path: Path, where: str) -> bytes:
    try:
        return core.read_regular_bytes(path, where)
    except core.PlanError as exc:
        raise MaintenanceError(str(exc)) from exc


def parse_utc(value: Any, where: str) -> dt.datetime:
    text = require_string(value, where)
    if not text.endswith("Z"):
        raise MaintenanceError(f"{where} must use canonical UTC Z time")
    try:
        parsed = dt.datetime.fromisoformat(text[:-1] + "+00:00")
    except ValueError as exc:
        raise MaintenanceError(f"{where} is not RFC3339-compatible UTC") from exc
    if parsed.tzinfo != dt.timezone.utc or parsed.microsecond != 0:
        raise MaintenanceError(f"{where} must be whole-second UTC")
    if parsed.strftime("%Y-%m-%dT%H:%M:%SZ") != text:
        raise MaintenanceError(f"{where} is not canonical UTC")
    return parsed


def path_text(path: Path) -> str:
    return str(path.resolve())


def require_absolute_path(value: Any, where: str) -> str:
    text = require_string(value, where)
    path = Path(text)
    if not path.is_absolute() or path_text(path) != text:
        raise MaintenanceError(f"{where} must be a resolved absolute path")
    return text


@dataclasses.dataclass(frozen=True)
class ProductionIdentity:
    role: str
    host: str
    unit: str
    pid: int
    invocation_id: str
    nrestarts: int
    process_start_ticks: int
    start_monotonic_us: int
    executable_sha256: str
    argv_sha256: str
    control_group: str
    listener_port: int
    listener_pid: int
    health_sha256: str | None

    @property
    def digest(self) -> str:
        return sha256_bytes(canonical_bytes(dataclasses.asdict(self)))


@dataclasses.dataclass(frozen=True)
class AuthorityReference:
    repository: str
    issue_number: int
    kind: str
    url: str
    node_id: str
    issuer_login: str
    issuer_account_id: int
    owner_login: str
    owner_account_id: int


@dataclasses.dataclass(frozen=True)
class Authorization:
    raw_sha256: str
    authorization_id: str
    execution_scope: str
    authority: AuthorityReference
    not_before: dt.datetime
    expires: dt.datetime
    nonce: str
    repository_commit: str
    incident_sha256: str
    adapter_plan_sha256: str
    adapter_policy_sha256: str
    schedule_index: int
    evidence_root: str
    unit_prefix: str
    coordinator_port: int
    worker_port: int
    production: dict[str, ProductionIdentity]
    request_sha256: str
    prompt_tokens: int
    generated_tokens: int


@dataclasses.dataclass(frozen=True)
class Policy:
    authorization_sha256: str
    authority: AuthorityReference
    repository_commit: str
    incident_path: str
    incident_sha256: str
    adapter_plan_path: str
    adapter_plan_sha256: str
    adapter_policy_path: str
    adapter_policy_sha256: str
    schedule_index: int
    stop_timeout_seconds: int
    start_timeout_seconds: int
    cleanup_timeout_seconds: int
    request_timeout_seconds: int


def parse_identity(value: Any, role: str, where: str) -> ProductionIdentity:
    item = require_exact(value, {
        "role", "host", "unit", "pid", "invocation_id", "nrestarts",
        "process_start_ticks", "start_monotonic_us", "executable_sha256",
        "argv_sha256", "control_group", "listener_port", "listener_pid",
        "health_sha256",
    }, where)
    if item["role"] != role:
        raise MaintenanceError(f"{where}.role differs from its map role")
    host = require_string(item["host"], f"{where}.host")
    expected_host = "nimo-1" if role == "coordinator" else "nimo-2"
    if host != expected_host:
        raise MaintenanceError(f"{where}.host differs from the frozen topology")
    unit = require_string(item["unit"], f"{where}.unit")
    if unit != PROTECTED_UNITS[role] or UNIT_RE.fullmatch(unit) is None:
        raise MaintenanceError(f"{where}.unit differs from protected authority")
    pid = require_int(item["pid"], f"{where}.pid", 1)
    invocation = require_string(item["invocation_id"], f"{where}.invocation_id").lower()
    if INVOCATION_RE.fullmatch(invocation) is None:
        raise MaintenanceError(f"{where}.invocation_id is malformed")
    control_group = require_string(item["control_group"], f"{where}.control_group")
    if not control_group.startswith("/") or not control_group.endswith("/" + unit) or ".." in control_group.split("/"):
        raise MaintenanceError(f"{where}.control_group is not the protected unit cgroup")
    port = require_int(item["listener_port"], f"{where}.listener_port", 1024)
    if port != PROTECTED_PORTS[role] or item["listener_pid"] != pid:
        raise MaintenanceError(f"{where} listener identity differs from the protected PID/port")
    health = item["health_sha256"]
    if role == "coordinator":
        health = require_hash(health, f"{where}.health_sha256")
    elif health is not None:
        raise MaintenanceError(f"{where}.health_sha256 must be null for worker")
    return ProductionIdentity(
        role, host, unit, pid, invocation,
        require_int(item["nrestarts"], f"{where}.nrestarts"),
        require_int(item["process_start_ticks"], f"{where}.process_start_ticks", 1),
        require_int(item["start_monotonic_us"], f"{where}.start_monotonic_us", 1),
        require_hash(item["executable_sha256"], f"{where}.executable_sha256"),
        require_hash(item["argv_sha256"], f"{where}.argv_sha256"),
        control_group, port, pid, health,
    )


def parse_authority(value: Any, where: str) -> AuthorityReference:
    item = require_exact(value, {
        "repository", "issue_number", "kind", "url", "node_id",
        "issuer_login", "issuer_account_id", "owner_login", "owner_account_id",
    }, where)
    if item["repository"] != "JCFrags/HaloFPX" or item["issue_number"] != 41 or \
            item["kind"] != "github_issue_comment":
        raise MaintenanceError(f"{where} is not the issue #41 GitHub comment authority")
    url = require_string(item["url"], f"{where}.url")
    if not url.startswith(ISSUE41_TRACKER + "#issuecomment-"):
        raise MaintenanceError(f"{where}.url is not an issue #41 comment URL")
    node = require_string(item["node_id"], f"{where}.node_id")
    if GITHUB_NODE_RE.fullmatch(node) is None:
        raise MaintenanceError(f"{where}.node_id is malformed")
    issuer = require_string(item["issuer_login"], f"{where}.issuer_login")
    owner = require_string(item["owner_login"], f"{where}.owner_login")
    if issuer != "JCFrags" or owner != "JCFrags":
        raise MaintenanceError(f"{where} issuer/owner differs from repository authority")
    issuer_id = require_int(item["issuer_account_id"], f"{where}.issuer_account_id", 1)
    owner_id = require_int(item["owner_account_id"], f"{where}.owner_account_id", 1)
    if issuer_id != owner_id:
        raise MaintenanceError(f"{where} issuer and owner account identities differ")
    return AuthorityReference(
        item["repository"], item["issue_number"], item["kind"], url, node,
        issuer, issuer_id, owner, owner_id,
    )


def load_authorization_bytes(
    content: bytes, *, expected_sha256: str, now: dt.datetime,
) -> Authorization:
    observed = sha256_bytes(content)
    if observed != expected_sha256:
        raise MaintenanceError("authorization bytes differ from the tracked policy digest")
    raw = parse_closed_json(content, "authorization receipt")
    require_exact(raw, {
        "schema", "authorization_id", "issue", "execution_scope",
        "approval_statement", "authority", "window", "nonce", "repository",
        "incident", "adapter", "allowed_disposable", "production_before",
        "recovery_probe",
    }, "authorization receipt")
    if raw["schema"] != AUTHORIZATION_SCHEMA or raw["issue"] != 41:
        raise MaintenanceError("authorization receipt schema/issue differs from v1/#41")
    authorization_id = require_string(raw["authorization_id"], "authorization_id")
    scope = require_string(raw["execution_scope"], "execution_scope")
    if scope not in {"offline-domain-simulation", "dual-strix-maintenance"}:
        raise MaintenanceError("authorization execution_scope is unsupported")
    expected_statement = OFFLINE_EXAMPLE_STATEMENT if scope == "offline-domain-simulation" else APPROVAL_STATEMENT
    if raw["approval_statement"] != expected_statement:
        raise MaintenanceError("authorization approval statement is not exact")
    authority = parse_authority(raw["authority"], "authorization.authority")
    window = require_exact(raw["window"], {"not_before_utc", "expires_utc"}, "authorization.window")
    not_before = parse_utc(window["not_before_utc"], "authorization.window.not_before_utc")
    expires = parse_utc(window["expires_utc"], "authorization.window.expires_utc")
    if expires <= not_before or expires - not_before > MAX_WINDOW:
        raise MaintenanceError("authorization window is empty or exceeds eight hours")
    if now.tzinfo != dt.timezone.utc or now < not_before or now >= expires:
        raise MaintenanceError("authorization is not active at the supplied UTC instant")
    nonce = require_string(raw["nonce"], "authorization.nonce")
    if NONCE_RE.fullmatch(nonce) is None:
        raise MaintenanceError("authorization nonce is malformed")
    repository = require_exact(raw["repository"], {"url", "commit"}, "authorization.repository")
    if repository["url"] != "https://github.com/JCFrags/HaloFPX.git":
        raise MaintenanceError("authorization repository URL differs from canonical origin")
    commit = require_hash(repository["commit"], "authorization.repository.commit", COMMIT_RE)
    incident = require_exact(raw["incident"], {"manifest_path", "manifest_sha256"}, "authorization.incident")
    if incident["manifest_path"] != ISSUE41_MANIFEST_RELATIVE.as_posix() or \
            incident["manifest_sha256"] != ISSUE41_MANIFEST_SHA256:
        raise MaintenanceError("authorization incident authority differs from issue #41")
    ab = require_exact(raw["adapter"], {
        "plan_sha256", "policy_sha256", "schedule_index"}, "authorization.adapter")
    schedule_index = require_int(ab["schedule_index"], "authorization.adapter.schedule_index")
    disposable = require_exact(raw["allowed_disposable"], {
        "evidence_root", "unit_prefix", "coordinator_port", "worker_port"},
        "authorization.allowed_disposable")
    evidence_root = require_absolute_path(disposable["evidence_root"], "allowed_disposable.evidence_root")
    if disposable["unit_prefix"] != "halofpx-ab-":
        raise MaintenanceError("authorization unit prefix differs from PR51 authority")
    coordinator_port = require_int(disposable["coordinator_port"], "coordinator_port", 1024)
    worker_port = require_int(disposable["worker_port"], "worker_port", 1024)
    if coordinator_port == worker_port or {coordinator_port, worker_port} & set(PROTECTED_PORTS.values()):
        raise MaintenanceError("authorization disposable ports collide")
    production = require_exact(raw["production_before"], {"coordinator", "worker"}, "production_before")
    identities = {role: parse_identity(production[role], role, f"production_before.{role}")
                  for role in ("coordinator", "worker")}
    probe = require_exact(raw["recovery_probe"], {
        "request_sha256", "prompt_tokens", "generated_tokens", "world_size",
        "performance_result"}, "authorization.recovery_probe")
    if probe["world_size"] != 2 or probe["performance_result"] is not False:
        raise MaintenanceError("authorization recovery probe must be two-rank and non-performance")
    return Authorization(
        observed, authorization_id, scope, authority, not_before, expires, nonce,
        commit, incident["manifest_sha256"],
        require_hash(ab["plan_sha256"], "authorization.adapter.plan_sha256"),
        require_hash(ab["policy_sha256"], "authorization.adapter.policy_sha256"),
        schedule_index, evidence_root, disposable["unit_prefix"], coordinator_port,
        worker_port, identities,
        require_hash(probe["request_sha256"], "recovery_probe.request_sha256"),
        require_int(probe["prompt_tokens"], "recovery_probe.prompt_tokens", 1),
        require_int(probe["generated_tokens"], "recovery_probe.generated_tokens", 1),
    )


def load_policy_bytes(content: bytes) -> Policy:
    raw = parse_closed_json(content, "maintenance policy")
    require_exact(raw, {
        "schema", "issue", "target_execution_enabled", "authorization",
        "repository", "incident", "adapter", "timeouts"}, "maintenance policy")
    if raw["schema"] != POLICY_SCHEMA or raw["issue"] != 41 or \
            raw["target_execution_enabled"] is not False:
        raise MaintenanceError("maintenance policy must be issue #41 v1 and target-disabled")
    auth = require_exact(raw["authorization"], {"receipt_sha256", "authority"}, "policy.authorization")
    authority = parse_authority(auth["authority"], "policy.authorization.authority")
    repository = require_exact(raw["repository"], {"url", "commit"}, "policy.repository")
    if repository["url"] != "https://github.com/JCFrags/HaloFPX.git":
        raise MaintenanceError("maintenance policy repository URL differs from canonical origin")
    incident = require_exact(raw["incident"], {"manifest_path", "manifest_sha256"}, "policy.incident")
    ab = require_exact(raw["adapter"], {
        "plan_path", "plan_sha256", "policy_path", "policy_sha256", "schedule_index"},
        "policy.adapter")
    timeouts = require_exact(raw["timeouts"], {
        "stop_seconds", "start_seconds", "cleanup_seconds", "request_seconds"},
        "policy.timeouts")
    checked_timeouts: list[int] = []
    for name in ("stop_seconds", "start_seconds", "cleanup_seconds", "request_seconds"):
        value = require_int(timeouts[name], f"policy.timeouts.{name}", 1)
        if value > 3600:
            raise MaintenanceError(f"policy.timeouts.{name} exceeds 3600")
        checked_timeouts.append(value)
    return Policy(
        require_hash(auth["receipt_sha256"], "policy.authorization.receipt_sha256"),
        authority,
        require_hash(repository["commit"], "policy.repository.commit", COMMIT_RE),
        require_string(incident["manifest_path"], "policy.incident.manifest_path"),
        require_hash(incident["manifest_sha256"], "policy.incident.manifest_sha256"),
        require_string(ab["plan_path"], "policy.adapter.plan_path"),
        require_hash(ab["plan_sha256"], "policy.adapter.plan_sha256"),
        require_string(ab["policy_path"], "policy.adapter.policy_path"),
        require_hash(ab["policy_sha256"], "policy.adapter.policy_sha256"),
        require_int(ab["schedule_index"], "policy.adapter.schedule_index"),
        *checked_timeouts,
    )


def _resolve_repository_file(root: Path, relative: str, where: str) -> Path:
    value = Path(relative)
    if value.is_absolute() or not value.parts or any(part in {"", ".", ".."} for part in value.parts):
        raise MaintenanceError(f"{where} must be a canonical repository-relative path")
    candidate = root.joinpath(*value.parts)
    resolved_root = root.resolve()
    resolved = candidate.resolve()
    try:
        resolved.relative_to(resolved_root)
    except ValueError as exc:
        raise MaintenanceError(f"{where} escapes the repository") from exc
    if resolved != candidate.absolute():
        raise MaintenanceError(f"{where} traverses a symlink or junction")
    return resolved


def validate_inputs(
    repository_root: Path, policy_path: Path, authorization_path: Path, *, now: dt.datetime,
) -> tuple[bytes, Policy, bytes, Authorization, dict[str, Any]]:
    root = repository_root.resolve()
    policy_bytes = read_regular(policy_path, "maintenance policy")
    policy = load_policy_bytes(policy_bytes)
    authorization_bytes = read_regular(authorization_path, "authorization receipt")
    authorization = load_authorization_bytes(
        authorization_bytes, expected_sha256=policy.authorization_sha256, now=now)
    if authorization.authority != policy.authority:
        raise MaintenanceError("authorization authority differs from the tracked policy")
    if authorization.repository_commit != policy.repository_commit:
        raise MaintenanceError("authorization repository commit differs from policy")
    if policy.incident_path != ISSUE41_MANIFEST_RELATIVE.as_posix() or \
            policy.incident_sha256 != ISSUE41_MANIFEST_SHA256 or \
            authorization.incident_sha256 != ISSUE41_MANIFEST_SHA256:
        raise MaintenanceError("policy/authorization incident binding differs from issue #41")
    incident_path = _resolve_repository_file(root, policy.incident_path, "policy.incident.manifest_path")
    incident_bytes = read_regular(incident_path, "issue #41 incident manifest")
    if sha256_bytes(incident_bytes) != ISSUE41_MANIFEST_SHA256:
        raise MaintenanceError("issue #41 incident manifest bytes changed")
    plan_path = _resolve_repository_file(root, policy.adapter_plan_path, "policy.adapter.plan_path")
    adapter_policy_path = _resolve_repository_file(root, policy.adapter_policy_path, "policy.adapter.policy_path")
    plan_bytes = read_regular(plan_path, "adapter plan")
    adapter_policy_bytes = read_regular(adapter_policy_path, "adapter policy")
    if sha256_bytes(plan_bytes) != policy.adapter_plan_sha256 or \
            sha256_bytes(adapter_policy_bytes) != policy.adapter_policy_sha256:
        raise MaintenanceError("PR51 adapter plan/policy raw bytes differ from maintenance policy")
    if authorization.adapter_plan_sha256 != policy.adapter_plan_sha256 or \
            authorization.adapter_policy_sha256 != policy.adapter_policy_sha256 or \
            authorization.schedule_index != policy.schedule_index:
        raise MaintenanceError("authorization PR51 adapter binding differs from policy")
    plan = core.validate_plan(parse_closed_json(plan_bytes, "adapter plan"))
    adapter_policy = adapter.load_policy_bytes(adapter_policy_bytes, plan)
    if adapter_policy.unit_prefix != authorization.unit_prefix or \
            adapter_policy.coordinator_port != authorization.coordinator_port or \
            adapter_policy.worker_port != authorization.worker_port:
        raise MaintenanceError("authorization disposable authority differs from PR51 policy")
    schedule = core.make_schedule(plan)
    if policy.schedule_index >= len(schedule["entries"]):
        raise MaintenanceError("authorized PR51 schedule index is outside the frozen schedule")
    return policy_bytes, policy, authorization_bytes, authorization, {
        "plan": plan,
        "schedule": schedule,
        "plan_path": plan_path,
        "adapter_policy_path": adapter_policy_path,
        "adapter_policy": adapter_policy,
    }


class OfflineRunner(Protocol):
    """Domain seam for fake-only qualification; no real implementation ships."""

    offline_fake: bool

    def snapshot_production(self) -> dict[str, Any]: ...
    def kernel_baseline(self) -> dict[str, Any]: ...
    def gpu_census(self) -> dict[str, Any]: ...
    def stop_production(self, identity: ProductionIdentity, timeout_seconds: int) -> dict[str, Any]: ...
    def prove_production_absent(self, role: str) -> dict[str, Any]: ...
    def run_adapter(self, plan_path: Path, policy_path: Path, evidence_root: Path) -> bytes: ...
    def cleanup_adapter(self, timeout_seconds: int) -> dict[str, Any]: ...
    def start_production(self, role: str, timeout_seconds: int) -> dict[str, Any]: ...
    def prove_recovery_ready(self, identity: ProductionIdentity, timeout_seconds: int) -> dict[str, Any]: ...
    def minimal_two_rank_inference(
        self, coordinator: ProductionIdentity, worker: ProductionIdentity,
        request_sha256: str, timeout_seconds: int,
    ) -> dict[str, Any]: ...


class EvidenceCustody:
    def __init__(self, root: Path, authorization_bytes: bytes, policy_bytes: bytes):
        if root.exists():
            raise MaintenanceError("maintenance evidence root must not already exist")
        root.mkdir(parents=True, mode=0o700)
        self.root = root
        self.index = 0
        self._write_bytes(root / "authorization.raw.json", authorization_bytes)
        self._write_bytes(root / "policy.raw.json", policy_bytes)

    @staticmethod
    def _sync_parent(path: Path) -> None:
        if os.name != "posix":
            return
        fd = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)

    def _write_bytes(self, path: Path, content: bytes) -> None:
        path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
        with path.open("xb") as handle:
            handle.write(content)
            handle.flush()
            os.fsync(handle.fileno())
        self._sync_parent(path)

    def write_json(self, name: str, value: Any) -> Path:
        path = self.root / name
        self._write_bytes(path, json.dumps(value, indent=2, sort_keys=True).encode("utf-8") + b"\n")
        return path

    def event(self, stage: str, status: str, observation: Any) -> Path:
        index = self.index
        self.index += 1
        safe = re.sub(r"[^a-z0-9]+", "-", stage.lower()).strip("-")[:80]
        return self.write_json(f"events/{index:03d}-{safe}.json", {
            "schema": EVENT_SCHEMA, "index": index, "stage": stage,
            "status": status, "observation": observation,
        })

    def finalize_hashes(self) -> Path:
        rows = []
        for path in sorted(self.root.rglob("*")):
            if path.is_file() and path.name != "SHA256SUMS":
                content = path.read_bytes()
                rows.append(f"{sha256_bytes(content)}  {path.relative_to(self.root).as_posix()}\n")
        path = self.root / "SHA256SUMS"
        self._write_bytes(path, "".join(rows).encode("utf-8"))
        return path


def parse_snapshot(value: Any) -> dict[str, ProductionIdentity]:
    root = require_exact(value, {"schema", "complete", "roles", "errors"}, "production snapshot")
    if root["schema"] != "halofpx.strix-maintenance-production-snapshot.v1" or \
            root["complete"] is not True or root["errors"] != []:
        raise MaintenanceError("production snapshot is incomplete")
    roles = require_exact(root["roles"], {"coordinator", "worker"}, "production snapshot roles")
    return {role: parse_identity(roles[role], role, f"production snapshot.{role}")
            for role in ("coordinator", "worker")}


def validate_census(
    value: Any, expected: dict[str, set[int] | tuple[set[int], ...]], where: str,
    *, identities: Sequence[ProductionIdentity] = (),
) -> dict[str, Any]:
    root = require_exact(value, {"schema", "elevated", "complete", "hosts", "errors"}, where)
    if root["schema"] != GPU_CENSUS_SCHEMA or root["elevated"] is not True or \
            root["complete"] is not True or root["errors"] != []:
        raise MaintenanceError(f"{where} is not a complete elevated census")
    hosts = require_exact(root["hosts"], {"nimo-1", "nimo-2"}, f"{where}.hosts")
    identity_by_host_pid = {(identity.host, identity.pid): identity for identity in identities}
    for host, allowed in expected.items():
        item = require_exact(hosts[host], {"devices", "owners", "errors"}, f"{where}.{host}")
        devices = item["devices"]
        if not isinstance(devices, list) or "/dev/kfd" not in devices or \
                not any(isinstance(path, str) and path.startswith("/dev/dri/renderD") for path in devices) or \
                item["errors"] != []:
            raise MaintenanceError(f"{where}.{host} device set/custody is incomplete")
        owners = item["owners"]
        if not isinstance(owners, list):
            raise MaintenanceError(f"{where}.{host}.owners is not a list")
        pids: list[int] = []
        for index, owner in enumerate(owners):
            row = require_exact(owner, {"pid", "unit", "control_group", "gpu_active_kib"},
                                f"{where}.{host}.owners[{index}]")
            pid = require_int(row["pid"], "GPU owner PID", 1)
            pids.append(pid)
            unit = require_string(row["unit"], "GPU owner unit")
            control_group = require_string(row["control_group"], "GPU owner cgroup")
            require_int(row["gpu_active_kib"], "GPU owner gpu_active_kib")
            identity = identity_by_host_pid.get((host, pid))
            if identity is not None and (unit != identity.unit or control_group != identity.control_group):
                raise MaintenanceError(f"{where}.{host} owner authority differs from frozen identity")
        allowed_sets = (allowed,) if isinstance(allowed, set) else allowed
        if len(pids) != len(set(pids)) or not any(set(pids) == option for option in allowed_sets):
            raise MaintenanceError(f"{where}.{host} owners differ from exact protected set")
    return root


def census_pids(value: dict[str, Any]) -> dict[str, set[int]]:
    return {
        host: {owner["pid"] for owner in value["hosts"][host]["owners"]}
        for host in ("nimo-1", "nimo-2")
    }


def validate_kernel(value: Any, where: str) -> dict[str, Any]:
    root = require_exact(value, {"schema", "complete", "hosts", "errors"}, where)
    if root["schema"] != KERNEL_BASELINE_SCHEMA or root["complete"] is not True or root["errors"] != []:
        raise MaintenanceError(f"{where} is incomplete")
    hosts = require_exact(root["hosts"], {"nimo-1", "nimo-2"}, f"{where}.hosts")
    for host, item in hosts.items():
        row = require_exact(item, {
            "boot_id", "monotonic_ns", "journal_cursor", "global_oom_count",
            "oom_kill_count", "amdgpu_fault_count", "kfd_fault_count",
            "gpu_reset_count", "errors"}, f"{where}.{host}")
        require_string(row["boot_id"], f"{where}.{host}.boot_id")
        require_int(row["monotonic_ns"], f"{where}.{host}.monotonic_ns", 1)
        require_string(row["journal_cursor"], f"{where}.{host}.journal_cursor")
        for name in ("global_oom_count", "oom_kill_count", "amdgpu_fault_count", "kfd_fault_count", "gpu_reset_count"):
            require_int(row[name], f"{where}.{host}.{name}")
        if row["errors"] != []:
            raise MaintenanceError(f"{where}.{host} contains collection errors")
    return root


def compare_kernel(before: dict[str, Any], after: dict[str, Any]) -> None:
    for host in ("nimo-1", "nimo-2"):
        old = before["hosts"][host]
        new = after["hosts"][host]
        if new["boot_id"] != old["boot_id"] or new["monotonic_ns"] <= old["monotonic_ns"]:
            raise MaintenanceError(f"{host} rebooted or kernel baseline did not advance")
        for name in ("global_oom_count", "oom_kill_count", "amdgpu_fault_count", "kfd_fault_count", "gpu_reset_count"):
            if new[name] != old[name]:
                raise MaintenanceError(f"{host} kernel {name} changed during maintenance")


def validate_stop(value: Any, identity: ProductionIdentity, where: str) -> dict[str, Any]:
    row = require_exact(value, {
        "host", "unit", "stopped_identity_sha256", "active", "main_pid",
        "listener_pids", "control_group_absent"}, where)
    if row != {
        "host": identity.host, "unit": identity.unit,
        "stopped_identity_sha256": identity.digest, "active": False,
        "main_pid": 0, "listener_pids": [], "control_group_absent": True,
    }:
        raise MaintenanceError(f"{where} does not prove exact stopped identity/absence")
    return row


def validate_absent(value: Any, role: str, where: str) -> dict[str, Any]:
    row = require_exact(value, {"role", "unit", "active", "main_pid", "listener_pids"}, where)
    if row != {"role": role, "unit": PROTECTED_UNITS[role], "active": False,
               "main_pid": 0, "listener_pids": []}:
        raise MaintenanceError(f"{where} does not prove protected absence")
    return row


def validate_new_identity(value: Any, role: str, before: ProductionIdentity) -> ProductionIdentity:
    current = parse_identity(value, role, f"recovered {role}")
    if current.pid == before.pid or current.invocation_id == before.invocation_id or \
            current.process_start_ticks == before.process_start_ticks or \
            current.start_monotonic_us == before.start_monotonic_us:
        raise MaintenanceError(f"recovered {role} identity is not fresh")
    if current.nrestarts != before.nrestarts or \
            current.executable_sha256 != before.executable_sha256 or \
            current.argv_sha256 != before.argv_sha256:
        raise MaintenanceError(f"recovered {role} artifact/argv/restart authority changed")
    return current


def validate_ready(value: Any, identity: ProductionIdentity, where: str) -> dict[str, Any]:
    keys = {"role", "identity_sha256", "listener_pids", "ready", "health", "rpc_protocol"}
    row = require_exact(value, keys, where)
    if row["role"] != identity.role or row["identity_sha256"] != identity.digest or \
            row["listener_pids"] != [identity.pid] or row["ready"] is not True:
        raise MaintenanceError(f"{where} is not bound to the recovered identity/listener")
    if identity.role == "worker":
        if row["rpc_protocol"] != "4.0.1" or row["health"] is not None:
            raise MaintenanceError("worker recovery lacks RPC HELLO readiness")
    elif not isinstance(row["health"], dict) or row["health"].get("status") != 200 or \
            require_hash(row["health"].get("body_sha256"), "coordinator health hash") != identity.health_sha256 or \
            row["rpc_protocol"] is not None:
        raise MaintenanceError("coordinator recovery lacks exact HTTP health")
    return row


def validate_adapter_receipt(content: bytes, policy: Policy, context: dict[str, Any]) -> dict[str, Any]:
    value = parse_closed_json(content, "PR51 adapter receipt")
    required = {
        "schema", "issue", "experiment_id", "plan_sha256", "policy_sha256",
        "policy_binding", "schedule_index", "entry", "input_bindings", "model_binding",
        "preflight_sha256", "production_before", "production_after",
        "gpu_admission_before_intent", "model_binding_after", "cycles", "errors",
        "outcome", "execution_qualified", "measurement_ready", "performance_claim",
    }
    require_exact(value, required, "PR51 adapter receipt")
    plan = context["plan"]
    expected_entry = context["schedule"]["entries"][policy.schedule_index]
    if value["schema"] != adapter.RECEIPT_SCHEMA or value["issue"] != 37 or \
            value["experiment_id"] != plan["experiment_id"] or \
            value["plan_sha256"] != core.plan_digest(plan) or \
            value["policy_sha256"] != policy.adapter_policy_sha256 or \
            value["schedule_index"] != policy.schedule_index or value["entry"] != expected_entry or \
            value["errors"] != [] or value["outcome"] != {"status": "success", "failure_code": None} or \
            value["execution_qualified"] is not False or value["measurement_ready"] is not False or \
            value["performance_claim"] is not False:
        raise MaintenanceError("PR51 adapter receipt differs from the frozen successful entry")
    if value["production_before"] != value["production_after"]:
        raise MaintenanceError("PR51 adapter changed protected production authority")
    return value


def validate_adapter_cleanup(value: Any) -> dict[str, Any]:
    row = require_exact(value, {
        "complete", "stop_order", "cleanup_order", "units_absent", "ports_closed",
        "paths_removed", "errors"}, "adapter cleanup")
    if row["complete"] is not True or row["stop_order"] != ["coordinator", "worker"] or \
            row["cleanup_order"] != ["coordinator", "worker"] or \
            row["units_absent"] is not True or row["ports_closed"] is not True or \
            row["paths_removed"] is not True or row["errors"] != []:
        raise MaintenanceError("adapter cleanup is incomplete or ordered incorrectly")
    return row


def validate_probe(
    value: Any, authorization: Authorization, coordinator: ProductionIdentity,
    worker: ProductionIdentity,
) -> dict[str, Any]:
    row = require_exact(value, {
        "schema", "request_sha256", "prompt_tokens", "generated_tokens",
        "world_size", "coordinator_identity_sha256", "worker_identity_sha256",
        "completed", "performance_result"}, "recovery probe")
    expected = {
        "schema": PROBE_SCHEMA,
        "request_sha256": authorization.request_sha256,
        "prompt_tokens": authorization.prompt_tokens,
        "generated_tokens": authorization.generated_tokens,
        "world_size": 2,
        "coordinator_identity_sha256": coordinator.digest,
        "worker_identity_sha256": worker.digest,
        "completed": True,
        "performance_result": False,
    }
    if row != expected:
        raise MaintenanceError("recovery probe is not the exact two-rank contract")
    return row


def execute_offline_domain(
    repository_root: Path, policy_path: Path, authorization_path: Path,
    runner: OfflineRunner, *, now: dt.datetime,
) -> Path:
    """Exercise the maintenance transaction with an explicit offline fake only."""
    if getattr(runner, "offline_fake", None) is not True:
        raise MaintenanceError("only an explicit offline fake Runner is admitted")
    policy_bytes, policy, authorization_bytes, authorization, context = validate_inputs(
        repository_root, policy_path, authorization_path, now=now)
    if authorization.execution_scope != "offline-domain-simulation":
        raise MaintenanceError("offline domain execution requires offline-domain-simulation scope")
    evidence_root = Path(authorization.evidence_root)
    custody = EvidenceCustody(evidence_root, authorization_bytes, policy_bytes)
    errors: list[dict[str, str]] = []
    before = authorization.production
    before_kernel: dict[str, Any] | None = None
    recovered: dict[str, ProductionIdentity] = {}
    recovery_census_complete = False
    recovery_probe_complete = False
    first_mutation = False
    adapter_cleanup_needed = False
    stop_attempted: set[str] = set()

    def custody_error(after_stage: str, exc: BaseException) -> None:
        errors.append({
            "stage": "evidence custody",
            "type": type(exc).__name__,
            "detail": f"after {after_stage}: {exc}",
        })

    def record(stage: str, observation: Any) -> None:
        try:
            custody.event(stage, "pass", observation)
        except BaseException as custody_exc:
            custody_error(stage, custody_exc)

    def error(stage: str, exc: BaseException) -> None:
        item = {"stage": stage, "type": type(exc).__name__, "detail": str(exc)}
        errors.append(item)
        try:
            custody.event(stage, "fail", item)
        except BaseException as custody_exc:
            custody_error(stage, custody_exc)

    intent = {
        "schema": INTENT_SCHEMA,
        "authorization_id": authorization.authorization_id,
        "authorization_sha256": authorization.raw_sha256,
        "policy_sha256": sha256_bytes(policy_bytes),
        "repository_commit": authorization.repository_commit,
        "schedule_index": authorization.schedule_index,
        "no_retry_after_intent": True,
        "target_execution_enabled": False,
    }
    custody.write_json("intent.json", intent)
    try:
        snapshot_raw = runner.snapshot_production()
        snapshot = parse_snapshot(snapshot_raw)
        if snapshot != before:
            raise MaintenanceError("live production identities differ from authorization")
        record("production before", snapshot_raw)
        before_kernel = validate_kernel(runner.kernel_baseline(), "kernel before")
        record("kernel before", before_kernel)
        pre_expected = {
            "nimo-1": {before["coordinator"].pid},
            "nimo-2": {before["worker"].pid},
        }
        pre_census = validate_census(
            runner.gpu_census(), pre_expected, "pre-stop GPU census",
            identities=tuple(before.values()))
        record("pre-stop GPU census", pre_census)

        first_mutation = True
        stop_attempted.add("coordinator")
        stopped_coordinator = validate_stop(
            runner.stop_production(before["coordinator"], policy.stop_timeout_seconds),
            before["coordinator"], "coordinator stop")
        record("coordinator stop", stopped_coordinator)
        record("coordinator absence", validate_absent(
            runner.prove_production_absent("coordinator"), "coordinator", "coordinator absence"))

        worker_only = validate_census(
            runner.gpu_census(), {"nimo-1": set(), "nimo-2": {before["worker"].pid}},
            "between-stop GPU census", identities=(before["worker"],))
        record("between-stop GPU census", worker_only)
        stop_attempted.add("worker")
        stopped_worker = validate_stop(
            runner.stop_production(before["worker"], policy.stop_timeout_seconds),
            before["worker"], "worker stop")
        record("worker stop", stopped_worker)
        record("worker absence", validate_absent(
            runner.prove_production_absent("worker"), "worker", "worker absence"))
        empty = validate_census(
            runner.gpu_census(), {"nimo-1": set(), "nimo-2": set()},
            "post-stop empty GPU census")
        record("post-stop empty GPU census", empty)

        adapter_cleanup_needed = True
        receipt_bytes = runner.run_adapter(
            context["plan_path"], context["adapter_policy_path"], evidence_root / "adapter")
        adapter_receipt = validate_adapter_receipt(receipt_bytes, policy, context)
        custody._write_bytes(custody.root / "adapter-receipt.raw.json", receipt_bytes)
        record("adapter handoff", {
            "receipt_sha256": sha256_bytes(receipt_bytes),
            "schedule_index": adapter_receipt["schedule_index"],
            "outcome": adapter_receipt["outcome"],
        })
    except BaseException as exc:
        error("maintenance body", exc)
    finally:
        if first_mutation:
            cleanup_complete = not adapter_cleanup_needed
            if adapter_cleanup_needed:
                try:
                    cleanup = validate_adapter_cleanup(
                        runner.cleanup_adapter(policy.cleanup_timeout_seconds))
                    record("adapter cleanup", cleanup)
                    cleanup_complete = True
                except BaseException as exc:
                    error("adapter cleanup", exc)
            recovery_admitted = cleanup_complete
            restart_required: set[str] = set()
            try:
                options: dict[str, set[int] | tuple[set[int], ...]] = {
                    "nimo-1": ((set(), {before["coordinator"].pid})
                               if "coordinator" in stop_attempted else {before["coordinator"].pid}),
                    "nimo-2": ((set(), {before["worker"].pid})
                               if "worker" in stop_attempted else {before["worker"].pid}),
                }
                observed = validate_census(
                    runner.gpu_census(), options, "pre-recovery reconciled GPU census",
                    identities=tuple(before.values()))
                observed_pids = census_pids(observed)
                if "coordinator" in stop_attempted and not observed_pids["nimo-1"]:
                    restart_required.add("coordinator")
                if "worker" in stop_attempted and not observed_pids["nimo-2"]:
                    restart_required.add("worker")
                record("pre-recovery reconciled GPU census", observed)
            except BaseException as exc:
                recovery_admitted = False
                error("pre-recovery reconciled GPU census", exc)

            if recovery_admitted:
                try:
                    if "worker" in restart_required:
                        worker_raw = runner.start_production("worker", policy.start_timeout_seconds)
                        worker = validate_new_identity(worker_raw, "worker", before["worker"])
                        record("worker recovery identity", worker_raw)
                    else:
                        worker = before["worker"]
                        record("worker preserved identity", dataclasses.asdict(worker))
                    worker_ready = validate_ready(
                        runner.prove_recovery_ready(worker, policy.start_timeout_seconds),
                        worker, "worker recovery readiness")
                    recovered["worker"] = worker
                    record("worker recovery readiness", worker_ready)
                except BaseException as exc:
                    error("worker recovery", exc)
            else:
                error("worker recovery", MaintenanceError(
                    "worker recovery is forbidden until cleanup and census admission pass"))

            if "worker" in recovered:
                try:
                    if "coordinator" in restart_required:
                        coordinator_raw = runner.start_production(
                            "coordinator", policy.start_timeout_seconds)
                        coordinator = validate_new_identity(
                            coordinator_raw, "coordinator", before["coordinator"])
                        record("coordinator recovery identity", coordinator_raw)
                    else:
                        coordinator = before["coordinator"]
                        record("coordinator preserved identity", dataclasses.asdict(coordinator))
                    coordinator_ready = validate_ready(
                        runner.prove_recovery_ready(coordinator, policy.start_timeout_seconds),
                        coordinator, "coordinator recovery readiness")
                    recovered["coordinator"] = coordinator
                    record("coordinator recovery readiness", coordinator_ready)
                except BaseException as exc:
                    error("coordinator recovery", exc)
            else:
                error("coordinator recovery", MaintenanceError(
                    "coordinator start is forbidden until worker recovery passes"))

            if set(recovered) == {"worker", "coordinator"}:
                try:
                    recovery_census = validate_census(runner.gpu_census(), {
                        "nimo-1": {recovered["coordinator"].pid},
                        "nimo-2": {recovered["worker"].pid},
                    }, "recovered production GPU census",
                        identities=tuple(recovered.values()))
                    record("recovered production GPU census", recovery_census)
                    recovery_census_complete = True
                except BaseException as exc:
                    error("recovered production GPU census", exc)
                if recovery_census_complete:
                    try:
                        probe = validate_probe(
                            runner.minimal_two_rank_inference(
                                recovered["coordinator"], recovered["worker"],
                                authorization.request_sha256, policy.request_timeout_seconds),
                            authorization, recovered["coordinator"], recovered["worker"])
                        record("minimal two-rank inference contract", probe)
                        recovery_probe_complete = True
                    except BaseException as exc:
                        error("minimal two-rank inference contract", exc)
            if before_kernel is not None:
                try:
                    after_kernel = validate_kernel(runner.kernel_baseline(), "kernel after")
                    compare_kernel(before_kernel, after_kernel)
                    record("kernel after", after_kernel)
                except BaseException as exc:
                    error("kernel after", exc)

    terminal = {
        "schema": TERMINAL_SCHEMA,
        "authorization_id": authorization.authorization_id,
        "authorization_sha256": authorization.raw_sha256,
        "policy_sha256": sha256_bytes(policy_bytes),
        "status": "success" if not errors else "failure",
        "target_execution_enabled": False,
        "offline_domain_only": True,
        "first_mutation": first_mutation,
        "services_ready": set(recovered) == {"worker", "coordinator"},
        "recovery_census_complete": recovery_census_complete,
        "recovery_probe_complete": recovery_probe_complete,
        "recovery_complete": (
            set(recovered) == {"worker", "coordinator"} and
            recovery_census_complete and recovery_probe_complete),
        "production_before": {role: dataclasses.asdict(value) for role, value in before.items()},
        "production_recovered": {role: dataclasses.asdict(value) for role, value in recovered.items()},
        "errors": errors,
        "performance_result": None,
    }
    terminal_path = custody.write_json("terminal.json", terminal)
    custody.finalize_hashes()
    if errors:
        raise MaintenanceRunFailed(terminal_path, errors)
    return terminal_path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--policy", type=Path, required=True)
    parser.add_argument("--authorization", type=Path, required=True)
    parser.add_argument("--now-utc", required=True, help="canonical whole-second UTC timestamp ending in Z")
    parser.add_argument("command", choices=("validate", "execute"))
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        now = parse_utc(args.now_utc, "--now-utc")
        if args.command == "execute":
            if not TARGET_EXECUTION_ENABLED:
                raise MaintenanceError(
                    "real target execution is hard-disabled: this controller defines no target Runner; issue #41 remains the authority gate")
            raise MaintenanceError("no real target Runner is implemented")
        policy_bytes, policy, authorization_bytes, authorization, context = validate_inputs(
            args.repository_root, args.policy, args.authorization, now=now)
        if authorization.execution_scope != "offline-domain-simulation":
            raise MaintenanceError(
                "public validation is offline-domain-simulation only; real maintenance authorization is not implemented")
        print(json.dumps({
            "valid": True,
            "target_execution_enabled": False,
            "authorization_sha256": sha256_bytes(authorization_bytes),
            "policy_sha256": sha256_bytes(policy_bytes),
            "authorization_id": authorization.authorization_id,
            "execution_scope": authorization.execution_scope,
            "repository_commit": authorization.repository_commit,
            "adapter_plan_digest": core.plan_digest(context["plan"]),
            "schedule_index": policy.schedule_index,
        }, indent=2, sort_keys=True))
        return 0
    except MaintenanceError as exc:
        print(f"maintenance admission refused: {exc}", file=__import__("sys").stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
