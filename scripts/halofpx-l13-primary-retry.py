#!/usr/bin/env python3
"""One-shot bounded L15 primary canary, intended only as controller child."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path


NIMO1 = "nimo-1"
NIMO2 = "nimo-2"
PORT = 50179
WORKER_BIN = "/var/tmp/halofpx-l15-src-nimo1/build-l15/bin/rpc-server"
CANARY_BIN = "/var/tmp/halofpx-l15-src-nimo2/build-l15/bin/test-halofpx-distributed-state-canary"
READINESS_PROBE = "/var/tmp/halofpx-l15-src-nimo2/scripts/halofpx_rpc_readiness.py"
READINESS_PROBE_SHA = "f2db27e26567b33a4d4e69c5cb248cf61b63dfa3765aa218d09668225905c980"
MODEL = (
    "/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/"
    "dba517197f2854f3d362529e13abddcdcad6c10b/"
    "saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf"
)
MODEL_SHA = "96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6"
MODEL_BYTES = 159873097824
CANARY_SHA = "5ecf44d64006e1c00f0a9a86d44395d4a27ad5a3e3648d70ff4d617a91913759"
WORKER_SHA = "919cb4c6a3144da20d1bb8d8a4b09d5a9bd2d4c9da256495079f187e066339ea"
PROMPT_SHA = "f20c7c7a4137de98b991f1bfe6de27e194a93c7257d9496944e312085923143f"
PROMPT = "/var/tmp/halofpx-l13-primary-20260721/prompt.txt"
REMOTE_EVIDENCE = "/var/tmp/halofpx-l15-primary-20260721"
COORDINATOR_ROOT = "/var/tmp/halofpx-l15-primary-coordinator-20260721"
WORKER_ROOT = "/var/tmp/halofpx-l15-primary-worker-20260721"
CONTROL = REMOTE_EVIDENCE + "/control.key"
WORKER_CONTROL = "/var/tmp/halofpx-l15-primary-control.key"
CHECKPOINT = "421016c41e1af022aa65feef9c7b9329fdc1b49ff0b1c4df4aaad10cf13bf816"
ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"

MODEL_DIGEST = MODEL_SHA
COMPATIBILITY = "a8f921ae8742823eac2942004094d1d11f47962bae0607c4b2fce6ce5a81c36f"
PLAN = "0268cc6071a8d78983f6351fe45d510e767d8cd26618a8bdffc972b6655f7967"
TOPOLOGY = "09b71fe40ae05c841a5be563f6e2b27ad2529d893b9420412e5280541ae53e1f"
PLACEMENT = "d4aa0d3c14a3bec4ba5de733e00b6447f79f94d5dbeda6e3593be74ce84f917e"


class CanaryError(RuntimeError):
    pass


def run(argv, *, timeout=900, check=True):
    result = subprocess.run(argv, text=True, capture_output=True, timeout=timeout, check=False)
    if check and result.returncode != 0:
        raise CanaryError(
            f"command failed ({result.returncode}): {argv!r}\n{result.stdout}\n{result.stderr}"
        )
    return result


def ssh(host, *argv, timeout=900, check=True):
    return run(["ssh", "-o", "BatchMode=yes", host, *argv], timeout=timeout, check=check)


def write_log(root: Path, name: str, result) -> None:
    (root / name).write_text(result.stdout + result.stderr, encoding="utf-8", newline="\n")


def listener_pid(text: str, port: int) -> int:
    matches = []
    for line in text.splitlines():
        fields = line.split()
        if len(fields) >= 4 and fields[3].endswith(f":{port}"):
            matches.append(line)
    if len(matches) != 1:
        return 0
    match = re.search(r"pid=(\d+)", matches[0])
    return int(match.group(1)) if match else 0


def start_worker(local_state: bool, unit: str, evidence_root: Path | None = None) -> tuple[int, str, dict[str, object]]:
    command = [
        "systemd-run", "--user", f"--unit={unit}", "--property=RuntimeMaxSec=90min",
        "--setenv=GGML_RPC_DEBUG=1", WORKER_BIN,
        "--host", "10.44.0.1", "--port", str(PORT), "--device", "ROCm0",
    ]
    if local_state:
        command.extend([
            "--halofpx-local-state", "--halofpx-state-root", WORKER_ROOT,
            "--halofpx-state-key-file", WORKER_CONTROL,
            "--halofpx-state-rank", "1", "--halofpx-state-world", "2",
            "--halofpx-state-key-generation", "7",
        ])
    ssh(NIMO1, *command)
    probe_command = [
        "python3", READINESS_PROBE,
        "--endpoint", f"10.44.0.1:{PORT}",
        "--timeout-seconds", "120",
        "--attempt-timeout-seconds", "2",
        "--initial-backoff-seconds", "0.1",
        "--maximum-backoff-seconds", "1",
    ]
    if local_state:
        probe_command.extend([
            "--logical-rank", "1",
            "--world-size", "2",
            "--key-generation", "7",
            "--expected-channel-key-file", CONTROL,
        ])
    else:
        probe_command.append("--expect-feature-off")
    readiness = ssh(NIMO2, *probe_command, timeout=130, check=False)
    if readiness.returncode != 0:
        raise CanaryError(f"worker {unit} failed HaloFPX CAPS readiness: {readiness.stdout}{readiness.stderr}")
    try:
        readiness_result = json.loads(readiness.stdout)
    except json.JSONDecodeError as exc:
        raise CanaryError(f"worker {unit} returned malformed readiness evidence") from exc
    expected_result = readiness_result.get("admitted") is True if local_state else (
        readiness_result.get("admitted") is False and readiness_result.get("feature_off_confirmed") is True
    )
    if not expected_result or readiness_result.get("endpoint") != f"10.44.0.1:{PORT}":
        raise CanaryError(f"worker {unit} returned mismatched readiness evidence")
    if evidence_root is not None:
        (evidence_root / f"{unit}-readiness.json").write_text(
            json.dumps(readiness_result, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n"
        )
    active = ssh(NIMO1, "systemctl", "--user", "is-active", f"{unit}.service", check=False)
    if active.stdout.strip() != "active":
        raise CanaryError(f"worker {unit} left active state after CAPS readiness")
    pid = int(ssh(NIMO1, "systemctl", "--user", "show", f"{unit}.service", "-p", "MainPID", "--value").stdout.strip())
    invocation_id = ssh(
        NIMO1, "systemctl", "--user", "show", f"{unit}.service", "-p", "InvocationID", "--value"
    ).stdout.strip()
    if not re.fullmatch(r"[0-9a-fA-F]{32}", invocation_id):
        raise CanaryError(f"worker {unit} has invalid InvocationID")
    listeners = ssh(NIMO1, "ss", "-H", "-ltnp", check=False).stdout
    if listener_pid(listeners, PORT) != pid:
        raise CanaryError(f"worker {unit} listener no longer matches MainPID after CAPS readiness")
    return pid, invocation_id.lower(), readiness_result


def stop_worker(unit: str) -> None:
    def stopped() -> tuple[bool, str]:
        show = ssh(
            NIMO1, "systemctl", "--user", "show", f"{unit}.service",
            "-p", "LoadState", "-p", "ActiveState", "-p", "SubState", "-p", "MainPID",
            check=False,
        ).stdout
        listeners = ssh(NIMO1, "ss", "-H", "-ltnp", check=False).stdout
        props = dict(line.split("=", 1) for line in show.splitlines() if "=" in line)
        return (
            props.get("ActiveState") == "inactive"
            and props.get("SubState") == "dead"
            and props.get("MainPID") == "0"
            and listener_pid(listeners, PORT) == 0,
            repr(props),
        )

    already_stopped, _ = stopped()
    if already_stopped:
        return
    ssh(NIMO1, "systemctl", "--user", "stop", f"{unit}.service")
    deadline = time.monotonic() + 30
    last = ""
    while time.monotonic() < deadline:
        is_stopped, last = stopped()
        if is_stopped:
            return
        time.sleep(1)
    raise CanaryError(f"disposable worker cleanup not verified for {unit}: {last}")


def canary(mode: str, unit_label: str, *, plan: str = PLAN):
    canary_command = [
        CANARY_BIN,
        "--hfx-mode", mode,
        "--hfx-artifact-root", COORDINATOR_ROOT,
        "--hfx-model-digest", MODEL_DIGEST,
        "--hfx-compatibility-root", COMPATIBILITY,
        "--hfx-plan-digest", plan,
        "--hfx-topology-digest", TOPOLOGY,
        "--hfx-placement-digest", PLACEMENT,
        "--hfx-checkpoint-digest", CHECKPOINT,
        "--hfx-control-file", CONTROL,
        "--hfx-expected-prompt-tokens", "1129",
        "--model", MODEL,
        "--rpc", f"10.44.0.1:{PORT}",
        "--split-mode", "layer",
        "--tensor-split", "1,1",
        "--n-gpu-layers", "999",
        "--fit", "off",
        "--no-mmap",
        "--direct-io",
        "--flash-attn", "on",
        "--ctx-size", "4096",
        "--batch-size", "512",
        "--ubatch-size", "512",
        "--cache-type-k", "q8_0",
        "--cache-type-v", "q8_0",
        "--parallel", "1",
        "--threads", "16",
        "--threads-batch", "16",
        "--file", PROMPT,
        "--n-predict", "128",
        "--seed", "1234",
        "--temp", "0",
    ]
    unit = f"halofpx-l15-primary-canary-{unit_label}-20260721"
    command = [
        "systemd-run", "--user", f"--unit={unit}", "--property=RuntimeMaxSec=20min",
        "--wait", "--collect", "--pipe", *canary_command,
    ]
    invocation = "invocation=" + " ".join(canary_command) + "\ntransient_unit=" + unit + "\n"
    result = ssh(NIMO2, *command, timeout=900, check=False)
    result.stdout = invocation + result.stdout
    if result.returncode != 0:
        raise CanaryError(result.stdout + result.stderr)
    return result


def output_fields(text: str) -> dict[str, str]:
    line = next((line for line in reversed(text.splitlines()) if line.startswith("mode=")), "")
    if not line:
        raise CanaryError("canary output has no result line")
    fields = {}
    for match in re.finditer(r"(?:^| )([a-z_]+)=([^ ]+)", line):
        fields[match.group(1)] = match.group(2)
    return fields


def fetch_suffix(root: Path, remote_name: str, local_name: str) -> tuple[str, str]:
    token_remote = f"{ARTIFACT_DIR}/{remote_name}-suffix.bin"
    text_remote = f"{ARTIFACT_DIR}/{remote_name}-suffix.txt"
    token_local = root / f"{local_name}-suffix.bin"
    text_local = root / f"{local_name}-suffix.txt"
    run(["scp", f"{NIMO2}:{token_remote}", str(token_local)])
    run(["scp", f"{NIMO2}:{text_remote}", str(text_local)])
    return (
        hashlib.sha256(token_local.read_bytes()).hexdigest(),
        hashlib.sha256(text_local.read_bytes()).hexdigest(),
    )


def worker_journal(unit: str, invocation_id: str, pid: int) -> str:
    journal = ssh(
        NIMO1,
        "journalctl", "--user", "-u", f"{unit}.service",
        f"_SYSTEMD_INVOCATION_ID={invocation_id}", "--no-pager", "-o", "short",
    ).stdout
    state_lines = [line for line in journal.splitlines() if "[halofpx-state]" in line]
    if any(re.search(rf"\[{pid}\]:", line) is None for line in state_lines):
        raise CanaryError(f"worker {unit} state journal is not bound to admitted PID {pid}")
    return journal


def require_result(
    fields: dict[str, str],
    mode: str,
    *,
    fallback_reason: str | None = None,
    require_worker_state: bool = False,
) -> None:
    exact = {
        "mode": mode,
        "prompt_tokens": "1129",
        "saved_boundary": "1128",
        "n_batch": "512",
    }
    for name, expected in exact.items():
        if fields.get(name) != expected:
            raise CanaryError(f"{mode} result {name} mismatch: {fields.get(name)!r}")
    if fallback_reason is None:
        if "fallback" in fields or "reason" in fields:
            raise CanaryError(f"{mode} unexpectedly cold-fell back: {fields}")
    elif fields.get("fallback") != "cold" or fields.get("reason") != fallback_reason:
        raise CanaryError(f"{mode} fallback mismatch: expected {fallback_reason}, got {fields}")
    if require_worker_state and (
        int(fields.get("worker_bytes", "0")) <= 0
        or int(fields.get("worker_components", "0")) <= 0
    ):
        raise CanaryError(f"{mode} did not report positive worker state")


def state_windows(capture: str, restore: str) -> tuple[list[str], list[str]]:
    capture_lines = capture.splitlines()
    stored = next(i for i, line in enumerate(capture_lines) if "[halofpx-state] stored" in line)
    stored_bytes = re.search(r"bytes=(\d+)", capture_lines[stored])
    if not stored_bytes:
        raise CanaryError("capture state byte count is absent")
    capture_start = next(
        i for i in range(stored - 1, -1, -1)
        if "[alloc_buffer]" in capture_lines[i] and f"size: {stored_bytes.group(1)}" in capture_lines[i]
    )
    capture_window = capture_lines[capture_start:stored + 1]

    restore_lines = restore.splitlines()
    ready = next(i for i, line in enumerate(restore_lines) if "[halofpx-state] ready" in line)
    applied = next(i for i, line in enumerate(restore_lines[ready + 1:], ready + 1) if "[halofpx-state] apply" in line)
    ready_bytes = re.search(r"bytes=(\d+)", restore_lines[ready])
    if not ready_bytes:
        raise CanaryError("restore state byte count is absent")
    restore_start = next(
        i for i in range(ready - 1, -1, -1)
        if "[alloc_buffer]" in restore_lines[i] and f"size: {ready_bytes.group(1)}" in restore_lines[i]
    )
    restore_end = next(
        i for i in range(applied + 1, len(restore_lines)) if "[free_buffer]" in restore_lines[i]
    )
    restore_window = restore_lines[restore_start:restore_end + 1]
    combined = "\n".join(capture_window + restore_window).lower()
    if "[get_tensor]" in combined or "[set_tensor]" in combined:
        raise CanaryError("state window contains GET_TENSOR/SET_TENSOR")
    return capture_window, restore_window


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True, type=Path)
    args = parser.parse_args()
    root = args.evidence_dir.resolve()
    root.mkdir(mode=0o700, parents=True, exist_ok=False)
    local_units = []
    results = {}
    suffixes = {}
    try:
        if ssh(NIMO1, "systemctl", "is-active", "minimax-m27-q6-server.service", check=False).stdout.strip() != "inactive":
            raise CanaryError("production coordinator is not inactive")
        if ssh(NIMO2, "systemctl", "is-active", "minimax-m27-rpc-worker.service", check=False).stdout.strip() != "inactive":
            raise CanaryError("production worker is not inactive")
        if f":8081" in ssh(NIMO1, "ss", "-H", "-ltnp").stdout or f":50052" in ssh(NIMO2, "ss", "-H", "-ltnp").stdout:
            raise CanaryError("production listener remains open")
        model_stat = ssh(NIMO2, "stat", "-c", "%s", MODEL).stdout.strip()
        if int(model_stat) != MODEL_BYTES:
            raise CanaryError("model size mismatch")
        if ssh(NIMO2, "sha256sum", MODEL, timeout=300).stdout.split()[0] != MODEL_SHA:
            raise CanaryError("model SHA-256 mismatch")
        if ssh(NIMO2, "sha256sum", CANARY_BIN).stdout.split()[0] != CANARY_SHA:
            raise CanaryError("coordinator canary binary mismatch")
        if ssh(NIMO1, "sha256sum", WORKER_BIN).stdout.split()[0] != WORKER_SHA:
            raise CanaryError("worker binary mismatch")
        if ssh(NIMO2, "sha256sum", READINESS_PROBE).stdout.split()[0] != READINESS_PROBE_SHA:
            raise CanaryError("readiness probe mismatch")
        if ssh(NIMO2, "sha256sum", PROMPT).stdout.split()[0] != PROMPT_SHA:
            raise CanaryError("prompt SHA-256 mismatch")
        free_worker = int(ssh(NIMO1, "df", "-B1", "--output=avail", "/var/tmp").stdout.splitlines()[-1])
        if free_worker < 2_000_000_000:
            raise CanaryError("worker free space below 2 GB gate")
        ssh(NIMO2, "rm", "-rf", "--", REMOTE_EVIDENCE, COORDINATOR_ROOT)
        ssh(NIMO2, "install", "-d", "-m", "700", REMOTE_EVIDENCE, COORDINATOR_ROOT)
        ssh(NIMO2, "bash", "-c", f"umask 077; openssl rand -hex 64 | fold -w 64 > {CONTROL}")
        ssh(NIMO1, "rm", "-rf", "--", WORKER_ROOT)
        ssh(NIMO1, "install", "-d", "-m", "700", WORKER_ROOT)
        run(["scp", f"{NIMO2}:{CONTROL}", f"{NIMO1}:{WORKER_CONTROL}"])
        ssh(NIMO1, "chmod", "600", WORKER_CONTROL)

        (root / "diskstats-nimo1-before.txt").write_text(ssh(NIMO1, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        (root / "diskstats-nimo2-before.txt").write_text(ssh(NIMO2, "cat", "/proc/diskstats").stdout, encoding="utf-8")

        unit1 = "halofpx-l15-primary-worker-capture-20260721"
        local_units.append(unit1)
        pid_capture, invocation_capture, readiness_capture = start_worker(True, unit1, root)
        capture = canary("capture", "capture")
        write_log(root, "capture.log", capture)
        results["capture"] = output_fields(capture.stdout)
        require_result(results["capture"], "capture", require_worker_state=True)
        suffixes["capture"] = fetch_suffix(root, "capture", "capture")
        capture_journal = worker_journal(unit1, invocation_capture, pid_capture)
        (root / "worker-capture.log").write_text(capture_journal, encoding="utf-8")
        stop_worker(unit1)

        unit2 = "halofpx-l15-primary-worker-restore-20260721"
        local_units.append(unit2)
        pid_restore, invocation_restore, readiness_restore = start_worker(True, unit2, root)

        cold = canary("cold", "cold")
        write_log(root, "cold.log", cold)
        results["cold"] = output_fields(cold.stdout)
        require_result(results["cold"], "cold")
        suffixes["cold"] = fetch_suffix(root, "cold", "cold")

        restore = canary("restore", "restore")
        write_log(root, "restore.log", restore)
        results["restore"] = output_fields(restore.stdout)
        require_result(results["restore"], "restore", require_worker_state=True)
        suffixes["restore"] = fetch_suffix(root, "restore", "restore")

        objects = ssh(NIMO1, "find", WORKER_ROOT + "/objects", "-type", "f", "-name", "*.hfx").stdout.splitlines()
        if len(objects) != 1:
            raise CanaryError(f"expected one worker object, found {len(objects)}")
        object_path = objects[0]
        object_bytes = int(ssh(NIMO1, "stat", "-c", "%s", object_path).stdout.strip())
        object_sha = ssh(NIMO1, "sha256sum", object_path).stdout.split()[0]
        ssh(NIMO1, "mv", "--", object_path, object_path + ".missing")
        try:
            missing = canary("restore", "missing")
        finally:
            ssh(NIMO1, "mv", "--", object_path + ".missing", object_path, check=False)
        write_log(root, "missing-object.log", missing)
        results["missing_object"] = output_fields(missing.stdout)
        suffixes["missing_object"] = fetch_suffix(root, "restore", "missing-object")
        require_result(results["missing_object"], "restore", fallback_reason="worker-stage")

        mismatch = canary("restore", "mismatch", plan="f" * 64)
        write_log(root, "plan-mismatch.log", mismatch)
        results["plan_mismatch"] = output_fields(mismatch.stdout)
        suffixes["plan_mismatch"] = fetch_suffix(root, "restore", "plan-mismatch")
        require_result(results["plan_mismatch"], "restore", fallback_reason="coordinator-artifact")

        restore_journal = worker_journal(unit2, invocation_restore, pid_restore)
        (root / "worker-restore.log").write_text(restore_journal, encoding="utf-8")
        capture_window, restore_window = state_windows(capture_journal, restore_journal)
        (root / "capture-state-window.log").write_text("\n".join(capture_window) + "\n", encoding="utf-8")
        (root / "restore-state-window.log").write_text("\n".join(restore_window) + "\n", encoding="utf-8")
        stop_worker(unit2)

        unit3 = "halofpx-l15-primary-worker-runtime-off-20260721"
        local_units.append(unit3)
        pid_runtime_off, invocation_runtime_off, readiness_runtime_off = start_worker(False, unit3, root)
        runtime_off = canary("cold", "runtime-off")
        write_log(root, "runtime-off-cold.log", runtime_off)
        results["runtime_off"] = output_fields(runtime_off.stdout)
        require_result(results["runtime_off"], "cold")
        suffixes["runtime_off"] = fetch_suffix(root, "cold", "runtime-off-cold")

        if len(set(suffixes.values())) != 1:
            raise CanaryError(f"suffix mismatch: {suffixes}")
        cold_ms = float(results["cold"]["prompt_ms"]) + float(results["cold"]["generation_ms"])
        off_ms = float(results["runtime_off"]["prompt_ms"]) + float(results["runtime_off"]["generation_ms"])
        if cold_ms > max(off_ms * 1.5, off_ms + 50.0):
            raise CanaryError(f"obvious retained cold slowdown: enabled={cold_ms} off={off_ms}")

        summary = {
            "schema": "halofpx.l15.primary-result.v1",
            "model_sha256": MODEL_SHA,
            "model_bytes": MODEL_BYTES,
            "pids": {"capture": pid_capture, "restore": pid_restore, "runtime_off": pid_runtime_off},
            "invocation_ids": {
                "capture": invocation_capture,
                "restore": invocation_restore,
                "runtime_off": invocation_runtime_off,
            },
            "readiness": {
                "capture": readiness_capture,
                "restore": readiness_restore,
                "runtime_off": readiness_runtime_off,
            },
            "results": results,
            "suffix_hashes": {name: {"tokens": value[0], "text": value[1]} for name, value in suffixes.items()},
            "worker_object": {"path": object_path, "bytes": object_bytes, "sha256": object_sha},
            "state_window_get_set": 0,
            "cold_enabled_ms": cold_ms,
            "cold_runtime_off_ms": off_ms,
            "cold_ratio": cold_ms / off_ms,
        }
        (root / "result.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        (root / "diskstats-nimo1-after.txt").write_text(ssh(NIMO1, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        (root / "diskstats-nimo2-after.txt").write_text(ssh(NIMO2, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        return 0
    except Exception as exc:
        (root / "failure.txt").write_text(str(exc) + "\n", encoding="utf-8")
        print(f"L15 primary canary failed: {exc}", file=sys.stderr)
        return 1
    finally:
        cleanup_errors = []
        for unit in reversed(local_units):
            try:
                stop_worker(unit)
            except Exception as exc:
                cleanup_errors.append(f"{unit}: {exc}")
        if cleanup_errors:
            raise CanaryError("; ".join(cleanup_errors))


if __name__ == "__main__":
    raise SystemExit(main())
