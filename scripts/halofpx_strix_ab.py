#!/usr/bin/env python3
"""Small fail-closed A/B evidence harness for the two HaloFPX target nodes.

This tool deliberately does not own production services or infer that a command
was executed.  It freezes an exact plan and schedule, verifies artifacts on
each node, imports raw request evidence, and performs paired analysis.  A run
adapter may execute the frozen argv arrays, but only imported raw evidence is
analyzed.
"""

from __future__ import annotations

import argparse
import base64
import datetime as dt
import decimal
import hashlib
import importlib.util
import json
import math
import os
import platform
import random
import re
import shutil
import socket
import stat
import statistics
import sys
import tempfile
from pathlib import Path, PurePosixPath, PureWindowsPath
from typing import Any


PLAN_SCHEMA_V1 = "halofpx.strix-ab-plan.v1"
PLAN_SCHEMA_V2 = "halofpx.strix-ab-plan.v2"
# Keep the historical public alias pinned to v1. Existing callers and fixtures
# use it when constructing v1 plans, and initialized v1 runs are never migrated.
PLAN_SCHEMA = PLAN_SCHEMA_V1
PREFLIGHT_SCHEMA = "halofpx.strix-ab-preflight.v2"
SAMPLE_SCHEMA = "halofpx.strix-ab-sample.v1"
ANALYSIS_SCHEMA_V1 = "halofpx.strix-ab-analysis.v1"
ANALYSIS_SCHEMA_V2 = "halofpx.strix-ab-analysis.v2"
ANALYSIS_SCHEMA = ANALYSIS_SCHEMA_V1
CLIENT_SCHEMA = "halofpx.client-timing.v1"
CLIENT_SCHEMA_V2 = "halofpx.client-timing.v2"
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
COMMIT_RE = re.compile(r"^[0-9a-f]{40}$")
ID_RE = re.compile(r"^[a-z0-9][a-z0-9._-]{2,63}$")
ENV_RE = re.compile(r"^[A-Z_][A-Z0-9_]*$")
MAX_PAIRS = 64
MAX_WARMUPS_PER_CONDITION = 16
MAX_OUTPUT_TOKENS = 65_536
MAX_PROMPT_TOKENS = 1_048_576
MAX_CONTEXT = 1_114_112
MAX_CYCLE_OUTPUT_TOKEN_RECORDS = 262_144
MAX_JSON_NUMBER_CHARS = 128


class PlanError(ValueError):
    pass


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")


def digest_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def reject_symlink_components(path: Path, where: str) -> None:
    absolute = path.absolute()
    for component in reversed((absolute, *absolute.parents)):
        try:
            if component.is_symlink():
                raise PlanError(f"{where} has a symbolic-link path component: {component}")
        except OSError as exc:
            raise PlanError(f"cannot inspect {where} path component: {component}: {exc}") from exc


def read_regular_bytes(path: Path, where: str) -> bytes:
    reject_symlink_components(path, where)
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0) | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags)
        try:
            before = os.fstat(descriptor)
            if not stat.S_ISREG(before.st_mode):
                raise PlanError(f"{where} is not a regular file: {path}")
            with os.fdopen(descriptor, "rb", closefd=False) as handle:
                content = handle.read()
            after = os.fstat(descriptor)
            if (before.st_dev, before.st_ino, before.st_size) != (after.st_dev, after.st_ino, after.st_size) or \
                    len(content) != after.st_size:
                raise PlanError(f"{where} changed while it was read: {path}")
            return content
        finally:
            os.close(descriptor)
    except PlanError:
        raise
    except OSError as exc:
        raise PlanError(f"cannot read regular {where}: {path}: {exc}") from exc


def regular_file_identity(path: Path, where: str = "hashed artifact") -> tuple[str, int]:
    reject_symlink_components(path, where)
    hasher = hashlib.sha256()
    flags = os.O_RDONLY | getattr(os, "O_BINARY", 0) | getattr(os, "O_NOFOLLOW", 0)
    descriptor = os.open(path, flags)
    try:
        before = os.fstat(descriptor)
        if not stat.S_ISREG(before.st_mode):
            raise PlanError(f"{where} is not a regular file: {path}")
        with os.fdopen(descriptor, "rb", closefd=False) as handle:
            for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                hasher.update(chunk)
        after = os.fstat(descriptor)
        if (before.st_dev, before.st_ino, before.st_size) != (after.st_dev, after.st_ino, after.st_size):
            raise PlanError(f"{where} changed while it was read: {path}")
    finally:
        os.close(descriptor)
    return hasher.hexdigest(), after.st_size


def digest_file(path: Path) -> str:
    return regular_file_identity(path)[0]


def reject_json_constant(value: str) -> None:
    raise ValueError(f"non-finite JSON number {value}")


def strict_json_int(value: str) -> int:
    if len(value) > MAX_JSON_NUMBER_CHARS:
        raise ValueError("JSON integer exceeds the bounded numeric token length")
    return int(value, 10)


def strict_json_float(value: str) -> float:
    if len(value) > MAX_JSON_NUMBER_CHARS:
        raise ValueError("JSON float exceeds the bounded numeric token length")
    try:
        exact = decimal.Decimal(value)
        result = float(exact)
    except (decimal.InvalidOperation, OverflowError) as exc:
        raise ValueError("JSON float is not a finite bounded decimal") from exc
    if not exact.is_finite() or not math.isfinite(result) or (exact != 0 and result == 0):
        raise ValueError("JSON float is not a finite representable decimal")
    return result


def unique_json_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key {key!r}")
        result[key] = value
    return result


def validate_completion_request(raw: bytes, plan: dict[str, Any]) -> dict[str, Any]:
    """Validate the exact deterministic cache-off request bytes.

    The adapter sends these same bytes.  Keeping this check byte-oriented avoids
    accepting one file at preflight and a reserialized or replaced body later.
    """
    try:
        request = json.loads(
            raw.decode("utf-8", errors="strict"),
            object_pairs_hook=unique_json_object,
            parse_constant=reject_json_constant,
            parse_int=strict_json_int,
            parse_float=strict_json_float,
        )
    except (UnicodeError, json.JSONDecodeError, ValueError, RecursionError, MemoryError) as exc:
        raise PlanError(f"request is not strict duplicate-free UTF-8 JSON: {exc}") from exc
    request = require_mapping(request, "request")
    required = {
        "prompt", "n_predict", "stream", "cache_prompt", "seed", "temperature", "ignore_eos",
    }
    require_keys(request, required, set(), "request")
    if not isinstance(request["prompt"], str) or not request["prompt"] or "\x00" in request["prompt"]:
        raise PlanError("request.prompt must be a nonempty string without NUL")
    n_predict = request["n_predict"]
    if isinstance(n_predict, bool) or not isinstance(n_predict, int) or n_predict != plan["request"]["output_tokens"]:
        raise PlanError("request.n_predict must be the planned output-token integer")
    if request["stream"] is not True:
        raise PlanError("request.stream must be true")
    if request["cache_prompt"] is not False:
        raise PlanError("request.cache_prompt must be false")
    if request["ignore_eos"] is not True:
        raise PlanError("request.ignore_eos must be true for fixed-length measurement")
    seed = request["seed"]
    if isinstance(seed, bool) or not isinstance(seed, int) or not 0 <= seed < 0xffffffff:
        raise PlanError("request.seed must be a fixed integer in [0, 0xffffffff)")
    temperature = request["temperature"]
    if isinstance(temperature, bool) or not isinstance(temperature, (int, float)) or \
            not math.isfinite(temperature) or float(temperature) != 0.0:
        raise PlanError("request.temperature must be finite zero")
    return request


def parse_json_bytes(content: bytes, where: str) -> Any:
    try:
        return json.loads(
            content.decode("utf-8", errors="strict"),
            object_pairs_hook=unique_json_object,
            parse_constant=reject_json_constant,
            parse_int=strict_json_int,
            parse_float=strict_json_float,
        )
    except (UnicodeError, json.JSONDecodeError, ValueError, RecursionError, MemoryError) as exc:
        raise PlanError(f"cannot parse {where}: {exc}") from exc


def read_json(path: Path) -> Any:
    return parse_json_bytes(read_regular_bytes(path, "JSON document"), str(path))


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def require_mapping(value: Any, where: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise PlanError(f"{where} must be an object")
    return value


def require_keys(value: dict[str, Any], required: set[str], optional: set[str], where: str) -> None:
    missing = required - value.keys()
    unknown = value.keys() - required - optional
    if missing:
        raise PlanError(f"{where} is missing: {', '.join(sorted(missing))}")
    if unknown:
        raise PlanError(f"{where} has unknown keys: {', '.join(sorted(unknown))}")


def require_string(value: Any, where: str) -> str:
    if not isinstance(value, str) or not value:
        raise PlanError(f"{where} must be a non-empty string")
    if any(ord(char) < 32 for char in value):
        raise PlanError(f"{where} contains a control character")
    return value


def require_int(value: Any, where: str, minimum: int = 0) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < minimum:
        raise PlanError(f"{where} must be an integer >= {minimum}")
    return value


def admit_resource_profile(plan: Any) -> dict[str, int]:
    """Fail before schedule expansion when the closed PR51 profile is too large."""
    document = require_mapping(plan, "plan")
    execution = require_mapping(document.get("execution"), "plan.execution")
    request = require_mapping(document.get("request"), "plan.request")
    runtime = require_mapping(document.get("runtime"), "plan.runtime")
    pairs = require_int(execution.get("pairs"), "plan.execution.pairs", 1)
    warmups = require_int(
        execution.get("warmups_per_condition"),
        "plan.execution.warmups_per_condition", 1)
    retained = require_int(
        execution.get("retained_per_condition_per_pair"),
        "plan.execution.retained_per_condition_per_pair", 1)
    prompt_tokens = require_int(
        request.get("prompt_tokens"), "plan.request.prompt_tokens", 1)
    output_tokens = require_int(
        request.get("output_tokens"), "plan.request.output_tokens", 2)
    context = require_int(runtime.get("context"), "plan.runtime.context", 1)
    if pairs > MAX_PAIRS or warmups > MAX_WARMUPS_PER_CONDITION or retained != 1:
        raise PlanError("plan exceeds bounded pairs/warmups/retained cardinality")
    if prompt_tokens > MAX_PROMPT_TOKENS:
        raise PlanError("plan.request.prompt_tokens exceeds its bounded maximum")
    if output_tokens > MAX_OUTPUT_TOKENS:
        raise PlanError("plan.request.output_tokens exceeds its bounded maximum")
    if context > MAX_CONTEXT:
        raise PlanError("plan.runtime.context exceeds its bounded maximum")
    schedule_entries = pairs * 2 * retained
    cycle_output_token_records = schedule_entries * (warmups + 1) * output_tokens
    if cycle_output_token_records > MAX_CYCLE_OUTPUT_TOKEN_RECORDS:
        raise PlanError(
            "plan exceeds the coupled retained cycle/output-token record bound")
    return {
        "schedule_entries": schedule_entries,
        "cycle_output_token_records": cycle_output_token_records,
    }


def require_hash(value: Any, where: str, pattern: re.Pattern[str] = SHA256_RE) -> str:
    text = require_string(value, where)
    if not pattern.fullmatch(text):
        raise PlanError(f"{where} has the wrong identity format")
    return text


def require_argv(value: Any, where: str, *, allow_empty: bool = False) -> list[str]:
    if not isinstance(value, list) or (not allow_empty and not value):
        qualifier = "an argv array" if allow_empty else "a non-empty argv array"
        raise PlanError(f"{where} must be {qualifier}")
    return [require_string(item, f"{where}[{index}]") for index, item in enumerate(value)]


def is_canonical_absolute_path(value: str) -> bool:
    """Accept canonical absolute paths for the host preparing a plan.

    The evidence core is intentionally usable for offline fixture preparation
    on Windows as well as on the CachyOS targets. The target-only execution
    adapter applies the stricter POSIX-path gate before it can launch anything.
    """
    posix = PurePosixPath(value)
    if posix.is_absolute() and posix.as_posix() == value and not any(
            part in {".", ".."} for part in posix.parts):
        return True
    windows = PureWindowsPath(value)
    return windows.is_absolute() and str(windows) == value and not any(
        part in {".", ".."} for part in windows.parts)


def validate_artifact(value: Any, where: str) -> dict[str, Any]:
    artifact = require_mapping(value, where)
    require_keys(artifact, {"path", "sha256"}, set(), where)
    path = require_string(artifact["path"], f"{where}.path")
    if not is_canonical_absolute_path(path):
        raise PlanError(f"{where}.path must be absolute")
    require_hash(artifact["sha256"], f"{where}.sha256")
    return artifact


def validate_node(value: Any, where: str) -> dict[str, Any]:
    node = require_mapping(value, where)
    require_keys(node, {"host", "device", "authority_receipt"}, set(), where)
    require_string(node["host"], f"{where}.host")
    require_string(node["device"], f"{where}.device")
    validate_artifact(node["authority_receipt"], f"{where}.authority_receipt")
    return node


def condition_commands(plan: dict[str, Any], name: str) -> dict[str, list[str]]:
    condition = plan["conditions"][name]
    runtime = plan["runtime"]
    generated_coordinator_args: list[str] = []
    if plan["schema"] == PLAN_SCHEMA_V2:
        # In v2 the condition batch is a typed plan field. It is rendered here
        # exactly once so raw argv cannot silently select a different A/B.
        generated_coordinator_args = ["--batch-size", str(runtime["batch_by_condition"][name])]
    return {
        "worker": [condition["worker_binary"]["path"]]
        + runtime["common_worker_args"]
        + condition["worker_args"],
        "coordinator": [condition["coordinator_binary"]["path"]]
        + runtime["common_coordinator_args"]
        + generated_coordinator_args
        + condition["coordinator_args"],
    }


def require_argv_option(args: list[str], flag: str, expected: str, where: str) -> None:
    matches = []
    for index, item in enumerate(args):
        if item == flag:
            if index + 1 >= len(args):
                raise PlanError(f"{where} ends with {flag} but has no value")
            matches.append(args[index + 1])
        elif item.startswith(flag + "="):
            matches.append(item.split("=", 1)[1])
    if matches != [expected]:
        raise PlanError(f"{where} must contain exactly one {flag} value equal to {expected!r}")


def require_argv_aliases_absent(args: list[str], aliases: set[str], where: str) -> None:
    for item in args:
        option = item.split("=", 1)[0]
        if option.startswith("--"):
            # llama.cpp normalizes underscores in long options before lookup.
            option = option.replace("_", "-")
        if option in aliases:
            raise PlanError(
                f"{where} must omit {item.split('=', 1)[0]}; plan v2 generates the typed condition value")


def require_canonical_argv_option(args: list[str], flag: str, expected: str, where: str) -> None:
    matches = []
    for index, item in enumerate(args):
        option = item.split("=", 1)[0]
        if option.startswith("--"):
            option = option.replace("_", "-")
        if option == flag:
            matches.append(index)
    if len(matches) != 1 or args[matches[0]] != flag or \
            matches[0] + 1 >= len(args) or args[matches[0] + 1] != expected:
        raise PlanError(
            f"{where} must contain one canonical {flag} {expected} pair")


def validate_plan(value: Any) -> dict[str, Any]:
    plan = require_mapping(value, "plan")
    schema = plan.get("schema")
    if schema not in {PLAN_SCHEMA_V1, PLAN_SCHEMA_V2}:
        raise PlanError(
            f"plan.schema must be {PLAN_SCHEMA_V1} or {PLAN_SCHEMA_V2}")
    required_plan_keys = {
        "schema", "experiment_id", "issues", "source", "model", "request",
        "topology", "runtime", "execution", "conditions",
    }
    if schema == PLAN_SCHEMA_V2:
        required_plan_keys.add("comparison")
    require_keys(
        plan,
        required_plan_keys,
        {"notes"},
        "plan",
    )
    if not ID_RE.fullmatch(require_string(plan["experiment_id"], "plan.experiment_id")):
        raise PlanError("plan.experiment_id must be a safe lowercase identifier")
    issues = plan["issues"]
    if not isinstance(issues, list) or any(
            isinstance(item, bool) or not isinstance(item, int) for item in issues) or \
            not {15, 16}.issubset(set(issues)):
        raise PlanError("plan.issues must include GitHub issues 15 and 16")

    source = require_mapping(plan["source"], "plan.source")
    require_keys(source, {"repository", "off_commit", "on_commit"}, set(), "plan.source")
    require_string(source["repository"], "plan.source.repository")
    require_hash(source["off_commit"], "plan.source.off_commit", COMMIT_RE)
    require_hash(source["on_commit"], "plan.source.on_commit", COMMIT_RE)

    comparison: dict[str, Any] | None = None
    if schema == PLAN_SCHEMA_V2:
        comparison = require_mapping(plan["comparison"], "plan.comparison")
        require_keys(comparison, {"kind", "control", "candidate"}, set(), "plan.comparison")
        if comparison["kind"] not in {"runtime_n_batch", "feature_build"}:
            raise PlanError("plan.comparison.kind must be runtime_n_batch or feature_build")
        if comparison["control"] != "off" or comparison["candidate"] != "on":
            raise PlanError("plan.comparison must bind control=off and candidate=on")

    model = require_mapping(plan["model"], "plan.model")
    require_keys(
        model,
        {"path", "sha256", "size_bytes", "format_family", "architecture"},
        {"provenance"},
        "plan.model",
    )
    model_path = require_string(model["path"], "plan.model.path")
    if not is_canonical_absolute_path(model_path):
        raise PlanError("plan.model.path must be absolute")
    require_hash(model["sha256"], "plan.model.sha256")
    require_int(model["size_bytes"], "plan.model.size_bytes", 1)
    if model["format_family"] not in {"rocmfpx", "rocmfp4", "conventional-control"}:
        raise PlanError("plan.model.format_family must declare ROCmFPX, ROCmFP4, or a control")
    require_string(model["architecture"], "plan.model.architecture")

    request = require_mapping(plan["request"], "plan.request")
    require_keys(
        request,
        {"path", "sha256", "prompt_tokens", "output_tokens", "require_content_parity"},
        {"expected_content_sha256", "tokenizer_sha256"},
        "plan.request",
    )
    request_path = require_string(request["path"], "plan.request.path")
    if not is_canonical_absolute_path(request_path):
        raise PlanError("plan.request.path must be absolute")
    require_hash(request["sha256"], "plan.request.sha256")
    require_int(request["prompt_tokens"], "plan.request.prompt_tokens", 1)
    require_int(request["output_tokens"], "plan.request.output_tokens", 2)
    if request["require_content_parity"] is not True:
        raise PlanError("plan.request.require_content_parity must be true for A/B qualification")
    if request.get("expected_content_sha256") is not None:
        require_hash(request["expected_content_sha256"], "plan.request.expected_content_sha256")
    if request.get("tokenizer_sha256") is not None:
        require_hash(request["tokenizer_sha256"], "plan.request.tokenizer_sha256")

    topology = require_mapping(plan["topology"], "plan.topology")
    require_keys(topology, {"world_size", "rpc_endpoint", "coordinator", "worker"}, set(), "plan.topology")
    if topology["world_size"] != 2:
        raise PlanError("plan.topology.world_size must be exactly 2")
    require_string(topology["rpc_endpoint"], "plan.topology.rpc_endpoint")
    coordinator = validate_node(topology["coordinator"], "plan.topology.coordinator")
    worker = validate_node(topology["worker"], "plan.topology.worker")
    if coordinator["host"].split(".", 1)[0] == worker["host"].split(".", 1)[0]:
        raise PlanError("coordinator and worker hosts must be distinct")

    runtime = require_mapping(plan["runtime"], "plan.runtime")
    runtime_keys = {
        "lane", "cache_class", "context", "ubatch", "flash_attention",
        "kv_k", "kv_v", "common_environment", "common_worker_args",
        "common_coordinator_args",
    }
    runtime_keys.add("batch" if schema == PLAN_SCHEMA_V1 else "batch_by_condition")
    require_keys(
        runtime,
        runtime_keys,
        set(),
        "plan.runtime",
    )
    if runtime["lane"] != "cold_prompt_generation" or runtime["cache_class"] != "cold_cache_off":
        if schema == PLAN_SCHEMA_V1:
            raise PlanError("v1 qualifies only lane=cold_prompt_generation with cache_class=cold_cache_off")
        raise PlanError("v2 qualifies only lane=cold_prompt_generation with cache_class=cold_cache_off")
    context = require_int(runtime["context"], "plan.runtime.context", 1)
    ubatch = require_int(runtime["ubatch"], "plan.runtime.ubatch", 1)
    if schema == PLAN_SCHEMA_V1:
        batch_by_condition = {
            "off": require_int(runtime["batch"], "plan.runtime.batch", 1),
            "on": runtime["batch"],
        }
    else:
        raw_batches = require_mapping(
            runtime["batch_by_condition"], "plan.runtime.batch_by_condition")
        require_keys(raw_batches, {"off", "on"}, set(), "plan.runtime.batch_by_condition")
        batch_by_condition = {
            name: require_int(
                raw_batches[name], f"plan.runtime.batch_by_condition.{name}", 1)
            for name in ("off", "on")
        }
        assert comparison is not None
        expected_batches = (
            {"off": 512, "on": 2048}
            if comparison["kind"] == "runtime_n_batch"
            else {"off": 512, "on": 512}
        )
        if batch_by_condition != expected_batches or ubatch != 512:
            raise PlanError(
                f"plan v2 {comparison['kind']} requires batch_by_condition={expected_batches} and ubatch=512")
    if any(batch < ubatch for batch in batch_by_condition.values()):
        raise PlanError("every planned runtime batch must be >= ubatch")
    if context < request["prompt_tokens"] + request["output_tokens"]:
        raise PlanError("plan.runtime.context is smaller than prompt plus output tokens")
    if not isinstance(runtime["flash_attention"], bool):
        raise PlanError("plan.runtime.flash_attention must be boolean")
    require_string(runtime["kv_k"], "plan.runtime.kv_k")
    require_string(runtime["kv_v"], "plan.runtime.kv_v")
    environment = require_mapping(runtime["common_environment"], "plan.runtime.common_environment")
    for name, item in environment.items():
        if not ENV_RE.fullmatch(name) or not isinstance(item, str):
            raise PlanError("common_environment must map safe variable names to strings")
    if schema == PLAN_SCHEMA_V2 and any(
            name in environment for name in ("LLAMA_ARG_BATCH", "LLAMA_ARG_UBATCH")):
        raise PlanError(
            "plan v2 common_environment must omit LLAMA_ARG_BATCH and LLAMA_ARG_UBATCH")
    runtime["common_worker_args"] = require_argv(
        runtime["common_worker_args"], "plan.runtime.common_worker_args", allow_empty=True)
    runtime["common_coordinator_args"] = require_argv(
        runtime["common_coordinator_args"], "plan.runtime.common_coordinator_args", allow_empty=True)
    required_coordinator_options = {
        "--model": model["path"],
        "--rpc": topology["rpc_endpoint"],
        "--ctx-size": str(context),
        "--ubatch-size": str(ubatch),
        "--cache-type-k": runtime["kv_k"],
        "--cache-type-v": runtime["kv_v"],
        "--flash-attn": "on" if runtime["flash_attention"] else "off",
    }
    for flag, expected in required_coordinator_options.items():
        require_argv_option(
            runtime["common_coordinator_args"], flag, expected,
            "plan.runtime.common_coordinator_args",
        )
    if schema == PLAN_SCHEMA_V1:
        require_argv_option(
            runtime["common_coordinator_args"], "--batch-size",
            str(batch_by_condition["off"]), "plan.runtime.common_coordinator_args")
    else:
        require_argv_aliases_absent(
            runtime["common_coordinator_args"],
            {"-b", "--batch-size", "-ub"},
            "plan.runtime.common_coordinator_args")
        require_canonical_argv_option(
            runtime["common_coordinator_args"], "--ubatch-size", str(ubatch),
            "plan.runtime.common_coordinator_args")

    execution = require_mapping(plan["execution"], "plan.execution")
    require_keys(
        execution,
        {"pairs", "order_seed", "warmups_per_condition", "retained_per_condition_per_pair", "profiling_separate"},
        set(),
        "plan.execution",
    )
    require_int(execution["pairs"], "plan.execution.pairs", 1)
    require_int(execution["order_seed"], "plan.execution.order_seed", 0)
    require_int(execution["warmups_per_condition"], "plan.execution.warmups_per_condition", 1)
    retained = require_int(
        execution["retained_per_condition_per_pair"],
        "plan.execution.retained_per_condition_per_pair",
        1,
    )
    if retained != 1:
        raise PlanError("each schedule pair currently retains exactly one sample per condition")
    if execution["profiling_separate"] is not True:
        raise PlanError("profiling runs must be declared separate from timing samples")

    conditions = require_mapping(plan["conditions"], "plan.conditions")
    if set(conditions) != {"off", "on"}:
        raise PlanError("plan.conditions must contain exactly off and on")
    for name, expected_commit in (("off", source["off_commit"]), ("on", source["on_commit"])):
        condition = require_mapping(conditions[name], f"plan.conditions.{name}")
        require_keys(
            condition,
            {"source_commit", "coordinator_binary", "worker_binary", "coordinator_args", "worker_args"},
            set(),
            f"plan.conditions.{name}",
        )
        if require_hash(condition["source_commit"], f"plan.conditions.{name}.source_commit", COMMIT_RE) != expected_commit:
            raise PlanError(f"plan.conditions.{name}.source_commit differs from plan.source")
        validate_artifact(condition["coordinator_binary"], f"plan.conditions.{name}.coordinator_binary")
        validate_artifact(condition["worker_binary"], f"plan.conditions.{name}.worker_binary")
        condition["coordinator_args"] = require_argv(
            condition["coordinator_args"], f"plan.conditions.{name}.coordinator_args", allow_empty=True)
        condition["worker_args"] = require_argv(
            condition["worker_args"], f"plan.conditions.{name}.worker_args", allow_empty=True)

    for name in ("off", "on"):
        for role in ("coordinator_args", "worker_args"):
            if conditions[name][role]:
                raise PlanError(
                    f"plan.conditions.{name}.{role} must be empty; " + (
                        "v1 compares feature branches/builds with matched common controls"
                        if schema == PLAN_SCHEMA_V1
                        else "v2 conditions use only frozen common and generated controls"))

    off_fingerprint = canonical_bytes({
        "commit": conditions["off"]["source_commit"],
        "coordinator": conditions["off"]["coordinator_binary"]["sha256"],
        "worker": conditions["off"]["worker_binary"]["sha256"],
        "commands": condition_commands(plan, "off"),
    })
    on_fingerprint = canonical_bytes({
        "commit": conditions["on"]["source_commit"],
        "coordinator": conditions["on"]["coordinator_binary"]["sha256"],
        "worker": conditions["on"]["worker_binary"]["sha256"],
        "commands": condition_commands(plan, "on"),
    })
    if schema == PLAN_SCHEMA_V1:
        if off_fingerprint == on_fingerprint:
            raise PlanError("off and on conditions are identical")
    else:
        assert comparison is not None
        if comparison["kind"] == "runtime_n_batch":
            if source["off_commit"] != source["on_commit"]:
                raise PlanError("runtime_n_batch requires identical OFF and ON source commits")
            for role in ("coordinator_binary", "worker_binary"):
                if conditions["off"][role] != conditions["on"][role]:
                    raise PlanError(
                        f"runtime_n_batch requires identical OFF and ON {role} path and SHA-256")
            off_commands = condition_commands(plan, "off")
            on_commands = condition_commands(plan, "on")
            if off_commands["worker"] != on_commands["worker"]:
                raise PlanError("runtime_n_batch worker commands must be identical")
            off_coordinator = list(off_commands["coordinator"])
            on_coordinator = list(on_commands["coordinator"])
            if off_coordinator[-2:] != ["--batch-size", "512"] or \
                    on_coordinator[-2:] != ["--batch-size", "2048"] or \
                    off_coordinator[:-2] != on_coordinator[:-2]:
                raise PlanError(
                    "runtime_n_batch coordinator commands must differ only in the generated batch value")
        else:
            if all(
                    conditions["off"][role]["sha256"] == conditions["on"][role]["sha256"]
                    for role in ("coordinator_binary", "worker_binary")):
                raise PlanError("feature_build requires a distinct OFF/ON binary SHA-256")
            if off_fingerprint == on_fingerprint:
                raise PlanError("off and on feature_build conditions are identical")
    admit_resource_profile(plan)
    return plan


def load_plan(path: Path) -> dict[str, Any]:
    return validate_plan(read_json(path))


def plan_digest(plan: dict[str, Any]) -> str:
    return digest_bytes(canonical_bytes(plan))


def make_schedule(plan: dict[str, Any]) -> dict[str, Any]:
    # This defensive gate is intentional: callers may pass an already-parsed
    # mapping instead of a value returned directly by validate_plan().
    admit_resource_profile(plan)
    count = plan["execution"]["pairs"]
    first = ["off"] * ((count + 1) // 2) + ["on"] * (count // 2)
    random.Random(plan["execution"]["order_seed"]).shuffle(first)
    entries = []
    for pair_id, first_condition in enumerate(first, 1):
        second_condition = "on" if first_condition == "off" else "off"
        for order_index, condition in enumerate((first_condition, second_condition)):
            entries.append({"pair_id": pair_id, "order_index": order_index, "condition": condition})
    return {
        "schema": "halofpx.strix-ab-schedule.v1",
        "experiment_id": plan["experiment_id"],
        "plan_sha256": plan_digest(plan),
        "entries": entries,
    }


def commands_document(plan: dict[str, Any]) -> dict[str, Any]:
    return {
        "schema": "halofpx.strix-ab-commands.v1",
        "experiment_id": plan["experiment_id"],
        "plan_sha256": plan_digest(plan),
        "environment": plan["runtime"]["common_environment"],
        "conditions": {name: condition_commands(plan, name) for name in ("off", "on")},
    }


def validate_run_contract(root: Path, plan: dict[str, Any]) -> dict[str, Any]:
    schedule = read_json(root / "schedule.json")
    if schedule != make_schedule(plan):
        raise PlanError("frozen schedule differs from the validated plan")
    if read_json(root / "commands.json") != commands_document(plan):
        raise PlanError("frozen commands differ from the validated plan")
    return schedule


def init_run(plan_path: Path, root: Path) -> None:
    plan = load_plan(plan_path)
    if root.exists():
        raise PlanError(f"run root already exists: {root}")
    root.mkdir(parents=True, mode=0o700)
    (root / "raw").mkdir(mode=0o700)
    (root / "preflight").mkdir(mode=0o700)
    write_json(root / "plan.json", plan)
    write_json(root / "schedule.json", make_schedule(plan))
    write_json(root / "commands.json", commands_document(plan))
    write_json(root / "status.json", {
        "schema": "halofpx.strix-ab-status.v1",
        "experiment_id": plan["experiment_id"],
        "state": "initialized",
        "performance_claim": False,
    })


def host_matches(expected: str, observed: str) -> bool:
    return expected.lower().split(".", 1)[0] == observed.lower().split(".", 1)[0]


def checked_artifact(
    artifact: dict[str, Any], where: str, *, retain_content: bool = False,
) -> dict[str, Any]:
    path = Path(artifact["path"])
    content = read_regular_bytes(path, where) if retain_content else None
    if content is not None:
        actual, size = digest_bytes(content), len(content)
    else:
        actual, size = regular_file_identity(path, where)
    if actual != artifact["sha256"]:
        raise PlanError(f"{where} SHA-256 mismatch: {actual}")
    result = {"path": str(path), "size_bytes": size, "sha256": actual}
    if content is not None:
        result["content_base64"] = base64.b64encode(content).decode("ascii")
    return result


def decode_retained_artifact(value: Any, where: str) -> bytes:
    record = require_mapping(value, where)
    require_keys(record, {"path", "size_bytes", "sha256", "content_base64"}, set(), where)
    require_string(record["path"], f"{where}.path")
    size = require_int(record["size_bytes"], f"{where}.size_bytes", 1)
    expected = require_hash(record["sha256"], f"{where}.sha256")
    encoded = require_string(record["content_base64"], f"{where}.content_base64")
    try:
        content = base64.b64decode(encoded, validate=True)
    except (ValueError, base64.binascii.Error) as exc:
        raise PlanError(f"{where}.content_base64 is invalid") from exc
    if base64.b64encode(content).decode("ascii") != encoded:
        raise PlanError(f"{where}.content_base64 is not canonical")
    if len(content) != size or digest_bytes(content) != expected:
        raise PlanError(f"{where} retained bytes differ from size/SHA-256")
    return content


def collect_preflight(plan: dict[str, Any], role: str, observed_hostname: str | None = None) -> dict[str, Any]:
    if role not in {"coordinator", "worker"}:
        raise PlanError("preflight role must be coordinator or worker")
    observed = observed_hostname or socket.gethostname()
    node = plan["topology"][role]
    if not host_matches(node["host"], observed):
        raise PlanError(f"expected host {node['host']}, observed {observed}")
    artifacts = {
        "authority_receipt": checked_artifact(
            node["authority_receipt"], f"{role} authority receipt", retain_content=True),
    }
    binary_key = "coordinator_binary" if role == "coordinator" else "worker_binary"
    for condition in ("off", "on"):
        artifacts[f"{condition}_binary"] = checked_artifact(
            plan["conditions"][condition][binary_key], f"{role} {condition} binary")
    if role == "coordinator":
        artifacts["model"] = checked_artifact(
            {"path": plan["model"]["path"], "sha256": plan["model"]["sha256"]}, "model")
        if artifacts["model"]["size_bytes"] != plan["model"]["size_bytes"]:
            raise PlanError("model size differs from the plan")
        artifacts["request"] = checked_artifact(
            {"path": plan["request"]["path"], "sha256": plan["request"]["sha256"]},
            "request", retain_content=True)
        validate_completion_request(
            decode_retained_artifact(artifacts["request"], "preflight request"), plan)
    os_release = ""
    os_release_path = Path("/etc/os-release")
    if os_release_path.is_file():
        os_release = os_release_path.read_text(encoding="utf-8", errors="strict")
    return {
        "schema": PREFLIGHT_SCHEMA,
        "experiment_id": plan["experiment_id"],
        "plan_sha256": plan_digest(plan),
        "role": role,
        "expected_host": node["host"],
        "observed_host": observed,
        "commands": {name: condition_commands(plan, name)[role] for name in ("off", "on")},
        "artifacts": artifacts,
        "system": {
            "platform": platform.platform(),
            "uname": list(platform.uname()),
            "python": sys.version,
            "os_release": os_release,
        },
        "ok": True,
    }


def planned_role_artifacts(plan: dict[str, Any], role: str) -> dict[str, dict[str, Any]]:
    binary_key = "coordinator_binary" if role == "coordinator" else "worker_binary"
    artifacts = {
        "authority_receipt": plan["topology"][role]["authority_receipt"],
        "off_binary": plan["conditions"]["off"][binary_key],
        "on_binary": plan["conditions"]["on"][binary_key],
    }
    if role == "coordinator":
        artifacts["model"] = {
            "path": plan["model"]["path"],
            "sha256": plan["model"]["sha256"],
            "size_bytes": plan["model"]["size_bytes"],
        }
        artifacts["request"] = {
            "path": plan["request"]["path"],
            "sha256": plan["request"]["sha256"],
        }
    return artifacts


def validate_preflight_receipt(plan: dict[str, Any], receipt: dict[str, Any]) -> str:
    required = {
        "schema", "experiment_id", "plan_sha256", "role", "expected_host",
        "observed_host", "commands", "artifacts", "system", "ok",
    }
    require_keys(receipt, required, set(), "preflight receipt")
    if receipt["schema"] != PREFLIGHT_SCHEMA or receipt["experiment_id"] != plan["experiment_id"]:
        raise PlanError("preflight receipt identity mismatch")
    if receipt["plan_sha256"] != plan_digest(plan) or receipt["ok"] is not True:
        raise PlanError("preflight receipt does not approve the frozen plan")
    role = receipt["role"]
    if role not in {"coordinator", "worker"}:
        raise PlanError("preflight role is invalid")
    node = plan["topology"][role]
    if receipt["expected_host"] != node["host"] or not host_matches(node["host"], receipt["observed_host"]):
        raise PlanError("preflight role or host mismatch")
    if receipt["commands"] != {name: condition_commands(plan, name)[role] for name in ("off", "on")}:
        raise PlanError("preflight command receipt differs from the plan")
    expected_artifacts = planned_role_artifacts(plan, role)
    artifacts = require_mapping(receipt["artifacts"], "preflight receipt.artifacts")
    if set(artifacts) != set(expected_artifacts):
        raise PlanError("preflight artifact set differs from the plan")
    for name, expected in expected_artifacts.items():
        actual = require_mapping(artifacts[name], f"preflight receipt.artifacts.{name}")
        retained = name in {"authority_receipt", "request"}
        required = {"path", "size_bytes", "sha256", "content_base64"} if retained else {
            "path", "size_bytes", "sha256"}
        require_keys(actual, required, set(), f"preflight receipt.artifacts.{name}")
        require_int(actual["size_bytes"], f"preflight receipt.artifacts.{name}.size_bytes", 1)
        if actual["path"] != str(Path(expected["path"])) or actual["sha256"] != expected["sha256"]:
            raise PlanError(f"preflight {name} identity differs from the plan")
        if "size_bytes" in expected and actual["size_bytes"] != expected["size_bytes"]:
            raise PlanError(f"preflight {name} size differs from the plan")
        if retained:
            content = decode_retained_artifact(actual, f"preflight receipt.artifacts.{name}")
            if name == "request":
                validate_completion_request(content, plan)
    require_mapping(receipt["system"], "preflight receipt.system")
    return role


def retained_input_path(root: Path, role: str, name: str) -> Path:
    if name == "request" and role == "coordinator":
        return root / "inputs" / "request.raw"
    if name == "authority_receipt" and role in {"coordinator", "worker"}:
        return root / "inputs" / f"authority-{role}.raw"
    raise PlanError("invalid retained input role/name")


def write_bytes_exclusive(path: Path, content: bytes) -> None:
    reject_symlink_components(path.parent, "retained-input parent")
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    reject_symlink_components(path.parent, "retained-input parent")
    try:
        flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_BINARY", 0) | getattr(os, "O_NOFOLLOW", 0)
        descriptor = os.open(path, flags, 0o600)
        with os.fdopen(descriptor, "wb") as handle:
            handle.write(content)
            handle.flush()
            os.fsync(handle.fileno())
    except FileExistsError as exc:
        raise PlanError(f"retained input already exists: {path}") from exc


def import_preflight(root: Path, receipt_path: Path) -> None:
    plan = load_plan(root / "plan.json")
    validate_run_contract(root, plan)
    receipt_bytes = read_regular_bytes(receipt_path, "preflight receipt")
    receipt = require_mapping(parse_json_bytes(receipt_bytes, "preflight receipt"), "preflight receipt")
    role = validate_preflight_receipt(plan, receipt)
    destination = root / "preflight" / f"{role}.json"
    if destination.exists():
        raise PlanError(f"preflight already imported for {role}")
    retained = {"authority_receipt": decode_retained_artifact(
        receipt["artifacts"]["authority_receipt"], "authority receipt")}
    if role == "coordinator":
        retained["request"] = decode_retained_artifact(receipt["artifacts"]["request"], "request")
    written: list[Path] = []
    try:
        for name, content in retained.items():
            target = retained_input_path(root, role, name)
            write_bytes_exclusive(target, content)
            written.append(target)
        write_bytes_exclusive(destination, receipt_bytes)
    except Exception:
        for path in written:
            path.unlink(missing_ok=True)
        raise


def parse_timestamp(value: Any, where: str) -> dt.datetime:
    text = require_string(value, where)
    try:
        parsed = dt.datetime.fromisoformat(text.replace("Z", "+00:00"))
    except ValueError as exc:
        raise PlanError(f"{where} must be an ISO-8601 timestamp") from exc
    if parsed.tzinfo is None:
        raise PlanError(f"{where} must include a timezone")
    return parsed


def parse_client(path: Path, output_tokens: int) -> dict[str, Any]:
    client = require_mapping(read_json(path), "client timing")
    schema = client.get("schema")
    required = {"schema", "started_at", "ended_at", "http_status", "wall_ms", "ttft_ms", "itl_ms"}
    if schema == CLIENT_SCHEMA_V2:
        required |= {
            "remote_started_monotonic_ns", "remote_ended_monotonic_ns",
            "event_monotonic_ns",
        }
    require_keys(
        client,
        required,
        set(),
        "client timing",
    )
    if schema not in {CLIENT_SCHEMA, CLIENT_SCHEMA_V2}:
        raise PlanError(
            f"client timing schema must be {CLIENT_SCHEMA} or {CLIENT_SCHEMA_V2}")
    started = parse_timestamp(client["started_at"], "client.started_at")
    ended = parse_timestamp(client["ended_at"], "client.ended_at")
    if ended <= started:
        raise PlanError("client.ended_at must be after client.started_at")
    require_int(client["http_status"], "client.http_status", 100)
    for name in ("wall_ms", "ttft_ms"):
        value = client[name]
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
            raise PlanError(f"client.{name} must be finite and positive")
    itl = client.get("itl_ms", [])
    if not isinstance(itl, list) or any(
        isinstance(item, bool) or not isinstance(item, (int, float)) or not math.isfinite(item) or item <= 0
        for item in itl
    ):
        raise PlanError("client.itl_ms must contain finite positive numbers")
    if schema == CLIENT_SCHEMA:
        if len(itl) != output_tokens - 1:
            raise PlanError(
                "client.itl_ms must contain one interval after each generated token except the first")
    else:
        remote_started = require_int(
            client["remote_started_monotonic_ns"],
            "client.remote_started_monotonic_ns", 1)
        remote_ended = require_int(
            client["remote_ended_monotonic_ns"],
            "client.remote_ended_monotonic_ns", 1)
        events = client["event_monotonic_ns"]
        if remote_ended <= remote_started or not isinstance(events, list) or \
                len(events) != output_tokens:
            raise PlanError("client v2 remote/event monotonic evidence is empty or reversed")
        previous = remote_started
        for index, event in enumerate(events):
            observed = require_int(event, f"client.event_monotonic_ns[{index}]", 1)
            if observed <= previous or observed >= remote_ended:
                raise PlanError("client v2 event monotonic evidence is not strictly inside the request")
            previous = observed
        derived_ttft = (events[0] - remote_started) / 1e6
        derived_itl = [(right - left) / 1e6 for left, right in zip(events, events[1:])]
        derived_wall = (remote_ended - remote_started) / 1e6
        if len(itl) != len(events) - 1 or any(
                not math.isclose(observed, expected, rel_tol=1e-12, abs_tol=1e-9)
                for observed, expected in zip(itl, derived_itl)) or \
                not math.isclose(client["ttft_ms"], derived_ttft, rel_tol=1e-12, abs_tol=1e-9) or \
                not math.isclose(client["wall_ms"], derived_wall, rel_tol=1e-12, abs_tol=1e-9):
            raise PlanError("client v2 timing summaries differ from retained monotonic events")
    if client["ttft_ms"] > client["wall_ms"]:
        raise PlanError("client.ttft_ms cannot exceed client.wall_ms")
    observed_generation_span = client["ttft_ms"] + sum(itl)
    tolerance_ms = max(5.0, client["wall_ms"] * 0.01)
    if observed_generation_span > client["wall_ms"] + tolerance_ms:
        raise PlanError("client TTFT plus inter-token intervals exceeds client wall time")
    return client


def parse_response(path: Path, plan: dict[str, Any]) -> dict[str, Any]:
    response = require_mapping(read_json(path), "server response")
    timings = require_mapping(response.get("timings"), "server response.timings")
    required = {
        "cache_n", "prompt_n", "predicted_n", "prompt_ms", "predicted_ms",
        "prompt_per_second", "predicted_per_second",
    }
    if not required.issubset(timings):
        raise PlanError("server response.timings is incomplete")
    cache_n = timings["cache_n"]
    if isinstance(cache_n, bool) or not isinstance(cache_n, int):
        raise PlanError("server response.timings.cache_n must be an integer")
    if cache_n != 0:
        raise PlanError("cold-cache-off samples require server response.timings.cache_n == 0")
    if timings["prompt_n"] != plan["request"]["prompt_tokens"] or timings["predicted_n"] != plan["request"]["output_tokens"]:
        raise PlanError("server response token counts differ from the plan")
    numeric = {}
    for name in required - {"cache_n", "prompt_n", "predicted_n"}:
        value = timings[name]
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
            raise PlanError(f"server response.timings.{name} must be finite and positive")
        numeric[name] = float(value)
    expected_rates = {
        # Match llama-server's source operation order exactly. Reassociation
        # can differ by an ulp for non-round retained timings.
        "prompt_per_second": 1_000.0 / numeric["prompt_ms"] * timings["prompt_n"],
        "predicted_per_second": 1_000.0 / numeric["predicted_ms"] * timings["predicted_n"],
    }
    for name, expected_rate in expected_rates.items():
        if not math.isclose(numeric[name], expected_rate, rel_tol=1e-12, abs_tol=1e-9):
            raise PlanError(f"server response.timings.{name} is inconsistent with count and duration")
    content = response.get("content")
    if not isinstance(content, str):
        raise PlanError("server response.content must be a string")
    content_sha256 = digest_bytes(content.encode("utf-8"))
    expected = plan["request"].get("expected_content_sha256")
    if expected is not None and content_sha256 != expected:
        raise PlanError("server response content differs from the expected golden hash")
    return {
        "cache_n": cache_n,
        "prompt_n": timings["prompt_n"],
        "predicted_n": timings["predicted_n"],
        "prompt_ms": numeric["prompt_ms"],
        "generation_ms": numeric["predicted_ms"],
        "prompt_tokens_per_second": expected_rates["prompt_per_second"],
        "generation_tokens_per_second": expected_rates["predicted_per_second"],
        "content_sha256": content_sha256,
    }


def expected_schedule_entry(
    root: Path, plan: dict[str, Any], pair_id: int, condition: str, order_index: int,
) -> dict[str, Any]:
    schedule = validate_run_contract(root, plan)
    matches = [entry for entry in schedule["entries"] if entry == {
        "pair_id": pair_id, "order_index": order_index, "condition": condition}]
    if len(matches) != 1:
        raise PlanError("sample does not match the frozen A/B schedule")
    return matches[0]


def record_sample(
    root: Path,
    pair_id: int,
    condition: str,
    order_index: int,
    response_path: Path | None,
    client_path: Path | None,
    status: str,
    failure_code: str | None,
    extra_paths: list[Path],
) -> None:
    plan = load_plan(root / "plan.json")
    expected_schedule_entry(root, plan, pair_id, condition, order_index)
    if status not in {"success", "failure"}:
        raise PlanError("sample status must be success or failure")
    if status == "success" and (response_path is None or client_path is None):
        raise PlanError("successful samples require response and client timing JSON")
    if status == "failure" and not failure_code:
        raise PlanError("failed samples require a failure code")
    destination = root / "raw" / f"pair-{pair_id:03d}-order-{order_index}-{condition}"
    if destination.exists():
        raise PlanError(f"sample already exists: {destination}")
    staging = Path(tempfile.mkdtemp(prefix=".record-", dir=root / "raw"))
    try:
        copied: dict[str, dict[str, Any]] = {}
        for name, source in (("response", response_path), ("client", client_path)):
            if source is not None:
                if not source.is_file():
                    raise PlanError(f"raw {name} is not a file: {source}")
                target = staging / f"{name}.json"
                shutil.copyfile(source, target)
                copied[name] = {
                    "path": target.name,
                    "sha256": digest_file(target),
                    "size_bytes": target.stat().st_size,
                }
        for index, source in enumerate(extra_paths):
            if not source.is_file():
                raise PlanError(f"extra raw evidence is not a file: {source}")
            target = staging / f"extra-{index:02d}-{source.name}"
            shutil.copyfile(source, target)
            copied[f"extra_{index}"] = {
                "path": target.name,
                "sha256": digest_file(target),
                "size_bytes": target.stat().st_size,
            }

        result: dict[str, Any] | None = None
        client: dict[str, Any] | None = None
        if status == "success":
            result = parse_response(staging / "response.json", plan)
            client = parse_client(staging / "client.json", plan["request"]["output_tokens"])
            if client["http_status"] != 200:
                raise PlanError("successful sample did not return HTTP 200")
        sample = {
            "schema": SAMPLE_SCHEMA,
            "experiment_id": plan["experiment_id"],
            "plan_sha256": plan_digest(plan),
            "pair_id": pair_id,
            "order_index": order_index,
            "condition": condition,
            "status": status,
            "failure_code": failure_code,
            "identity": {
                "source_commit": plan["conditions"][condition]["source_commit"],
                "coordinator_binary_sha256": plan["conditions"][condition]["coordinator_binary"]["sha256"],
                "worker_binary_sha256": plan["conditions"][condition]["worker_binary"]["sha256"],
                "model_sha256": plan["model"]["sha256"],
                "request_sha256": plan["request"]["sha256"],
                "commands_sha256": digest_bytes(canonical_bytes(condition_commands(plan, condition))),
            },
            "client": client,
            "result": result,
            "raw": copied,
        }
        write_json(staging / "sample.json", sample)
        staging.rename(destination)
    except Exception:
        if staging.exists():
            shutil.rmtree(staging)
        raise


def validate_preflights(root: Path, plan: dict[str, Any]) -> dict[str, str]:
    hashes = {}
    for role in ("coordinator", "worker"):
        path = root / "preflight" / f"{role}.json"
        if not path.is_file():
            raise PlanError(f"missing {role} preflight receipt")
        receipt_bytes = read_regular_bytes(path, f"{role} preflight")
        receipt = require_mapping(parse_json_bytes(receipt_bytes, f"{role} preflight"), f"{role} preflight")
        if validate_preflight_receipt(plan, receipt) != role:
            raise PlanError(f"invalid {role} preflight receipt")
        retained_names = ["authority_receipt"] + (["request"] if role == "coordinator" else [])
        for name in retained_names:
            expected = decode_retained_artifact(
                receipt["artifacts"][name], f"{role} preflight {name}")
            retained_path = retained_input_path(root, role, name)
            try:
                observed = read_regular_bytes(retained_path, f"retained {role} {name}")
            except PlanError as exc:
                raise PlanError(f"retained {role} {name} bytes are missing or changed") from exc
            if observed != expected:
                raise PlanError(f"retained {role} {name} bytes are missing or changed")
        hashes[role] = digest_bytes(receipt_bytes)
    return hashes


def validate_raw_evidence(sample_path: Path, sample: dict[str, Any], plan: dict[str, Any]) -> None:
    raw = require_mapping(sample.get("raw"), "sample.raw")
    seen_paths = set()
    for name, value in raw.items():
        record = require_mapping(value, f"sample.raw.{name}")
        require_keys(record, {"path", "sha256", "size_bytes"}, set(), f"sample.raw.{name}")
        relative = require_string(record["path"], f"sample.raw.{name}.path")
        if Path(relative).name != relative or relative in seen_paths:
            raise PlanError("sample raw paths must be unique filenames")
        seen_paths.add(relative)
        require_hash(record["sha256"], f"sample.raw.{name}.sha256")
        require_int(record["size_bytes"], f"sample.raw.{name}.size_bytes", 1)
        artifact = sample_path.parent / relative
        if not artifact.is_file():
            raise PlanError(f"sample raw artifact is missing: {relative}")
        if artifact.stat().st_size != record["size_bytes"] or digest_file(artifact) != record["sha256"]:
            raise PlanError(f"sample raw artifact identity changed: {relative}")
    if sample["status"] == "success":
        if not {"response", "client"}.issubset(raw):
            raise PlanError("successful sample is missing response or client raw evidence")
        result = parse_response(sample_path.parent / raw["response"]["path"], plan)
        client = parse_client(
            sample_path.parent / raw["client"]["path"], plan["request"]["output_tokens"])
        if result != sample.get("result") or client != sample.get("client"):
            raise PlanError("sample summary differs from reparsed raw evidence")
    elif sample.get("result") is not None or sample.get("client") is not None:
        raise PlanError("failed sample cannot contain successful timing summaries")


def mean(values: list[float]) -> float:
    return statistics.fmean(values)


def metric_summary(pair_values: list[dict[str, float]], metric: str, lower_is_better: bool) -> dict[str, Any]:
    off = [item["off"] for item in pair_values]
    on = [item["on"] for item in pair_values]
    deltas = [candidate - control for control, candidate in zip(off, on)]
    improvement = [
        ((control - candidate) if lower_is_better else (candidate - control)) / control * 100.0
        for control, candidate in zip(off, on)
    ]
    return {
        "metric": metric,
        "lower_is_better": lower_is_better,
        "pair_count": len(pair_values),
        "off_mean": mean(off),
        "on_mean": mean(on),
        "paired_on_minus_off_mean": mean(deltas),
        "paired_improvement_percent_mean": mean(improvement),
        "paired_improvement_percent_median": statistics.median(improvement),
        "paired_improvement_percent_sample_sd": statistics.stdev(improvement) if len(improvement) >= 2 else None,
        "pairs": [
            {"pair_id": index + 1, "off": control, "on": candidate, "on_minus_off": delta, "improvement_percent": gain}
            for index, (control, candidate, delta, gain) in enumerate(zip(off, on, deltas, improvement))
        ],
    }


def analyze_run(root: Path) -> dict[str, Any]:
    plan = load_plan(root / "plan.json")
    preflight_hashes = validate_preflights(root, plan)
    schedule = validate_run_contract(root, plan)
    expected = {(entry["pair_id"], entry["order_index"], entry["condition"]) for entry in schedule["entries"]}
    sample_paths = sorted((root / "raw").glob("*/sample.json"))
    samples = [read_json(path) for path in sample_paths]
    observed: dict[tuple[int, int, str], dict[str, Any]] = {}
    for sample_path, sample_value in zip(sample_paths, samples):
        sample = require_mapping(sample_value, "sample")
        require_keys(
            sample,
            {
                "schema", "experiment_id", "plan_sha256", "pair_id", "order_index",
                "condition", "status", "failure_code", "identity", "client", "result", "raw",
            },
            set(),
            "sample",
        )
        if sample["schema"] != SAMPLE_SCHEMA or sample["experiment_id"] != plan["experiment_id"]:
            raise PlanError("sample schema or experiment identity mismatch")
        if sample["plan_sha256"] != plan_digest(plan):
            raise PlanError("sample plan identity mismatch")
        require_int(sample["pair_id"], "sample.pair_id", 1)
        require_int(sample["order_index"], "sample.order_index", 0)
        if sample["condition"] not in {"off", "on"} or sample["status"] not in {"success", "failure"}:
            raise PlanError("sample condition or status is invalid")
        if sample["status"] == "success" and sample["failure_code"] is not None:
            raise PlanError("successful sample cannot contain a failure code")
        if sample["status"] == "failure":
            require_string(sample["failure_code"], "sample.failure_code")
        key = (sample["pair_id"], sample["order_index"], sample["condition"])
        if key not in expected or key in observed:
            raise PlanError("sample is extra, duplicated, or outside the schedule")
        expected_identity = {
            "source_commit": plan["conditions"][sample["condition"]]["source_commit"],
            "coordinator_binary_sha256": plan["conditions"][sample["condition"]]["coordinator_binary"]["sha256"],
            "worker_binary_sha256": plan["conditions"][sample["condition"]]["worker_binary"]["sha256"],
            "model_sha256": plan["model"]["sha256"],
            "request_sha256": plan["request"]["sha256"],
            "commands_sha256": digest_bytes(canonical_bytes(condition_commands(plan, sample["condition"]))),
        }
        if sample.get("identity") != expected_identity:
            raise PlanError("sample artifact or command identity mismatch")
        validate_raw_evidence(sample_path, sample, plan)
        observed[key] = sample
    missing = sorted(expected - observed.keys())
    failures = [
        {"pair_id": item[0], "order_index": item[1], "condition": item[2], "failure_code": sample["failure_code"]}
        for item, sample in observed.items() if sample["status"] != "success"
    ]
    complete = not missing and not failures
    report: dict[str, Any] = {
        "schema": ANALYSIS_SCHEMA_V2 if plan["schema"] == PLAN_SCHEMA_V2 else ANALYSIS_SCHEMA_V1,
        "experiment_id": plan["experiment_id"],
        "plan_sha256": plan_digest(plan),
        "lane": plan["runtime"]["lane"],
        "cache_class": plan["runtime"]["cache_class"],
        "preflight_sha256": preflight_hashes,
        "scheduled_samples": len(expected),
        "retained_samples": len(samples),
        "missing": [dict(pair_id=a, order_index=b, condition=c) for a, b, c in missing],
        "failures": failures,
        "evidence_core_complete": complete,
        "execution_qualified": False,
        "measurement_ready": False,
        "minimum_five_pairs_met": plan["execution"]["pairs"] >= 5,
        "performance_claim": False,
        "metrics": {},
    }
    if plan["schema"] == PLAN_SCHEMA_V2:
        report["comparison"] = {
            "kind": plan["comparison"]["kind"],
            "control": plan["comparison"]["control"],
            "candidate": plan["comparison"]["candidate"],
            "batch_by_condition": dict(plan["runtime"]["batch_by_condition"]),
            "ubatch": plan["runtime"]["ubatch"],
            "condition_commands_sha256": {
                name: digest_bytes(canonical_bytes(condition_commands(plan, name)))
                for name in ("off", "on")
            },
        }
    if complete:
        by_pair: dict[int, dict[str, list[dict[str, Any]]]] = {}
        all_content = set()
        for sample in samples:
            by_pair.setdefault(sample["pair_id"], {"off": [], "on": []})[sample["condition"]].append(sample)
            all_content.add(sample["result"]["content_sha256"])
        expected_per_cell = plan["execution"]["retained_per_condition_per_pair"]
        for pair_id, conditions in by_pair.items():
            if any(len(conditions[name]) != expected_per_cell for name in ("off", "on")):
                raise PlanError(f"pair {pair_id} has the wrong retained sample count")
        if len(all_content) != 1:
            raise PlanError("deterministic content parity failed across A/B samples")
        fields = {
            "prompt_tokens_per_second": (lambda sample: sample["result"]["prompt_tokens_per_second"], False),
            "generation_tokens_per_second": (lambda sample: sample["result"]["generation_tokens_per_second"], False),
            "client_wall_ms": (lambda sample: float(sample["client"]["wall_ms"]), True),
            "ttft_ms": (lambda sample: float(sample["client"]["ttft_ms"]), True),
            "mean_itl_ms": (lambda sample: mean([float(value) for value in sample["client"]["itl_ms"]]), True),
        }
        for metric, (extract, lower_is_better) in fields.items():
            values = []
            for pair_id in sorted(by_pair):
                values.append({
                    name: mean([extract(sample) for sample in by_pair[pair_id][name]])
                    for name in ("off", "on")
                })
            report["metrics"][metric] = metric_summary(values, metric, lower_is_better)
        report["content_sha256"] = next(iter(all_content))
    sampling_sync_plan = root / "sampling-output-sync-plan.json"
    sampling_sync_analysis = root / "sampling-output-sync-analysis.json"
    sampling_sync_evidence = list((root / "raw").glob("**/sampling-output-sync"))
    if any(path.exists() or path.is_symlink() for path in (
            sampling_sync_plan, sampling_sync_analysis)) or sampling_sync_evidence:
        sidecar_path = Path(__file__).with_name("halofpx_strix_ab_sampling_sync.py")
        spec = importlib.util.spec_from_file_location("halofpx_strix_ab_sampling_sync_sidecar", sidecar_path)
        if spec is None or spec.loader is None:
            raise PlanError(f"cannot load sampling-output-sync sidecar: {sidecar_path}")
        sidecar = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(sidecar)
        try:
            sidecar.validate_frozen_run(root, plan)
        except sidecar.PlanError as exc:
            raise PlanError(str(exc)) from exc
    write_json(root / "analysis.json", report)
    with (root / "samples.jsonl").open("w", encoding="utf-8", newline="\n") as handle:
        for sample in sorted(samples, key=lambda item: (item["pair_id"], item["order_index"])):
            handle.write(json.dumps(sample, sort_keys=True, separators=(",", ":")) + "\n")
    write_json(root / "status.json", {
        "schema": "halofpx.strix-ab-status.v1",
        "experiment_id": plan["experiment_id"],
        "state": "evidence-core-complete" if complete else "evidence-core-incomplete",
        "execution_qualified": False,
        "measurement_ready": False,
        "performance_claim": False,
    })
    evidence_files = sorted(
        path for path in root.rglob("*")
        if path.is_file() and path.name != "SHA256SUMS"
    )
    (root / "SHA256SUMS").write_text(
        "".join(f"{digest_file(path)}  {path.relative_to(root).as_posix()}\n" for path in evidence_files),
        encoding="utf-8",
    )
    return report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate = subparsers.add_parser("validate", help="validate and normalize a plan")
    validate.add_argument("plan", type=Path)
    initialize = subparsers.add_parser("init", help="freeze a plan and deterministic paired schedule")
    initialize.add_argument("plan", type=Path)
    initialize.add_argument("run_root", type=Path)
    preflight = subparsers.add_parser("preflight", help="verify this node's planned artifacts")
    preflight.add_argument("plan", type=Path)
    preflight.add_argument("--role", choices=("coordinator", "worker"), required=True)
    preflight.add_argument("--output", type=Path)
    import_pf = subparsers.add_parser("import-preflight", help="import a node preflight receipt")
    import_pf.add_argument("run_root", type=Path)
    import_pf.add_argument("receipt", type=Path)
    record = subparsers.add_parser("record", help="copy and validate one raw scheduled sample")
    record.add_argument("run_root", type=Path)
    record.add_argument("--pair", type=int, required=True)
    record.add_argument("--condition", choices=("off", "on"), required=True)
    record.add_argument("--order-index", type=int, choices=(0, 1), required=True)
    record.add_argument("--status", choices=("success", "failure"), required=True)
    record.add_argument("--response", type=Path)
    record.add_argument("--client", type=Path)
    record.add_argument("--failure-code")
    record.add_argument("--extra", type=Path, action="append", default=[])
    analyze = subparsers.add_parser("analyze", help="validate completeness and emit paired results")
    analyze.add_argument("run_root", type=Path)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        if args.command == "validate":
            plan = load_plan(args.plan)
            print(json.dumps({"schema": plan["schema"], "experiment_id": plan["experiment_id"], "plan_sha256": plan_digest(plan)}, sort_keys=True))
        elif args.command == "init":
            init_run(args.plan, args.run_root)
        elif args.command == "preflight":
            receipt = collect_preflight(load_plan(args.plan), args.role)
            if args.output:
                write_json(args.output, receipt)
            else:
                print(json.dumps(receipt, indent=2, sort_keys=True))
        elif args.command == "import-preflight":
            import_preflight(args.run_root, args.receipt)
        elif args.command == "record":
            record_sample(
                args.run_root, args.pair, args.condition, args.order_index,
                args.response, args.client, args.status, args.failure_code, args.extra,
            )
        elif args.command == "analyze":
            report = analyze_run(args.run_root)
            print(json.dumps(report, indent=2, sort_keys=True))
            return 0 if report["evidence_core_complete"] else 1
    except PlanError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
