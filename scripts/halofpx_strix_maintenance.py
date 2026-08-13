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
import stat
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


core = _load_sibling("halofpx_strix_ab", "halofpx_strix_ab.py")
adapter = _load_sibling("halofpx_strix_ab_cachyos", "halofpx_strix_ab_cachyos.py")
production_identity_contract = _load_exact_sibling(
    "halofpx_strix_production_identity", "halofpx_strix_production_identity.py")


AUTHORIZATION_SCHEMA = "halofpx.strix-maintenance-authorization.v1"
POLICY_SCHEMA = "halofpx.strix-maintenance-policy.v1"
TERMINAL_SCHEMA = "halofpx.strix-maintenance-terminal.v1"
COMMIT_SCHEMA = "halofpx.strix-maintenance-offline-commit.v1"
FAILURE_MARKER_SCHEMA = "halofpx.strix-maintenance-offline-failure.v1"
INTENT_SCHEMA = "halofpx.strix-maintenance-intent.v1"
EVENT_SCHEMA = "halofpx.strix-maintenance-event.v1"
GPU_CENSUS_SCHEMA = "halofpx.strix-maintenance-gpu-census.v1"
KERNEL_BASELINE_SCHEMA = "halofpx.strix-maintenance-kernel-baseline.v1"
PROBE_SCHEMA = "halofpx.strix-maintenance-two-rank-probe.v1"
PRODUCTION_SNAPSHOT_SCHEMA = "halofpx.strix-maintenance-production-snapshot.v3"
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


class MarkerPublicationError(MaintenanceError):
    def __init__(self, detail: str, *, marker_may_exist: bool):
        self.marker_may_exist = marker_may_exist
        super().__init__(detail)


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


def require_repository_relative_path(value: Any, where: str) -> str:
    text = require_string(value, where)
    if "\\" in text or text.startswith("/") or ":" in text or \
            any(part in {"", ".", ".."} for part in text.split("/")):
        raise MaintenanceError(f"{where} must be a canonical repository-relative path")
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
        return production_identity_contract.production_identity_digest(
            dataclasses.asdict(self))


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
    listener_pid = require_int(item["listener_pid"], f"{where}.listener_pid", 1)
    if port != PROTECTED_PORTS[role] or listener_pid != pid:
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
    issue_number = require_int(item["issue_number"], f"{where}.issue_number", 1)
    if item["repository"] != "JCFrags/HaloFPX" or issue_number != 41 or \
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
    issue = require_int(raw["issue"], "authorization.issue", 1)
    if raw["schema"] != AUTHORIZATION_SCHEMA or issue != 41:
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
    if type(probe["world_size"]) is not int or probe["world_size"] != 2 or \
            probe["performance_result"] is not False:
        raise MaintenanceError("authorization recovery probe must be two-rank and non-performance")
    prompt_tokens = require_int(probe["prompt_tokens"], "recovery_probe.prompt_tokens", 1)
    generated_tokens = require_int(probe["generated_tokens"], "recovery_probe.generated_tokens", 1)
    if prompt_tokens != 5 or generated_tokens != 1:
        raise MaintenanceError("authorization recovery probe must be the bounded five-prompt/one-generated-token contract")
    return Authorization(
        observed, authorization_id, scope, authority, not_before, expires, nonce,
        commit, incident["manifest_sha256"],
        require_hash(ab["plan_sha256"], "authorization.adapter.plan_sha256"),
        require_hash(ab["policy_sha256"], "authorization.adapter.policy_sha256"),
        schedule_index, evidence_root, disposable["unit_prefix"], coordinator_port,
        worker_port, identities,
        require_hash(probe["request_sha256"], "recovery_probe.request_sha256"),
        prompt_tokens, generated_tokens,
    )


def load_policy_bytes(content: bytes) -> Policy:
    raw = parse_closed_json(content, "maintenance policy")
    require_exact(raw, {
        "schema", "issue", "target_execution_enabled", "authorization",
        "repository", "incident", "adapter", "timeouts"}, "maintenance policy")
    issue = require_int(raw["issue"], "policy.issue", 1)
    if raw["schema"] != POLICY_SCHEMA or issue != 41 or \
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
        require_repository_relative_path(
            incident["manifest_path"], "policy.incident.manifest_path"),
        require_hash(incident["manifest_sha256"], "policy.incident.manifest_sha256"),
        require_repository_relative_path(ab["plan_path"], "policy.adapter.plan_path"),
        require_hash(ab["plan_sha256"], "policy.adapter.plan_sha256"),
        require_repository_relative_path(ab["policy_path"], "policy.adapter.policy_path"),
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
        "plan_bytes": plan_bytes,
        "adapter_policy_bytes": adapter_policy_bytes,
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
    def run_adapter(self, plan_path: Path, policy_path: Path, evidence_root: Path) -> bytes: ...
    def cleanup_adapter(self, timeout_seconds: int) -> dict[str, Any]: ...
    def prove_adapter_absent(self) -> dict[str, Any]: ...
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

    def _publish_marker(
        self, *, terminal_path: Path, hashes_path: Path, schema: str,
        staging_name: str, final_name: str,
    ) -> Path:
        marker = {
            "schema": schema,
            "terminal_path": terminal_path.relative_to(self.root).as_posix(),
            "terminal_sha256": sha256_bytes(terminal_path.read_bytes()),
            "hashes_path": hashes_path.relative_to(self.root).as_posix(),
            "hashes_sha256": sha256_bytes(hashes_path.read_bytes()),
        }
        try:
            staged = self.write_json(staging_name, marker)
        except BaseException as exc:
            raise MarkerPublicationError(
                f"failed while staging {final_name}: {exc}", marker_may_exist=False) from exc
        final = self.root / final_name
        try:
            os.replace(staged, final)
        except BaseException as exc:
            # An actuator-style lost response is possible here too: rename
            # may have taken effect before the exception reached us. Inspect
            # and withdraw the final name before permitting any terminal
            # rewrite. If absence cannot be established, bound payload bytes
            # remain immutable and the surviving tree must face the verifier.
            marker_withdrawn = self._withdraw_marker(final)
            raise MarkerPublicationError(
                f"{final_name} rename response was ambiguous: {exc}",
                marker_may_exist=not marker_withdrawn) from exc
        try:
            self._sync_parent(final)
        except BaseException as exc:
            # The rename may be visible but not crash-durable. Try to withdraw
            # the marker. If withdrawal itself is ambiguous, never rewrite the
            # bytes already bound by that marker; consumers must use the full
            # verifier and treat a coherent surviving tree as the only local
            # acceptance authority.
            marker_may_exist = not self._withdraw_marker(final)
            raise MarkerPublicationError(
                f"{final_name} rename occurred but parent sync failed: {exc}",
                marker_may_exist=marker_may_exist,
            ) from exc
        return final

    def _withdraw_marker(self, final: Path) -> bool:
        """Best-effort proof that an ambiguously published marker is absent."""
        try:
            final.unlink(missing_ok=True)
            self._sync_parent(final)
            return not final.exists()
        except BaseException:
            return False

    def commit_success(self, terminal_path: Path, hashes_path: Path) -> Path:
        """Publish the offline bundle's sole success marker last."""
        return self._publish_marker(
            terminal_path=terminal_path, hashes_path=hashes_path,
            schema=COMMIT_SCHEMA, staging_name="COMMITTING.json",
            final_name="COMMITTED.json")

    def commit_failure(self, terminal_path: Path, hashes_path: Path) -> Path:
        """Publish a distinct finalized-failure marker; never a success marker."""
        return self._publish_marker(
            terminal_path=terminal_path, hashes_path=hashes_path,
            schema=FAILURE_MARKER_SCHEMA, staging_name="FAILING.json",
            final_name="FAILED.json")

    def withdraw_success_marker(self) -> bool:
        return self._withdraw_marker(self.root / "COMMITTED.json")

    def write_terminal_failure_after_finalize_error(
        self, terminal_path: Path, terminal: dict[str, Any], exc: BaseException,
    ) -> None:
        """Best-effort replacement of a provisional success terminal.

        Prior raw custody is left untouched. No completion marker is written,
        so consumers can never accept this bundle as a completed offline run.
        """
        failed = dict(terminal)
        failed["status"] = "failure"
        failed["errors"] = [*terminal["errors"], {
            "stage": "evidence finalization", "type": type(exc).__name__,
            "detail": str(exc),
        }]
        data = json.dumps(failed, indent=2, sort_keys=True).encode("utf-8") + b"\n"
        replacement = self.root / ".terminal.failure.tmp"
        self._write_bytes(replacement, data)
        os.replace(replacement, terminal_path)
        self._sync_parent(terminal_path)


def parse_snapshot(
    value: Any, expected: dict[str, ProductionIdentity] | None = None,
) -> dict[str, ProductionIdentity | None]:
    root = require_exact(value, {"schema", "complete", "roles", "errors"}, "production snapshot")
    if root["schema"] != PRODUCTION_SNAPSHOT_SCHEMA or \
            root["complete"] is not True or root["errors"] != []:
        raise MaintenanceError("production snapshot is incomplete")
    roles = require_exact(root["roles"], {"coordinator", "worker"}, "production snapshot roles")
    observed: dict[str, ProductionIdentity | None] = {}
    for role in ("coordinator", "worker"):
        row = require_exact(
            roles[role], {
                "active", "host", "unit", "unit_active_state", "unit_sub_state",
                "main_pid", "listener_pids", "control_group",
                "control_group_exists", "control_group_pids", "identity",
            },
            f"production snapshot.{role}")
        if type(row["active"]) is not bool or \
                not isinstance(row["unit_active_state"], str) or \
                not isinstance(row["unit_sub_state"], str) or \
                type(row["main_pid"]) is not int or \
                type(row["control_group_exists"]) is not bool or \
                not isinstance(row["listener_pids"], list) or any(
                    type(pid) is not int for pid in row["listener_pids"]) or \
                not isinstance(row["control_group_pids"], list) or any(
                    type(pid) is not int for pid in row["control_group_pids"]):
            raise MaintenanceError(f"production snapshot.{role} has wrong state field types")
        authority = expected[role] if expected is not None else None
        if row["active"]:
            identity = parse_identity(row["identity"], role, f"production snapshot.{role}.identity")
            if row["host"] != identity.host or row["unit"] != identity.unit or \
                    row["control_group"] != identity.control_group or \
                    row["unit_active_state"] != "active" or \
                    row["unit_sub_state"] != "running" or \
                    row["main_pid"] != identity.pid or \
                    row["listener_pids"] != [identity.pid] or \
                    row["control_group_exists"] is not True or \
                    row["control_group_pids"] != [identity.pid] or \
                    (authority is not None and (
                        identity.host != authority.host or identity.unit != authority.unit or
                        identity.control_group != authority.control_group)):
                raise MaintenanceError(
                    f"production snapshot.{role} active state differs from its exact identity")
            observed[role] = identity
        else:
            if authority is None:
                raise MaintenanceError(
                    f"production snapshot.{role} absence lacks frozen topology authority")
            if row != {
                    "active": False, "host": authority.host, "unit": authority.unit,
                    "unit_active_state": "inactive",
                    "unit_sub_state": "dead", "main_pid": 0,
                    "listener_pids": [], "control_group": authority.control_group,
                    "control_group_exists": False, "control_group_pids": [],
                    "identity": None}:
                raise MaintenanceError(
                    f"production snapshot.{role} absence state is not exact")
            observed[role] = None
    return observed


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
    if type(row["active"]) is not bool or type(row["control_group_absent"]) is not bool or \
            type(row["main_pid"]) is not int or \
            not isinstance(row["listener_pids"], list) or any(
                type(pid) is not int for pid in row["listener_pids"]):
        raise MaintenanceError(f"{where} has wrong boolean/integer field types")
    if row != {
        "host": identity.host, "unit": identity.unit,
        "stopped_identity_sha256": identity.digest, "active": False,
        "main_pid": 0, "listener_pids": [], "control_group_absent": True,
    }:
        raise MaintenanceError(f"{where} does not prove exact stopped identity/absence")
    return row


def validate_new_identity(value: Any, role: str, before: ProductionIdentity) -> ProductionIdentity:
    current = parse_identity(value, role, f"recovered {role}")
    if current.pid == before.pid or current.invocation_id == before.invocation_id or \
            current.process_start_ticks <= before.process_start_ticks or \
            current.start_monotonic_us <= before.start_monotonic_us:
        raise MaintenanceError(f"recovered {role} identity is not fresh")
    if current.nrestarts != before.nrestarts or current.control_group != before.control_group or \
            current.executable_sha256 != before.executable_sha256 or \
            current.argv_sha256 != before.argv_sha256 or \
            current.health_sha256 != before.health_sha256:
        raise MaintenanceError(
            f"recovered {role} artifact/argv/restart/cgroup/health authority changed")
    return current


def validate_ready(value: Any, identity: ProductionIdentity, where: str) -> dict[str, Any]:
    keys = {"role", "identity_sha256", "listener_pids", "ready", "health", "rpc_protocol"}
    row = require_exact(value, keys, where)
    if not isinstance(row["listener_pids"], list) or any(
            type(pid) is not int for pid in row["listener_pids"]):
        raise MaintenanceError(f"{where} has wrong listener PID field types")
    if row["role"] != identity.role or row["identity_sha256"] != identity.digest or \
            row["listener_pids"] != [identity.pid] or row["ready"] is not True:
        raise MaintenanceError(f"{where} is not bound to the recovered identity/listener")
    if identity.role == "worker":
        if row["rpc_protocol"] != "4.0.1" or row["health"] is not None:
            raise MaintenanceError("worker recovery lacks RPC HELLO readiness")
    elif not isinstance(row["health"], dict) or \
            require_int(row["health"].get("status"), "coordinator health status", 100) != 200 or \
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
    issue = require_int(value["issue"], "PR51 adapter receipt.issue", 1)
    schedule_index = require_int(
        value["schedule_index"], "PR51 adapter receipt.schedule_index")
    if value["schema"] != adapter.RECEIPT_SCHEMA or issue != 37 or \
            value["experiment_id"] != plan["experiment_id"] or \
            value["plan_sha256"] != core.plan_digest(plan) or \
            value["policy_sha256"] != policy.adapter_policy_sha256 or \
            schedule_index != policy.schedule_index or value["entry"] != expected_entry or \
            value["errors"] != [] or value["outcome"] != {"status": "success", "failure_code": None} or \
            value["execution_qualified"] is not False or value["measurement_ready"] is not False or \
            value["performance_claim"] is not False:
        raise MaintenanceError("PR51 adapter receipt differs from the frozen successful entry")
    if value["production_before"] != value["production_after"]:
        raise MaintenanceError("PR51 adapter changed protected production authority")
    return value


def validate_retained_adapter_receipt(
    content: bytes, authorization: Authorization, policy: Policy,
    plan_bytes: bytes, adapter_policy_bytes: bytes,
) -> dict[str, Any]:
    """Re-run the live sparse-receipt validator from retained frozen inputs."""
    if sha256_bytes(plan_bytes) != authorization.adapter_plan_sha256 or \
            sha256_bytes(plan_bytes) != policy.adapter_plan_sha256 or \
            sha256_bytes(adapter_policy_bytes) != authorization.adapter_policy_sha256 or \
            sha256_bytes(adapter_policy_bytes) != policy.adapter_policy_sha256:
        raise MaintenanceError("retained PR51 plan/policy raw bytes differ from authority")
    plan = core.validate_plan(parse_closed_json(plan_bytes, "retained adapter plan"))
    adapter_policy = adapter.load_policy_bytes(adapter_policy_bytes, plan)
    if adapter_policy.unit_prefix != authorization.unit_prefix or \
            adapter_policy.coordinator_port != authorization.coordinator_port or \
            adapter_policy.worker_port != authorization.worker_port:
        raise MaintenanceError("retained adapter policy differs from disposable authority")
    try:
        return validate_adapter_receipt(content, policy, {
            "plan": plan, "schedule": core.make_schedule(plan),
        })
    except MaintenanceError as exc:
        raise MaintenanceError(f"retained PR51 adapter receipt is invalid: {exc}") from exc


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


def validate_adapter_absent(value: Any) -> dict[str, Any]:
    row = require_exact(value, {
        "units_absent", "ports_closed", "paths_removed", "errors",
    }, "adapter absence")
    if row["units_absent"] is not True or row["ports_closed"] is not True or \
            row["paths_removed"] is not True or row["errors"] != []:
        raise MaintenanceError("adapter absence is not independently proven")
    return row


def validate_probe(
    value: Any, authorization: Authorization, coordinator: ProductionIdentity,
    worker: ProductionIdentity,
) -> dict[str, Any]:
    row = require_exact(value, {
        "schema", "request_sha256", "prompt_tokens", "generated_tokens",
        "world_size", "coordinator_identity_sha256", "worker_identity_sha256",
        "completed", "performance_result"}, "recovery probe")
    if type(row["prompt_tokens"]) is not int or type(row["generated_tokens"]) is not int or \
            type(row["world_size"]) is not int or type(row["completed"]) is not bool or \
            type(row["performance_result"]) is not bool:
        raise MaintenanceError("recovery probe has wrong integer/boolean field types")
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


SUCCESS_EVENT_STAGES = (
    "production before",
    "kernel before",
    "pre-stop GPU census",
    "coordinator stop",
    "coordinator stop postcondition raw",
    "between-stop GPU census",
    "worker stop",
    "worker stop postcondition raw",
    "post-stop empty GPU census",
    "adapter handoff",
    "adapter cleanup",
    "adapter absence",
    "pre-recovery production raw",
    "pre-recovery reconciled GPU census",
    "worker recovery actuation receipt",
    "worker recovery postcondition raw",
    "worker recovery readiness",
    "coordinator recovery actuation receipt",
    "coordinator recovery postcondition raw",
    "coordinator recovery readiness",
    "recovered production GPU census",
    "minimal two-rank inference contract",
    "kernel after",
    "production final observed raw",
)


def event_relative_path(index: int, stage: str) -> str:
    safe = re.sub(r"[^a-z0-9]+", "-", stage.lower()).strip("-")[:80]
    return f"events/{index:03d}-{safe}.json"


def _pretty_json_bytes(value: Any) -> bytes:
    return json.dumps(value, indent=2, sort_keys=True).encode("utf-8") + b"\n"


def _closed_regular_tree(root: Path) -> tuple[dict[str, Path], set[str]]:
    if root.is_symlink() or not root.is_dir():
        raise MaintenanceError("committed bundle root must be a real directory")
    files: dict[str, Path] = {}
    directories: set[str] = set()
    for current, dirnames, filenames in os.walk(root, followlinks=False):
        current_path = Path(current)
        for name in dirnames:
            path = current_path / name
            relative = path.relative_to(root).as_posix()
            mode = path.lstat().st_mode
            if path.is_symlink() or not stat.S_ISDIR(mode):
                raise MaintenanceError(f"committed bundle directory is not regular: {relative}")
            directories.add(relative)
        for name in filenames:
            path = current_path / name
            relative = path.relative_to(root).as_posix()
            mode = path.lstat().st_mode
            if path.is_symlink() or not stat.S_ISREG(mode):
                raise MaintenanceError(f"committed bundle file is not regular: {relative}")
            files[relative] = path
    return files, directories


def _parse_hash_manifest(content: bytes) -> dict[str, str]:
    try:
        text = content.decode("ascii", errors="strict")
    except UnicodeError as exc:
        raise MaintenanceError("SHA256SUMS is not ASCII") from exc
    if not text or not text.endswith("\n") or "\r" in text:
        raise MaintenanceError("SHA256SUMS is not canonical newline-delimited text")
    rows: dict[str, str] = {}
    for line in text.splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9][A-Za-z0-9._/-]*)", line)
        if match is None:
            raise MaintenanceError("SHA256SUMS contains a malformed row")
        digest, relative = match.groups()
        parts = relative.split("/")
        if relative.startswith("/") or "\\" in relative or any(
                part in {"", ".", ".."} for part in parts):
            raise MaintenanceError("SHA256SUMS contains an unsafe path")
        if relative in rows or relative in {"SHA256SUMS", "COMMITTED.json", "COMMITTING.json"}:
            raise MaintenanceError("SHA256SUMS contains a duplicate or reserved path")
        rows[relative] = digest
    canonical = "".join(f"{rows[path]}  {path}\n" for path in sorted(rows))
    if canonical.encode("ascii") != content:
        raise MaintenanceError("SHA256SUMS rows are not in canonical order")
    return rows


def verify_committed_bundle(root: Path) -> dict[str, Any]:
    """Return the terminal only for one coherent successful offline bundle.

    Marker existence alone is never authority. This verifier is the sole local
    acceptance seam and intentionally recognizes only the exact v1 successful
    fake-domain inventory. It is not a signature, two-node receipt, or target
    promotion mechanism.
    """
    files, directories = _closed_regular_tree(root)
    marker_path = files.get("COMMITTED.json")
    hashes_path = files.get("SHA256SUMS")
    if marker_path is None or hashes_path is None or "COMMITTING.json" in files:
        raise MaintenanceError("offline bundle has no sole final commit marker")
    marker_bytes = marker_path.read_bytes()
    marker = parse_closed_json(marker_bytes, "offline commit marker")
    require_exact(marker, {
        "schema", "terminal_path", "terminal_sha256", "hashes_path", "hashes_sha256",
    }, "offline commit marker")
    if marker_bytes != _pretty_json_bytes(marker) or marker["schema"] != COMMIT_SCHEMA or \
            marker["terminal_path"] != "terminal.json" or marker["hashes_path"] != "SHA256SUMS":
        raise MaintenanceError("offline commit marker is not the exact canonical v1 marker")
    terminal_digest = require_hash(marker["terminal_sha256"], "commit terminal digest")
    hashes_digest = require_hash(marker["hashes_sha256"], "commit manifest digest")
    hashes_bytes = hashes_path.read_bytes()
    if sha256_bytes(hashes_bytes) != hashes_digest:
        raise MaintenanceError("commit marker does not bind current SHA256SUMS bytes")
    manifest = _parse_hash_manifest(hashes_bytes)
    expected_payload = {
        "authorization.raw.json", "policy.raw.json", "intent.json",
        "adapter-plan.raw.json", "adapter-policy.raw.json",
        "adapter-receipt.raw.json", "terminal.json",
        *(event_relative_path(index, stage)
          for index, stage in enumerate(SUCCESS_EVENT_STAGES)),
    }
    if directories != {"events"} or set(manifest) != expected_payload or \
            set(files) != expected_payload | {"SHA256SUMS", "COMMITTED.json"}:
        raise MaintenanceError("offline bundle inventory is not the exact successful v1 tree")
    for relative, expected_digest in manifest.items():
        if sha256_bytes(files[relative].read_bytes()) != expected_digest:
            raise MaintenanceError(f"offline bundle digest mismatch: {relative}")
    terminal_bytes = files["terminal.json"].read_bytes()
    if sha256_bytes(terminal_bytes) != terminal_digest or \
            manifest["terminal.json"] != terminal_digest:
        raise MaintenanceError("terminal digest does not agree across marker, manifest, and bytes")
    terminal = parse_closed_json(terminal_bytes, "maintenance terminal")
    require_exact(terminal, {
        "schema", "authorization_id", "authorization_sha256", "policy_sha256",
        "status", "target_execution_enabled", "offline_domain_only", "first_mutation",
        "services_ready", "recovery_census_complete", "recovery_probe_complete",
        "final_observation_matches_recovery", "recovery_complete", "production_before",
        "production_recovered", "production_final_observed", "errors", "performance_result",
    }, "maintenance terminal")
    if terminal_bytes != _pretty_json_bytes(terminal) or terminal["schema"] != TERMINAL_SCHEMA or \
            terminal["status"] != "success" or terminal["errors"] != [] or \
            terminal["target_execution_enabled"] is not False or \
            terminal["offline_domain_only"] is not True or terminal["first_mutation"] is not True or \
            any(terminal[name] is not True for name in (
                "services_ready", "recovery_census_complete", "recovery_probe_complete",
                "final_observation_matches_recovery", "recovery_complete")) or \
            terminal["performance_result"] is not None:
        raise MaintenanceError("committed terminal is not an exact successful offline result")
    authorization_sha = require_hash(
        terminal["authorization_sha256"], "terminal authorization digest")
    policy_sha = require_hash(terminal["policy_sha256"], "terminal policy digest")
    authorization_bytes = files["authorization.raw.json"].read_bytes()
    policy_bytes = files["policy.raw.json"].read_bytes()
    if sha256_bytes(authorization_bytes) != authorization_sha or \
            sha256_bytes(policy_bytes) != policy_sha:
        raise MaintenanceError("terminal input digests do not bind retained raw inputs")
    authorization_root = parse_closed_json(authorization_bytes, "retained authorization")
    window = require_exact(
        authorization_root.get("window"), {"not_before_utc", "expires_utc"},
        "retained authorization.window")
    authorization = load_authorization_bytes(
        authorization_bytes, expected_sha256=authorization_sha,
        now=parse_utc(window["not_before_utc"], "retained authorization.window.not_before_utc"))
    policy = load_policy_bytes(policy_bytes)
    terminal_authorization_id = require_string(
        terminal["authorization_id"], "terminal authorization_id")
    if authorization.execution_scope != "offline-domain-simulation" or \
            Path(authorization.evidence_root) != root.resolve() or \
            terminal_authorization_id != authorization.authorization_id or \
            policy.authorization_sha256 != authorization_sha or \
            policy.authority != authorization.authority or \
            policy.repository_commit != authorization.repository_commit or \
            policy.incident_path != ISSUE41_MANIFEST_RELATIVE.as_posix() or \
            policy.incident_sha256 != authorization.incident_sha256 or \
            policy.adapter_plan_sha256 != authorization.adapter_plan_sha256 or \
            policy.adapter_policy_sha256 != authorization.adapter_policy_sha256 or \
            policy.schedule_index != authorization.schedule_index:
        raise MaintenanceError("retained policy/authorization/terminal authority differs")
    before_raw = require_exact(
        terminal["production_before"], {"coordinator", "worker"}, "terminal production before")
    recovered_raw = require_exact(
        terminal["production_recovered"], {"coordinator", "worker"},
        "terminal production recovered")
    before = {role: parse_identity(before_raw[role], role, f"terminal before.{role}")
              for role in ("coordinator", "worker")}
    recovered = {role: parse_identity(recovered_raw[role], role, f"terminal recovered.{role}")
                 for role in ("coordinator", "worker")}
    if before != authorization.production:
        raise MaintenanceError("terminal before-state differs from retained authorization")
    final_state = parse_snapshot(terminal["production_final_observed"], before)
    if any(final_state[role] is None for role in ("coordinator", "worker")) or \
            any(final_state[role] != recovered[role] for role in ("coordinator", "worker")):
        raise MaintenanceError("terminal final service state differs from recovered identities")
    for role in ("coordinator", "worker"):
        validate_new_identity(dataclasses.asdict(recovered[role]), role, before[role])
    intent = parse_closed_json(files["intent.json"].read_bytes(), "maintenance intent")
    require_exact(intent, {
        "schema", "authorization_id", "authorization_sha256", "policy_sha256",
        "repository_commit", "schedule_index", "no_retry_after_intent",
        "target_execution_enabled",
    }, "maintenance intent")
    intent_commit = require_hash(intent["repository_commit"], "intent repository commit", COMMIT_RE)
    intent_schedule_index = require_int(intent["schedule_index"], "intent schedule index")
    if intent["schema"] != INTENT_SCHEMA or intent["authorization_id"] != terminal_authorization_id or \
            intent["authorization_sha256"] != authorization_sha or intent["policy_sha256"] != policy_sha or \
            intent_commit != authorization.repository_commit or \
            intent_schedule_index != authorization.schedule_index or \
            intent["no_retry_after_intent"] is not True or \
            intent["target_execution_enabled"] is not False:
        raise MaintenanceError("maintenance intent differs from the committed terminal authority")
    events: list[dict[str, Any]] = []
    for index, stage in enumerate(SUCCESS_EVENT_STAGES):
        relative = event_relative_path(index, stage)
        event_bytes = files[relative].read_bytes()
        event = parse_closed_json(event_bytes, f"event {index}")
        require_exact(event, {"schema", "index", "stage", "status", "observation"}, f"event {index}")
        if event_bytes != _pretty_json_bytes(event) or event["schema"] != EVENT_SCHEMA or \
                type(event["index"]) is not int or \
                event["index"] != index or event["stage"] != stage or event["status"] != "pass":
            raise MaintenanceError(f"event {index} differs from the exact successful sequence")
        events.append(event)
    observations = {event["stage"]: event["observation"] for event in events}
    if parse_snapshot(observations["production before"], before) != before:
        raise MaintenanceError("production-before event differs from terminal authority")
    kernel_before = validate_kernel(observations["kernel before"], "committed kernel before")
    validate_census(
        observations["pre-stop GPU census"],
        {"nimo-1": {before["coordinator"].pid}, "nimo-2": {before["worker"].pid}},
        "committed pre-stop GPU census", identities=tuple(before.values()))
    validate_stop(
        observations["coordinator stop"], before["coordinator"], "committed coordinator stop")
    coordinator_stopped = parse_snapshot(
        observations["coordinator stop postcondition raw"], before)
    if coordinator_stopped != {"coordinator": None, "worker": before["worker"]}:
        raise MaintenanceError("coordinator stop postcondition is not exact")
    validate_census(
        observations["between-stop GPU census"],
        {"nimo-1": set(), "nimo-2": {before["worker"].pid}},
        "committed between-stop GPU census", identities=(before["worker"],))
    validate_stop(observations["worker stop"], before["worker"], "committed worker stop")
    worker_stopped = parse_snapshot(observations["worker stop postcondition raw"], before)
    if worker_stopped != {"coordinator": None, "worker": None}:
        raise MaintenanceError("worker stop postcondition is not exact")
    validate_census(
        observations["post-stop empty GPU census"],
        {"nimo-1": set(), "nimo-2": set()}, "committed post-stop GPU census")
    validate_adapter_cleanup(observations["adapter cleanup"])
    validate_adapter_absent(observations["adapter absence"])
    if parse_snapshot(observations["pre-recovery production raw"], before) != {
            "coordinator": None, "worker": None}:
        raise MaintenanceError("pre-recovery production state is not exactly absent")
    validate_census(
        observations["pre-recovery reconciled GPU census"],
        {"nimo-1": set(), "nimo-2": set()}, "committed pre-recovery GPU census")
    worker_receipt = validate_new_identity(
        observations["worker recovery actuation receipt"], "worker", before["worker"])
    if worker_receipt != recovered["worker"]:
        raise MaintenanceError("worker start receipt differs from recovered terminal identity")
    worker_started = parse_snapshot(observations["worker recovery postcondition raw"], before)
    if worker_started != {"coordinator": None, "worker": recovered["worker"]}:
        raise MaintenanceError("worker recovery postcondition is not exact")
    validate_ready(
        observations["worker recovery readiness"], recovered["worker"],
        "committed worker readiness")
    coordinator_receipt = validate_new_identity(
        observations["coordinator recovery actuation receipt"], "coordinator",
        before["coordinator"])
    if coordinator_receipt != recovered["coordinator"]:
        raise MaintenanceError("coordinator start receipt differs from recovered terminal identity")
    coordinator_started = parse_snapshot(
        observations["coordinator recovery postcondition raw"], before)
    if coordinator_started != recovered:
        raise MaintenanceError("coordinator recovery postcondition is not exact")
    validate_ready(
        observations["coordinator recovery readiness"], recovered["coordinator"],
        "committed coordinator readiness")
    validate_census(
        observations["recovered production GPU census"],
        {"nimo-1": {recovered["coordinator"].pid}, "nimo-2": {recovered["worker"].pid}},
        "committed recovered GPU census", identities=tuple(recovered.values()))
    probe = require_exact(observations["minimal two-rank inference contract"], {
        "schema", "request_sha256", "prompt_tokens", "generated_tokens",
        "world_size", "coordinator_identity_sha256", "worker_identity_sha256",
        "completed", "performance_result",
    }, "committed recovery probe")
    if probe != {
            "schema": PROBE_SCHEMA,
            "request_sha256": authorization.request_sha256,
            "prompt_tokens": 5,
            "generated_tokens": 1,
            "world_size": 2,
            "coordinator_identity_sha256": recovered["coordinator"].digest,
            "worker_identity_sha256": recovered["worker"].digest,
            "completed": True,
            "performance_result": False,
    }:
        raise MaintenanceError("committed recovery probe differs from exact authority")
    kernel_after = validate_kernel(observations["kernel after"], "committed kernel after")
    compare_kernel(kernel_before, kernel_after)
    retained_adapter = validate_retained_adapter_receipt(
        files["adapter-receipt.raw.json"].read_bytes(), authorization, policy,
        files["adapter-plan.raw.json"].read_bytes(),
        files["adapter-policy.raw.json"].read_bytes())
    adapter_handoff = require_exact(
        events[SUCCESS_EVENT_STAGES.index("adapter handoff")]["observation"],
        {"receipt_sha256", "schedule_index", "outcome"}, "adapter handoff event")
    handoff_schedule_index = require_int(
        adapter_handoff["schedule_index"], "adapter handoff schedule index")
    if sha256_bytes(files["adapter-receipt.raw.json"].read_bytes()) != \
            require_hash(adapter_handoff["receipt_sha256"], "adapter receipt event digest") or \
            handoff_schedule_index != authorization.schedule_index or \
            adapter_handoff["outcome"] != {"status": "success", "failure_code": None} or \
            adapter_handoff["outcome"] != retained_adapter["outcome"]:
        raise MaintenanceError("adapter handoff event does not bind the retained receipt")
    if events[-1]["observation"] != terminal["production_final_observed"]:
        raise MaintenanceError("final observation event differs from the terminal state")
    return terminal


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
    custody._write_bytes(custody.root / "adapter-plan.raw.json", context["plan_bytes"])
    custody._write_bytes(
        custody.root / "adapter-policy.raw.json", context["adapter_policy_bytes"])
    errors: list[dict[str, str]] = []
    before = authorization.production
    before_kernel: dict[str, Any] | None = None
    recovered: dict[str, ProductionIdentity] = {}
    final_snapshot_raw: Any = None
    final_observation_matches_recovery = False
    recovery_census_complete = False
    recovery_probe_complete = False
    first_mutation = False
    adapter_cleanup_needed = False
    proven_absent: set[str] = set()

    def custody_error(after_stage: str, exc: BaseException) -> None:
        errors.append({
            "stage": "evidence custody",
            "type": type(exc).__name__,
            "detail": f"after {after_stage}: {exc}",
        })

    def record(stage: str, observation: Any, *, recovery: bool = False) -> None:
        try:
            custody.event(stage, "pass", observation)
        except BaseException as custody_exc:
            custody_error(stage, custody_exc)
            # Forward admission and mutation evidence is mandatory. Abort the
            # maintenance body immediately if it cannot be durably retained;
            # the surrounding finally block still performs cleanup and
            # worker-first recovery. During recovery itself, never let a
            # receipt failure prevent the next safety action.
            if not recovery:
                raise MaintenanceError(
                    f"mandatory evidence custody failed after {stage}: {custody_exc}") from custody_exc

    def error(stage: str, exc: BaseException) -> None:
        item = {"stage": stage, "type": type(exc).__name__, "detail": str(exc)}
        errors.append(item)
        try:
            custody.event(stage, "fail", item)
        except BaseException as custody_exc:
            custody_error(stage, custody_exc)

    def stop_and_reconcile(role: str) -> None:
        """Attempt one stop, then independently observe its exact effect."""
        actuation_error: BaseException | None = None
        try:
            raw = runner.stop_production(before[role], policy.stop_timeout_seconds)
            record(f"{role} stop", validate_stop(raw, before[role], f"{role} stop"))
        except BaseException as exc:
            actuation_error = exc
        postcondition_error: BaseException | None = None
        try:
            raw_snapshot = runner.snapshot_production()
            try:
                record(f"{role} stop postcondition raw", raw_snapshot)
            except BaseException as exc:
                # Losing mandatory custody aborts the forward body, but the
                # already-observed state must still inform safe recovery. In
                # particular, an absence observed here must keep the old
                # identity forbidden from later being treated as preserved.
                postcondition_error = exc
            state = parse_snapshot(raw_snapshot, before)
            # Retain every independently observed absence even if a later
            # cross-role check fails. Otherwise a stopped role could reappear
            # under its stale identity and be misclassified as preserved.
            proven_absent.update(
                observed_role for observed_role, identity in state.items()
                if identity is None)
            other = "worker" if role == "coordinator" else "coordinator"
            expected_other: ProductionIdentity | None = (
                None if other in proven_absent else before[other])
            if state[other] != expected_other:
                raise MaintenanceError(
                    f"{role} stop changed the other protected production role")
            if state[role] is None:
                pass
            elif state[role] != before[role]:
                raise MaintenanceError(f"{role} stop left an unauthorized identity active")
            elif actuation_error is None:
                raise MaintenanceError(f"{role} stop receipt claimed success without absence")
        except BaseException as exc:
            if postcondition_error is None:
                postcondition_error = exc
            else:
                postcondition_error = MaintenanceError(
                    f"{postcondition_error}; state validation: {exc}")
        if actuation_error is not None or postcondition_error is not None:
            details = []
            if actuation_error is not None:
                details.append(f"actuation: {actuation_error}")
            if postcondition_error is not None:
                details.append(f"postcondition: {postcondition_error}")
            raise MaintenanceError(f"{role} stop failed reconciliation ({'; '.join(details)})")

    def recover_role(role: str, restart_required: set[str]) -> ProductionIdentity:
        """Recover or preserve one role using an independent active-state read."""
        if role not in restart_required:
            identity = before[role]
            record(f"{role} preserved identity", dataclasses.asdict(identity), recovery=True)
        else:
            actuation_raw: Any = None
            actuation_error: BaseException | None = None
            try:
                actuation_raw = runner.start_production(role, policy.start_timeout_seconds)
                record(f"{role} recovery actuation receipt", actuation_raw, recovery=True)
            except BaseException as exc:
                actuation_error = exc

            raw_snapshot = runner.snapshot_production()
            record(f"{role} recovery postcondition raw", raw_snapshot, recovery=True)
            state = parse_snapshot(raw_snapshot, before)
            identity = state[role]
            if identity is None:
                if actuation_error is not None:
                    raise MaintenanceError(
                        f"{role} start failed and independent postcondition is absent: {actuation_error}")
                raise MaintenanceError(f"{role} start returned without an active postcondition")
            identity = validate_new_identity(dataclasses.asdict(identity), role, before[role])
            if role == "worker":
                expected_coordinator = (
                    None if "coordinator" in restart_required else before["coordinator"])
                if state["coordinator"] != expected_coordinator:
                    raise MaintenanceError("worker start changed coordinator production state")
            else:
                if state["worker"] != recovered.get("worker"):
                    raise MaintenanceError("coordinator start changed recovered worker state")
            if actuation_raw is not None:
                try:
                    returned = validate_new_identity(actuation_raw, role, before[role])
                    if returned != identity:
                        raise MaintenanceError(
                            f"{role} start receipt differs from independent postcondition")
                except BaseException as exc:
                    actuation_error = exc
            if actuation_error is not None:
                error(f"{role} recovery actuation", actuation_error)

        ready = validate_ready(
            runner.prove_recovery_ready(identity, policy.start_timeout_seconds),
            identity, f"{role} recovery readiness")
        recovered[role] = identity
        record(f"{role} recovery readiness", ready, recovery=True)
        return identity

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
        record("production before", snapshot_raw)
        snapshot = parse_snapshot(snapshot_raw, before)
        if snapshot != before:
            raise MaintenanceError("live active production identities differ from authorization")
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
        stop_and_reconcile("coordinator")

        worker_only = validate_census(
            runner.gpu_census(), {"nimo-1": set(), "nimo-2": {before["worker"].pid}},
            "between-stop GPU census", identities=(before["worker"],))
        record("between-stop GPU census", worker_only)
        stop_and_reconcile("worker")
        empty = validate_census(
            runner.gpu_census(), {"nimo-1": set(), "nimo-2": set()},
            "post-stop empty GPU census")
        record("post-stop empty GPU census", empty)

        adapter_cleanup_needed = True
        receipt_bytes = runner.run_adapter(
            context["plan_path"], context["adapter_policy_path"], evidence_root / "adapter")
        try:
            custody._write_bytes(custody.root / "adapter-receipt.raw.json", receipt_bytes)
        except BaseException as exc:
            custody_error("adapter receipt raw", exc)
            raise MaintenanceError(f"mandatory adapter receipt custody failed: {exc}") from exc
        adapter_receipt = validate_adapter_receipt(receipt_bytes, policy, context)
        record("adapter handoff", {
            "receipt_sha256": sha256_bytes(receipt_bytes),
            "schedule_index": adapter_receipt["schedule_index"],
            "outcome": adapter_receipt["outcome"],
        })
    except BaseException as exc:
        error("maintenance body", exc)
    finally:
        if first_mutation:
            recovery_admitted = not adapter_cleanup_needed
            if adapter_cleanup_needed:
                try:
                    cleanup_raw = runner.cleanup_adapter(policy.cleanup_timeout_seconds)
                    record("adapter cleanup", cleanup_raw, recovery=True)
                    validate_adapter_cleanup(cleanup_raw)
                except BaseException as exc:
                    error("adapter cleanup actuation", exc)
                try:
                    adapter_absent_raw = runner.prove_adapter_absent()
                    record("adapter absence", adapter_absent_raw, recovery=True)
                    validate_adapter_absent(adapter_absent_raw)
                    recovery_admitted = True
                except BaseException as exc:
                    recovery_admitted = False
                    error("adapter absence", exc)
            restart_required: set[str] = set()
            try:
                raw_snapshot = runner.snapshot_production()
                record("pre-recovery production raw", raw_snapshot, recovery=True)
                state = parse_snapshot(raw_snapshot, before)
                options: dict[str, set[int]] = {}
                for role, host in (("coordinator", "nimo-1"), ("worker", "nimo-2")):
                    identity = state[role]
                    if identity is None:
                        restart_required.add(role)
                        options[host] = set()
                    elif identity == before[role] and role not in proven_absent:
                        options[host] = {identity.pid}
                    elif identity == before[role]:
                        raise MaintenanceError(
                            f"proven-absent {role} reappeared with its stale pre-stop identity")
                    else:
                        raise MaintenanceError(
                            f"pre-recovery {role} differs from absent or exact preserved authority")
                observed = validate_census(
                    runner.gpu_census(), options, "pre-recovery reconciled GPU census",
                    identities=tuple(before.values()))
                record("pre-recovery reconciled GPU census", observed, recovery=True)
            except BaseException as exc:
                recovery_admitted = False
                error("pre-recovery reconciled GPU census", exc)

            if recovery_admitted:
                try:
                    recover_role("worker", restart_required)
                except BaseException as exc:
                    error("worker recovery", exc)
            else:
                error("worker recovery", MaintenanceError(
                    "worker recovery is forbidden until cleanup and census admission pass"))

            if "worker" in recovered:
                try:
                    recover_role("coordinator", restart_required)
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
                    record("recovered production GPU census", recovery_census, recovery=True)
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
                        record("minimal two-rank inference contract", probe, recovery=True)
                        recovery_probe_complete = True
                    except BaseException as exc:
                        error("minimal two-rank inference contract", exc)
            if before_kernel is not None:
                try:
                    after_kernel = validate_kernel(runner.kernel_baseline(), "kernel after")
                    compare_kernel(before_kernel, after_kernel)
                    record("kernel after", after_kernel, recovery=True)
                except BaseException as exc:
                    error("kernel after", exc)

            # Actuator errors are not proof of no effect. Always close the
            # recovery section with a fresh authoritative snapshot so the
            # terminal exposes any service that started despite a lost/error
            # response. This observation does not promote an unvalidated
            # identity into ``production_recovered`` or recovery success.
            try:
                final_snapshot_raw = runner.snapshot_production()
                record("production final observed raw", final_snapshot_raw, recovery=True)
                final_observed = parse_snapshot(final_snapshot_raw, before)
                if set(recovered) == {"worker", "coordinator"} and final_observed != recovered:
                    raise MaintenanceError(
                        "final observed production identities differ from validated recovery")
                final_observation_matches_recovery = (
                    set(recovered) == {"worker", "coordinator"} and
                    final_observed == recovered)
            except BaseException as exc:
                error("production final observation", exc)

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
        "final_observation_matches_recovery": final_observation_matches_recovery,
        "recovery_complete": (
            set(recovered) == {"worker", "coordinator"} and
            recovery_census_complete and recovery_probe_complete and
            final_observation_matches_recovery),
        "production_before": {role: dataclasses.asdict(value) for role, value in before.items()},
        "production_recovered": {role: dataclasses.asdict(value) for role, value in recovered.items()},
        "production_final_observed": final_snapshot_raw,
        "errors": errors,
        "performance_result": None,
    }
    terminal_path = custody.write_json("terminal.json", terminal)
    finalization_errors: list[dict[str, str]] = []
    try:
        hashes_path = custody.finalize_hashes()
    except BaseException as exc:
        finalization_errors.append({
            "stage": "evidence finalization", "type": type(exc).__name__, "detail": str(exc)})
        try:
            custody.write_terminal_failure_after_finalize_error(terminal_path, terminal, exc)
        except BaseException as rewrite_exc:
            finalization_errors.append({
                "stage": "failure terminal rewrite", "type": type(rewrite_exc).__name__,
                "detail": str(rewrite_exc)})
        raise MaintenanceRunFailed(terminal_path, [*errors, *finalization_errors]) from exc
    if errors:
        try:
            custody.commit_failure(terminal_path, hashes_path)
        except BaseException as exc:
            finalization_errors.append({
                "stage": "failure marker publication", "type": type(exc).__name__,
                "detail": str(exc)})
        raise MaintenanceRunFailed(terminal_path, [*errors, *finalization_errors])
    try:
        committed_path = custody.commit_success(terminal_path, hashes_path)
    except MarkerPublicationError as exc:
        finalization_errors.append({
            "stage": "success marker publication", "type": type(exc).__name__,
            "detail": str(exc)})
        if not exc.marker_may_exist:
            try:
                custody.write_terminal_failure_after_finalize_error(terminal_path, terminal, exc)
            except BaseException as rewrite_exc:
                finalization_errors.append({
                    "stage": "failure terminal rewrite", "type": type(rewrite_exc).__name__,
                    "detail": str(rewrite_exc)})
        raise MaintenanceRunFailed(terminal_path, finalization_errors) from exc
    try:
        verify_committed_bundle(custody.root)
    except BaseException as exc:
        custody.withdraw_success_marker()
        raise MaintenanceRunFailed(terminal_path, [{
            "stage": "committed bundle verification", "type": type(exc).__name__,
            "detail": str(exc),
        }]) from exc
    return committed_path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--policy", type=Path)
    parser.add_argument("--authorization", type=Path)
    parser.add_argument("--now-utc", help="canonical whole-second UTC timestamp ending in Z")
    parser.add_argument("--evidence-root", type=Path)
    parser.add_argument("command", choices=("validate", "execute", "verify-bundle"))
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.command == "verify-bundle":
            if args.evidence_root is None:
                raise MaintenanceError("verify-bundle requires --evidence-root")
            terminal = verify_committed_bundle(args.evidence_root)
            print(json.dumps({
                "valid": True,
                "offline_domain_only": True,
                "authorization_id": terminal["authorization_id"],
                "recovery_complete": True,
                "performance_result": None,
            }, indent=2, sort_keys=True))
            return 0
        if args.now_utc is None:
            raise MaintenanceError(f"{args.command} requires --now-utc")
        now = parse_utc(args.now_utc, "--now-utc")
        if args.command == "execute":
            if not TARGET_EXECUTION_ENABLED:
                raise MaintenanceError(
                    "real target execution is hard-disabled: this controller defines no target Runner; issue #41 remains the authority gate")
            raise MaintenanceError("no real target Runner is implemented")
        if args.policy is None or args.authorization is None:
            raise MaintenanceError("validate requires --policy and --authorization")
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
