#!/usr/bin/env python3
"""Offline-only two-node maintenance nonce protocol model.

This module deliberately has no target transport or filesystem implementation.
It models append-only records in memory, serializes a closed synthetic
transcript, and verifies that transcript.  A successful simulation is never
maintenance authorization and never evidence of real distributed atomicity.
"""

from __future__ import annotations

import argparse
import copy
import datetime as dt
import hashlib
import json
import re
import sys
from dataclasses import dataclass
from typing import Any, Iterable, Mapping, Sequence


TARGET_EXECUTION_ENABLED = False

BINDING_SCHEMA = "halofpx.maintenance-nonce-binding.v1"
SOURCE_SCHEMA = "halofpx.maintenance-nonce-source.v1"
STATE_SCHEMA = "halofpx.maintenance-nonce-state.v1"
RECORD_SCHEMA = "halofpx.maintenance-nonce-record.v1"
RECORD_ENTRY_SCHEMA = "halofpx.maintenance-nonce-record-entry.v1"
NODE_LEDGER_SCHEMA = "halofpx.maintenance-nonce-node-ledger.v1"
EVENT_SCHEMA = "halofpx.maintenance-nonce-event.v1"
EVENT_ENTRY_SCHEMA = "halofpx.maintenance-nonce-event-entry.v1"
PAIR_CERTIFICATE_SCHEMA = "halofpx.maintenance-nonce-pair-certificate.v1"
TERMINAL_SCHEMA = "halofpx.maintenance-nonce-terminal.v1"
TRANSCRIPT_SCHEMA = "halofpx.maintenance-nonce-simulation.v1"
VERIFICATION_SCHEMA = "halofpx.maintenance-nonce-verification.v1"

EXECUTION_SCOPE = "offline-domain-simulation"
TRANSPORT_KIND = "in-memory-fake-only"
DURABILITY_KIND = "simulated-append-only"
CANONICAL_REPOSITORY = "https://github.com/JCFrags/HaloFPX.git"
# The deterministic success path emits twelve request/record/response clock
# ticks, then PAIR_VERIFY, and serializes verification one tick later.
SUCCESS_VERIFICATION_OFFSET_SECONDS = 13

ROLES = ("coordinator", "worker")
NODE_IDS = {
    "coordinator": "synthetic-node-coordinator",
    "worker": "synthetic-node-worker",
}
PHASES = ("PREPARED", "COMMITTED", "ABORTED")
STATUSES = ("SIMULATED_COMPLETE", "REFUSED", "IN_DOUBT")
TERMINAL_REASONS = (
    "SIMULATED_PAIR_COMMIT_OBSERVED",
    "POLICY_ABORT",
    "ONE_NODE_PREPARE",
    "BINDING_DIVERGENCE",
    "WINDOW_INACTIVE",
    "STALE_EPOCH",
    "NONCE_REPLAY",
    "LOST_RESPONSE",
    "PHASE_REORDER",
    "PARTIAL_COMMIT",
    "CORRUPT_DURABLE_RECORD",
    "INVALID_TRANSCRIPT",
)
TERMINAL_REASONS_BY_STATUS = {
    "SIMULATED_COMPLETE": {"SIMULATED_PAIR_COMMIT_OBSERVED"},
    "REFUSED": {
        "POLICY_ABORT",
        "ONE_NODE_PREPARE",
        "BINDING_DIVERGENCE",
        "WINDOW_INACTIVE",
        "STALE_EPOCH",
        "NONCE_REPLAY",
        "PHASE_REORDER",
        "INVALID_TRANSCRIPT",
    },
    "IN_DOUBT": {
        "LOST_RESPONSE",
        "PARTIAL_COMMIT",
        "CORRUPT_DURABLE_RECORD",
        "WINDOW_INACTIVE",
        "INVALID_TRANSCRIPT",
    },
}
SCENARIOS = (
    "success",
    "policy-abort",
    "one-node",
    "split-brain",
    "expired",
    "stale-epoch",
    "replay",
    "lost-prepare-response",
    "lost-commit-response",
    "lost-abort-response",
    "reordered-commit",
    "corrupt-commit-record",
    "late-window",
)

_PREPARE_BOTH = (
    ("PREPARE_REQUEST", "coordinator", "SENT"),
    ("PREPARE_RESPONSE", "coordinator", "ACK"),
    ("PREPARE_REQUEST", "worker", "SENT"),
    ("PREPARE_RESPONSE", "worker", "ACK"),
)
_COMMIT_BOTH = (
    ("COMMIT_REQUEST", "coordinator", "SENT"),
    ("COMMIT_RESPONSE", "coordinator", "ACK"),
    ("COMMIT_REQUEST", "worker", "SENT"),
    ("COMMIT_RESPONSE", "worker", "ACK"),
)
_ABORT_BOTH = (
    ("ABORT_REQUEST", "coordinator", "SENT"),
    ("ABORT_RESPONSE", "coordinator", "ACK"),
    ("ABORT_REQUEST", "worker", "SENT"),
    ("ABORT_RESPONSE", "worker", "ACK"),
)
SCENARIO_GRAMMAR = {
    "success": (
        _PREPARE_BOTH + _COMMIT_BOTH + (("PAIR_VERIFY", "pair", "COMPLETE"),),
        {"coordinator": ("PREPARED", "COMMITTED"), "worker": ("PREPARED", "COMMITTED")},
        "SIMULATED_COMPLETE",
        "SIMULATED_PAIR_COMMIT_OBSERVED",
        True,
    ),
    "policy-abort": (
        _PREPARE_BOTH + _ABORT_BOTH + (("PAIR_VERIFY", "pair", "REFUSED"),),
        {"coordinator": ("PREPARED", "ABORTED"), "worker": ("PREPARED", "ABORTED")},
        "REFUSED",
        "POLICY_ABORT",
        False,
    ),
    "one-node": (
        (
            ("PREPARE_REQUEST", "coordinator", "SENT"),
            ("PREPARE_RESPONSE", "coordinator", "ACK"),
            ("ABORT_REQUEST", "coordinator", "SENT"),
            ("ABORT_RESPONSE", "coordinator", "ACK"),
            ("ABORT_REQUEST", "worker", "SENT"),
            ("ABORT_RESPONSE", "worker", "ACK"),
            ("PAIR_VERIFY", "pair", "REFUSED"),
        ),
        {"coordinator": ("PREPARED", "ABORTED"), "worker": ("ABORTED",)},
        "REFUSED",
        "ONE_NODE_PREPARE",
        False,
    ),
    "split-brain": (
        _PREPARE_BOTH + _ABORT_BOTH + (("PAIR_VERIFY", "pair", "REFUSED"),),
        {"coordinator": ("PREPARED", "ABORTED"), "worker": ("PREPARED", "ABORTED")},
        "REFUSED",
        "BINDING_DIVERGENCE",
        False,
    ),
    "expired": (
        (
            ("PREPARE_REQUEST", "coordinator", "SENT"),
            ("PREPARE_RESPONSE", "coordinator", "REJECTED"),
            ("PAIR_VERIFY", "pair", "REFUSED"),
        ),
        {"coordinator": (), "worker": ()},
        "REFUSED",
        "WINDOW_INACTIVE",
        False,
    ),
    "stale-epoch": (
        (
            ("PREPARE_REQUEST", "coordinator", "SENT"),
            ("PREPARE_RESPONSE", "coordinator", "REJECTED"),
            ("PAIR_VERIFY", "pair", "REFUSED"),
        ),
        {"coordinator": (), "worker": ()},
        "REFUSED",
        "STALE_EPOCH",
        False,
    ),
    "replay": (
        (
            ("PREPARE_REQUEST", "coordinator", "SENT"),
            ("PREPARE_RESPONSE", "coordinator", "REJECTED"),
            ("PAIR_VERIFY", "pair", "REFUSED"),
        ),
        {"coordinator": (), "worker": ()},
        "REFUSED",
        "NONCE_REPLAY",
        False,
    ),
    "lost-prepare-response": (
        (
            ("PREPARE_REQUEST", "coordinator", "SENT"),
            ("PREPARE_RESPONSE", "coordinator", "LOST"),
            ("PAIR_VERIFY", "pair", "IN_DOUBT"),
        ),
        {"coordinator": ("PREPARED",), "worker": ()},
        "IN_DOUBT",
        "LOST_RESPONSE",
        False,
    ),
    "lost-commit-response": (
        _PREPARE_BOTH
        + (
            ("COMMIT_REQUEST", "coordinator", "SENT"),
            ("COMMIT_RESPONSE", "coordinator", "LOST"),
            ("PAIR_VERIFY", "pair", "IN_DOUBT"),
        ),
        {"coordinator": ("PREPARED", "COMMITTED"), "worker": ("PREPARED",)},
        "IN_DOUBT",
        "LOST_RESPONSE",
        False,
    ),
    "lost-abort-response": (
        _PREPARE_BOTH
        + (
            ("ABORT_REQUEST", "coordinator", "SENT"),
            ("ABORT_RESPONSE", "coordinator", "LOST"),
            ("PAIR_VERIFY", "pair", "IN_DOUBT"),
        ),
        {"coordinator": ("PREPARED", "ABORTED"), "worker": ("PREPARED",)},
        "IN_DOUBT",
        "LOST_RESPONSE",
        False,
    ),
    "reordered-commit": (
        (
            ("COMMIT_REQUEST", "coordinator", "SENT"),
            ("COMMIT_RESPONSE", "coordinator", "REJECTED"),
            ("PAIR_VERIFY", "pair", "REFUSED"),
        ),
        {"coordinator": (), "worker": ()},
        "REFUSED",
        "PHASE_REORDER",
        False,
    ),
    "corrupt-commit-record": (
        _PREPARE_BOTH + _COMMIT_BOTH + (("PAIR_VERIFY", "pair", "IN_DOUBT"),),
        {"coordinator": ("PREPARED", "COMMITTED"), "worker": ("PREPARED", "COMMITTED")},
        "IN_DOUBT",
        "CORRUPT_DURABLE_RECORD",
        False,
    ),
    "late-window": (
        _PREPARE_BOTH
        + _COMMIT_BOTH
        + (("PAIR_VERIFY", "pair", "IN_DOUBT"),),
        {
            "coordinator": ("PREPARED", "COMMITTED"),
            "worker": ("PREPARED", "COMMITTED"),
        },
        "IN_DOUBT",
        "WINDOW_INACTIVE",
        False,
    ),
}
SCENARIO_NODE_BINDING = {
    "success": {"coordinator": True, "worker": True},
    "policy-abort": {"coordinator": True, "worker": True},
    "one-node": {"coordinator": True, "worker": True},
    "split-brain": {"coordinator": True, "worker": True},
    "expired": {"coordinator": False, "worker": False},
    "stale-epoch": {"coordinator": True, "worker": False},
    "replay": {"coordinator": True, "worker": False},
    "lost-prepare-response": {"coordinator": True, "worker": False},
    "lost-commit-response": {"coordinator": True, "worker": True},
    "lost-abort-response": {"coordinator": True, "worker": True},
    "reordered-commit": {"coordinator": True, "worker": False},
    "corrupt-commit-record": {"coordinator": True, "worker": True},
    "late-window": {"coordinator": True, "worker": True},
}
SCENARIO_ALLOWED_ISSUES = {
    "success": frozenset(),
    "policy-abort": frozenset(),
    "one-node": frozenset(),
    "split-brain": frozenset({"BINDING_DIVERGENCE"}),
    "expired": frozenset({"WINDOW_INACTIVE"}),
    "stale-epoch": frozenset({"STALE_EPOCH"}),
    "replay": frozenset({"NONCE_REPLAY"}),
    "lost-prepare-response": frozenset(),
    "lost-commit-response": frozenset(),
    "lost-abort-response": frozenset(),
    "reordered-commit": frozenset({"COMMIT_BEFORE_BOTH_PREPARED"}),
    "corrupt-commit-record": frozenset(
        {
            "CORRUPT_DURABLE_RECORD",
            "RECORD_RESPONSE_BIJECTION",
            "RESPONSE_RECORD_DIVERGENCE",
        }
    ),
    "late-window": frozenset({"WINDOW_INACTIVE"}),
}
# For every ordinary scenario, the last listed offset is its serialized
# verification timestamp.  Because authorization windows are half-open, the
# endpoint itself must still be active.  ``expired`` and ``late-window`` are
# the two deliberate inactive-window grammars and are handled separately.
SCENARIO_ACTIVE_THROUGH_OFFSET_SECONDS = {
    "success": SUCCESS_VERIFICATION_OFFSET_SECONDS,
    "policy-abort": 13,
    "one-node": 10,
    "split-brain": 13,
    "stale-epoch": 4,
    "replay": 4,
    "lost-prepare-response": 4,
    "lost-commit-response": 10,
    "lost-abort-response": 10,
    "reordered-commit": 4,
    "corrupt-commit-record": 13,
}
LATE_WINDOW_START_OFFSET_SECONDS = 12

ZERO_HASH = "0" * 64
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")
UTC_RE = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$")

DOMAIN_BINDING = b"halofpx.maintenance-nonce-binding.v1\0"
DOMAIN_NONCE = b"halofpx.maintenance-nonce-commitment.v1\0"
DOMAIN_TRANSACTION = b"halofpx.maintenance-nonce-transaction.v1\0"
DOMAIN_RECORD = b"halofpx.maintenance-nonce-record.v1\0"
DOMAIN_EVENT = b"halofpx.maintenance-nonce-event.v1\0"
DOMAIN_CERTIFICATE = b"halofpx.maintenance-nonce-pair-certificate.v1\0"


class ProtocolError(RuntimeError):
    """A definite refusal in the synthetic protocol model."""


def canonical_bytes(value: Any) -> bytes:
    return json.dumps(
        value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    ).encode("utf-8")


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def domain_digest(domain: bytes, value: Any) -> str:
    return sha256_bytes(domain + canonical_bytes(value))


def _object_pairs_no_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ProtocolError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def parse_closed_json(content: bytes, where: str) -> dict[str, Any]:
    try:
        value = json.loads(
            content.decode("utf-8"), object_pairs_hook=_object_pairs_no_duplicates
        )
    except (
        UnicodeDecodeError,
        json.JSONDecodeError,
        RecursionError,
        ValueError,
        OverflowError,
    ) as exc:
        raise ProtocolError(f"{where} is not strict UTF-8 JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise ProtocolError(f"{where} must be a JSON object")
    return value


def require_exact(value: Any, keys: set[str], where: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ProtocolError(f"{where} must be an object")
    observed = set(value)
    if observed != keys:
        raise ProtocolError(
            f"{where} has wrong closed field set: "
            f"missing={sorted(keys - observed)} extra={sorted(observed - keys)}"
        )
    return value


def require_string(value: Any, where: str) -> str:
    if not isinstance(value, str) or not value:
        raise ProtocolError(f"{where} must be a non-empty string")
    return value


def require_bool(value: Any, where: str) -> bool:
    if type(value) is not bool:
        raise ProtocolError(f"{where} must be an exact Boolean")
    return value


def require_int(value: Any, where: str, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        raise ProtocolError(f"{where} must be an integer >= {minimum}")
    return value


def require_hash(value: Any, where: str) -> str:
    result = require_string(value, where)
    if SHA256_RE.fullmatch(result) is None:
        raise ProtocolError(f"{where} must be lowercase SHA-256 hex")
    return result


def parse_utc(value: Any, where: str) -> dt.datetime:
    text = require_string(value, where)
    if UTC_RE.fullmatch(text) is None:
        raise ProtocolError(f"{where} must be canonical UTC seconds with Z")
    try:
        parsed = dt.datetime.strptime(text, "%Y-%m-%dT%H:%M:%SZ")
    except ValueError as exc:
        raise ProtocolError(f"{where} is not a real UTC timestamp") from exc
    return parsed.replace(tzinfo=dt.timezone.utc)


def format_utc(value: dt.datetime) -> str:
    if value.tzinfo is None:
        raise ProtocolError("synthetic time must be timezone-aware")
    value = value.astimezone(dt.timezone.utc).replace(microsecond=0)
    return value.strftime("%Y-%m-%dT%H:%M:%SZ")


@dataclass(frozen=True)
class ParsedBinding:
    raw: dict[str, Any]
    digest: str
    transaction_id: str
    nonce_commitment: str
    not_before: dt.datetime
    expires: dt.datetime
    epoch: int


def parse_binding(value: Any, where: str = "binding") -> ParsedBinding:
    raw = require_exact(
        value,
        {
            "schema",
            "authorization_sha256",
            "nonce",
            "not_before_utc",
            "expires_utc",
            "source",
            "plan_sha256",
            "policy_sha256",
            "incident_sha256",
            "epoch",
            "attempt_id",
        },
        where,
    )
    if raw["schema"] != BINDING_SCHEMA:
        raise ProtocolError(f"{where}.schema is unsupported")
    authorization_sha256 = require_hash(
        raw["authorization_sha256"], f"{where}.authorization_sha256"
    )
    nonce = require_hash(raw["nonce"], f"{where}.nonce")
    not_before = parse_utc(raw["not_before_utc"], f"{where}.not_before_utc")
    expires = parse_utc(raw["expires_utc"], f"{where}.expires_utc")
    if expires <= not_before:
        raise ProtocolError(f"{where} window must be positive")
    if expires - not_before > dt.timedelta(hours=8):
        raise ProtocolError(f"{where} window exceeds eight hours")

    source = require_exact(
        raw["source"],
        {
            "schema",
            "repository_url",
            "commit",
            "tree_sha256",
            "executable_sha256",
        },
        f"{where}.source",
    )
    if source["schema"] != SOURCE_SCHEMA:
        raise ProtocolError(f"{where}.source.schema is unsupported")
    if source["repository_url"] != CANONICAL_REPOSITORY:
        raise ProtocolError(f"{where}.source.repository_url is not canonical")
    commit = require_string(source["commit"], f"{where}.source.commit")
    if COMMIT_RE.fullmatch(commit) is None:
        raise ProtocolError(f"{where}.source.commit must be lowercase 40-hex")
    require_hash(source["tree_sha256"], f"{where}.source.tree_sha256")
    require_hash(source["executable_sha256"], f"{where}.source.executable_sha256")
    require_hash(raw["plan_sha256"], f"{where}.plan_sha256")
    require_hash(raw["policy_sha256"], f"{where}.policy_sha256")
    require_hash(raw["incident_sha256"], f"{where}.incident_sha256")
    epoch = require_int(raw["epoch"], f"{where}.epoch", 1)
    require_hash(raw["attempt_id"], f"{where}.attempt_id")

    owned = copy.deepcopy(raw)
    digest = domain_digest(DOMAIN_BINDING, owned)
    nonce_commitment = sha256_bytes(DOMAIN_NONCE + bytes.fromhex(nonce))
    transaction_id = sha256_bytes(
        DOMAIN_TRANSACTION
        + bytes.fromhex(digest)
        + bytes.fromhex(raw["attempt_id"])
    )
    return ParsedBinding(
        raw=owned,
        digest=digest,
        transaction_id=transaction_id,
        nonce_commitment=nonce_commitment,
        not_before=not_before,
        expires=expires,
        epoch=epoch,
    )


def active_at(binding: ParsedBinding, observed: dt.datetime) -> bool:
    return binding.not_before <= observed < binding.expires


def _state(
    epoch: int,
    burned: Iterable[str],
    quarantine_transaction_id: str = ZERO_HASH,
) -> dict[str, Any]:
    return {
        "schema": STATE_SCHEMA,
        "epoch_high_water": epoch,
        "burned_nonce_commitments": sorted(burned),
        "quarantine_transaction_id": quarantine_transaction_id,
    }


def _record_digest(record: Mapping[str, Any]) -> str:
    return domain_digest(DOMAIN_RECORD, record)


class FakeDurableNode:
    """In-memory fake whose state survives synthetic process-attempt resets."""

    def __init__(
        self,
        role: str,
        *,
        epoch_high_water: int = 0,
        burned_nonce_commitments: Iterable[str] = (),
        quarantine_transaction_id: str = ZERO_HASH,
    ) -> None:
        if role not in ROLES:
            raise ProtocolError("fake node role is unsupported")
        self.role = role
        self.node_id = NODE_IDS[role]
        self.epoch_high_water = require_int(
            epoch_high_water, "fake epoch high-water", 0
        )
        burned = list(burned_nonce_commitments)
        for index, item in enumerate(burned):
            require_hash(item, f"fake burned nonce {index}")
        if len(set(burned)) != len(burned):
            raise ProtocolError("fake burned nonce commitments must be unique")
        self.burned_nonce_commitments = set(burned)
        self.quarantine_transaction_id = require_hash(
            quarantine_transaction_id, "fake quarantine transaction identity"
        )
        self._before = _state(
            self.epoch_high_water,
            self.burned_nonce_commitments,
            self.quarantine_transaction_id,
        )
        self._binding: ParsedBinding | None = None
        self._records: list[dict[str, Any]] = []
        self._phase: str | None = None

    def begin_attempt(self) -> None:
        if self.quarantine_transaction_id != ZERO_HASH:
            raise ProtocolError(
                f"{self.role} is quarantined by an unresolved transaction"
            )
        self._before = _state(
            self.epoch_high_water,
            self.burned_nonce_commitments,
            self.quarantine_transaction_id,
        )
        self._binding = None
        self._records = []
        self._phase = None

    def quarantine(self, binding: ParsedBinding) -> None:
        if self.quarantine_transaction_id not in (ZERO_HASH, binding.transaction_id):
            raise ProtocolError(f"{self.role} quarantine identity differs")
        self.quarantine_transaction_id = binding.transaction_id

    def _same_binding(self, binding: ParsedBinding) -> None:
        if self._binding is None:
            self._binding = binding
        elif canonical_bytes(self._binding.raw) != canonical_bytes(binding.raw):
            raise ProtocolError(f"{self.role} binding changed within one attempt")

    def _burn_new(self, binding: ParsedBinding) -> None:
        if binding.nonce_commitment in self.burned_nonce_commitments:
            raise ProtocolError(f"{self.role} nonce replay")
        if binding.epoch != self.epoch_high_water + 1:
            raise ProtocolError(f"{self.role} stale or nonconsecutive epoch")
        self.burned_nonce_commitments.add(binding.nonce_commitment)
        self.epoch_high_water = binding.epoch

    def _append(self, phase: str, binding: ParsedBinding, observed: dt.datetime) -> str:
        previous = ZERO_HASH
        if self._records:
            previous = self._records[-1]["record_sha256"]
        record = {
            "schema": RECORD_SCHEMA,
            "role": self.role,
            "node_id": self.node_id,
            "sequence": len(self._records) + 1,
            "phase": phase,
            "binding_sha256": binding.digest,
            "transaction_id": binding.transaction_id,
            "authorization_sha256": binding.raw["authorization_sha256"],
            "nonce_commitment": binding.nonce_commitment,
            "epoch": binding.epoch,
            "observed_at_utc": format_utc(observed),
            "previous_record_sha256": previous,
        }
        digest = _record_digest(record)
        self._records.append(
            {
                "schema": RECORD_ENTRY_SCHEMA,
                "simulated_durable": True,
                "record": record,
                "record_sha256": digest,
            }
        )
        self._phase = phase
        return digest

    def prepare(self, binding: ParsedBinding, observed: dt.datetime) -> str:
        if self._phase is not None:
            raise ProtocolError(f"{self.role} prepare is replayed or reordered")
        if not active_at(binding, observed):
            raise ProtocolError(f"{self.role} authorization window is inactive")
        self._same_binding(binding)
        self._burn_new(binding)
        return self._append("PREPARED", binding, observed)

    def commit(self, binding: ParsedBinding, observed: dt.datetime) -> str:
        self._same_binding(binding)
        if self._phase != "PREPARED":
            raise ProtocolError(f"{self.role} commit does not follow prepare")
        if not active_at(binding, observed):
            raise ProtocolError(f"{self.role} authorization expired before commit")
        return self._append("COMMITTED", binding, observed)

    def abort(self, binding: ParsedBinding, observed: dt.datetime) -> str:
        if self._phase == "COMMITTED":
            raise ProtocolError(f"{self.role} cannot abort a committed nonce")
        if self._phase == "ABORTED":
            raise ProtocolError(f"{self.role} abort is replayed")
        self._same_binding(binding)
        if self._phase is None:
            self._burn_new(binding)
        elif self._phase != "PREPARED":
            raise ProtocolError(f"{self.role} abort state is invalid")
        return self._append("ABORTED", binding, observed)

    def export(self) -> dict[str, Any]:
        binding = self._binding.raw if self._binding is not None else None
        binding_sha256 = self._binding.digest if self._binding is not None else ZERO_HASH
        return {
            "schema": NODE_LEDGER_SCHEMA,
            "role": self.role,
            "node_id": self.node_id,
            "binding": copy.deepcopy(binding),
            "binding_sha256": binding_sha256,
            "before": copy.deepcopy(self._before),
            "records": copy.deepcopy(self._records),
            "after": _state(
                self.epoch_high_water,
                self.burned_nonce_commitments,
                self.quarantine_transaction_id,
            ),
        }


class FakeEventLog:
    def __init__(self) -> None:
        self.entries: list[dict[str, Any]] = []

    def append(
        self,
        *,
        kind: str,
        role: str,
        outcome: str,
        binding_sha256: str,
        record_sha256: str,
        observed: dt.datetime,
    ) -> None:
        previous = ZERO_HASH
        if self.entries:
            previous = self.entries[-1]["event_sha256"]
        event = {
            "schema": EVENT_SCHEMA,
            "sequence": len(self.entries) + 1,
            "kind": kind,
            "role": role,
            "outcome": outcome,
            "binding_sha256": binding_sha256,
            "record_sha256": record_sha256,
            "observed_at_utc": format_utc(observed),
            "previous_event_sha256": previous,
        }
        self.entries.append(
            {
                "schema": EVENT_ENTRY_SCHEMA,
                "event": event,
                "event_sha256": domain_digest(DOMAIN_EVENT, event),
            }
        )


@dataclass
class _Clock:
    current: dt.datetime

    def tick(self) -> dt.datetime:
        result = self.current
        self.current += dt.timedelta(seconds=1)
        return result


def _send(
    log: FakeEventLog,
    node: FakeDurableNode,
    binding: ParsedBinding,
    phase: str,
    clock: _Clock,
    *,
    lost: bool = False,
) -> tuple[str, str]:
    phase_upper = phase.upper()
    log.append(
        kind=f"{phase_upper}_REQUEST",
        role=node.role,
        outcome="SENT",
        binding_sha256=binding.digest,
        record_sha256=ZERO_HASH,
        observed=clock.tick(),
    )
    try:
        if phase == "prepare":
            record_sha256 = node.prepare(binding, clock.tick())
        elif phase == "commit":
            record_sha256 = node.commit(binding, clock.tick())
        elif phase == "abort":
            record_sha256 = node.abort(binding, clock.tick())
        else:
            raise ProtocolError("synthetic phase is unsupported")
    except ProtocolError as exc:
        log.append(
            kind=f"{phase_upper}_RESPONSE",
            role=node.role,
            outcome="REJECTED",
            binding_sha256=binding.digest,
            record_sha256=ZERO_HASH,
            observed=clock.tick(),
        )
        return "REJECTED", str(exc)
    log.append(
        kind=f"{phase_upper}_RESPONSE",
        role=node.role,
        outcome="LOST" if lost else "ACK",
        binding_sha256=binding.digest,
        record_sha256=record_sha256,
        observed=clock.tick(),
    )
    return ("LOST" if lost else "ACK"), ""


def _pair_event(
    log: FakeEventLog,
    status: str,
    binding: ParsedBinding,
    clock: _Clock,
) -> None:
    outcome = {
        "SIMULATED_COMPLETE": "COMPLETE",
        "REFUSED": "REFUSED",
        "IN_DOUBT": "IN_DOUBT",
    }[status]
    log.append(
        kind="PAIR_VERIFY",
        role="pair",
        outcome=outcome,
        binding_sha256=binding.digest,
        record_sha256=ZERO_HASH,
        observed=clock.tick(),
    )


def _pair_certificate(
    binding: ParsedBinding, ledgers: Mapping[str, dict[str, Any]]
) -> dict[str, Any]:
    commits: dict[str, str] = {}
    for role in ROLES:
        records = ledgers[role]["records"]
        commits[role] = records[-1]["record_sha256"]
    certificate = {
        "schema": PAIR_CERTIFICATE_SCHEMA,
        "binding_sha256": binding.digest,
        "transaction_id": binding.transaction_id,
        "authorization_sha256": binding.raw["authorization_sha256"],
        "nonce_commitment": binding.nonce_commitment,
        "epoch": binding.epoch,
        "node_commit_record_sha256": commits,
        "pair_state": "SIMULATED_COMMITTED",
        "authorized": False,
        "simulation_qualified": False,
        "target_execution_qualified": False,
        "distributed_atomicity_proven": False,
    }
    certificate["certificate_sha256"] = domain_digest(
        DOMAIN_CERTIFICATE, certificate
    )
    return certificate


def _terminal(status: str, reason: str, certificate: dict[str, Any] | None) -> dict[str, Any]:
    return {
        "schema": TERMINAL_SCHEMA,
        "status": status,
        "reason": reason,
        "authorized": False,
        "simulation_qualified": False,
        "pair_certificate_sha256": (
            certificate["certificate_sha256"] if certificate is not None else ZERO_HASH
        ),
    }


def simulate_pair(
    binding_value: Mapping[str, Any],
    *,
    observed_at: dt.datetime,
    scenario: str = "success",
    nodes: Mapping[str, FakeDurableNode] | None = None,
) -> dict[str, Any]:
    """Run one deterministic fake transaction and return its closed transcript."""

    if scenario not in SCENARIOS:
        raise ProtocolError("synthetic fault scenario is unsupported")
    if observed_at.tzinfo is None:
        raise ProtocolError("synthetic observed time must be timezone-aware")
    binding = parse_binding(copy.deepcopy(dict(binding_value)))
    if nodes is None:
        initial_epoch = binding.epoch - 1
        if scenario == "stale-epoch":
            initial_epoch = binding.epoch
        nodes = {
            role: FakeDurableNode(role, epoch_high_water=initial_epoch)
            for role in ROLES
        }
    if set(nodes) != set(ROLES):
        raise ProtocolError("fake node set must contain exactly coordinator and worker")
    for role in ROLES:
        if type(nodes[role]) is not FakeDurableNode:
            raise ProtocolError("only the exact in-memory fake node is admitted")
        if nodes[role].role != role:
            raise ProtocolError("fake node mapping does not match role")
        if nodes[role].quarantine_transaction_id != ZERO_HASH:
            raise ProtocolError(
                f"{role} is quarantined by an unresolved transaction"
            )
    persistent_states = {
        (
            nodes[role].epoch_high_water,
            tuple(sorted(nodes[role].burned_nonce_commitments)),
            nodes[role].quarantine_transaction_id,
        )
        for role in ROLES
    }
    if len(persistent_states) != 1:
        raise ProtocolError("fake node persistent prestates differ")

    clock = _Clock(observed_at.astimezone(dt.timezone.utc).replace(microsecond=0))
    if scenario == "late-window":
        if binding.expires - binding.not_before < dt.timedelta(
            seconds=LATE_WINDOW_START_OFFSET_SECONDS
        ):
            raise ProtocolError(
                "late-window synthetic schedule does not fit the authorization window"
            )
        clock.current = binding.expires - dt.timedelta(
            seconds=LATE_WINDOW_START_OFFSET_SECONDS
        )
    elif scenario != "expired":
        active_through = clock.current + dt.timedelta(
            seconds=SCENARIO_ACTIVE_THROUGH_OFFSET_SECONDS[scenario]
        )
        if not active_at(binding, clock.current) or not active_at(
            binding, active_through
        ):
            raise ProtocolError(
                f"{scenario} synthetic schedule does not fit the authorization window"
            )

    for role in ROLES:
        nodes[role].begin_attempt()

    if scenario == "replay":
        for role in ROLES:
            if binding.nonce_commitment not in nodes[role].burned_nonce_commitments:
                nodes[role].burned_nonce_commitments.add(binding.nonce_commitment)
            nodes[role]._before = _state(  # synthetic fixture setup, not an API seam
                nodes[role].epoch_high_water,
                nodes[role].burned_nonce_commitments,
                nodes[role].quarantine_transaction_id,
            )

    log = FakeEventLog()
    status = "REFUSED"
    reason = "INVALID_TRANSCRIPT"
    reported_scenario = scenario

    if scenario == "expired":
        clock.current = binding.expires + dt.timedelta(seconds=1)
        _send(log, nodes["coordinator"], binding, "prepare", clock)
        status, reason = "REFUSED", "WINDOW_INACTIVE"
    elif scenario in ("stale-epoch", "replay"):
        _, detail = _send(log, nodes["coordinator"], binding, "prepare", clock)
        status = "REFUSED"
        reason = "NONCE_REPLAY" if "nonce replay" in detail else "STALE_EPOCH"
    elif scenario == "reordered-commit":
        _send(log, nodes["coordinator"], binding, "commit", clock)
        status, reason = "REFUSED", "PHASE_REORDER"
    elif scenario == "lost-prepare-response":
        _send(log, nodes["coordinator"], binding, "prepare", clock, lost=True)
        status, reason = "IN_DOUBT", "LOST_RESPONSE"
    elif scenario == "one-node":
        _send(log, nodes["coordinator"], binding, "prepare", clock)
        _send(log, nodes["coordinator"], binding, "abort", clock)
        _send(log, nodes["worker"], binding, "abort", clock)
        status, reason = "REFUSED", "ONE_NODE_PREPARE"
    elif scenario == "split-brain":
        alternate = copy.deepcopy(binding.raw)
        alternate["source"]["tree_sha256"] = "f" * 64
        if alternate["source"]["tree_sha256"] == binding.raw["source"]["tree_sha256"]:
            alternate["source"]["tree_sha256"] = "e" * 64
        worker_binding = parse_binding(alternate, "split worker binding")
        _send(log, nodes["coordinator"], binding, "prepare", clock)
        _send(log, nodes["worker"], worker_binding, "prepare", clock)
        _send(log, nodes["coordinator"], binding, "abort", clock)
        _send(log, nodes["worker"], worker_binding, "abort", clock)
        status, reason = "REFUSED", "BINDING_DIVERGENCE"
    else:
        first, detail = _send(log, nodes["coordinator"], binding, "prepare", clock)
        if first != "ACK":
            status, reason = "REFUSED", (
                "NONCE_REPLAY" if "nonce replay" in detail else "STALE_EPOCH"
            )
            reported_scenario = (
                "replay" if reason == "NONCE_REPLAY" else "stale-epoch"
            )
        else:
            second, detail = _send(log, nodes["worker"], binding, "prepare", clock)
            if second != "ACK":
                _send(log, nodes["coordinator"], binding, "abort", clock)
                status, reason = "REFUSED", (
                    "NONCE_REPLAY" if "nonce replay" in detail else "STALE_EPOCH"
                )
            elif scenario == "policy-abort":
                _send(log, nodes["coordinator"], binding, "abort", clock)
                _send(log, nodes["worker"], binding, "abort", clock)
                status, reason = "REFUSED", "POLICY_ABORT"
            elif scenario == "lost-abort-response":
                _send(log, nodes["coordinator"], binding, "abort", clock, lost=True)
                status, reason = "IN_DOUBT", "LOST_RESPONSE"
            else:
                lost_commit = scenario == "lost-commit-response"
                first_commit, _ = _send(
                    log,
                    nodes["coordinator"],
                    binding,
                    "commit",
                    clock,
                    lost=lost_commit,
                )
                if first_commit == "LOST":
                    status, reason = "IN_DOUBT", "LOST_RESPONSE"
                else:
                    second_commit, _ = _send(
                        log, nodes["worker"], binding, "commit", clock
                    )
                    if second_commit != "ACK":
                        status, reason = "IN_DOUBT", "PARTIAL_COMMIT"
                    elif scenario == "corrupt-commit-record":
                        worker_entry = nodes["worker"]._records[-1]
                        digest = worker_entry["record_sha256"]
                        worker_entry["record_sha256"] = (
                            ("0" if digest[0] != "0" else "1") + digest[1:]
                        )
                        status, reason = "IN_DOUBT", "CORRUPT_DURABLE_RECORD"
                    else:
                        status, reason = (
                            "SIMULATED_COMPLETE",
                            "SIMULATED_PAIR_COMMIT_OBSERVED",
                        )

    if status == "SIMULATED_COMPLETE" and reported_scenario == "late-window":
        status, reason = "IN_DOUBT", "WINDOW_INACTIVE"
    _pair_event(log, status, binding, clock)
    if status == "IN_DOUBT":
        for role in ROLES:
            nodes[role].quarantine(binding)
    ledgers = {role: nodes[role].export() for role in ROLES}
    certificate = (
        _pair_certificate(binding, ledgers)
        if status == "SIMULATED_COMPLETE"
        else None
    )
    transcript = {
        "schema": TRANSCRIPT_SCHEMA,
        "execution_scope": EXECUTION_SCOPE,
        "target_execution_enabled": False,
        "transport": TRANSPORT_KIND,
        "durability": DURABILITY_KIND,
        "fault_scenario": reported_scenario,
        "binding": copy.deepcopy(binding.raw),
        "binding_sha256": binding.digest,
        "verification_time_utc": format_utc(clock.current),
        "nodes": ledgers,
        "events": copy.deepcopy(log.entries),
        "pair_certificate": certificate,
        "terminal": _terminal(status, reason, certificate),
    }
    return transcript


@dataclass
class _LedgerResult:
    role: str
    binding: ParsedBinding | None
    record_digests: dict[str, str]
    record_digest_sequence: tuple[str, ...]
    record_observed: dict[str, dt.datetime]
    phases: tuple[str, ...]
    terminal_phase: str | None
    issues: list[str]
    commit_observed: bool
    before_state: tuple[int, tuple[str, ...], str] | None
    before_quarantine: str
    after_quarantine: str


def _parse_state(value: Any, where: str) -> tuple[int, list[str], str]:
    raw = require_exact(
        value,
        {
            "schema",
            "epoch_high_water",
            "burned_nonce_commitments",
            "quarantine_transaction_id",
        },
        where,
    )
    if raw["schema"] != STATE_SCHEMA:
        raise ProtocolError(f"{where}.schema is unsupported")
    epoch = require_int(raw["epoch_high_water"], f"{where}.epoch_high_water")
    burned = raw["burned_nonce_commitments"]
    if not isinstance(burned, list):
        raise ProtocolError(f"{where}.burned_nonce_commitments must be a list")
    for index, item in enumerate(burned):
        require_hash(item, f"{where}.burned_nonce_commitments[{index}]")
    if burned != sorted(set(burned)):
        raise ProtocolError(f"{where}.burned_nonce_commitments must be sorted and unique")
    quarantine = require_hash(
        raw["quarantine_transaction_id"], f"{where}.quarantine_transaction_id"
    )
    return epoch, list(burned), quarantine


def _verify_ledger(value: Any, role: str) -> _LedgerResult:
    issues: list[str] = []
    commit_observed = False
    record_digests: dict[str, str] = {}
    record_digest_sequence: list[str] = []
    record_observed: dict[str, dt.datetime] = {}
    terminal_phase: str | None = None
    binding: ParsedBinding | None = None
    before_state: tuple[int, tuple[str, ...], str] | None = None
    before_quarantine = ZERO_HASH
    after_quarantine = ZERO_HASH
    phases: list[str] = []
    try:
        raw = require_exact(
            value,
            {
                "schema",
                "role",
                "node_id",
                "binding",
                "binding_sha256",
                "before",
                "records",
                "after",
            },
            f"nodes.{role}",
        )
        if raw["schema"] != NODE_LEDGER_SCHEMA:
            raise ProtocolError(f"nodes.{role}.schema is unsupported")
        if raw["role"] != role or raw["node_id"] != NODE_IDS[role]:
            raise ProtocolError(f"nodes.{role} role or node identity differs")
        binding_sha256 = require_hash(
            raw["binding_sha256"], f"nodes.{role}.binding_sha256"
        )
        if raw["binding"] is None:
            if binding_sha256 != ZERO_HASH:
                raise ProtocolError(f"nodes.{role} null binding has a nonzero digest")
        else:
            binding = parse_binding(raw["binding"], f"nodes.{role}.binding")
            if binding.digest != binding_sha256:
                raise ProtocolError(f"nodes.{role} binding digest differs")
        before_epoch, before_burned, before_quarantine = _parse_state(
            raw["before"], f"nodes.{role}.before"
        )
        before_state = (
            before_epoch,
            tuple(before_burned),
            before_quarantine,
        )
        after_epoch, after_burned, after_quarantine = _parse_state(
            raw["after"], f"nodes.{role}.after"
        )
        if before_quarantine != ZERO_HASH:
            issues.append("QUARANTINED_STATE")
        records = raw["records"]
        if not isinstance(records, list) or len(records) > 2:
            raise ProtocolError(f"nodes.{role}.records must contain at most two entries")
        expected_previous = ZERO_HASH
        previous_observed: dt.datetime | None = None
        for index, item in enumerate(records):
            entry = require_exact(
                item,
                {"schema", "simulated_durable", "record", "record_sha256"},
                f"nodes.{role}.records[{index}]",
            )
            if entry["schema"] != RECORD_ENTRY_SCHEMA:
                raise ProtocolError(f"nodes.{role}.records[{index}].schema is unsupported")
            if require_bool(
                entry["simulated_durable"],
                f"nodes.{role}.records[{index}].simulated_durable",
            ) is not True:
                raise ProtocolError(f"nodes.{role} record is not simulated durable")
            record = require_exact(
                entry["record"],
                {
                    "schema",
                    "role",
                    "node_id",
                    "sequence",
                    "phase",
                    "binding_sha256",
                    "transaction_id",
                    "authorization_sha256",
                    "nonce_commitment",
                    "epoch",
                    "observed_at_utc",
                    "previous_record_sha256",
                },
                f"nodes.{role}.records[{index}].record",
            )
            if record["schema"] != RECORD_SCHEMA:
                raise ProtocolError(f"nodes.{role} record schema is unsupported")
            if record["role"] != role or record["node_id"] != NODE_IDS[role]:
                raise ProtocolError(f"nodes.{role} record identity differs")
            if require_int(record["sequence"], "record sequence", 1) != index + 1:
                raise ProtocolError(f"nodes.{role} record sequence is not contiguous")
            phase = require_string(record["phase"], "record phase")
            if phase not in PHASES:
                raise ProtocolError(f"nodes.{role} record phase is unsupported")
            if phase == "COMMITTED":
                commit_observed = True
            record_binding_sha256 = require_hash(
                record["binding_sha256"], "record binding digest"
            )
            require_hash(record["transaction_id"], "record transaction identity")
            require_hash(record["authorization_sha256"], "record authorization digest")
            require_hash(record["nonce_commitment"], "record nonce commitment")
            require_int(record["epoch"], "record epoch", 1)
            observed = parse_utc(record["observed_at_utc"], "record observed time")
            if previous_observed is not None and observed <= previous_observed:
                issues.append("PHASE_REORDER")
            previous_observed = observed
            if record["previous_record_sha256"] != expected_previous:
                raise ProtocolError(f"nodes.{role} record hash chain differs")
            claimed = require_hash(entry["record_sha256"], "record digest")
            actual = _record_digest(record)
            if claimed != actual:
                issues.append("CORRUPT_DURABLE_RECORD")
            expected_previous = claimed
            record_digests[claimed] = phase
            record_digest_sequence.append(claimed)
            record_observed[claimed] = observed
            phases.append(phase)
            if binding is None:
                issues.append("MISSING_NODE_BINDING")
            else:
                if record_binding_sha256 != binding.digest:
                    issues.append("BINDING_DIVERGENCE")
                if record["transaction_id"] != binding.transaction_id:
                    issues.append("TRANSACTION_DIVERGENCE")
                if record["authorization_sha256"] != binding.raw["authorization_sha256"]:
                    issues.append("AUTHORIZATION_DIVERGENCE")
                if record["nonce_commitment"] != binding.nonce_commitment:
                    issues.append("NONCE_DIVERGENCE")
                if record["epoch"] != binding.epoch:
                    issues.append("STALE_EPOCH")
                if phase in ("PREPARED", "COMMITTED") and not active_at(binding, observed):
                    issues.append("WINDOW_INACTIVE")

        allowed = (
            [],
            ["ABORTED"],
            ["PREPARED"],
            ["PREPARED", "ABORTED"],
            ["PREPARED", "COMMITTED"],
        )
        if phases not in allowed:
            issues.append("PHASE_REORDER")
        terminal_phase = phases[-1] if phases else None

        if records:
            if binding is None:
                issues.append("MISSING_NODE_BINDING")
            else:
                if binding.nonce_commitment in before_burned:
                    issues.append("NONCE_REPLAY")
                if binding.epoch != before_epoch + 1:
                    issues.append("STALE_EPOCH")
                expected_burned = sorted(set(before_burned) | {binding.nonce_commitment})
                if after_epoch != binding.epoch or after_burned != expected_burned:
                    issues.append("STATE_TRANSITION_DIVERGENCE")
        elif after_epoch != before_epoch or after_burned != before_burned:
            issues.append("STATE_TRANSITION_DIVERGENCE")
        elif binding is not None:
            # A rejected prepare still exports the exact binding the fake node
            # evaluated.  This lets the closed transcript distinguish a
            # replay/stale refusal from an unclassified empty journal.
            if binding.nonce_commitment in before_burned:
                issues.append("NONCE_REPLAY")
            if binding.epoch != before_epoch + 1:
                issues.append("STALE_EPOCH")
    except ProtocolError as exc:
        issues.append(f"MALFORMED_LEDGER:{exc}")
    return _LedgerResult(
        role=role,
        binding=binding,
        record_digests=record_digests,
        record_digest_sequence=tuple(record_digest_sequence),
        record_observed=record_observed,
        phases=tuple(phases),
        terminal_phase=terminal_phase,
        issues=issues,
        commit_observed=commit_observed,
        before_state=before_state,
        before_quarantine=before_quarantine,
        after_quarantine=after_quarantine,
    )


@dataclass
class _EventResult:
    issues: list[str]
    lost_response: bool
    commit_requested: bool
    commit_acks: set[str]
    prepare_acks: dict[str, str]
    abort_acks: set[str]
    pair_outcome: str | None
    signatures: tuple[tuple[str, str, str], ...]
    observed_times: tuple[dt.datetime, ...]
    response_record_counts: dict[str, dict[str, int]]
    final_observed: dt.datetime | None
    record_response_bijection: bool


def _verify_events(
    value: Any,
    ledgers: Mapping[str, _LedgerResult],
    requested_binding: ParsedBinding,
) -> _EventResult:
    issues: list[str] = []
    lost_response = False
    commit_requested = False
    commit_acks: set[str] = set()
    prepare_acks: dict[str, str] = {}
    abort_acks: set[str] = set()
    pair_outcome: str | None = None
    signatures: list[tuple[str, str, str]] = []
    observed_times: list[dt.datetime] = []
    response_record_counts: dict[str, dict[str, int]] = {
        role: {} for role in ROLES
    }
    pending: tuple[str, str, str, dt.datetime] | None = None
    seen_pair = False
    expected_previous = ZERO_HASH
    seen_requests: set[tuple[str, str]] = set()
    seen_responses: set[tuple[str, str]] = set()
    previous_observed: dt.datetime | None = None
    if not isinstance(value, list):
        return _EventResult(
            ["MALFORMED_EVENTS:events must be a list"],
            False,
            False,
            set(),
            {},
            set(),
            None,
            (),
            (),
            {role: {} for role in ROLES},
            None,
            False,
        )
    for index, item in enumerate(value):
        try:
            entry = require_exact(
                item,
                {"schema", "event", "event_sha256"},
                f"events[{index}]",
            )
            if entry["schema"] != EVENT_ENTRY_SCHEMA:
                raise ProtocolError("event entry schema is unsupported")
            event = require_exact(
                entry["event"],
                {
                    "schema",
                    "sequence",
                    "kind",
                    "role",
                    "outcome",
                    "binding_sha256",
                    "record_sha256",
                    "observed_at_utc",
                    "previous_event_sha256",
                },
                f"events[{index}].event",
            )
            if event["schema"] != EVENT_SCHEMA:
                raise ProtocolError("event schema is unsupported")
            if require_int(event["sequence"], "event sequence", 1) != index + 1:
                raise ProtocolError("event sequence is not contiguous")
            kind = require_string(event["kind"], "event kind")
            role = require_string(event["role"], "event role")
            outcome = require_string(event["outcome"], "event outcome")
            signatures.append((kind, role, outcome))
            binding_sha256 = require_hash(event["binding_sha256"], "event binding digest")
            record_sha256 = require_hash(event["record_sha256"], "event record digest")
            observed = parse_utc(event["observed_at_utc"], "event observed time")
            observed_times.append(observed)
            if previous_observed is not None and observed <= previous_observed:
                issues.append("PHASE_REORDER")
            previous_observed = observed
            if event["previous_event_sha256"] != expected_previous:
                raise ProtocolError("event hash chain differs")
            claimed = require_hash(entry["event_sha256"], "event digest")
            actual = domain_digest(DOMAIN_EVENT, event)
            if claimed != actual:
                issues.append("CORRUPT_EVENT")
            expected_previous = claimed
            if seen_pair:
                issues.append("EVENT_AFTER_TERMINAL")
            if binding_sha256 != requested_binding.digest:
                issues.append("BINDING_DIVERGENCE")

            if kind == "PAIR_VERIFY":
                if role != "pair" or record_sha256 != ZERO_HASH:
                    issues.append("MALFORMED_PAIR_VERIFY")
                if outcome not in ("COMPLETE", "REFUSED", "IN_DOUBT"):
                    issues.append("MALFORMED_PAIR_VERIFY")
                if pending is not None:
                    issues.append("LOST_OR_UNPAIRED_RESPONSE")
                pair_outcome = outcome
                seen_pair = True
                continue

            if role not in ROLES:
                issues.append("UNKNOWN_ROLE")
                continue
            if kind.endswith("_REQUEST"):
                phase = kind.removesuffix("_REQUEST")
                if phase not in ("PREPARE", "COMMIT", "ABORT"):
                    issues.append("UNKNOWN_EVENT")
                    continue
                if outcome != "SENT" or record_sha256 != ZERO_HASH:
                    issues.append("MALFORMED_REQUEST")
                phase_role = (phase, role)
                if phase_role in seen_requests:
                    issues.append("EVENT_REPLAY")
                seen_requests.add(phase_role)
                if pending is not None:
                    issues.append("PHASE_REORDER")
                pending = (phase, role, binding_sha256, observed)
                if phase == "COMMIT":
                    commit_requested = True
                    if set(prepare_acks) != set(ROLES):
                        issues.append("COMMIT_BEFORE_BOTH_PREPARED")
                    elif any(value != requested_binding.digest for value in prepare_acks.values()):
                        issues.append("BINDING_DIVERGENCE")
                if phase == "ABORT" and (commit_requested or commit_acks):
                    issues.append("ABORT_AFTER_COMMIT_INTENT")
                continue

            if kind.endswith("_RESPONSE"):
                phase = kind.removesuffix("_RESPONSE")
                phase_role = (phase, role)
                request_observed: dt.datetime | None = None
                if phase_role in seen_responses:
                    issues.append("EVENT_REPLAY")
                seen_responses.add(phase_role)
                if pending is None or pending[:2] != (phase, role):
                    issues.append("PHASE_REORDER")
                    request_binding = binding_sha256
                else:
                    request_binding = pending[2]
                    request_observed = pending[3]
                if request_binding != binding_sha256:
                    issues.append("BINDING_DIVERGENCE")
                pending = None
                if outcome not in ("ACK", "REJECTED", "LOST"):
                    issues.append("MALFORMED_RESPONSE")
                    continue
                if outcome == "REJECTED":
                    if record_sha256 != ZERO_HASH:
                        issues.append("REJECTED_RESPONSE_HAS_RECORD")
                    continue
                if outcome == "LOST":
                    lost_response = True
                expected_phase = {
                    "PREPARE": "PREPARED",
                    "COMMIT": "COMMITTED",
                    "ABORT": "ABORTED",
                }.get(phase)
                if expected_phase is None:
                    issues.append("UNKNOWN_EVENT")
                    continue
                observed_phase = ledgers[role].record_digests.get(record_sha256)
                response_record_counts[role][record_sha256] = (
                    response_record_counts[role].get(record_sha256, 0) + 1
                )
                if observed_phase != expected_phase:
                    issues.append("RESPONSE_RECORD_DIVERGENCE")
                elif request_observed is None:
                    issues.append("PHASE_REORDER")
                else:
                    record_observed = ledgers[role].record_observed[record_sha256]
                    if not request_observed < record_observed < observed:
                        issues.append("PHASE_REORDER")
                if outcome == "ACK":
                    if phase == "PREPARE":
                        prepare_acks[role] = binding_sha256
                    elif phase == "COMMIT":
                        commit_acks.add(role)
                    elif phase == "ABORT":
                        abort_acks.add(role)
                continue
            issues.append("UNKNOWN_EVENT")
        except ProtocolError as exc:
            issues.append(f"MALFORMED_EVENT:{exc}")
    if not seen_pair:
        issues.append("MISSING_PAIR_VERIFY")
    if pending is not None:
        issues.append("LOST_OR_UNPAIRED_RESPONSE")
    record_response_bijection = True
    for role in ROLES:
        expected_counts: dict[str, int] = {}
        for digest in ledgers[role].record_digest_sequence:
            expected_counts[digest] = expected_counts.get(digest, 0) + 1
        if response_record_counts[role] != expected_counts:
            issues.append("RECORD_RESPONSE_BIJECTION")
            record_response_bijection = False
    return _EventResult(
        issues=issues,
        lost_response=lost_response,
        commit_requested=commit_requested,
        commit_acks=commit_acks,
        prepare_acks=prepare_acks,
        abort_acks=abort_acks,
        pair_outcome=pair_outcome,
        signatures=tuple(signatures),
        observed_times=tuple(observed_times),
        response_record_counts=response_record_counts,
        final_observed=previous_observed,
        record_response_bijection=record_response_bijection,
    )


RAW_SCAN_MAX_DEPTH = 64
RAW_SCAN_MAX_NODES = 20_000


@dataclass(frozen=True)
class _RawEvidence:
    possible_commit: bool
    lost_response: bool
    durable_transition: bool
    incomplete: bool


def _scan_raw_evidence(value: Any) -> _RawEvidence:
    """Bounded iterative scan used only to preserve uncertainty on bad input."""

    possible_commit = False
    lost_response = False
    durable_transition = False
    incomplete = False
    visited: set[int] = set()
    pending: list[tuple[Any, int]] = [(value, 0)]
    visited_nodes = 0
    try:
        while pending:
            item, depth = pending.pop()
            visited_nodes += 1
            if depth > RAW_SCAN_MAX_DEPTH or visited_nodes > RAW_SCAN_MAX_NODES:
                incomplete = True
                possible_commit = True
                break
            if isinstance(item, dict):
                identity = id(item)
                if identity in visited:
                    incomplete = True
                    possible_commit = True
                    break
                visited.add(identity)
                if (
                    item.get("phase") == "COMMITTED"
                    or item.get("kind") in {"COMMIT_REQUEST", "COMMIT_RESPONSE"}
                    or item.get("pair_state") == "SIMULATED_COMMITTED"
                    or item.get("status") == "SIMULATED_COMPLETE"
                    or item.get("pair_certificate") is not None
                ):
                    possible_commit = True
                if item.get("phase") in PHASES:
                    durable_transition = True
                if item.get("outcome") == "LOST":
                    lost_response = True
                if len(pending) + len(item) > RAW_SCAN_MAX_NODES:
                    incomplete = True
                    possible_commit = True
                    break
                pending.extend((child, depth + 1) for child in item.values())
            elif isinstance(item, list):
                identity = id(item)
                if identity in visited:
                    incomplete = True
                    possible_commit = True
                    break
                visited.add(identity)
                if len(pending) + len(item) > RAW_SCAN_MAX_NODES:
                    incomplete = True
                    possible_commit = True
                    break
                pending.extend((child, depth + 1) for child in item)
    except Exception:
        incomplete = True
        possible_commit = True
    return _RawEvidence(
        possible_commit,
        lost_response,
        durable_transition,
        incomplete,
    )


def _verify_scenario_grammar(
    scenario: str,
    binding: ParsedBinding,
    ledgers: Mapping[str, _LedgerResult],
    events: _EventResult,
    terminal: Mapping[str, Any] | None,
    certificate: Any,
    observed_issues: Sequence[str],
) -> list[str]:
    """Require one exact event/journal/terminal language per fake scenario."""

    expected_events, expected_phases, expected_status, expected_reason, has_cert = (
        SCENARIO_GRAMMAR[scenario]
    )
    issues: list[str] = []
    if events.signatures != expected_events:
        issues.append("SCENARIO_EVENT_GRAMMAR")
    for role in ROLES:
        if ledgers[role].phases != expected_phases[role]:
            issues.append("SCENARIO_LEDGER_GRAMMAR")
        if (ledgers[role].binding is not None) is not SCENARIO_NODE_BINDING[scenario][
            role
        ]:
            issues.append("SCENARIO_LEDGER_GRAMMAR")
    if terminal is None or (
        terminal.get("status") != expected_status
        or terminal.get("reason") != expected_reason
    ):
        issues.append("SCENARIO_TERMINAL_GRAMMAR")
    if (certificate is not None) is not has_cert:
        issues.append("SCENARIO_CERTIFICATE_GRAMMAR")
    if scenario == "expired":
        if not events.observed_times or any(
            active_at(binding, observed) for observed in events.observed_times
        ):
            issues.append("SCENARIO_FAULT_PREDICATE")
    elif scenario == "late-window":
        # A reserved success path may reach the pair event immediately before
        # expiry or exactly at expiry; verification must still occur later and
        # is checked separately.  Every request, durable transition, and
        # response preceding PAIR_VERIFY must have remained inside the window.
        if len(events.observed_times) != len(expected_events) or any(
            not active_at(binding, observed)
            for observed in events.observed_times[:-1]
        ):
            issues.append("SCENARIO_FAULT_PREDICATE")
    elif any(not active_at(binding, observed) for observed in events.observed_times):
        issues.append("SCENARIO_FAULT_PREDICATE")
    required_issue = {
        "split-brain": "BINDING_DIVERGENCE",
        "stale-epoch": "STALE_EPOCH",
        "replay": "NONCE_REPLAY",
        "reordered-commit": "COMMIT_BEFORE_BOTH_PREPARED",
        "corrupt-commit-record": "CORRUPT_DURABLE_RECORD",
    }.get(scenario)
    if required_issue is not None and required_issue not in observed_issues:
        issues.append("SCENARIO_FAULT_PREDICATE")
    return sorted(set(issues))


def _verify_certificate(
    value: Any,
    binding: ParsedBinding,
    ledgers: Mapping[str, _LedgerResult],
) -> list[str]:
    issues: list[str] = []
    try:
        raw = require_exact(
            value,
            {
                "schema",
                "binding_sha256",
                "transaction_id",
                "authorization_sha256",
                "nonce_commitment",
                "epoch",
                "node_commit_record_sha256",
                "pair_state",
                "authorized",
                "simulation_qualified",
                "target_execution_qualified",
                "distributed_atomicity_proven",
                "certificate_sha256",
            },
            "pair_certificate",
        )
        if raw["schema"] != PAIR_CERTIFICATE_SCHEMA:
            raise ProtocolError("pair certificate schema is unsupported")
        if raw["binding_sha256"] != binding.digest:
            issues.append("CERTIFICATE_BINDING_DIVERGENCE")
        if raw["transaction_id"] != binding.transaction_id:
            issues.append("CERTIFICATE_TRANSACTION_DIVERGENCE")
        if raw["authorization_sha256"] != binding.raw["authorization_sha256"]:
            issues.append("CERTIFICATE_AUTHORIZATION_DIVERGENCE")
        if raw["nonce_commitment"] != binding.nonce_commitment:
            issues.append("CERTIFICATE_NONCE_DIVERGENCE")
        if raw["epoch"] != binding.epoch:
            issues.append("CERTIFICATE_EPOCH_DIVERGENCE")
        commits = require_exact(
            raw["node_commit_record_sha256"], set(ROLES), "certificate commits"
        )
        for role in ROLES:
            digest = require_hash(commits[role], f"certificate commits.{role}")
            if ledgers[role].record_digests.get(digest) != "COMMITTED":
                issues.append("CERTIFICATE_COMMIT_DIVERGENCE")
        if raw["pair_state"] != "SIMULATED_COMMITTED":
            issues.append("CERTIFICATE_STATE_DIVERGENCE")
        for field in (
            "authorized",
            "simulation_qualified",
            "target_execution_qualified",
            "distributed_atomicity_proven",
        ):
            if require_bool(raw[field], f"pair_certificate.{field}") is not False:
                issues.append("UNSAFE_CERTIFICATE_CLAIM")
        claimed = require_hash(raw["certificate_sha256"], "certificate digest")
        unhashed = copy.deepcopy(raw)
        del unhashed["certificate_sha256"]
        if claimed != domain_digest(DOMAIN_CERTIFICATE, unhashed):
            issues.append("CORRUPT_PAIR_CERTIFICATE")
    except ProtocolError as exc:
        issues.append(f"MALFORMED_PAIR_CERTIFICATE:{exc}")
    return issues


def verify_transcript(value: Any) -> dict[str, Any]:
    """Verify a closed fake transcript and return a non-authorizing result."""

    issues: list[str] = []
    raw_evidence = _scan_raw_evidence(value)
    raw_possible_commit = raw_evidence.possible_commit
    possible_commit = raw_possible_commit
    lost_response = raw_evidence.lost_response
    ambiguity_evidence = raw_evidence.incomplete
    if raw_evidence.incomplete:
        issues.append("RAW_EVIDENCE_SCAN_LIMIT")
    top_parsed = False
    binding: ParsedBinding | None = None
    scenario: str | None = None
    ledger_results: dict[str, _LedgerResult] = {}
    event_result: _EventResult | None = None
    terminal_raw: dict[str, Any] | None = None
    certificate_value: Any = None
    verification_time: dt.datetime | None = None
    try:
        raw = require_exact(
            value,
            {
                "schema",
                "execution_scope",
                "target_execution_enabled",
                "transport",
                "durability",
                "fault_scenario",
                "binding",
                "binding_sha256",
                "verification_time_utc",
                "nodes",
                "events",
                "pair_certificate",
                "terminal",
            },
            "transcript",
        )
        if raw["schema"] != TRANSCRIPT_SCHEMA:
            raise ProtocolError("transcript schema is unsupported")
        if raw["execution_scope"] != EXECUTION_SCOPE:
            raise ProtocolError("transcript execution scope is not offline simulation")
        if require_bool(raw["target_execution_enabled"], "target_execution_enabled") is not False:
            raise ProtocolError("target execution must remain false")
        if raw["transport"] != TRANSPORT_KIND or raw["durability"] != DURABILITY_KIND:
            raise ProtocolError("transcript uses a non-fake transport or durability kind")
        scenario = require_string(raw["fault_scenario"], "fault_scenario")
        if scenario not in SCENARIOS:
            raise ProtocolError("transcript fault scenario is unsupported")
        binding = parse_binding(raw["binding"])
        if require_hash(raw["binding_sha256"], "binding_sha256") != binding.digest:
            issues.append("BINDING_DIGEST_DIVERGENCE")
        verification_time = parse_utc(raw["verification_time_utc"], "verification_time_utc")
        nodes_raw = require_exact(raw["nodes"], set(ROLES), "nodes")
        ledger_results = {
            role: _verify_ledger(nodes_raw[role], role) for role in ROLES
        }
        parsed_prestates = {
            ledger_results[role].before_state for role in ROLES
        }
        if None not in parsed_prestates and len(parsed_prestates) != 1:
            issues.append("PERSISTENT_PRESTATE_DIVERGENCE")
        for result in ledger_results.values():
            issues.extend(result.issues)
            possible_commit = possible_commit or result.commit_observed
            if result.binding is not None and result.binding.digest != binding.digest:
                issues.append("BINDING_DIVERGENCE")
        event_result = _verify_events(raw["events"], ledger_results, binding)
        issues.extend(event_result.issues)
        lost_response = lost_response or event_result.lost_response
        possible_commit = possible_commit or event_result.commit_requested
        certificate_value = raw["pair_certificate"]
        terminal_raw = require_exact(
            raw["terminal"],
            {
                "schema",
                "status",
                "reason",
                "authorized",
                "simulation_qualified",
                "pair_certificate_sha256",
            },
            "terminal",
        )
        if terminal_raw["schema"] != TERMINAL_SCHEMA:
            issues.append("MALFORMED_TERMINAL")
        terminal_status = require_string(terminal_raw["status"], "terminal.status")
        if terminal_status not in STATUSES:
            issues.append("MALFORMED_TERMINAL")
        reason = require_string(terminal_raw["reason"], "terminal.reason")
        if reason not in TERMINAL_REASONS:
            issues.append("MALFORMED_TERMINAL")
        elif (
            terminal_status in TERMINAL_REASONS_BY_STATUS
            and reason not in TERMINAL_REASONS_BY_STATUS[terminal_status]
        ):
            issues.append("MALFORMED_TERMINAL")
        if require_bool(terminal_raw["authorized"], "terminal.authorized") is not False:
            issues.append("UNSAFE_TERMINAL_CLAIM")
        if require_bool(
            terminal_raw["simulation_qualified"], "terminal.simulation_qualified"
        ) is not False:
            issues.append("UNSAFE_TERMINAL_CLAIM")
        require_hash(
            terminal_raw["pair_certificate_sha256"],
            "terminal.pair_certificate_sha256",
        )
        scenario_issues = _verify_scenario_grammar(
            scenario,
            binding,
            ledger_results,
            event_result,
            terminal_raw,
            certificate_value,
            issues,
        )
        issues.extend(scenario_issues)
        ambiguity_evidence = ambiguity_evidence or bool(scenario_issues)
        if (
            event_result.final_observed is None
            or verification_time <= event_result.final_observed
        ):
            issues.append("VERIFICATION_TIME_REORDER")
            ambiguity_evidence = True
        top_parsed = True
    except (ProtocolError, RecursionError, TypeError, ValueError) as exc:
        issues.append(f"MALFORMED_TRANSCRIPT:{exc}")

    if not top_parsed and raw_evidence.durable_transition:
        issues.append("UNRESOLVED_DURABLE_EVIDENCE")
        ambiguity_evidence = True

    if top_parsed and event_result is not None and len(ledger_results) == 2:
        # A well-formed, explicit REJECTED response with no committed record is
        # a definite pre-commit refusal.  Malformed evidence keeps the raw
        # conservative scan so deleting an ACK cannot downgrade uncertainty.
        event_malformed = any(
            item.startswith("MALFORMED")
            or item in {
                "CORRUPT_EVENT",
                "LOST_OR_UNPAIRED_RESPONSE",
                "PHASE_REORDER",
                "RESPONSE_RECORD_DIVERGENCE",
                "RECORD_RESPONSE_BIJECTION",
            }
            for item in event_result.issues
        )
        if not event_result.record_response_bijection:
            ambiguity_evidence = True
        committed_record = any(
            result.commit_observed for result in ledger_results.values()
        )
        possible_commit = (
            committed_record
            or bool(event_result.commit_acks)
            or lost_response
            or (event_result.commit_requested and event_malformed)
        )

    complete_candidate = False
    if binding is not None and event_result is not None and len(ledger_results) == 2:
        complete_candidate = (
            not lost_response
            and all(
                ledger_results[role].terminal_phase == "COMMITTED" for role in ROLES
            )
            and event_result.commit_acks == set(ROLES)
            and event_result.prepare_acks == {
                role: binding.digest for role in ROLES
            }
            and event_result.pair_outcome == "COMPLETE"
            and verification_time is not None
            and active_at(binding, verification_time)
        )
        if verification_time is not None and not active_at(binding, verification_time):
            issues.append("WINDOW_INACTIVE")

    if complete_candidate:
        if certificate_value is None:
            issues.append("MISSING_PAIR_CERTIFICATE")
        else:
            issues.extend(
                _verify_certificate(certificate_value, binding, ledger_results)
            )
        if terminal_raw is None:
            issues.append("MISSING_TERMINAL")
        else:
            if terminal_raw.get("status") != "SIMULATED_COMPLETE":
                issues.append("TERMINAL_CLASSIFICATION_DIVERGENCE")
            expected_certificate = (
                certificate_value.get("certificate_sha256")
                if isinstance(certificate_value, dict)
                else ZERO_HASH
            )
            if terminal_raw.get("pair_certificate_sha256") != expected_certificate:
                issues.append("TERMINAL_CERTIFICATE_DIVERGENCE")
    else:
        if certificate_value is not None:
            issues.append("UNSAFE_PAIR_CERTIFICATE")
        if terminal_raw is not None and terminal_raw.get(
            "pair_certificate_sha256"
        ) != ZERO_HASH:
            issues.append("UNSAFE_TERMINAL_CERTIFICATE")

    if top_parsed and scenario is not None:
        # Each closed synthetic scenario has one exact semantic issue set.
        # Known refusal evidence (for example, an expired window) remains a
        # definite refusal only while no additional integrity contradiction is
        # present.  Any extra or missing issue makes the outcome ambiguous.
        if frozenset(issues) != SCENARIO_ALLOWED_ISSUES[scenario]:
            issues.append("SCENARIO_ISSUE_DIVERGENCE")
            ambiguity_evidence = True

    if binding is not None and len(ledger_results) == 2:
        # A quarantine appearing on an otherwise clean complete/refusal path
        # is itself durable ambiguity.  Make that ambiguity sticky before
        # deriving the pair-wide quarantine requirement.
        in_doubt_evidence = lost_response or possible_commit or ambiguity_evidence
        clean_complete = complete_candidate and not issues
        quarantine_required = in_doubt_evidence and not clean_complete
        if not quarantine_required and any(
            ledger_results[role].after_quarantine != ZERO_HASH for role in ROLES
        ):
            issues.append("UNEXPECTED_PAIR_QUARANTINE")
            ambiguity_evidence = True
            quarantine_required = True
        expected_quarantine = (
            binding.transaction_id if quarantine_required else ZERO_HASH
        )
        for role in ROLES:
            if ledger_results[role].after_quarantine != expected_quarantine:
                issues.append(
                    "MISSING_PAIR_QUARANTINE"
                    if quarantine_required
                    else "UNEXPECTED_PAIR_QUARANTINE"
                )

    in_doubt_evidence = lost_response or possible_commit or ambiguity_evidence
    issues = sorted(set(issues))
    if complete_candidate and not issues:
        classification = "SIMULATED_COMPLETE"
        pair_committed = True
    elif in_doubt_evidence:
        classification = "IN_DOUBT"
        pair_committed = False
    else:
        classification = "REFUSED"
        pair_committed = False

    if terminal_raw is not None:
        if terminal_raw.get("status") != classification:
            issues = sorted(set(issues + ["TERMINAL_CLASSIFICATION_DIVERGENCE"]))
            if in_doubt_evidence:
                classification = "IN_DOUBT"
            else:
                classification = "REFUSED"
        if classification != "SIMULATED_COMPLETE" and terminal_raw.get(
            "pair_certificate_sha256"
        ) != ZERO_HASH:
            issues = sorted(set(issues + ["UNSAFE_TERMINAL_CERTIFICATE"]))

    return {
        "schema": VERIFICATION_SCHEMA,
        "classification": classification,
        "authorized": False,
        "simulation_qualified": False,
        "target_execution_qualified": False,
        "distributed_atomicity_proven": False,
        "pair_committed": pair_committed,
        "transcript_accepted": classification == "SIMULATED_COMPLETE" and not issues,
        "issues": issues,
    }


def example_binding() -> dict[str, Any]:
    """Return deterministic synthetic inputs; every digest is fixture-only."""

    return {
        "schema": BINDING_SCHEMA,
        "authorization_sha256": "11" * 32,
        "nonce": "22" * 32,
        "not_before_utc": "2026-08-13T06:00:00Z",
        "expires_utc": "2026-08-13T08:00:00Z",
        "source": {
            "schema": SOURCE_SCHEMA,
            "repository_url": CANONICAL_REPOSITORY,
            "commit": "3" * 40,
            "tree_sha256": "44" * 32,
            "executable_sha256": "55" * 32,
        },
        "plan_sha256": "66" * 32,
        "policy_sha256": "77" * 32,
        "incident_sha256": "88" * 32,
        "epoch": 7,
        "attempt_id": "99" * 32,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Offline-only HaloFPX maintenance nonce protocol model"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)
    simulate = subparsers.add_parser(
        "simulate", help="write one deterministic synthetic transcript to stdout"
    )
    simulate.add_argument("--scenario", choices=SCENARIOS, default="success")
    subparsers.add_parser(
        "verify", help="verify one synthetic transcript read only from stdin"
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    if TARGET_EXECUTION_ENABLED:
        raise ProtocolError("target execution invariant was violated")
    if args.command == "simulate":
        observed = dt.datetime(2026, 8, 13, 7, 0, tzinfo=dt.timezone.utc)
        transcript = simulate_pair(
            example_binding(), observed_at=observed, scenario=args.scenario
        )
        sys.stdout.buffer.write(canonical_bytes(transcript) + b"\n")
        return 0
    if args.command == "verify":
        try:
            transcript = parse_closed_json(sys.stdin.buffer.read(), "stdin transcript")
            result = verify_transcript(transcript)
        except ProtocolError as exc:
            result = {
                "schema": VERIFICATION_SCHEMA,
                "classification": "REFUSED",
                "authorized": False,
                "simulation_qualified": False,
                "target_execution_qualified": False,
                "distributed_atomicity_proven": False,
                "pair_committed": False,
                "transcript_accepted": False,
                "issues": [f"MALFORMED_TRANSCRIPT:{exc}"],
            }
        sys.stdout.buffer.write(canonical_bytes(result) + b"\n")
        return 0 if result["classification"] == "SIMULATED_COMPLETE" else 2
    raise ProtocolError("unsupported command")


if __name__ == "__main__":
    raise SystemExit(main())
