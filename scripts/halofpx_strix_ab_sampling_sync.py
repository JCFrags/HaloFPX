#!/usr/bin/env python3
"""Offline evidence sidecar for sampling-output synchronization counters.

This module never sends a request and never contacts a target.  It freezes an
optional, default-off observability contract next to an initialized Strix A/B
run, imports raw before/after Prometheus snapshots plus a capture receipt, and
validates exact unsigned-64-bit deltas.  The ordinary A/B plan and sample
schemas remain unchanged.
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import re
import shutil
import sys
import tempfile
from pathlib import Path
from typing import Any


def _load_core() -> Any:
    path = Path(__file__).with_name("halofpx_strix_ab.py")
    spec = importlib.util.spec_from_file_location("halofpx_strix_ab_sidecar_core", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load Strix A/B evidence core: {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


CORE = _load_core()
PlanError = CORE.PlanError

CONTRACT = "sampling_output_sync_prometheus_v1"
PLAN_SCHEMA = "halofpx.strix-ab-sampling-output-sync-prometheus.v1"
CAPTURE_SCHEMA = "halofpx.sampling-output-sync-capture.v1"
SAMPLE_SCHEMA = "halofpx.sampling-output-sync-sample.v1"
ANALYSIS_SCHEMA = "halofpx.sampling-output-sync-analysis.v1"
PLAN_FILENAME = "sampling-output-sync-plan.json"
ANALYSIS_FILENAME = "sampling-output-sync-analysis.json"
EVIDENCE_DIRECTORY = "sampling-output-sync"
EVIDENCE_VALIDATOR_ROOT_FILES = (PLAN_FILENAME, ANALYSIS_FILENAME)
EVIDENCE_VALIDATOR_SAMPLE_FILES = (
    "before.prom", "after.prom", "capture.json", "summary.json")
UINT64_MAX = (1 << 64) - 1
INVOCATION_RE = re.compile(r"^[0-9a-f]{32}$")
DECIMAL_RE = re.compile(r"^(?:0|[1-9][0-9]*)$")
PROMETHEUS_SAMPLE_RE = re.compile(
    r"^([A-Za-z_:][A-Za-z0-9_:]*)(\{[^}]*\})?[ \t]+([^ \t]+)(?:[ \t]+([^ \t]+))?[ \t]*$")

METRICS = {
    "output_epochs": "llamacpp:halofpx_sampling_sync_output_epochs_total",
    "completed_barriers": "llamacpp:halofpx_sampling_sync_completed_barriers_total",
    "reused_barriers": "llamacpp:halofpx_sampling_sync_reused_barriers_total",
    "graph_submissions": "llamacpp:halofpx_sampling_sync_graph_submissions_total",
    "output_transfers": "llamacpp:halofpx_sampling_sync_output_transfers_total",
}
METRIC_BY_WIRE_NAME = {wire: name for name, wire in METRICS.items()}
STALE_METRIC = "llamacpp:halofpx_sampling_sync_generations_total"


def require_uint64(value: Any, where: str, minimum: int = 0) -> int:
    result = CORE.require_int(value, where, minimum)
    if result > UINT64_MAX:
        raise CORE.PlanError(f"{where} exceeds UINT64_MAX")
    return result


def _normalized_option(item: str) -> str:
    option = item.split("=", 1)[0]
    return option.replace("_", "-") if option.startswith("--") else option


def _require_literal_once(args: list[str], flag: str, where: str) -> None:
    matches = [item for item in args if _normalized_option(item) == flag]
    if matches != [flag]:
        raise CORE.PlanError(f"{where} must contain exactly one literal {flag}")


def _require_pair_once(args: list[str], flag: str, expected: str, where: str) -> None:
    value = _canonical_pair_value(args, flag, where)
    if value != expected:
        raise CORE.PlanError(f"{where} must contain the canonical {flag} {expected} pair")


def _canonical_pair_value(args: list[str], flag: str, where: str) -> str:
    matches: list[int] = []
    for index, item in enumerate(args):
        normalized = _normalized_option(item)
        if normalized == flag or normalized.startswith(flag + "="):
            matches.append(index)
    if len(matches) != 1:
        raise CORE.PlanError(f"{where} must contain exactly one canonical {flag} pair")
    index = matches[0]
    if args[index] != flag or index + 1 >= len(args) or args[index + 1].startswith("-"):
        raise CORE.PlanError(f"{where} must contain one literal {flag} VALUE pair")
    return args[index + 1]


def validate_observability_plan(value: Any, core_plan: dict[str, Any]) -> dict[str, Any]:
    plan = CORE.require_mapping(value, "sampling-output-sync plan")
    CORE.require_keys(
        plan,
        {
            "schema", "lane", "enabled", "issue", "core_plan_sha256", "endpoint",
            "completion_endpoint", "metrics", "conditions",
        },
        set(),
        "sampling-output-sync plan",
    )
    if plan["schema"] != PLAN_SCHEMA:
        raise CORE.PlanError(f"sampling-output-sync plan.schema must be {PLAN_SCHEMA}")
    if plan["lane"] != CONTRACT:
        raise CORE.PlanError(f"sampling-output-sync plan.lane must be {CONTRACT}")
    if not isinstance(plan["enabled"], bool):
        raise CORE.PlanError("sampling-output-sync plan.enabled must be boolean")
    if isinstance(plan["issue"], bool) or not isinstance(plan["issue"], int) or plan["issue"] != 28:
        raise CORE.PlanError("sampling-output-sync plan.issue must equal 28")
    if plan["endpoint"] != "/metrics":
        raise CORE.PlanError("sampling-output-sync plan.endpoint must equal /metrics")
    if plan["completion_endpoint"] != "/completion":
        raise CORE.PlanError("sampling-output-sync plan.completion_endpoint must equal /completion")
    metrics = CORE.require_mapping(plan["metrics"], "sampling-output-sync plan.metrics")
    if metrics != METRICS:
        raise CORE.PlanError("sampling-output-sync plan.metrics must name the exact five v1 counters")
    conditions = CORE.require_mapping(plan["conditions"], "sampling-output-sync plan.conditions")
    if conditions != {"off": {"coalescing": False}, "on": {"coalescing": True}}:
        raise CORE.PlanError("sampling-output-sync plan.conditions must bind OFF/ON coalescing semantics")
    expected_digest = CORE.plan_digest(core_plan)
    if CORE.require_hash(
            plan["core_plan_sha256"], "sampling-output-sync plan.core_plan_sha256") != expected_digest:
        raise CORE.PlanError("sampling-output-sync plan does not bind the frozen core plan")
    if not plan["enabled"]:
        return plan
    if 28 not in core_plan["issues"]:
        raise CORE.PlanError("enabled sampling-output-sync observation requires GitHub issue 28")
    if core_plan.get("comparison", {}).get("kind") == "runtime_n_batch":
        raise CORE.PlanError(f"{CONTRACT} requires a feature-build comparison")
    if core_plan["source"]["off_commit"] != core_plan["source"]["on_commit"]:
        raise CORE.PlanError(f"{CONTRACT} requires identical OFF and ON source commits")
    environment = core_plan["runtime"]["common_environment"]
    forbidden_environment = {
        "LLAMA_ARG_ENDPOINT_METRICS", "LLAMA_ARG_N_PARALLEL", "LLAMA_ARG_CONT_BATCHING"}
    if forbidden_environment.intersection(environment):
        raise CORE.PlanError("sampling-output-sync plan forbids environment overrides of its request controls")
    ports: set[int] = set()
    for condition in ("off", "on"):
        args = CORE.condition_commands(core_plan, condition)["coordinator"]
        where = f"generated {condition} coordinator argv"
        _require_literal_once(args, "--metrics", where)
        _require_pair_once(args, "--parallel", "1", where)
        _require_literal_once(args, "--no-cont-batching", where)
        _require_literal_once(args, "--no-warmup", where)
        port_text = _canonical_pair_value(args, "--port", where)
        if DECIMAL_RE.fullmatch(port_text) is None or not 1 <= int(port_text, 10) <= 65535:
            raise CORE.PlanError(f"{where} must bind one decimal TCP port")
        ports.add(int(port_text, 10))
        normalized = {_normalized_option(item) for item in args}
        if normalized.intersection({"-np", "-cb", "-nocb", "--cont-batching", "--warmup"}):
            raise CORE.PlanError(f"{where} contains a control that contradicts {CONTRACT}")
    if len(ports) != 1:
        raise CORE.PlanError("sampling-output-sync OFF/ON coordinator endpoint ports differ")
    return plan


def load_observability_plan(path: Path, core_plan: dict[str, Any]) -> dict[str, Any]:
    return validate_observability_plan(CORE.read_json(path), core_plan)


def freeze_observability_plan(root: Path, plan_path: Path) -> dict[str, Any]:
    core_plan = CORE.load_plan(root / "plan.json")
    CORE.validate_run_contract(root, core_plan)
    if any((root / "raw").iterdir()):
        raise CORE.PlanError("sampling-output-sync plan must be frozen before raw samples")
    _, analysis_exists = _inspect_analysis_destination(root)
    if analysis_exists:
        raise CORE.PlanError("sampling-output-sync analysis exists before its sidecar plan")
    destination = root / PLAN_FILENAME
    CORE.reject_symlink_components(destination, "sampling-output-sync plan")
    try:
        if destination.exists():
            raise CORE.PlanError(f"sampling-output-sync plan already exists: {destination}")
    except OSError as exc:
        raise CORE.PlanError(f"cannot inspect sampling-output-sync plan: {destination}: {exc}") from exc
    plan = load_observability_plan(plan_path, core_plan)
    CORE.write_json(destination, plan)
    return plan


def parse_prometheus_snapshot(content: bytes, where: str) -> dict[str, int]:
    try:
        text = content.decode("utf-8", errors="strict")
    except UnicodeError as exc:
        raise CORE.PlanError(f"{where} is not strict UTF-8") from exc
    found: dict[str, int] = {}
    type_counts = {name: 0 for name in METRICS}
    for line_number, raw_line in enumerate(text.splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            if line.startswith("# TYPE "):
                parts = line.split()
                if len(parts) >= 3 and parts[2] == STALE_METRIC:
                    raise CORE.PlanError(f"{where}:{line_number} uses the stale generations metric name")
                if len(parts) >= 3 and parts[2] in METRIC_BY_WIRE_NAME:
                    logical_name = METRIC_BY_WIRE_NAME[parts[2]]
                    if parts != ["#", "TYPE", parts[2], "counter"]:
                        raise CORE.PlanError(
                            f"{where}:{line_number} must declare {parts[2]} as exactly one counter TYPE")
                    type_counts[logical_name] += 1
            continue
        match = PROMETHEUS_SAMPLE_RE.fullmatch(line)
        if match is None:
            first = line.split(None, 1)[0].split("{", 1)[0]
            if first in METRIC_BY_WIRE_NAME or first == STALE_METRIC:
                raise CORE.PlanError(f"{where}:{line_number} has malformed sampling-sync telemetry")
            continue
        wire_name, labels, value_text, timestamp = match.groups()
        if wire_name == STALE_METRIC:
            raise CORE.PlanError(f"{where}:{line_number} uses the stale generations metric name")
        logical_name = METRIC_BY_WIRE_NAME.get(wire_name)
        if logical_name is None:
            continue
        if logical_name in found:
            raise CORE.PlanError(f"{where} duplicates {wire_name}")
        if labels is not None:
            raise CORE.PlanError(f"{where}:{line_number} labels {wire_name}; one context is required")
        if timestamp is not None or DECIMAL_RE.fullmatch(value_text) is None:
            raise CORE.PlanError(f"{where}:{line_number} must encode {wire_name} as one exact uint64 decimal")
        value = int(value_text, 10)
        if value > UINT64_MAX:
            raise CORE.PlanError(f"{where}:{line_number} exceeds UINT64_MAX")
        found[logical_name] = value
    missing = set(METRICS) - found.keys()
    if missing:
        names = ", ".join(METRICS[name] for name in sorted(missing))
        raise CORE.PlanError(f"{where} is missing sampling-sync metrics: {names}")
    invalid_types = [METRICS[name] for name, count in type_counts.items() if count != 1]
    if invalid_types:
        raise CORE.PlanError(
            f"{where} must contain exactly one counter TYPE for: {', '.join(invalid_types)}")
    return found


def parse_identity(value: Any, where: str) -> dict[str, Any]:
    identity = CORE.require_mapping(value, where)
    CORE.require_keys(
        identity,
        {"pid", "invocation_id", "process_start_ticks", "metrics_process_start_time_unix"},
        set(), where)
    pid = CORE.require_int(identity["pid"], f"{where}.pid", 1)
    invocation_id = CORE.require_string(identity["invocation_id"], f"{where}.invocation_id")
    if INVOCATION_RE.fullmatch(invocation_id) is None:
        raise CORE.PlanError(f"{where}.invocation_id must be 32 lowercase hex characters")
    process_start_ticks = require_uint64(
        identity["process_start_ticks"], f"{where}.process_start_ticks", 1)
    metrics_process_start_time_unix = require_uint64(
        identity["metrics_process_start_time_unix"],
        f"{where}.metrics_process_start_time_unix", 1)
    return {
        "pid": pid,
        "invocation_id": invocation_id,
        "process_start_ticks": process_start_ticks,
        "metrics_process_start_time_unix": metrics_process_start_time_unix,
    }


def parse_capture_receipt(
    value: Any,
    condition: str,
    request_sha256: str,
    before_sha256: str,
    after_sha256: str,
    response_sha256: str,
    client_sha256: str,
    endpoint_identity: dict[str, Any],
) -> dict[str, Any]:
    receipt = CORE.require_mapping(value, "sampling-output-sync capture")
    CORE.require_keys(
        receipt,
        {"schema", "contract", "condition", "endpoint", "sequence", "before", "request", "after"},
        set(),
        "sampling-output-sync capture",
    )
    if receipt["schema"] != CAPTURE_SCHEMA or receipt["contract"] != CONTRACT:
        raise CORE.PlanError("sampling-output-sync capture schema or contract mismatch")
    if receipt["condition"] != condition:
        raise CORE.PlanError("sampling-output-sync capture condition mismatch")
    if receipt["endpoint"] != endpoint_identity:
        raise CORE.PlanError("sampling-output-sync capture endpoint identity mismatch")
    if receipt["sequence"] != ["metrics_before", "request", "metrics_after"]:
        raise CORE.PlanError("sampling-output-sync capture must declare the immediate three-step sequence")

    snapshots: dict[str, dict[str, Any]] = {}
    for name, expected_hash in (("before", before_sha256), ("after", after_sha256)):
        snapshot = CORE.require_mapping(receipt[name], f"sampling-output-sync capture.{name}")
        CORE.require_keys(
            snapshot, {"captured_monotonic_ns", "identity", "metrics_sha256"}, set(),
            f"sampling-output-sync capture.{name}")
        observed_hash = CORE.require_hash(
            snapshot["metrics_sha256"], f"sampling-output-sync capture.{name}.metrics_sha256")
        if observed_hash != expected_hash:
            raise CORE.PlanError(f"sampling-output-sync capture.{name} does not bind the raw metrics")
        snapshots[name] = {
            "captured_monotonic_ns": require_uint64(
                snapshot["captured_monotonic_ns"],
                f"sampling-output-sync capture.{name}.captured_monotonic_ns", 1),
            "identity": parse_identity(
                snapshot["identity"], f"sampling-output-sync capture.{name}.identity"),
            "metrics_sha256": observed_hash,
        }

    request = CORE.require_mapping(receipt["request"], "sampling-output-sync capture.request")
    CORE.require_keys(
        request,
        {
            "started_monotonic_ns", "ended_monotonic_ns", "identity", "request_sha256",
            "response_sha256", "client_sha256", "request_count",
        },
        set(),
        "sampling-output-sync capture.request",
    )
    if CORE.require_int(
            request["request_count"], "sampling-output-sync capture.request.request_count", 1) != 1:
        raise CORE.PlanError("sampling-output-sync capture must contain exactly one request")
    if CORE.require_hash(
            request["request_sha256"], "sampling-output-sync capture.request.request_sha256") != request_sha256:
        raise CORE.PlanError("sampling-output-sync capture request differs from the frozen request")
    for name, expected in (("response_sha256", response_sha256), ("client_sha256", client_sha256)):
        if CORE.require_hash(
                request[name], f"sampling-output-sync capture.request.{name}") != expected:
            raise CORE.PlanError(f"sampling-output-sync capture does not bind the core {name}")
    parsed_request = {
        "started_monotonic_ns": require_uint64(
            request["started_monotonic_ns"],
            "sampling-output-sync capture.request.started_monotonic_ns", 1),
        "ended_monotonic_ns": require_uint64(
            request["ended_monotonic_ns"],
            "sampling-output-sync capture.request.ended_monotonic_ns", 1),
        "identity": parse_identity(request["identity"], "sampling-output-sync capture.request.identity"),
        "request_sha256": request_sha256,
        "response_sha256": response_sha256,
        "client_sha256": client_sha256,
        "request_count": 1,
    }
    identities = [snapshots["before"]["identity"], parsed_request["identity"], snapshots["after"]["identity"]]
    if any(identity != identities[0] for identity in identities[1:]):
        raise CORE.PlanError("sampling-output-sync capture changed PID, InvocationID, or process start identity")
    ordered = (
        snapshots["before"]["captured_monotonic_ns"],
        parsed_request["started_monotonic_ns"],
        parsed_request["ended_monotonic_ns"],
        snapshots["after"]["captured_monotonic_ns"],
    )
    if any(later <= earlier for earlier, later in zip(ordered, ordered[1:])):
        raise CORE.PlanError("sampling-output-sync before/request/after capture order is not strict")
    return {
        "endpoint": endpoint_identity,
        "identity": identities[0],
        "before_captured_monotonic_ns": ordered[0],
        "request_started_monotonic_ns": ordered[1],
        "request_ended_monotonic_ns": ordered[2],
        "after_captured_monotonic_ns": ordered[3],
        "request_count": 1,
    }


def counter_delta(before: dict[str, int], after: dict[str, int]) -> dict[str, int]:
    result: dict[str, int] = {}
    for name in METRICS:
        if after[name] < before[name]:
            raise CORE.PlanError(f"sampling-output-sync counter decreased: {METRICS[name]}")
        result[name] = after[name] - before[name]
    return result


def validate_condition_delta(condition: str, delta: dict[str, int]) -> None:
    reused = delta["reused_barriers"]
    if condition == "off" and reused != 0:
        raise CORE.PlanError("sampling-output-sync OFF reused-barrier delta must equal zero")
    if condition == "on" and reused <= 0:
        raise CORE.PlanError("sampling-output-sync ON reused-barrier delta must be positive")
    for name in ("output_epochs", "completed_barriers", "graph_submissions", "output_transfers"):
        if delta[name] <= 0:
            raise CORE.PlanError(f"sampling-output-sync {name} delta must be positive")


def validate_pair_deltas(off: dict[str, int], on: dict[str, int]) -> None:
    if on["completed_barriers"] >= off["completed_barriers"]:
        raise CORE.PlanError("sampling-output-sync ON completed-barrier delta is not lower than OFF")
    off_decisions = off["completed_barriers"] + off["reused_barriers"]
    on_decisions = on["completed_barriers"] + on["reused_barriers"]
    if off_decisions != on_decisions:
        raise CORE.PlanError(
            "sampling-output-sync OFF/ON total synchronization-decision deltas differ")
    for name in ("output_epochs", "graph_submissions", "output_transfers"):
        if on[name] != off[name]:
            raise CORE.PlanError(f"sampling-output-sync OFF/ON {name} deltas differ")


def _artifact(path: Path) -> dict[str, Any]:
    return {
        "path": path.name,
        "sha256": CORE.digest_file(path),
        "size_bytes": path.stat().st_size,
    }


def build_sample_summary(
    core_plan: dict[str, Any],
    side_plan: dict[str, Any],
    pair_id: int,
    condition: str,
    order_index: int,
    before_path: Path,
    after_path: Path,
    capture_path: Path,
    response_sha256: str,
    client_sha256: str,
) -> dict[str, Any]:
    coordinator_args = CORE.condition_commands(core_plan, condition)["coordinator"]
    port = int(_canonical_pair_value(
        coordinator_args, "--port", f"generated {condition} coordinator argv"), 10)
    endpoint_identity = {
        "scheme": "http",
        "host": core_plan["topology"]["coordinator"]["host"],
        "port": port,
        "metrics_path": side_plan["endpoint"],
        "completion_path": side_plan["completion_endpoint"],
    }
    before_bytes = CORE.read_regular_bytes(before_path, "sampling-output-sync before metrics")
    after_bytes = CORE.read_regular_bytes(after_path, "sampling-output-sync after metrics")
    before = parse_prometheus_snapshot(before_bytes, "sampling-output-sync before metrics")
    after = parse_prometheus_snapshot(after_bytes, "sampling-output-sync after metrics")
    capture = parse_capture_receipt(
        CORE.read_json(capture_path), condition, core_plan["request"]["sha256"],
        CORE.digest_bytes(before_bytes), CORE.digest_bytes(after_bytes),
        response_sha256, client_sha256, endpoint_identity)
    delta = counter_delta(before, after)
    validate_condition_delta(condition, delta)
    serialized_counters = {
        stage: {name: str(value) for name, value in values.items()}
        for stage, values in (("before", before), ("after", after), ("single_process_window_delta", delta))
    }
    return {
        "schema": SAMPLE_SCHEMA,
        "contract": CONTRACT,
        "core_plan_sha256": CORE.plan_digest(core_plan),
        "observability_plan_sha256": CORE.digest_bytes(CORE.canonical_bytes(side_plan)),
        "pair_id": pair_id,
        "order_index": order_index,
        "condition": condition,
        "identity": capture["identity"],
        "capture": capture,
        "counters": serialized_counters,
        "raw": {
            "before": _artifact(before_path),
            "after": _artifact(after_path),
            "capture": _artifact(capture_path),
        },
    }


def _load_frozen_side_plan(root: Path, core_plan: dict[str, Any]) -> dict[str, Any] | None:
    path = root / PLAN_FILENAME
    CORE.reject_symlink_components(path, "sampling-output-sync plan")
    try:
        if not path.exists():
            return None
        if not path.is_file():
            raise CORE.PlanError(f"sampling-output-sync plan is not a regular file: {path}")
    except OSError as exc:
        raise CORE.PlanError(f"cannot inspect sampling-output-sync plan: {path}: {exc}") from exc
    return load_observability_plan(path, core_plan)


def _inspect_analysis_destination(root: Path) -> tuple[Path, bool]:
    path = root / ANALYSIS_FILENAME
    CORE.reject_symlink_components(path, "sampling-output-sync analysis")
    try:
        exists = path.exists()
        if exists and not path.is_file():
            raise CORE.PlanError(f"sampling-output-sync analysis is not a regular file: {path}")
    except OSError as exc:
        raise CORE.PlanError(f"cannot inspect sampling-output-sync analysis: {path}: {exc}") from exc
    return path, exists


def record_observation(
    root: Path,
    pair_id: int,
    condition: str,
    order_index: int,
    before_path: Path,
    after_path: Path,
    capture_path: Path,
) -> dict[str, Any]:
    core_plan = CORE.load_plan(root / "plan.json")
    CORE.expected_schedule_entry(root, core_plan, pair_id, condition, order_index)
    side_plan = _load_frozen_side_plan(root, core_plan)
    if side_plan is None or not side_plan["enabled"]:
        raise CORE.PlanError(f"{CONTRACT} is not enabled for this run")
    sample_dir = root / "raw" / f"pair-{pair_id:03d}-order-{order_index}-{condition}"
    sample_path = sample_dir / "sample.json"
    if not sample_path.is_file():
        raise CORE.PlanError("record the ordinary successful A/B sample before its observability sidecar")
    sample = CORE.require_mapping(CORE.read_json(sample_path), "sample")
    if sample.get("status") != "success" or sample.get("pair_id") != pair_id or \
            sample.get("order_index") != order_index or sample.get("condition") != condition or \
            sample.get("plan_sha256") != CORE.plan_digest(core_plan):
        raise CORE.PlanError("sampling-output-sync sidecar does not match a successful core sample")
    CORE.validate_raw_evidence(sample_path, sample, core_plan)
    raw = CORE.require_mapping(sample["raw"], "sample.raw")
    response_sha256 = CORE.require_hash(raw["response"]["sha256"], "sample.raw.response.sha256")
    client_sha256 = CORE.require_hash(raw["client"]["sha256"], "sample.raw.client.sha256")
    destination = sample_dir / EVIDENCE_DIRECTORY
    if destination.exists():
        raise CORE.PlanError(f"sampling-output-sync evidence already exists: {destination}")
    staging = Path(tempfile.mkdtemp(prefix=".sampling-output-sync-", dir=sample_dir))
    try:
        targets = {
            "before": staging / "before.prom",
            "after": staging / "after.prom",
            "capture": staging / "capture.json",
        }
        for name, source in (("before", before_path), ("after", after_path), ("capture", capture_path)):
            if not source.is_file():
                raise CORE.PlanError(f"sampling-output-sync {name} evidence is not a file: {source}")
            content = CORE.read_regular_bytes(source, f"sampling-output-sync {name} source")
            CORE.write_bytes_exclusive(targets[name], content)
        summary = build_sample_summary(
            core_plan, side_plan, pair_id, condition, order_index,
            targets["before"], targets["after"], targets["capture"],
            response_sha256, client_sha256)
        CORE.write_json(staging / "summary.json", summary)
        staging.rename(destination)
        return summary
    except Exception:
        if staging.exists():
            shutil.rmtree(staging)
        raise


def _validate_retained_summary(
    root: Path,
    core_plan: dict[str, Any],
    side_plan: dict[str, Any],
    sample: dict[str, Any],
) -> dict[str, Any]:
    sample_dir = root / "raw" / (
        f"pair-{sample['pair_id']:03d}-order-{sample['order_index']}-{sample['condition']}")
    evidence_dir = sample_dir / EVIDENCE_DIRECTORY
    summary_path = evidence_dir / "summary.json"
    try:
        CORE.reject_symlink_components(evidence_dir, "sampling-output-sync evidence directory")
        entries = {entry.name: entry for entry in evidence_dir.iterdir()}
    except OSError as exc:
        raise CORE.PlanError(f"cannot inspect {CONTRACT} evidence: {evidence_dir}: {exc}") from exc
    expected_entries = set(EVIDENCE_VALIDATOR_SAMPLE_FILES)
    if set(entries) != expected_entries:
        raise CORE.PlanError(
            f"{CONTRACT} evidence must contain exactly: {', '.join(sorted(expected_entries))}")
    for name, entry in entries.items():
        CORE.reject_symlink_components(entry, f"sampling-output-sync evidence {name}")
        if not entry.is_file():
            raise CORE.PlanError(f"sampling-output-sync evidence is not a regular file: {entry}")
    observed = CORE.read_json(summary_path)
    expected = build_sample_summary(
        core_plan, side_plan, sample["pair_id"], sample["condition"], sample["order_index"],
        evidence_dir / "before.prom", evidence_dir / "after.prom", evidence_dir / "capture.json",
        sample["raw"]["response"]["sha256"], sample["raw"]["client"]["sha256"])
    if observed != expected:
        raise CORE.PlanError("sampling-output-sync summary differs from reparsed raw evidence")
    return expected


def validate_frozen_run(root: Path, core_plan: dict[str, Any] | None = None) -> dict[str, Any] | None:
    core_plan = core_plan or CORE.load_plan(root / "plan.json")
    schedule = CORE.validate_run_contract(root, core_plan)
    analysis_path, analysis_exists = _inspect_analysis_destination(root)
    side_plan = _load_frozen_side_plan(root, core_plan)
    evidence_dirs = set((root / "raw").glob(f"**/{EVIDENCE_DIRECTORY}"))
    if side_plan is None:
        if evidence_dirs:
            raise CORE.PlanError("sampling-output-sync evidence exists without a frozen sidecar plan")
        if analysis_exists:
            raise CORE.PlanError("sampling-output-sync analysis exists without a frozen sidecar plan")
        return None
    if not side_plan["enabled"]:
        if evidence_dirs:
            raise CORE.PlanError("disabled sampling-output-sync plan cannot retain counter evidence")
        report = {
            "schema": ANALYSIS_SCHEMA,
            "contract": CONTRACT,
            "core_plan_sha256": CORE.plan_digest(core_plan),
            "observability_plan_sha256": CORE.digest_bytes(CORE.canonical_bytes(side_plan)),
            "enabled": False,
            "evidence_complete": False,
            "pairs": [],
        }
        _inspect_analysis_destination(root)
        CORE.write_json(analysis_path, report)
        return report

    samples_by_key: dict[tuple[int, int, str], tuple[Path, dict[str, Any]]] = {}
    content_hashes: set[str] = set()
    for sample_path in sorted((root / "raw").glob("*/sample.json")):
        sample = CORE.require_mapping(CORE.read_json(sample_path), "sample")
        CORE.require_keys(
            sample,
            {
                "schema", "experiment_id", "plan_sha256", "pair_id", "order_index",
                "condition", "status", "failure_code", "identity", "client", "result", "raw",
            },
            set(),
            "sample",
        )
        if sample["schema"] != CORE.SAMPLE_SCHEMA or \
                sample["experiment_id"] != core_plan["experiment_id"] or \
                sample["plan_sha256"] != CORE.plan_digest(core_plan):
            raise CORE.PlanError("sampling-output-sync core sample identity mismatch")
        CORE.require_int(sample["pair_id"], "sample.pair_id", 1)
        order_index = CORE.require_int(sample["order_index"], "sample.order_index", 0)
        if order_index not in {0, 1}:
            raise CORE.PlanError("sampling-output-sync core sample order index is invalid")
        if sample["status"] not in {"success", "failure"}:
            raise CORE.PlanError("sampling-output-sync core sample status is invalid")
        if sample["status"] == "success" and sample["failure_code"] is not None:
            raise CORE.PlanError("successful sampling-output-sync core sample has a failure code")
        if sample["status"] == "failure":
            CORE.require_string(sample["failure_code"], "sample.failure_code")
        key = (sample.get("pair_id"), sample.get("order_index"), sample.get("condition"))
        if key in samples_by_key:
            raise CORE.PlanError("sampling-output-sync analysis found duplicate core samples")
        if sample.get("condition") not in {"off", "on"}:
            raise CORE.PlanError("sampling-output-sync core sample condition is invalid")
        expected_identity = {
            "source_commit": core_plan["conditions"][sample["condition"]]["source_commit"],
            "coordinator_binary_sha256": core_plan["conditions"][sample["condition"]]["coordinator_binary"]["sha256"],
            "worker_binary_sha256": core_plan["conditions"][sample["condition"]]["worker_binary"]["sha256"],
            "model_sha256": core_plan["model"]["sha256"],
            "request_sha256": core_plan["request"]["sha256"],
            "commands_sha256": CORE.digest_bytes(CORE.canonical_bytes(
                CORE.condition_commands(core_plan, sample["condition"]))),
        }
        if sample.get("identity") != expected_identity:
            raise CORE.PlanError("sampling-output-sync core sample artifact or command identity mismatch")
        CORE.validate_raw_evidence(sample_path, sample, core_plan)
        if sample.get("status") == "success":
            content_hashes.add(sample["result"]["content_sha256"])
        samples_by_key[key] = (sample_path, sample)
    expected_keys = {
        (entry["pair_id"], entry["order_index"], entry["condition"])
        for entry in schedule["entries"]
    }
    if set(samples_by_key) != expected_keys:
        raise CORE.PlanError("sampling-output-sync analysis requires every scheduled core sample")
    expected_evidence_dirs = {
        root / "raw" / f"pair-{pair_id:03d}-order-{order_index}-{condition}" / EVIDENCE_DIRECTORY
        for pair_id, order_index, condition in expected_keys
    }
    if evidence_dirs != expected_evidence_dirs:
        raise CORE.PlanError(
            "sampling-output-sync evidence directories differ from the frozen schedule")
    if len(content_hashes) != 1:
        raise CORE.PlanError("sampling-output-sync analysis requires deterministic output parity")
    summaries: list[dict[str, Any]] = []
    for key in sorted(expected_keys):
        _, sample = samples_by_key[key]
        if sample.get("status") != "success":
            raise CORE.PlanError("sampling-output-sync analysis requires successful scheduled samples")
        summaries.append(_validate_retained_summary(root, core_plan, side_plan, sample))

    by_pair: dict[int, dict[str, dict[str, Any]]] = {}
    for summary in summaries:
        pair = by_pair.setdefault(summary["pair_id"], {})
        if summary["condition"] in pair:
            raise CORE.PlanError("sampling-output-sync analysis found a duplicate pair condition")
        pair[summary["condition"]] = summary
    pairs: list[dict[str, Any]] = []
    for pair_id in sorted(by_pair):
        pair = by_pair[pair_id]
        if set(pair) != {"off", "on"}:
            raise CORE.PlanError("sampling-output-sync analysis requires one OFF and one ON sample per pair")
        off = {
            name: int(value, 10)
            for name, value in pair["off"]["counters"]["single_process_window_delta"].items()}
        on = {
            name: int(value, 10)
            for name, value in pair["on"]["counters"]["single_process_window_delta"].items()}
        validate_pair_deltas(off, on)
        pairs.append({
            "pair_id": pair_id,
            "off_single_process_window_delta": {
                name: str(value) for name, value in off.items()},
            "on_single_process_window_delta": {
                name: str(value) for name, value in on.items()},
            "completed_barriers_reduced_by": str(
                off["completed_barriers"] - on["completed_barriers"]),
        })
    report = {
        "schema": ANALYSIS_SCHEMA,
        "contract": CONTRACT,
        "core_plan_sha256": CORE.plan_digest(core_plan),
        "observability_plan_sha256": CORE.digest_bytes(CORE.canonical_bytes(side_plan)),
        "enabled": True,
        "evidence_complete": True,
        "pairs": pairs,
    }
    _inspect_analysis_destination(root)
    CORE.write_json(analysis_path, report)
    return report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate = subparsers.add_parser("validate-plan", help="validate a sidecar plan against a core plan")
    validate.add_argument("core_plan", type=Path)
    validate.add_argument("observability_plan", type=Path)
    freeze = subparsers.add_parser("freeze", help="freeze a sidecar plan before recording samples")
    freeze.add_argument("run_root", type=Path)
    freeze.add_argument("observability_plan", type=Path)
    record = subparsers.add_parser("record", help="import one raw before/request/after counter receipt")
    record.add_argument("run_root", type=Path)
    record.add_argument("--pair", type=int, required=True)
    record.add_argument("--condition", choices=("off", "on"), required=True)
    record.add_argument("--order-index", type=int, choices=(0, 1), required=True)
    record.add_argument("--before", type=Path, required=True)
    record.add_argument("--after", type=Path, required=True)
    record.add_argument("--capture", type=Path, required=True)
    analyze = subparsers.add_parser("analyze", help="validate all sidecar evidence and pair gates")
    analyze.add_argument("run_root", type=Path)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        if args.command == "validate-plan":
            core_plan = CORE.load_plan(args.core_plan)
            plan = load_observability_plan(args.observability_plan, core_plan)
            print(json.dumps({
                "schema": plan["schema"], "lane": CONTRACT, "enabled": plan["enabled"],
                "core_plan_sha256": CORE.plan_digest(core_plan)}, sort_keys=True))
        elif args.command == "freeze":
            plan = freeze_observability_plan(args.run_root, args.observability_plan)
            print(json.dumps({"lane": CONTRACT, "enabled": plan["enabled"]}, sort_keys=True))
        elif args.command == "record":
            summary = record_observation(
                args.run_root, args.pair, args.condition, args.order_index,
                args.before, args.after, args.capture)
            print(json.dumps({
                "lane": CONTRACT, "pair_id": summary["pair_id"],
                "condition": summary["condition"],
                "single_process_window_delta": summary["counters"]["single_process_window_delta"]},
                sort_keys=True))
        elif args.command == "analyze":
            report = validate_frozen_run(args.run_root)
            if report is None:
                print(json.dumps({"lane": CONTRACT, "enabled": False, "frozen": False}, sort_keys=True))
            else:
                print(json.dumps(report, indent=2, sort_keys=True))
    except CORE.PlanError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
