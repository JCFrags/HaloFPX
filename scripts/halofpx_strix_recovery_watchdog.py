#!/usr/bin/env python3
"""Offline-only dual-Strix recovery-watchdog state-machine model.

This module contains no target transport, process launcher, service manager, or
target runner.  It models two logically node-local watchdogs over built-in
in-memory fake nodes and retains deterministic local and paired receipts.  The
opaque authority input is assumed to have been verified by an external gate; the
model only checks its closed fields and SHA-256 binding.  That assumption is
not authentication and cannot authorize either physical Strix Halo machine.

The only command-line operation verifies an already-created offline pair.
"""

from __future__ import annotations

import argparse
import dataclasses
import enum
import hashlib
import json
import os
import re
from pathlib import Path, PurePosixPath
from typing import Any, Iterable, Sequence


TARGET_EXECUTION_ENABLED = False
OFFLINE_FAKE_ONLY = True

AUTHORITY_SCHEMA = "halofpx.strix-watchdog-preverified-authority.v1"
ARM_SCHEMA = "halofpx.strix-watchdog-arm.v1"
PEER_ARM_SCHEMA = "halofpx.strix-watchdog-peer-arm.v1"
EVENT_SCHEMA = "halofpx.strix-watchdog-event.v1"
READY_SCHEMA = "halofpx.strix-watchdog-ready.v1"
TERMINAL_SCHEMA = "halofpx.strix-watchdog-local-terminal.v1"
MANIFEST_SCHEMA = "halofpx.strix-watchdog-local-manifest.v1"
LOCAL_FINAL_SCHEMA = "halofpx.strix-watchdog-local-finalized.v1"
PAIR_ACK_SCHEMA = "halofpx.strix-watchdog-pair-ack.v1"
PAIR_FINAL_SCHEMA = "halofpx.strix-watchdog-pair-finalized.v1"

COORDINATOR_HOST = "nimo-1"
WORKER_HOST = "nimo-2"
COORDINATOR_UNIT = "minimax-m27-q6-server.service"
WORKER_UNIT = "minimax-m27-rpc-worker.service"
PROTECTED_PORTS = {"8081", "50052"}
HOST_ROLE = {COORDINATOR_HOST: "coordinator", WORKER_HOST: "worker"}
ROLE_HOST = {value: key for key, value in HOST_ROLE.items()}

HASH_RE = re.compile(r"^[0-9a-f]{64}$")
HEX32_RE = re.compile(r"^[0-9a-f]{32}$")
COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")
TOKEN_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._:-]{0,127}$")
UNIT_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.@:-]{0,126}\.service$")
PATH_RE = re.compile(r"^/var/tmp/halofpx-watchdog-[A-Za-z0-9][A-Za-z0-9._/-]{0,220}$")
MAX_JSON_BYTES = 1_048_576


class WatchdogError(RuntimeError):
    """Closed failure from the offline watchdog model or verifier."""


class LostResponse(WatchdogError):
    """The fake actuator completed or may have completed, but its reply was lost."""


class DeadlineExpired(WatchdogError):
    """The local monotonic recovery deadline was reached."""


class ControllerLossPoint(str, enum.Enum):
    BEFORE_FIRST_STOP = "before-first-stop"
    AFTER_COORDINATOR_STOP = "after-coordinator-stop"
    AFTER_BOTH_STOPS = "after-both-stops"
    DURING_EXPERIMENT = "during-experiment"


RECOVERY_STEPS = (
    "boot-reconcile",
    "pre-cleanup-scan",
    "cleanup-actuation",
    "cleanup-observation",
    "pre-recovery-hmm",
    "peer-ready-receipt",
    "service-observation",
    "service-recovery-actuation",
    "service-postcondition",
    "readiness",
    "final-hmm",
    "kernel-reconcile",
)
RECOVERY_PHASES = tuple(
    f"{role}:{step}" for role in ("worker", "coordinator") for step in RECOVERY_STEPS
)


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(
        value, allow_nan=False, ensure_ascii=False, sort_keys=True, separators=(",", ":"),
    ) + "\n").encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _require_hash(value: Any, where: str) -> str:
    if not isinstance(value, str) or HASH_RE.fullmatch(value) is None:
        raise WatchdogError(f"{where} must be one lowercase SHA-256 digest")
    return value


def _require_token(value: Any, where: str) -> str:
    if not isinstance(value, str) or TOKEN_RE.fullmatch(value) is None:
        raise WatchdogError(f"{where} is not one closed token")
    return value


def _require_exact(value: Any, keys: Iterable[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != set(keys):
        raise WatchdogError(f"{where} does not have the exact closed fields")
    return value


def _reject_duplicate_pairs(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise WatchdogError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _reject_json_constant(value: str) -> None:
    raise WatchdogError(f"non-finite JSON number is forbidden: {value}")


def parse_canonical_json(raw: bytes, where: str) -> dict[str, Any]:
    if not isinstance(raw, bytes) or not raw or len(raw) > MAX_JSON_BYTES:
        raise WatchdogError(f"{where} is empty, oversized, or not bytes")
    try:
        value = json.loads(
            raw.decode("utf-8"),
            object_pairs_hook=_reject_duplicate_pairs,
            parse_constant=_reject_json_constant,
        )
        encoded = canonical_bytes(value)
    except (
            UnicodeDecodeError, UnicodeEncodeError, json.JSONDecodeError,
            RecursionError, ValueError) as exc:
        raise WatchdogError(f"{where} is not canonical UTF-8 JSON") from exc
    if not isinstance(value, dict) or encoded != raw:
        raise WatchdogError(f"{where} is not in the canonical JSON encoding")
    return value


@dataclasses.dataclass(frozen=True)
class ServiceIdentity:
    host: str
    role: str
    unit: str
    pid: int
    invocation_id: str
    restart_count: int
    process_start_monotonic_ns: int
    active_enter_monotonic_ns: int
    boot_id: str

    def validate(self, where: str) -> None:
        if self.host not in HOST_ROLE or HOST_ROLE[self.host] != self.role:
            raise WatchdogError(f"{where} host/role is not the fixed two-node topology")
        expected_unit = COORDINATOR_UNIT if self.role == "coordinator" else WORKER_UNIT
        if self.unit != expected_unit:
            raise WatchdogError(f"{where} unit is not the protected production unit")
        if type(self.pid) is not int or not 1 <= self.pid <= 2**31 - 1:
            raise WatchdogError(f"{where} pid must be a positive signed 32-bit value")
        if not isinstance(self.invocation_id, str) or HEX32_RE.fullmatch(self.invocation_id) is None:
            raise WatchdogError(f"{where} invocation_id must be 32 lowercase hex characters")
        if type(self.restart_count) is not int or not 0 <= self.restart_count <= 2**63 - 1:
            raise WatchdogError(f"{where} restart_count must be a non-negative signed 64-bit value")
        for name in ("process_start_monotonic_ns", "active_enter_monotonic_ns"):
            value = getattr(self, name)
            if type(value) is not int or not 0 <= value <= 2**63 - 1:
                raise WatchdogError(f"{where} {name} must be a non-negative signed 64-bit value")
        if self.active_enter_monotonic_ns < self.process_start_monotonic_ns:
            raise WatchdogError(f"{where} active-enter identity precedes process start")
        if not isinstance(self.boot_id, str) or HEX32_RE.fullmatch(self.boot_id) is None:
            raise WatchdogError(f"{where} boot_id must be 32 lowercase hex characters")

    def as_dict(self) -> dict[str, Any]:
        return dataclasses.asdict(self)

    @classmethod
    def from_dict(cls, value: Any, where: str) -> "ServiceIdentity":
        row = _require_exact(value, {field.name for field in dataclasses.fields(cls)}, where)
        identity = cls(**row)
        identity.validate(where)
        return identity


@dataclasses.dataclass(frozen=True, order=True)
class DisposableTarget:
    host: str
    kind: str
    value: str

    def validate(self, where: str) -> None:
        if self.host not in HOST_ROLE:
            raise WatchdogError(f"{where} host is outside the fixed pair")
        if self.kind not in {"path", "port", "unit"}:
            raise WatchdogError(f"{where} kind is not path, port, or unit")
        if not isinstance(self.value, str) or not self.value or any(
                marker in self.value for marker in ("*", "?", "[", "]", "\x00")):
            raise WatchdogError(f"{where} is not one literal disposable target")
        if self.kind == "unit":
            if UNIT_RE.fullmatch(self.value) is None or not self.value.startswith("halofpx-watchdog-"):
                raise WatchdogError(f"{where} unit is outside the watchdog disposable namespace")
            if self.value in {COORDINATOR_UNIT, WORKER_UNIT}:
                raise WatchdogError(f"{where} aliases a protected production unit")
        elif self.kind == "port":
            if not self.value.isascii() or not self.value.isdigit():
                raise WatchdogError(f"{where} port is not canonical decimal")
            port = int(self.value)
            if str(port) != self.value or port < 1024 or port > 65535 or self.value in PROTECTED_PORTS:
                raise WatchdogError(f"{where} port is unsafe or protected")
        else:
            path = PurePosixPath(self.value)
            parts = path.parts
            if PATH_RE.fullmatch(self.value) is None or not path.is_absolute() or \
                    self.value in {"/", "/var", "/var/tmp"} or \
                    path.as_posix() != self.value or ".." in parts or \
                    not self.value.startswith("/var/tmp/halofpx-watchdog-"):
                raise WatchdogError(f"{where} path is outside the exact disposable root namespace")

    def as_dict(self) -> dict[str, str]:
        return dataclasses.asdict(self)

    @classmethod
    def from_dict(cls, value: Any, where: str) -> "DisposableTarget":
        row = _require_exact(value, {"host", "kind", "value"}, where)
        target = cls(**row)
        target.validate(where)
        return target


@dataclasses.dataclass(frozen=True)
class KernelState:
    host: str
    boot_id: str
    monotonic_ns: int
    global_oom_count: int
    oom_kill_count: int
    amdgpu_fault_count: int
    gpu_reset_count: int
    kfd_fault_count: int

    def validate(self, where: str) -> None:
        if self.host not in HOST_ROLE:
            raise WatchdogError(f"{where} host is unknown")
        if not isinstance(self.boot_id, str) or HEX32_RE.fullmatch(self.boot_id) is None:
            raise WatchdogError(f"{where} boot_id is invalid")
        for name in (
                "monotonic_ns", "global_oom_count", "oom_kill_count",
                "amdgpu_fault_count", "gpu_reset_count", "kfd_fault_count"):
            value = getattr(self, name)
            if type(value) is not int or not 0 <= value <= 2**63 - 1:
                raise WatchdogError(f"{where} {name} must be a non-negative signed 64-bit value")

    def as_dict(self) -> dict[str, Any]:
        return dataclasses.asdict(self)

    @classmethod
    def from_dict(cls, value: Any, where: str) -> "KernelState":
        row = _require_exact(value, {field.name for field in dataclasses.fields(cls)}, where)
        state = cls(**row)
        state.validate(where)
        return state


@dataclasses.dataclass(frozen=True)
class HMMOwner:
    host: str
    unit: str
    pid: int
    invocation_id: str
    hmm_bytes: int
    kfd_fd_count: int
    render_fd_count: int

    def validate(self, where: str) -> None:
        if self.host not in HOST_ROLE or self.unit not in {COORDINATOR_UNIT, WORKER_UNIT}:
            raise WatchdogError(f"{where} owner is outside protected production")
        if type(self.pid) is not int or not 1 <= self.pid <= 2**31 - 1:
            raise WatchdogError(f"{where} pid is invalid")
        if not isinstance(self.invocation_id, str) or HEX32_RE.fullmatch(self.invocation_id) is None:
            raise WatchdogError(f"{where} invocation is invalid")
        for name in ("hmm_bytes", "kfd_fd_count", "render_fd_count"):
            value = getattr(self, name)
            if type(value) is not int or not 1 <= value <= 2**63 - 1:
                raise WatchdogError(f"{where} {name} must be positive signed 64-bit")

    def as_dict(self) -> dict[str, Any]:
        return dataclasses.asdict(self)

    @classmethod
    def from_dict(cls, value: Any, where: str) -> "HMMOwner":
        row = _require_exact(value, {field.name for field in dataclasses.fields(cls)}, where)
        owner = cls(**row)
        owner.validate(where)
        return owner


@dataclasses.dataclass(frozen=True)
class PreverifiedAuthority:
    """Opaque externally preverified authority plus a local closed hash binding.

    Neither this type nor its validation authenticates a signer.  A future real
    controller must receive this object only from a separate cryptographic and
    two-node nonce-consumption gate.
    """

    schema: str
    transaction_id: str
    nonce_sha256: str
    source_commit: str
    executable_sha256: str
    verification_receipt_sha256: str
    maintenance_lease_ns: int
    recovery_timeout_ns: int
    coordinator_before: ServiceIdentity
    worker_before: ServiceIdentity
    disposable_allowlist: tuple[DisposableTarget, ...]
    authority_sha256: str

    def payload(self) -> dict[str, Any]:
        return {
            "schema": self.schema,
            "transaction_id": self.transaction_id,
            "nonce_sha256": self.nonce_sha256,
            "source_commit": self.source_commit,
            "executable_sha256": self.executable_sha256,
            "verification_receipt_sha256": self.verification_receipt_sha256,
            "maintenance_lease_ns": self.maintenance_lease_ns,
            "recovery_timeout_ns": self.recovery_timeout_ns,
            "coordinator_before": self.coordinator_before.as_dict(),
            "worker_before": self.worker_before.as_dict(),
            "disposable_allowlist": [target.as_dict() for target in self.disposable_allowlist],
        }

    def as_dict(self) -> dict[str, Any]:
        return {**self.payload(), "authority_sha256": self.authority_sha256}

    def validate(self) -> None:
        if self.schema != AUTHORITY_SCHEMA:
            raise WatchdogError("authority schema is not admitted")
        _require_token(self.transaction_id, "authority transaction_id")
        _require_hash(self.nonce_sha256, "authority nonce")
        if not isinstance(self.source_commit, str) or COMMIT_RE.fullmatch(self.source_commit) is None:
            raise WatchdogError("authority source_commit is not 40 lowercase hex characters")
        _require_hash(self.executable_sha256, "authority executable")
        _require_hash(self.verification_receipt_sha256, "authority verification receipt")
        _require_hash(self.authority_sha256, "authority binding")
        if type(self.maintenance_lease_ns) is not int or not 1_000_000 <= self.maintenance_lease_ns <= 28_800_000_000_000:
            raise WatchdogError("maintenance lease is outside one millisecond through eight hours")
        if type(self.recovery_timeout_ns) is not int or not 1_000_000 <= self.recovery_timeout_ns <= 3_600_000_000_000:
            raise WatchdogError("recovery timeout is outside one millisecond through one hour")
        if not isinstance(self.coordinator_before, ServiceIdentity) or \
                not isinstance(self.worker_before, ServiceIdentity):
            raise WatchdogError("authority before identities have the wrong type")
        self.coordinator_before.validate("authority coordinator")
        self.worker_before.validate("authority worker")
        if self.coordinator_before.host != COORDINATOR_HOST or self.worker_before.host != WORKER_HOST:
            raise WatchdogError("authority reverses the fixed coordinator/worker topology")
        if not isinstance(self.disposable_allowlist, tuple) or not self.disposable_allowlist:
            raise WatchdogError("authority disposable allowlist is not one non-empty tuple")
        for index, target in enumerate(self.disposable_allowlist):
            if not isinstance(target, DisposableTarget):
                raise WatchdogError(f"authority disposable_allowlist[{index}] has the wrong type")
            target.validate(f"authority disposable_allowlist[{index}]")
        if len(set(self.disposable_allowlist)) != len(self.disposable_allowlist):
            raise WatchdogError("authority disposable allowlist contains a duplicate")
        if self.disposable_allowlist != tuple(sorted(self.disposable_allowlist)):
            raise WatchdogError("authority disposable allowlist is not in canonical order")
        if {target.host for target in self.disposable_allowlist} != {COORDINATOR_HOST, WORKER_HOST}:
            raise WatchdogError("authority disposable allowlist does not cover both hosts")
        if sha256_bytes(canonical_bytes(self.payload())) != self.authority_sha256:
            raise WatchdogError("authority SHA-256 does not bind the exact closed payload")

    @classmethod
    def from_dict(cls, value: Any) -> "PreverifiedAuthority":
        row = _require_exact(value, {
            "schema", "transaction_id", "nonce_sha256", "source_commit",
            "executable_sha256", "verification_receipt_sha256", "maintenance_lease_ns",
            "recovery_timeout_ns", "coordinator_before", "worker_before",
            "disposable_allowlist", "authority_sha256",
        }, "preverified authority")
        if not isinstance(row["disposable_allowlist"], list):
            raise WatchdogError("authority disposable_allowlist is not a list")
        authority = cls(
            schema=row["schema"],
            transaction_id=row["transaction_id"],
            nonce_sha256=row["nonce_sha256"],
            source_commit=row["source_commit"],
            executable_sha256=row["executable_sha256"],
            verification_receipt_sha256=row["verification_receipt_sha256"],
            maintenance_lease_ns=row["maintenance_lease_ns"],
            recovery_timeout_ns=row["recovery_timeout_ns"],
            coordinator_before=ServiceIdentity.from_dict(row["coordinator_before"], "authority coordinator"),
            worker_before=ServiceIdentity.from_dict(row["worker_before"], "authority worker"),
            disposable_allowlist=tuple(
                DisposableTarget.from_dict(item, f"authority disposable_allowlist[{index}]")
                for index, item in enumerate(row["disposable_allowlist"])
            ),
            authority_sha256=row["authority_sha256"],
        )
        authority.validate()
        return authority


@dataclasses.dataclass(frozen=True)
class ModelFaults:
    peer_unreachable: bool = False
    reboot_host: str | None = None
    controller_deadline_phase: ControllerLossPoint | None = None
    recovery_deadline_phase: str | None = None
    cleanup_lost_response_host: str | None = None
    cleanup_no_effect_host: str | None = None
    worker_readiness_false: bool = False
    coordinator_start_lost_response: bool = False
    stale_reappearance_host: str | None = None
    unknown_disposable_host: str | None = None
    incomplete_disposable_scan_host: str | None = None
    foreign_hmm_owner_host: str | None = None
    incomplete_hmm_census_host: str | None = None
    non_elevated_hmm_census_host: str | None = None
    restart_drift_host: str | None = None
    kernel_delta_host: str | None = None
    tamper_worker_ready_receipt: bool = False
    monotonic_regression_host: str | None = None

    def validate(self) -> None:
        for field in (
                "peer_unreachable", "worker_readiness_false",
                "coordinator_start_lost_response", "tamper_worker_ready_receipt"):
            if type(getattr(self, field)) is not bool:
                raise WatchdogError(f"fault {field} is not a boolean")
        host_fields = (
            "reboot_host", "cleanup_lost_response_host", "cleanup_no_effect_host",
            "stale_reappearance_host", "unknown_disposable_host",
            "incomplete_disposable_scan_host", "foreign_hmm_owner_host",
            "incomplete_hmm_census_host", "non_elevated_hmm_census_host",
            "restart_drift_host", "kernel_delta_host", "monotonic_regression_host",
        )
        for name in host_fields:
            value = getattr(self, name)
            if value is not None and value not in HOST_ROLE:
                raise WatchdogError(f"fault {name} names an unknown host")
        if self.controller_deadline_phase is not None and \
                not isinstance(self.controller_deadline_phase, ControllerLossPoint):
            raise WatchdogError("fault controller_deadline_phase is not one closed loss point")
        if self.recovery_deadline_phase is not None and self.recovery_deadline_phase not in RECOVERY_PHASES:
            raise WatchdogError("fault recovery_deadline_phase is not one closed phase")


@dataclasses.dataclass
class _ModelClock:
    current_ns: int
    step_ns: int = 1_000
    last_read_ns: int = -1

    def read(self, *, force: int | None = None, regress: bool = False) -> int:
        if regress:
            value = max(0, self.last_read_ns - 1)
        elif force is not None:
            value = force
            self.current_ns = max(self.current_ns, force)
        else:
            self.current_ns += self.step_ns
            value = self.current_ns
        if self.last_read_ns >= 0 and value <= self.last_read_ns:
            raise WatchdogError("local monotonic clock did not advance")
        self.last_read_ns = value
        return value


@dataclasses.dataclass
class _OfflineNode:
    before: ServiceIdentity
    clock: _ModelClock
    active_identity: ServiceIdentity | None
    listener_open: bool
    application_ready: bool
    disposables: set[DisposableTarget]
    kernel: KernelState
    disposable_scan_complete: bool = True
    hmm_complete: bool = True
    hmm_elevated: bool = True
    foreign_hmm_owner: bool = False
    cleanup_lost_response: bool = False
    cleanup_no_effect: bool = False
    readiness_false: bool = False
    start_lost_response: bool = False
    restart_drift: bool = False

    @classmethod
    def initial(cls, before: ServiceIdentity) -> "_OfflineNode":
        return cls(
            before=before,
            clock=_ModelClock(max(
                before.process_start_monotonic_ns,
                before.active_enter_monotonic_ns,
            ) + 10_000),
            active_identity=before,
            listener_open=True,
            application_ready=True,
            disposables=set(),
            kernel=KernelState(
                host=before.host,
                boot_id=before.boot_id,
                monotonic_ns=max(
                    before.process_start_monotonic_ns,
                    before.active_enter_monotonic_ns,
                ) + 5_000,
                global_oom_count=0,
                oom_kill_count=0,
                amdgpu_fault_count=0,
                gpu_reset_count=0,
                kfd_fault_count=0,
            ),
        )

    @property
    def host(self) -> str:
        return self.before.host

    @property
    def role(self) -> str:
        return self.before.role

    def stop_by_controller(self) -> None:
        self.active_identity = None
        self.listener_open = False
        self.application_ready = False

    def start_production(self, transaction_id: str) -> ServiceIdentity:
        process_start = max(
            self.before.process_start_monotonic_ns + 1,
            self.clock.current_ns + 1,
        )
        fresh = ServiceIdentity(
            host=self.before.host,
            role=self.before.role,
            unit=self.before.unit,
            pid=self.before.pid + 10_000,
            invocation_id=hashlib.sha256(
                f"{transaction_id}:{self.host}:{self.role}:fresh".encode("ascii")
            ).hexdigest()[:32],
            restart_count=self.before.restart_count + (1 if self.restart_drift else 0),
            process_start_monotonic_ns=process_start,
            active_enter_monotonic_ns=max(
                self.before.active_enter_monotonic_ns + 1,
                process_start + 1,
            ),
            boot_id=self.kernel.boot_id,
        )
        self.active_identity = fresh
        self.listener_open = True
        self.application_ready = not self.readiness_false
        if self.start_lost_response:
            raise LostResponse("synthetic production-start response lost after effect")
        return fresh

    def cleanup(self, allowlist: set[DisposableTarget]) -> None:
        if not self.cleanup_no_effect:
            self.disposables.difference_update(allowlist)
        if self.cleanup_lost_response:
            raise LostResponse("synthetic cleanup response lost after effect")

    def hmm_census(self) -> dict[str, Any]:
        owners: list[HMMOwner] = []
        if self.active_identity is not None:
            owners.append(HMMOwner(
                host=self.host,
                unit=self.active_identity.unit,
                pid=self.active_identity.pid,
                invocation_id=self.active_identity.invocation_id,
                hmm_bytes=1_048_576,
                kfd_fd_count=1,
                render_fd_count=1,
            ))
        if self.foreign_hmm_owner:
            owners.append(HMMOwner(
                host=self.host,
                unit=self.before.unit,
                pid=999_999,
                invocation_id="f" * 32,
                hmm_bytes=4096,
                kfd_fd_count=1,
                render_fd_count=1,
            ))
        return {
            "complete": self.hmm_complete,
            "elevated": self.hmm_elevated,
            "errors": [] if self.hmm_complete and self.hmm_elevated else ["synthetic-incomplete"],
            "owners": [owner.as_dict() for owner in owners],
        }

    def kernel_observation(self) -> KernelState:
        return dataclasses.replace(
            self.kernel,
            monotonic_ns=max(self.kernel.monotonic_ns + 1, self.clock.current_ns),
        )

    def observation(self) -> dict[str, Any]:
        return {
            "service": None if self.active_identity is None else self.active_identity.as_dict(),
            "listener_open": self.listener_open,
            "application_ready": self.application_ready,
            "disposable_scan_complete": self.disposable_scan_complete,
            "disposables": [target.as_dict() for target in sorted(self.disposables)],
            "hmm_census": self.hmm_census(),
            "kernel": self.kernel_observation().as_dict(),
        }


class _Custody:
    def __init__(self, root: Path):
        self.root = root
        self._events: list[Path] = []
        self._counter = 0
        root.mkdir(parents=False, exist_ok=False)

    @staticmethod
    def _sync_parent(path: Path) -> str:
        if os.name == "nt":
            return "file-sync-only"
        descriptor = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
        return "file-and-directory-sync"

    def write_once(self, name: str, raw: bytes) -> Path:
        if not name or "/" in name or "\\" in name or name.startswith("."):
            raise WatchdogError("custody filename is not one literal leaf")
        path = self.root / name
        temporary = self.root / f".{name}.{self._counter:08d}.tmp"
        self._counter += 1
        try:
            with temporary.open("xb") as stream:
                stream.write(raw)
                stream.flush()
                os.fsync(stream.fileno())
            os.link(temporary, path)
            self._sync_parent(path)
        except FileExistsError as exc:
            raise WatchdogError(f"custody path already exists: {name}") from exc
        finally:
            try:
                temporary.unlink()
            except FileNotFoundError:
                pass
        return path

    def write_json(self, name: str, value: dict[str, Any]) -> Path:
        return self.write_once(name, canonical_bytes(value))

    def event(self, host: str, role: str, phase: str, monotonic_ns: int,
              outcome: str, detail: str) -> Path:
        if outcome not in {"accepted", "failed", "lost-response", "preserved", "skipped"}:
            raise WatchdogError("event outcome is not closed")
        if not isinstance(detail, str) or len(detail) > 512 or any(ord(char) < 32 for char in detail):
            raise WatchdogError("event detail is not bounded printable text")
        path = self.write_json(f"event-{len(self._events) + 1:04d}.json", {
            "schema": EVENT_SCHEMA,
            "sequence": len(self._events) + 1,
            "host": host,
            "role": role,
            "phase": phase,
            "monotonic_ns": monotonic_ns,
            "outcome": outcome,
            "detail": detail,
        })
        self._events.append(path)
        return path

    @property
    def event_count(self) -> int:
        return len(self._events)

    @property
    def last_event_sha256(self) -> str:
        return sha256_bytes(self._events[-1].read_bytes()) if self._events else "0" * 64

    def finalize_local(self, *, host: str, role: str, terminal: dict[str, Any]) -> tuple[str, str]:
        terminal_path = self.write_json("local-terminal.json", terminal)
        members = {
            path.name: sha256_bytes(path.read_bytes())
            for path in sorted(self.root.iterdir(), key=lambda item: item.name)
            if path.is_file() and not path.name.startswith(".")
        }
        manifest = {
            "schema": MANIFEST_SCHEMA,
            "host": host,
            "role": role,
            "files": members,
        }
        manifest_path = self.write_json("LOCAL-SHA256SUMS.json", manifest)
        marker = {
            "schema": LOCAL_FINAL_SCHEMA,
            "host": host,
            "role": role,
            "terminal_sha256": sha256_bytes(terminal_path.read_bytes()),
            "manifest_sha256": sha256_bytes(manifest_path.read_bytes()),
            "publication_durability": self._sync_parent(manifest_path),
        }
        self.write_json("LOCAL-FINALIZED.json", marker)
        return marker["terminal_sha256"], sha256_bytes(canonical_bytes(marker))

    def publish_pair(self, pair_ack: dict[str, Any]) -> None:
        ack_path = self.write_json("pair-ack.json", pair_ack)
        self.write_json("PAIR-FINALIZED.json", {
            "schema": PAIR_FINAL_SCHEMA,
            "host": self.root.name,
            "pair_ack_sha256": sha256_bytes(ack_path.read_bytes()),
            "publication_durability": self._sync_parent(ack_path),
        })


@dataclasses.dataclass(frozen=True)
class OfflineModelResult:
    evidence_root: Path
    status: str
    recovery_complete: bool
    paired: bool
    experiment_continuation_allowed: bool


class _NodeWatchdog:
    def __init__(self, *, authority: PreverifiedAuthority, node: _OfflineNode,
                 custody: _Custody, faults: ModelFaults):
        self.authority = authority
        self.node = node
        self.custody = custody
        self.faults = faults
        self.before = authority.worker_before if node.role == "worker" else authority.coordinator_before
        self.allowlist = {target for target in authority.disposable_allowlist if target.host == node.host}
        self.arm_monotonic_ns = 0
        self.controller_deadline_ns = 0
        self.recovery_deadline_ns = 0
        self.arm_sha256 = ""
        self.peer_arm_sha256 = ""
        self.observed_absent = False
        self.triggered = False
        self.errors: list[dict[str, str]] = []
        self.cleanup_complete = False
        self.recovery_ready = False
        self.worker_ready_receipt_sha256: str | None = None
        self.ready_receipt_sha256: str | None = None
        self.local_terminal_sha256 = ""
        self.local_marker_sha256 = ""

    def _read_clock(self, phase: str, *, deadline: int | None = None) -> int:
        expire = self.faults.recovery_deadline_phase == f"{self.node.role}:{phase}"
        regress = self.faults.monotonic_regression_host == self.node.host and phase == "boot-reconcile"
        return self.node.clock.read(force=deadline if expire else None, regress=regress)

    def _event(self, phase: str, outcome: str, detail: str, *, now: int | None = None) -> None:
        if now is None:
            now = self.node.clock.read()
        self.custody.event(self.node.host, self.node.role, phase, now, outcome, detail)

    def _error(self, phase: str, code: str, detail: str) -> None:
        self.errors.append({"phase": phase, "code": code, "detail": detail[:512]})

    def arm(self) -> dict[str, Any]:
        if self.triggered or self.arm_sha256:
            raise WatchdogError("watchdog arm is single-use")
        self.arm_monotonic_ns = self.node.clock.read()
        if self.arm_monotonic_ns > (2**63 - 1) - self.authority.maintenance_lease_ns:
            raise WatchdogError("controller deadline overflows signed range")
        self.controller_deadline_ns = self.arm_monotonic_ns + self.authority.maintenance_lease_ns
        if self.node.active_identity != self.before or not self.node.listener_open or not self.node.application_ready:
            raise WatchdogError(f"{self.node.host} protected before identity is not exact and ready")
        self._validate_hmm(self.node.hmm_census(), self.before, "arm HMM census")
        baseline = self.node.kernel_observation()
        baseline.validate("arm kernel")
        if baseline.boot_id != self.before.boot_id:
            raise WatchdogError("arm boot identity differs from protected service identity")
        self.custody.write_json("authority.json", self.authority.as_dict())
        arm = {
            "schema": ARM_SCHEMA,
            "host": self.node.host,
            "role": self.node.role,
            "authority_sha256": self.authority.authority_sha256,
            "verification_receipt_sha256": self.authority.verification_receipt_sha256,
            "arm_monotonic_ns": self.arm_monotonic_ns,
            "controller_deadline_ns": self.controller_deadline_ns,
            "before_identity": self.before.as_dict(),
            "kernel_baseline": baseline.as_dict(),
            "disposable_allowlist_sha256": sha256_bytes(canonical_bytes(
                [target.as_dict() for target in sorted(self.allowlist)]
            )),
            "target_execution_enabled": False,
            "offline_fake_only": True,
        }
        path = self.custody.write_json("arm.json", arm)
        self.arm_sha256 = sha256_bytes(path.read_bytes())
        self._event("armed", "accepted", f"arm_sha256={self.arm_sha256}")
        return {"host": self.node.host, "role": self.node.role, "arm_sha256": self.arm_sha256}

    def accept_peer_arm(self, peer: dict[str, Any]) -> None:
        if not self.arm_sha256 or self.peer_arm_sha256 or self.triggered:
            raise WatchdogError("peer arm is out of order or already consumed")
        row = _require_exact(peer, {"host", "role", "arm_sha256"}, "peer arm")
        expected_host = COORDINATOR_HOST if self.node.host == WORKER_HOST else WORKER_HOST
        if row["host"] != expected_host or row["role"] != HOST_ROLE[expected_host]:
            raise WatchdogError("peer arm names the wrong node or role")
        _require_hash(row["arm_sha256"], "peer arm digest")
        receipt = {
            "schema": PEER_ARM_SCHEMA,
            "host": self.node.host,
            "role": self.node.role,
            "authority_sha256": self.authority.authority_sha256,
            "local_arm_sha256": self.arm_sha256,
            "peer_host": row["host"],
            "peer_role": row["role"],
            "peer_arm_sha256": row["arm_sha256"],
        }
        path = self.custody.write_json("peer-arm.json", receipt)
        self.peer_arm_sha256 = sha256_bytes(path.read_bytes())
        self._event("peer-armed", "accepted", f"peer_arm_sha256={row['arm_sha256']}")

    def note_local_absence(self) -> None:
        if not self.peer_arm_sha256 or self.triggered or self.node.active_identity is not None:
            raise WatchdogError("local absence was not observed while paired and inactive")
        self.observed_absent = True

    def trigger(self, reason: str) -> None:
        if not self.peer_arm_sha256 or self.triggered:
            raise WatchdogError("watchdog cannot trigger before pairing or more than once")
        if reason not in {"maintenance-deadline-expired", "controller-or-control-network-lost"}:
            raise WatchdogError("watchdog trigger reason is not closed")
        now = self.node.clock.read()
        if reason == "maintenance-deadline-expired" and now < self.controller_deadline_ns:
            raise WatchdogError("deadline trigger precedes the armed controller deadline")
        if reason != "maintenance-deadline-expired" and now >= self.controller_deadline_ns:
            raise WatchdogError("controller-loss trigger crossed the armed controller deadline")
        if now > (2**63 - 1) - self.authority.recovery_timeout_ns:
            raise WatchdogError("recovery deadline overflows signed range")
        self.recovery_deadline_ns = now + self.authority.recovery_timeout_ns
        self.triggered = True
        self._event("trigger", "accepted", reason, now=now)

    def _checkpoint(self, step: str) -> int:
        if not self.triggered:
            raise WatchdogError("recovery cannot run before trigger")
        now = self._read_clock(step, deadline=self.recovery_deadline_ns)
        if now >= self.recovery_deadline_ns:
            raise DeadlineExpired(f"recovery deadline reached at {self.node.role}:{step}")
        return now

    @staticmethod
    def _validate_hmm(value: Any, expected: ServiceIdentity | None, where: str) -> None:
        row = _require_exact(value, {"complete", "elevated", "errors", "owners"}, where)
        if row["complete"] is not True or row["elevated"] is not True or row["errors"] != []:
            raise WatchdogError(f"{where} is not a complete elevated closed-world census")
        if not isinstance(row["owners"], list):
            raise WatchdogError(f"{where} owners are not a list")
        owners = [HMMOwner.from_dict(item, f"{where}.owners[{index}]")
                  for index, item in enumerate(row["owners"])]
        if expected is None:
            if owners:
                raise WatchdogError(f"{where} contains an owner while production is absent")
            return
        if len(owners) != 1:
            raise WatchdogError(f"{where} does not contain exactly the protected owner")
        owner = owners[0]
        if (owner.host, owner.unit, owner.pid, owner.invocation_id) != (
                expected.host, expected.unit, expected.pid, expected.invocation_id):
            raise WatchdogError(f"{where} owner identity differs from the protected service")

    def _validate_kernel(self, value: KernelState, baseline: KernelState, where: str) -> None:
        value.validate(where)
        if value.host != baseline.host or value.boot_id != baseline.boot_id:
            raise WatchdogError(f"{where} boot identity drifted")
        if value.monotonic_ns <= baseline.monotonic_ns:
            raise WatchdogError(f"{where} monotonic evidence did not advance")
        for name in (
                "global_oom_count", "oom_kill_count", "amdgpu_fault_count",
                "gpu_reset_count", "kfd_fault_count"):
            if getattr(value, name) != getattr(baseline, name):
                raise WatchdogError(f"{where} {name} changed")

    def _validate_service_identity(self, current: ServiceIdentity) -> str:
        current.validate("recovered service")
        if current.host != self.before.host or current.role != self.before.role or \
                current.unit != self.before.unit or current.boot_id != self.before.boot_id:
            raise WatchdogError("recovered service scope differs from authority")
        if current.restart_count != self.before.restart_count:
            raise WatchdogError("recovered service restart counter drifted")
        if self.observed_absent:
            if current.pid == self.before.pid or current.invocation_id == self.before.invocation_id:
                raise WatchdogError("a stale pre-stop service identity reappeared")
            if current.process_start_monotonic_ns <= self.before.process_start_monotonic_ns or \
                    current.active_enter_monotonic_ns <= self.before.active_enter_monotonic_ns:
                raise WatchdogError("recovered service monotonic identity is not fresh")
            return "fresh"
        if current != self.before:
            raise WatchdogError("an unobserved service identity change is not preservable")
        return "preserved"

    def _ready_receipt(self, current: ServiceIdentity) -> bytes:
        value = {
            "schema": READY_SCHEMA,
            "host": self.node.host,
            "role": self.node.role,
            "authority_sha256": self.authority.authority_sha256,
            "transaction_id": self.authority.transaction_id,
            "identity": current.as_dict(),
            "cleanup_complete": True,
            "hmm_reconciled": True,
            "kernel_reconciled": True,
            "listener_ready": True,
            "application_ready": True,
            "experiment_continuation_allowed": False,
        }
        path = self.custody.write_json("ready-receipt.json", value)
        self.ready_receipt_sha256 = sha256_bytes(path.read_bytes())
        return path.read_bytes()

    @staticmethod
    def validate_worker_ready(raw: bytes, authority: PreverifiedAuthority) -> dict[str, Any]:
        value = parse_canonical_json(raw, "worker ready receipt")
        row = _require_exact(value, {
            "schema", "host", "role", "authority_sha256", "transaction_id",
            "identity", "cleanup_complete", "hmm_reconciled", "kernel_reconciled",
            "listener_ready", "application_ready", "experiment_continuation_allowed",
        }, "worker ready receipt")
        if row["schema"] != READY_SCHEMA or row["host"] != WORKER_HOST or row["role"] != "worker" or \
                row["authority_sha256"] != authority.authority_sha256 or \
                row["transaction_id"] != authority.transaction_id or \
                any(row[name] is not True for name in (
                    "cleanup_complete", "hmm_reconciled", "kernel_reconciled",
                    "listener_ready", "application_ready",
                )) or row["experiment_continuation_allowed"] is not False:
            raise WatchdogError("worker ready receipt is not exact and recovery-only")
        identity = ServiceIdentity.from_dict(row["identity"], "worker ready identity")
        before = authority.worker_before
        if identity.host != WORKER_HOST or identity.role != "worker" or \
                identity.unit != WORKER_UNIT or identity.boot_id != before.boot_id or \
                identity.restart_count != before.restart_count:
            raise WatchdogError("worker ready identity is outside the armed production scope")
        if identity != before and (
                identity.pid == before.pid or identity.invocation_id == before.invocation_id or
                identity.process_start_monotonic_ns <= before.process_start_monotonic_ns or
                identity.active_enter_monotonic_ns <= before.active_enter_monotonic_ns):
            raise WatchdogError("worker ready identity is neither exact-preserved nor exact-fresh")
        return row

    def recover(self, worker_ready_raw: bytes | None) -> bytes | None:
        if self.local_terminal_sha256:
            raise WatchdogError("watchdog recovery is single-use")
        if self.node.role == "worker" and worker_ready_raw is not None:
            raise WatchdogError("worker recovery cannot consume a peer-ready receipt")
        baseline_raw = parse_canonical_json((self.custody.root / "arm.json").read_bytes(), "arm")
        baseline = KernelState.from_dict(baseline_raw["kernel_baseline"], "arm kernel baseline")
        blocked = False
        current: ServiceIdentity | None = None
        worker_receipt_valid = self.node.role == "worker"

        for step in RECOVERY_STEPS:
            phase = f"{self.node.role}:{step}"
            if blocked:
                self._event(phase, "skipped", "prior fail-closed recovery refusal")
                continue
            try:
                now = self._checkpoint(step)
                if step == "boot-reconcile":
                    if self.node.kernel.boot_id != baseline.boot_id:
                        raise WatchdogError("local boot_id drift invalidates the armed monotonic epoch")
                    self._event(phase, "accepted", "boot identity and monotonic epoch match", now=now)
                elif step == "pre-cleanup-scan":
                    if not self.node.disposable_scan_complete:
                        raise WatchdogError("disposable scan is incomplete")
                    unknown = self.node.disposables - self.allowlist
                    if unknown:
                        raise WatchdogError("disposable scan contains a target outside the exact allowlist")
                    self._event(phase, "accepted", f"observed={len(self.node.disposables)}", now=now)
                elif step == "cleanup-actuation":
                    try:
                        self.node.cleanup(self.allowlist)
                    except LostResponse as exc:
                        self._error(phase, "lost-response", str(exc))
                        self._event(phase, "lost-response", "independent absence observation required", now=now)
                    else:
                        self._event(phase, "accepted", "exact allowlist cleanup returned", now=now)
                elif step == "cleanup-observation":
                    if not self.node.disposable_scan_complete or self.node.disposables:
                        raise WatchdogError("independent disposable absence is incomplete or contains residue")
                    self.cleanup_complete = True
                    self._event(phase, "accepted", "complete closed-world disposable absence", now=now)
                elif step == "pre-recovery-hmm":
                    self._validate_hmm(self.node.hmm_census(), self.node.active_identity, phase)
                    self._event(phase, "accepted", "closed-world HMM/KFD/render ownership matches", now=now)
                elif step == "peer-ready-receipt":
                    if self.node.role == "worker":
                        self._event(phase, "accepted", "worker recovery precedes peer dependency", now=now)
                    else:
                        if worker_ready_raw is None:
                            raise WatchdogError("coordinator cannot recover without the durable worker-ready receipt")
                        self.validate_worker_ready(worker_ready_raw, self.authority)
                        self.worker_ready_receipt_sha256 = sha256_bytes(worker_ready_raw)
                        worker_receipt_valid = True
                        self._event(
                            phase, "accepted",
                            f"sha256={self.worker_ready_receipt_sha256}", now=now,
                        )
                elif step == "service-observation":
                    current = self.node.active_identity
                    if current is not None:
                        disposition = self._validate_service_identity(current)
                        self._event(phase, "preserved", disposition, now=now)
                    else:
                        self._event(phase, "accepted", "protected service is absent", now=now)
                elif step == "service-recovery-actuation":
                    if self.node.active_identity is None:
                        try:
                            self.node.start_production(self.authority.transaction_id)
                        except LostResponse as exc:
                            self._error(phase, "lost-response", str(exc))
                            self._event(phase, "lost-response", "independent start postcondition required", now=now)
                        else:
                            self._event(phase, "accepted", "protected production start returned", now=now)
                    else:
                        self._event(phase, "preserved", "exact active production was not restarted", now=now)
                elif step == "service-postcondition":
                    current = self.node.active_identity
                    if current is None:
                        raise WatchdogError("protected service remains absent")
                    disposition = self._validate_service_identity(current)
                    self._event(phase, "accepted", disposition, now=now)
                elif step == "readiness":
                    if current is None or not self.node.listener_open or not self.node.application_ready:
                        raise WatchdogError("active service did not pass listener and application readiness")
                    self._event(phase, "accepted", "listener and application readiness passed", now=now)
                elif step == "final-hmm":
                    if current is None:
                        raise WatchdogError("no service identity exists for final HMM ownership")
                    self._validate_hmm(self.node.hmm_census(), current, phase)
                    self._event(phase, "accepted", "exact recovered HMM/KFD/render owner", now=now)
                elif step == "kernel-reconcile":
                    self._validate_kernel(self.node.kernel_observation(), baseline, phase)
                    self._event(phase, "accepted", "boot/OOM/reset/KFD counters reconcile", now=now)
                else:  # pragma: no cover - closed tuple above
                    raise AssertionError(step)
            except WatchdogError as exc:
                self._error(phase, type(exc).__name__, str(exc))
                self._event(phase, "failed", str(exc))
                blocked = True

        current = self.node.active_identity
        local_safe = (
            not blocked and self.cleanup_complete and current is not None and
            self.node.listener_open and self.node.application_ready and worker_receipt_valid
        )
        self.recovery_ready = local_safe
        ready_raw: bytes | None = None
        if local_safe:
            ready_raw = self._ready_receipt(current)
        self._event(
            "local-terminal", "accepted" if local_safe else "failed",
            f"recovery_ready={str(local_safe).lower()}",
        )
        terminal = {
            "schema": TERMINAL_SCHEMA,
            "status": "success" if not self.errors else "failure",
            "host": self.node.host,
            "role": self.node.role,
            "authority_sha256": self.authority.authority_sha256,
            "verification_receipt_sha256": self.authority.verification_receipt_sha256,
            "transaction_id": self.authority.transaction_id,
            "target_execution_enabled": False,
            "offline_fake_only": True,
            "experiment_continuation_allowed": False,
            "observed_absent": self.observed_absent,
            "cleanup_complete": self.cleanup_complete,
            "worker_ready_receipt_sha256": self.worker_ready_receipt_sha256,
            "ready_receipt_sha256": self.ready_receipt_sha256,
            "recovery_ready": self.recovery_ready,
            "before_identity": self.before.as_dict(),
            "final_observation": self.node.observation(),
            "errors": self.errors,
            "event_count": self.custody.event_count,
            "last_event_sha256": self.custody.last_event_sha256,
        }
        self.local_terminal_sha256, self.local_marker_sha256 = self.custody.finalize_local(
            host=self.node.host, role=self.node.role, terminal=terminal,
        )
        return ready_raw


def _validate_local_bundle(root: Path, authority: PreverifiedAuthority) -> dict[str, Any]:
    if root.name not in HOST_ROLE or not root.is_dir() or root.is_symlink():
        raise WatchdogError("local watchdog root is not one admitted host directory")
    entries = list(root.iterdir())
    if any(not path.is_file() or path.is_symlink() for path in entries):
        raise WatchdogError(f"{root.name} bundle contains a non-file or link")
    files = {path.name: path for path in entries}
    required = {
        "authority.json", "arm.json", "peer-arm.json", "local-terminal.json",
        "LOCAL-SHA256SUMS.json", "LOCAL-FINALIZED.json", "pair-ack.json",
        "PAIR-FINALIZED.json",
    }
    if not required.issubset(files):
        raise WatchdogError(f"{root.name} bundle is missing terminal custody files")

    manifest = parse_canonical_json(files["LOCAL-SHA256SUMS.json"].read_bytes(), "local manifest")
    _require_exact(manifest, {"schema", "host", "role", "files"}, "local manifest")
    if manifest["schema"] != MANIFEST_SCHEMA or manifest["host"] != root.name or \
            manifest["role"] != HOST_ROLE[root.name] or not isinstance(manifest["files"], dict):
        raise WatchdogError("local manifest identity is invalid")
    expected_pre_manifest = set(files) - {
        "LOCAL-SHA256SUMS.json", "LOCAL-FINALIZED.json", "pair-ack.json", "PAIR-FINALIZED.json",
    }
    if set(manifest["files"]) != expected_pre_manifest:
        raise WatchdogError("local manifest does not bind the exact pre-finalization inventory")
    for name, digest in manifest["files"].items():
        _require_hash(digest, f"local manifest {name}")
        if sha256_bytes(files[name].read_bytes()) != digest:
            raise WatchdogError(f"local manifest digest differs for {name}")

    marker = parse_canonical_json(files["LOCAL-FINALIZED.json"].read_bytes(), "local marker")
    _require_exact(marker, {
        "schema", "host", "role", "terminal_sha256", "manifest_sha256",
        "publication_durability",
    }, "local marker")
    if marker["schema"] != LOCAL_FINAL_SCHEMA or marker["host"] != root.name or \
            marker["role"] != HOST_ROLE[root.name] or \
            marker["terminal_sha256"] != sha256_bytes(files["local-terminal.json"].read_bytes()) or \
            marker["manifest_sha256"] != sha256_bytes(files["LOCAL-SHA256SUMS.json"].read_bytes()) or \
            marker["publication_durability"] not in {"file-sync-only", "file-and-directory-sync"}:
        raise WatchdogError("local finalized marker does not bind terminal custody")

    retained_authority = PreverifiedAuthority.from_dict(parse_canonical_json(
        files["authority.json"].read_bytes(), "retained authority",
    ))
    if retained_authority != authority:
        raise WatchdogError("local retained authority differs from pair authority")
    role = HOST_ROLE[root.name]
    before = authority.worker_before if role == "worker" else authority.coordinator_before
    local_allowlist = tuple(target for target in authority.disposable_allowlist if target.host == root.name)

    arm = parse_canonical_json(files["arm.json"].read_bytes(), "retained arm")
    _require_exact(arm, {
        "schema", "host", "role", "authority_sha256", "verification_receipt_sha256",
        "arm_monotonic_ns", "controller_deadline_ns", "before_identity", "kernel_baseline",
        "disposable_allowlist_sha256", "target_execution_enabled", "offline_fake_only",
    }, "retained arm")
    if type(arm["arm_monotonic_ns"]) is not int or arm["arm_monotonic_ns"] < 0 or \
            type(arm["controller_deadline_ns"]) is not int or \
            arm["controller_deadline_ns"] != arm["arm_monotonic_ns"] + authority.maintenance_lease_ns or \
            arm["controller_deadline_ns"] > 2**63 - 1:
        raise WatchdogError("retained arm monotonic lease is invalid")
    arm_before = ServiceIdentity.from_dict(arm["before_identity"], "retained arm before identity")
    baseline = KernelState.from_dict(arm["kernel_baseline"], "retained arm kernel")
    if arm["schema"] != ARM_SCHEMA or arm["host"] != root.name or arm["role"] != role or \
            arm["authority_sha256"] != authority.authority_sha256 or \
            arm["verification_receipt_sha256"] != authority.verification_receipt_sha256 or \
            arm_before != before or baseline.host != root.name or baseline.boot_id != before.boot_id or \
            baseline.monotonic_ns != arm["arm_monotonic_ns"] or \
            arm["disposable_allowlist_sha256"] != sha256_bytes(canonical_bytes(
                [target.as_dict() for target in local_allowlist]
            )) or arm["target_execution_enabled"] is not False or arm["offline_fake_only"] is not True:
        raise WatchdogError("retained arm identity, baseline, allowlist, or hard-off boundary differs")
    arm_sha256 = sha256_bytes(files["arm.json"].read_bytes())

    peer_arm = parse_canonical_json(files["peer-arm.json"].read_bytes(), "retained peer arm")
    _require_exact(peer_arm, {
        "schema", "host", "role", "authority_sha256", "local_arm_sha256",
        "peer_host", "peer_role", "peer_arm_sha256",
    }, "retained peer arm")
    peer_host = COORDINATOR_HOST if root.name == WORKER_HOST else WORKER_HOST
    if peer_arm["schema"] != PEER_ARM_SCHEMA or peer_arm["host"] != root.name or \
            peer_arm["role"] != role or peer_arm["authority_sha256"] != authority.authority_sha256 or \
            peer_arm["local_arm_sha256"] != arm_sha256 or peer_arm["peer_host"] != peer_host or \
            peer_arm["peer_role"] != HOST_ROLE[peer_host]:
        raise WatchdogError("retained peer arm is outside the exact paired topology")
    _require_hash(peer_arm["peer_arm_sha256"], "retained peer arm digest")

    terminal = parse_canonical_json(files["local-terminal.json"].read_bytes(), "local terminal")
    _require_exact(terminal, {
        "schema", "status", "host", "role", "authority_sha256",
        "verification_receipt_sha256", "transaction_id", "target_execution_enabled",
        "offline_fake_only", "experiment_continuation_allowed", "observed_absent",
        "cleanup_complete", "worker_ready_receipt_sha256", "ready_receipt_sha256",
        "recovery_ready", "before_identity", "final_observation", "errors",
        "event_count", "last_event_sha256",
    }, "local terminal")
    for name in ("observed_absent", "cleanup_complete", "recovery_ready"):
        if type(terminal[name]) is not bool:
            raise WatchdogError(f"local terminal {name} is not a boolean")
    if terminal["schema"] != TERMINAL_SCHEMA or terminal["host"] != root.name or \
            terminal["role"] != role or terminal["authority_sha256"] != authority.authority_sha256 or \
            terminal["verification_receipt_sha256"] != authority.verification_receipt_sha256 or \
            terminal["transaction_id"] != authority.transaction_id or \
            terminal["target_execution_enabled"] is not False or \
            terminal["offline_fake_only"] is not True or \
            terminal["experiment_continuation_allowed"] is not False or \
            terminal["status"] not in {"success", "failure"} or not isinstance(terminal["errors"], list):
        raise WatchdogError("local terminal identity or hard-off boundary is invalid")
    if terminal["status"] != ("success" if terminal["errors"] == [] else "failure"):
        raise WatchdogError("local terminal status does not agree with errors")
    terminal_before = ServiceIdentity.from_dict(terminal["before_identity"], "terminal before identity")
    if terminal_before != before:
        raise WatchdogError("local terminal before identity differs from authority")

    parsed_errors: list[dict[str, str]] = []
    for index, value in enumerate(terminal["errors"]):
        error = _require_exact(value, {"phase", "code", "detail"}, f"terminal error {index}")
        if error["phase"] not in {f"{role}:{step}" for step in RECOVERY_STEPS} or \
                error["code"] not in {"lost-response", "WatchdogError", "DeadlineExpired"} or \
                not isinstance(error["detail"], str) or len(error["detail"]) > 512 or \
                any(ord(char) < 32 for char in error["detail"]):
            raise WatchdogError("local terminal error is not a closed bounded recovery error")
        parsed_errors.append(error)

    observation = _require_exact(terminal["final_observation"], {
        "service", "listener_open", "application_ready", "disposable_scan_complete",
        "disposables", "hmm_census", "kernel",
    }, "terminal final observation")
    for name in ("listener_open", "application_ready", "disposable_scan_complete"):
        if type(observation[name]) is not bool:
            raise WatchdogError(f"terminal final observation {name} is not a boolean")
    final_identity = None if observation["service"] is None else ServiceIdentity.from_dict(
        observation["service"], "terminal final service",
    )
    if final_identity is not None and final_identity.host != root.name:
        raise WatchdogError("terminal final service names the wrong host")
    if final_identity is None and (observation["listener_open"] or observation["application_ready"]):
        raise WatchdogError("terminal reports readiness without a service identity")
    if not isinstance(observation["disposables"], list):
        raise WatchdogError("terminal disposable observation is not a list")
    disposables = tuple(DisposableTarget.from_dict(
        value, f"terminal disposable[{index}]",
    ) for index, value in enumerate(observation["disposables"]))
    if any(target.host != root.name for target in disposables) or disposables != tuple(sorted(set(disposables))):
        raise WatchdogError("terminal disposable observation is non-canonical or cross-host")
    hmm = _require_exact(
        observation["hmm_census"], {"complete", "elevated", "errors", "owners"},
        "terminal final HMM census",
    )
    if type(hmm["complete"]) is not bool or type(hmm["elevated"]) is not bool or \
            not isinstance(hmm["errors"], list) or not isinstance(hmm["owners"], list) or \
            hmm["errors"] not in ([], ["synthetic-incomplete"]):
        raise WatchdogError("terminal final HMM census has invalid closed fields")
    if (hmm["complete"] and hmm["elevated"]) != (hmm["errors"] == []):
        raise WatchdogError("terminal final HMM completeness disagrees with its errors")
    for index, value in enumerate(hmm["owners"]):
        owner = HMMOwner.from_dict(value, f"terminal final HMM owner {index}")
        if owner.host != root.name:
            raise WatchdogError("terminal final HMM owner names the wrong host")
    kernel = KernelState.from_dict(observation["kernel"], "terminal final kernel")
    if kernel.host != root.name:
        raise WatchdogError("terminal final kernel names the wrong host")

    events = sorted(
        (path for name, path in files.items() if re.fullmatch(r"event-[0-9]{4}\.json", name)),
        key=lambda path: path.name,
    )
    expected_phases = ["armed", "peer-armed", "trigger"] + [
        f"{role}:{step}" for step in RECOVERY_STEPS
    ] + ["local-terminal"]
    if type(terminal["event_count"]) is not int or terminal["event_count"] != len(events) or \
            len(events) != len(expected_phases):
        raise WatchdogError("local terminal event count differs from the exact state machine")
    previous_time = -1
    parsed_events: list[dict[str, Any]] = []
    blocked = False
    recovery_deadline_ns = 0
    for index, path in enumerate(events, start=1):
        event = parse_canonical_json(path.read_bytes(), f"event {index}")
        _require_exact(event, {
            "schema", "sequence", "host", "role", "phase", "monotonic_ns", "outcome", "detail",
        }, f"event {index}")
        if event["schema"] != EVENT_SCHEMA or event["sequence"] != index or \
                event["host"] != root.name or event["role"] != role or \
                event["phase"] != expected_phases[index - 1] or \
                event["outcome"] not in {"accepted", "failed", "lost-response", "preserved", "skipped"} or \
                type(event["monotonic_ns"]) is not int or \
                not 0 <= event["monotonic_ns"] <= 2**63 - 1 or \
                event["monotonic_ns"] <= previous_time or \
                not isinstance(event["detail"], str) or len(event["detail"]) > 512 or \
                any(ord(char) < 32 for char in event["detail"]):
            raise WatchdogError("local event sequence, identity, phase, or monotonic order is invalid")
        previous_time = event["monotonic_ns"]
        parsed_events.append(event)
        if index == 1 and (event["outcome"] != "accepted" or event["monotonic_ns"] <= arm["arm_monotonic_ns"] or
                           event["detail"] != f"arm_sha256={arm_sha256}"):
            raise WatchdogError("armed event does not bind the retained arm")
        if index == 2 and (event["outcome"] != "accepted" or
                           event["detail"] != f"peer_arm_sha256={peer_arm['peer_arm_sha256']}"):
            raise WatchdogError("peer-armed event does not bind the paired arm")
        if index == 3:
            if event["outcome"] != "accepted" or event["detail"] not in {
                    "maintenance-deadline-expired", "controller-or-control-network-lost"}:
                raise WatchdogError("trigger event is not one closed recovery trigger")
            if event["detail"] == "maintenance-deadline-expired" and \
                    event["monotonic_ns"] < arm["controller_deadline_ns"]:
                raise WatchdogError("maintenance deadline trigger precedes the armed deadline")
            if event["detail"] != "maintenance-deadline-expired" and \
                    event["monotonic_ns"] >= arm["controller_deadline_ns"]:
                raise WatchdogError("controller-loss trigger improperly crosses the armed deadline")
            recovery_deadline_ns = event["monotonic_ns"] + authority.recovery_timeout_ns
            if recovery_deadline_ns > 2**63 - 1:
                raise WatchdogError("retained recovery deadline overflows signed range")
        if 4 <= index < len(expected_phases):
            if blocked and event["outcome"] != "skipped":
                raise WatchdogError("recovery performed an action after a fail-closed refusal")
            if not blocked and event["outcome"] == "skipped":
                raise WatchdogError("recovery skipped a phase without a prior refusal")
            if event["outcome"] == "lost-response" and event["phase"] not in {
                    f"{role}:cleanup-actuation", f"{role}:service-recovery-actuation"}:
                raise WatchdogError("lost-response occurred outside an ambiguous actuator phase")
            if event["outcome"] == "preserved" and event["phase"] not in {
                    f"{role}:service-observation", f"{role}:service-recovery-actuation"}:
                raise WatchdogError("preserved occurred outside a production service phase")
            if event["outcome"] in {"accepted", "lost-response", "preserved"} and \
                    event["monotonic_ns"] >= recovery_deadline_ns:
                raise WatchdogError("recovery accepted an action at or after its monotonic deadline")
            if event["outcome"] == "failed":
                blocked = True
        if index == len(expected_phases) and event["outcome"] != (
                "accepted" if terminal["recovery_ready"] else "failed"):
            raise WatchdogError("local terminal event disagrees with recovery readiness")

    if terminal["recovery_ready"] != (not blocked):
        raise WatchdogError("local recovery readiness disagrees with fail-closed phase state")

    _require_hash(terminal["last_event_sha256"], "terminal last event")
    if terminal["last_event_sha256"] != sha256_bytes(events[-1].read_bytes()):
        raise WatchdogError("local terminal does not bind the last event")
    error_events = [
        event for event in parsed_events
        if event["phase"] in {f"{role}:{step}" for step in RECOVERY_STEPS} and
        event["outcome"] in {"failed", "lost-response"}
    ]
    if len(error_events) != len(parsed_errors) or any(
            error["phase"] != event["phase"] for error, event in zip(parsed_errors, error_events)
    ):
        raise WatchdogError("local terminal errors do not match fail-closed event order")
    for error, event in zip(parsed_errors, error_events):
        if event["outcome"] == "lost-response" and error["code"] != "lost-response":
            raise WatchdogError("lost-response event lacks its exact error classification")
        if event["outcome"] == "failed" and (
                error["code"] not in {"WatchdogError", "DeadlineExpired"} or
                error["detail"] != event["detail"]):
            raise WatchdogError("failed event differs from its exact terminal error")
        if event["monotonic_ns"] >= recovery_deadline_ns and error["code"] != "DeadlineExpired":
            raise WatchdogError("a post-deadline failure is not classified as deadline expiry")
        if event["monotonic_ns"] < recovery_deadline_ns and error["code"] == "DeadlineExpired":
            raise WatchdogError("a pre-deadline failure is misclassified as deadline expiry")

    expected_inventory = {
        "authority.json", "arm.json", "peer-arm.json", "local-terminal.json",
        *(path.name for path in events),
    }
    if terminal["ready_receipt_sha256"] is None:
        if "ready-receipt.json" in files or terminal["recovery_ready"] is True:
            raise WatchdogError("ready receipt absence disagrees with terminal")
    else:
        digest = _require_hash(terminal["ready_receipt_sha256"], "terminal ready receipt")
        if "ready-receipt.json" not in files or sha256_bytes(files["ready-receipt.json"].read_bytes()) != digest or \
                terminal["recovery_ready"] is not True or final_identity is None:
            raise WatchdogError("terminal ready receipt digest or readiness differs")
        ready = parse_canonical_json(files["ready-receipt.json"].read_bytes(), "local ready receipt")
        _require_exact(ready, {
            "schema", "host", "role", "authority_sha256", "transaction_id", "identity",
            "cleanup_complete", "hmm_reconciled", "kernel_reconciled", "listener_ready",
            "application_ready", "experiment_continuation_allowed",
        }, "local ready receipt")
        ready_identity = ServiceIdentity.from_dict(ready["identity"], "local ready identity")
        if ready["schema"] != READY_SCHEMA or ready["host"] != root.name or ready["role"] != role or \
                ready["authority_sha256"] != authority.authority_sha256 or \
                ready["transaction_id"] != authority.transaction_id or ready_identity != final_identity or \
                any(ready[name] is not True for name in (
                    "cleanup_complete", "hmm_reconciled", "kernel_reconciled",
                    "listener_ready", "application_ready",
                )) or ready["experiment_continuation_allowed"] is not False:
            raise WatchdogError("local ready receipt is not exact and recovery-only")
        expected_inventory.add("ready-receipt.json")
    if expected_pre_manifest != expected_inventory:
        raise WatchdogError("local manifest contains a file outside the closed state-machine inventory")

    if terminal["cleanup_complete"] is True and (
            observation["disposable_scan_complete"] is not True or disposables):
        raise WatchdogError("terminal cleanup completion lacks a closed-world absence observation")
    if terminal["recovery_ready"] is True:
        if terminal["cleanup_complete"] is not True or observation["listener_open"] is not True or \
                observation["application_ready"] is not True or final_identity is None:
            raise WatchdogError("recovery-ready terminal lacks cleanup or service readiness")
        if final_identity.restart_count != before.restart_count or final_identity.boot_id != before.boot_id:
            raise WatchdogError("terminal final service restart or boot identity drifted")
        if terminal["observed_absent"] is True:
            if final_identity.pid == before.pid or final_identity.invocation_id == before.invocation_id or \
                    final_identity.process_start_monotonic_ns <= before.process_start_monotonic_ns or \
                    final_identity.active_enter_monotonic_ns <= before.active_enter_monotonic_ns:
                raise WatchdogError("terminal accepted a stale identity after observed absence")
            action_event = next(
                event for event in parsed_events
                if event["phase"] == f"{role}:service-recovery-actuation"
            )
            if final_identity.process_start_monotonic_ns <= action_event["monotonic_ns"]:
                raise WatchdogError("terminal fresh service identity predates its recovery action")
        elif final_identity != before:
            raise WatchdogError("terminal accepted an unobserved identity change")
        _NodeWatchdog._validate_hmm(hmm, final_identity, "terminal final HMM")
        if kernel.boot_id != baseline.boot_id or kernel.monotonic_ns <= baseline.monotonic_ns or any(
                getattr(kernel, name) != getattr(baseline, name)
                for name in (
                    "global_oom_count", "oom_kill_count", "amdgpu_fault_count",
                    "gpu_reset_count", "kfd_fault_count",
                )
        ):
            raise WatchdogError("terminal accepted unreconciled kernel authority")

    coordinator_peer_events = [
        (index, event) for index, event in enumerate(parsed_events)
        if event["phase"] == "coordinator:peer-ready-receipt" and event["outcome"] == "accepted"
    ]
    coordinator_action_events = [
        index for index, event in enumerate(parsed_events)
        if event["phase"] == "coordinator:service-recovery-actuation" and event["outcome"] != "skipped"
    ]
    if role == "worker" and terminal["worker_ready_receipt_sha256"] is not None:
        raise WatchdogError("worker terminal unexpectedly claims a peer-ready receipt")
    if role == "coordinator" and terminal["recovery_ready"] is True:
        digest = _require_hash(
            terminal["worker_ready_receipt_sha256"], "coordinator worker-ready receipt",
        )
        if len(coordinator_peer_events) != 1 or len(coordinator_action_events) != 1 or \
                coordinator_peer_events[0][0] >= coordinator_action_events[0] or \
                coordinator_peer_events[0][1]["detail"] != f"sha256={digest}":
            raise WatchdogError("coordinator recovery is not ordered after exact worker readiness")
    elif role == "coordinator" and terminal["worker_ready_receipt_sha256"] is not None:
        _require_hash(terminal["worker_ready_receipt_sha256"], "coordinator worker-ready receipt")

    return {
        "arm_sha256": arm_sha256,
        "peer_arm": peer_arm,
        "terminal": terminal,
        "terminal_sha256": marker["terminal_sha256"],
        "local_marker_sha256": sha256_bytes(files["LOCAL-FINALIZED.json"].read_bytes()),
        "pair_ack": parse_canonical_json(files["pair-ack.json"].read_bytes(), "pair ack"),
        "pair_ack_sha256": sha256_bytes(files["pair-ack.json"].read_bytes()),
    }


def _verify_paired_bundle(evidence_root: Path, *, expected_authority_sha256: str) -> dict[str, Any]:
    _require_hash(expected_authority_sha256, "expected authority")
    root = evidence_root.resolve(strict=True)
    if not root.is_dir() or root.is_symlink():
        raise WatchdogError("paired evidence root is not one real directory")
    entries = {path.name: path for path in root.iterdir()}
    if set(entries) != {COORDINATOR_HOST, WORKER_HOST} or any(
            not path.is_dir() or path.is_symlink() for path in entries.values()):
        raise WatchdogError("paired root does not contain exactly the two independent node bundles")
    authority_raw = (entries[COORDINATOR_HOST] / "authority.json").read_bytes()
    if authority_raw != (entries[WORKER_HOST] / "authority.json").read_bytes():
        raise WatchdogError("node authority bytes differ")
    authority = PreverifiedAuthority.from_dict(parse_canonical_json(authority_raw, "paired authority"))
    if authority.authority_sha256 != expected_authority_sha256:
        raise WatchdogError("paired authority differs from the externally expected digest")
    local = {host: _validate_local_bundle(entries[host], authority) for host in HOST_ROLE}
    for host in HOST_ROLE:
        peer_host = COORDINATOR_HOST if host == WORKER_HOST else WORKER_HOST
        if local[host]["peer_arm"]["peer_arm_sha256"] != local[peer_host]["arm_sha256"]:
            raise WatchdogError("paired arm receipts do not cross-bind both node-local arms")
    if local[COORDINATOR_HOST]["pair_ack_sha256"] != local[WORKER_HOST]["pair_ack_sha256"] or \
            local[COORDINATOR_HOST]["pair_ack"] != local[WORKER_HOST]["pair_ack"]:
        raise WatchdogError("paired acknowledgement bytes differ between nodes")
    ack = local[COORDINATOR_HOST]["pair_ack"]
    _require_exact(ack, {
        "schema", "authority_sha256", "verification_receipt_sha256", "transaction_id",
        "host_by_role", "terminal_sha256_by_host", "local_marker_sha256_by_host",
        "pair_binding_sha256", "status", "recovery_complete",
        "experiment_continuation_allowed", "target_execution_enabled", "offline_fake_only",
    }, "paired acknowledgement")
    if ack["schema"] != PAIR_ACK_SCHEMA or ack["authority_sha256"] != authority.authority_sha256 or \
            ack["verification_receipt_sha256"] != authority.verification_receipt_sha256 or \
            ack["transaction_id"] != authority.transaction_id or \
            ack["host_by_role"] != {"coordinator": COORDINATOR_HOST, "worker": WORKER_HOST} or \
            ack["terminal_sha256_by_host"] != {
                host: local[host]["terminal_sha256"] for host in HOST_ROLE
            } or ack["local_marker_sha256_by_host"] != {
                host: local[host]["local_marker_sha256"] for host in HOST_ROLE
            } or ack["status"] not in {"success", "failure"} or \
            type(ack["recovery_complete"]) is not bool or \
            ack["experiment_continuation_allowed"] is not False or \
            ack["target_execution_enabled"] is not False or ack["offline_fake_only"] is not True:
        raise WatchdogError("paired acknowledgement identity or hard-off boundary differs")
    binding = {
        "authority_sha256": ack["authority_sha256"],
        "verification_receipt_sha256": ack["verification_receipt_sha256"],
        "transaction_id": ack["transaction_id"],
        "terminal_sha256_by_host": ack["terminal_sha256_by_host"],
        "local_marker_sha256_by_host": ack["local_marker_sha256_by_host"],
    }
    if ack["pair_binding_sha256"] != sha256_bytes(canonical_bytes(binding)):
        raise WatchdogError("paired acknowledgement digest does not bind both terminals")
    worker_ready_path = entries[WORKER_HOST] / "ready-receipt.json"
    coordinator_terminal = local[COORDINATOR_HOST]["terminal"]
    if coordinator_terminal["worker_ready_receipt_sha256"] is not None:
        if not worker_ready_path.is_file() or \
                sha256_bytes(worker_ready_path.read_bytes()) != coordinator_terminal["worker_ready_receipt_sha256"]:
            raise WatchdogError("coordinator does not bind the exact worker-ready bytes")
        _NodeWatchdog.validate_worker_ready(worker_ready_path.read_bytes(), authority)
    computed_complete = all(local[host]["terminal"]["recovery_ready"] is True for host in HOST_ROLE)
    computed_status = "success" if computed_complete and all(
        local[host]["terminal"]["status"] == "success" for host in HOST_ROLE
    ) else "failure"
    if ack["recovery_complete"] is not computed_complete or ack["status"] != computed_status:
        raise WatchdogError("paired status does not agree with both local terminals")
    for host, result in local.items():
        marker = parse_canonical_json(
            (entries[host] / "PAIR-FINALIZED.json").read_bytes(), "pair finalized marker",
        )
        _require_exact(marker, {
            "schema", "host", "pair_ack_sha256", "publication_durability",
        }, "pair finalized marker")
        if marker["schema"] != PAIR_FINAL_SCHEMA or marker["host"] != host or \
                marker["pair_ack_sha256"] != result["pair_ack_sha256"] or \
                marker["publication_durability"] not in {"file-sync-only", "file-and-directory-sync"}:
            raise WatchdogError("pair finalized marker does not bind the local pair acknowledgement")
    return ack


def verify_paired_bundle(evidence_root: Path, *, expected_authority_sha256: str) -> dict[str, Any]:
    """Verify one offline pair, translating filesystem failures to a closed refusal."""

    if not isinstance(evidence_root, Path):
        raise WatchdogError("paired evidence root must be a pathlib Path")
    if evidence_root.is_symlink():
        raise WatchdogError("paired evidence root cannot be a symbolic link")
    try:
        return _verify_paired_bundle(
            evidence_root,
            expected_authority_sha256=expected_authority_sha256,
        )
    except WatchdogError:
        raise
    except (OSError, ValueError, RuntimeError) as exc:
        raise WatchdogError("paired evidence is missing, unreadable, or not one real path") from exc


def _pair_ack(authority: PreverifiedAuthority, watchdogs: dict[str, _NodeWatchdog]) -> dict[str, Any]:
    terminal_digests = {host: watchdogs[host].local_terminal_sha256 for host in HOST_ROLE}
    marker_digests = {host: watchdogs[host].local_marker_sha256 for host in HOST_ROLE}
    binding = {
        "authority_sha256": authority.authority_sha256,
        "verification_receipt_sha256": authority.verification_receipt_sha256,
        "transaction_id": authority.transaction_id,
        "terminal_sha256_by_host": terminal_digests,
        "local_marker_sha256_by_host": marker_digests,
    }
    recovery_complete = all(watchdogs[host].recovery_ready for host in HOST_ROLE)
    status = "success" if recovery_complete and all(not watchdogs[host].errors for host in HOST_ROLE) else "failure"
    return {
        "schema": PAIR_ACK_SCHEMA,
        **binding,
        "host_by_role": {"coordinator": COORDINATOR_HOST, "worker": WORKER_HOST},
        "pair_binding_sha256": sha256_bytes(canonical_bytes(binding)),
        "status": status,
        "recovery_complete": recovery_complete,
        "experiment_continuation_allowed": False,
        "target_execution_enabled": False,
        "offline_fake_only": True,
    }


def run_offline_model(authority: PreverifiedAuthority, evidence_root: Path,
                      loss_point: ControllerLossPoint,
                      faults: ModelFaults = ModelFaults()) -> OfflineModelResult:
    """Run the built-in deterministic fake model and retain its evidence.

    This is deliberately not a generic operations seam: callers cannot inject
    a runner, command, transport, host, unit actuator, or file mutator.
    """

    if TARGET_EXECUTION_ENABLED is not False or OFFLINE_FAKE_ONLY is not True:
        raise WatchdogError("offline watchdog hard-off invariant changed")
    if not isinstance(authority, PreverifiedAuthority):
        raise WatchdogError("authority must be the closed preverified type")
    authority.validate()
    if not isinstance(loss_point, ControllerLossPoint):
        raise WatchdogError("controller loss point is not closed")
    if not isinstance(faults, ModelFaults):
        raise WatchdogError("fault injection must use the closed offline model type")
    faults.validate()
    if not isinstance(evidence_root, Path):
        raise WatchdogError("offline evidence root must be a pathlib Path")
    if evidence_root.is_symlink():
        raise WatchdogError("offline evidence root cannot be a symbolic link")
    root = evidence_root.resolve(strict=False)
    if root.exists() or root.parent == root or not root.parent.is_dir() or root.parent.is_symlink():
        raise WatchdogError("offline evidence root must be absent under an existing real parent")
    root.mkdir()
    nodes = {
        COORDINATOR_HOST: _OfflineNode.initial(authority.coordinator_before),
        WORKER_HOST: _OfflineNode.initial(authority.worker_before),
    }
    custodies = {
        host: _Custody(root / host) for host in (COORDINATOR_HOST, WORKER_HOST)
    }
    watchdogs = {
        host: _NodeWatchdog(
            authority=authority, node=nodes[host], custody=custodies[host], faults=faults,
        ) for host in HOST_ROLE
    }
    arms = {host: watchdogs[host].arm() for host in HOST_ROLE}
    watchdogs[COORDINATOR_HOST].accept_peer_arm(arms[WORKER_HOST])
    watchdogs[WORKER_HOST].accept_peer_arm(arms[COORDINATOR_HOST])

    # The following mutations are in-memory fake controller progress only.  Each
    # local watchdog independently records its own observed protected absence.
    if loss_point in {
            ControllerLossPoint.AFTER_COORDINATOR_STOP,
            ControllerLossPoint.AFTER_BOTH_STOPS,
            ControllerLossPoint.DURING_EXPERIMENT,
    }:
        nodes[COORDINATOR_HOST].stop_by_controller()
        watchdogs[COORDINATOR_HOST].note_local_absence()
    if loss_point in {ControllerLossPoint.AFTER_BOTH_STOPS, ControllerLossPoint.DURING_EXPERIMENT}:
        nodes[WORKER_HOST].stop_by_controller()
        watchdogs[WORKER_HOST].note_local_absence()
    if loss_point == ControllerLossPoint.DURING_EXPERIMENT:
        for target in authority.disposable_allowlist:
            nodes[target.host].disposables.add(target)

    if faults.stale_reappearance_host is not None:
        node = nodes[faults.stale_reappearance_host]
        node.active_identity = node.before
        node.listener_open = True
        node.application_ready = True
    if faults.unknown_disposable_host is not None:
        host = faults.unknown_disposable_host
        nodes[host].disposables.add(DisposableTarget(
            host=host, kind="unit", value="halofpx-watchdog-foreign.service",
        ))
    if faults.incomplete_disposable_scan_host is not None:
        nodes[faults.incomplete_disposable_scan_host].disposable_scan_complete = False
    if faults.reboot_host is not None:
        node = nodes[faults.reboot_host]
        node.kernel = dataclasses.replace(node.kernel, boot_id="e" * 32)
    if faults.foreign_hmm_owner_host is not None:
        nodes[faults.foreign_hmm_owner_host].foreign_hmm_owner = True
    if faults.incomplete_hmm_census_host is not None:
        nodes[faults.incomplete_hmm_census_host].hmm_complete = False
    if faults.non_elevated_hmm_census_host is not None:
        nodes[faults.non_elevated_hmm_census_host].hmm_elevated = False
    if faults.cleanup_lost_response_host is not None:
        nodes[faults.cleanup_lost_response_host].cleanup_lost_response = True
    if faults.cleanup_no_effect_host is not None:
        nodes[faults.cleanup_no_effect_host].cleanup_no_effect = True
    if faults.worker_readiness_false:
        nodes[WORKER_HOST].readiness_false = True
    if faults.coordinator_start_lost_response:
        nodes[COORDINATOR_HOST].start_lost_response = True
    if faults.restart_drift_host is not None:
        node = nodes[faults.restart_drift_host]
        node.restart_drift = True
        if node.active_identity is not None:
            node.active_identity = dataclasses.replace(
                node.active_identity,
                restart_count=node.active_identity.restart_count + 1,
            )
    if faults.kernel_delta_host is not None:
        node = nodes[faults.kernel_delta_host]
        node.kernel = dataclasses.replace(node.kernel, oom_kill_count=node.kernel.oom_kill_count + 1)

    deadline_triggered = faults.controller_deadline_phase == loss_point
    trigger_reason = "maintenance-deadline-expired" if deadline_triggered else "controller-or-control-network-lost"
    for watchdog in watchdogs.values():
        if deadline_triggered:
            watchdog.node.clock.read(force=watchdog.controller_deadline_ns)
        watchdog.trigger(trigger_reason)

    worker_ready_raw = watchdogs[WORKER_HOST].recover(None)
    delivered_worker_ready = None if faults.peer_unreachable else worker_ready_raw
    if delivered_worker_ready is not None and faults.tamper_worker_ready_receipt:
        changed = parse_canonical_json(delivered_worker_ready, "synthetic worker ready delivery")
        changed["authority_sha256"] = "0" * 64
        delivered_worker_ready = canonical_bytes(changed)
    watchdogs[COORDINATOR_HOST].recover(delivered_worker_ready)

    paired = not faults.peer_unreachable
    status = "failure"
    recovery_complete = False
    if paired:
        ack = _pair_ack(authority, watchdogs)
        for host in HOST_ROLE:
            custodies[host].publish_pair(ack)
        verified = verify_paired_bundle(
            root, expected_authority_sha256=authority.authority_sha256,
        )
        status = verified["status"]
        recovery_complete = verified["recovery_complete"]
    return OfflineModelResult(
        evidence_root=root,
        status=status,
        recovery_complete=recovery_complete,
        paired=paired,
        experiment_continuation_allowed=False,
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    verify = subparsers.add_parser("verify-pair", help="verify one offline fake paired bundle")
    verify.add_argument("evidence_root", type=Path)
    verify.add_argument("--expected-authority-sha256", required=True)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if args.command == "verify-pair":
        result = verify_paired_bundle(
            args.evidence_root,
            expected_authority_sha256=args.expected_authority_sha256,
        )
        print(json.dumps(result, sort_keys=True, separators=(",", ":")))
        return 0
    raise WatchdogError("target execution is not implemented")


if __name__ == "__main__":
    raise SystemExit(main())
