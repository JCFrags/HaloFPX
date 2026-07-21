#!/usr/bin/env python3
"""One-shot bounded L13 primary canary, intended only as controller child."""

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
PORT = 50176
WORKER_BIN = "/var/tmp/halofpx-l13-retry-src-nimo1/build-primary-bed36/bin/rpc-server"
CANARY_BIN = "/var/tmp/halofpx-l13-retry-src-nimo2/build-primary-bed36/bin/test-halofpx-distributed-state-canary"
MODEL = (
    "/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/"
    "dba517197f2854f3d362529e13abddcdcad6c10b/"
    "saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf"
)
MODEL_SHA = "96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6"
MODEL_BYTES = 159873097824
CANARY_SHA = "6cbfd802403adfd03b20c924f9f38aff8f384710aaabd3c915ae5fd7b547c19d"
WORKER_SHA = "7bc3b27776ec808f50b40e7fa9bb6d9026f2488dea92b9d768a4dd6e90405d14"
PROMPT_SHA = "f20c7c7a4137de98b991f1bfe6de27e194a93c7257d9496944e312085923143f"
PROMPT = "/var/tmp/halofpx-l13-primary-20260721/prompt.txt"
REMOTE_EVIDENCE = "/var/tmp/halofpx-l13r-primary-20260721"
COORDINATOR_ROOT = "/var/tmp/halofpx-l13r-primary-coordinator-20260721"
WORKER_ROOT = "/var/tmp/halofpx-l13r-primary-worker-20260721"
CONTROL = REMOTE_EVIDENCE + "/control.key"
CHECKPOINT = "421016c41e1af022aa65feef9c7b9329fdc1b49ff0b1c4df4aaad10cf13bf816"
ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"

MODEL_DIGEST = MODEL_SHA
COMPATIBILITY = "a8f921ae8742823eac2942004094d1d11f47962bae0607c4b2fce6ce5a81c36f"
PLAN = "0268cc6071a8d78983f6351fe45d510e767d8cd26618a8bdffc972b6655f7967"
TOPOLOGY = "09b71fe40ae05c841a5be563f6e2b27ad2529d893b9420412e5280541ae53e1f"
PLACEMENT = "d4aa0d3c14a3bec4ba5de733e00b6447f79f94d5dbeda6e3593be74ce84f917e"


class RetryError(RuntimeError):
    pass


def run(argv, *, timeout=900, check=True):
    result = subprocess.run(argv, text=True, capture_output=True, timeout=timeout, check=False)
    if check and result.returncode != 0:
        raise RetryError(
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


def start_worker(local_state: bool, unit: str) -> int:
    command = [
        "systemd-run", "--user", f"--unit={unit}", "--property=RuntimeMaxSec=90min",
        "--setenv=GGML_RPC_DEBUG=1", WORKER_BIN,
        "--host", "10.44.0.1", "--port", str(PORT), "--device", "ROCm0",
    ]
    if local_state:
        command.extend([
            "--halofpx-local-state", "--halofpx-state-root", WORKER_ROOT,
            "--halofpx-state-key-file", "/var/tmp/halofpx-l13r-primary-control.key",
            "--halofpx-state-rank", "1", "--halofpx-state-world", "2",
            "--halofpx-state-key-generation", "7",
        ])
    ssh(NIMO1, *command)
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        active = ssh(NIMO1, "systemctl", "--user", "is-active", f"{unit}.service", check=False)
        listeners = ssh(NIMO1, "ss", "-H", "-ltnp", check=False).stdout
        if active.stdout.strip() == "active":
            pid = int(ssh(NIMO1, "systemctl", "--user", "show", f"{unit}.service", "-p", "MainPID", "--value").stdout.strip())
            if listener_pid(listeners, PORT) != pid:
                time.sleep(1)
                continue
            return pid
        time.sleep(1)
    raise RetryError(f"worker {unit} did not become ready")


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
    raise RetryError(f"disposable worker cleanup not verified for {unit}: {last}")


def canary(mode: str, *, plan: str = PLAN):
    command = [
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
    invocation = "invocation=" + " ".join(command) + "\n"
    result = ssh(NIMO2, *command, timeout=900, check=False)
    result.stdout = invocation + result.stdout
    if result.returncode != 0:
        raise RetryError(result.stdout + result.stderr)
    return result


def output_fields(text: str) -> dict[str, str]:
    line = next((line for line in reversed(text.splitlines()) if line.startswith("mode=")), "")
    if not line:
        raise RetryError("canary output has no result line")
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


def worker_journal(unit: str) -> str:
    return ssh(NIMO1, "journalctl", "--user", "-u", f"{unit}.service", "--no-pager").stdout


def state_windows(capture: str, restore: str) -> tuple[list[str], list[str]]:
    capture_lines = capture.splitlines()
    stored = next(i for i, line in enumerate(capture_lines) if "[halofpx-state] stored" in line)
    stored_bytes = re.search(r"bytes=(\d+)", capture_lines[stored])
    if not stored_bytes:
        raise RetryError("capture state byte count is absent")
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
        raise RetryError("restore state byte count is absent")
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
        raise RetryError("state window contains GET_TENSOR/SET_TENSOR")
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
            raise RetryError("production coordinator is not inactive")
        if ssh(NIMO2, "systemctl", "is-active", "minimax-m27-rpc-worker.service", check=False).stdout.strip() != "inactive":
            raise RetryError("production worker is not inactive")
        if f":8081" in ssh(NIMO1, "ss", "-H", "-ltnp").stdout or f":50052" in ssh(NIMO2, "ss", "-H", "-ltnp").stdout:
            raise RetryError("production listener remains open")
        model_stat = ssh(NIMO2, "stat", "-c", "%s", MODEL).stdout.strip()
        if int(model_stat) != MODEL_BYTES:
            raise RetryError("model size mismatch")
        if ssh(NIMO2, "sha256sum", MODEL, timeout=300).stdout.split()[0] != MODEL_SHA:
            raise RetryError("model SHA-256 mismatch")
        if ssh(NIMO2, "sha256sum", CANARY_BIN).stdout.split()[0] != CANARY_SHA:
            raise RetryError("coordinator canary binary mismatch")
        if ssh(NIMO1, "sha256sum", WORKER_BIN).stdout.split()[0] != WORKER_SHA:
            raise RetryError("worker binary mismatch")
        if ssh(NIMO2, "sha256sum", PROMPT).stdout.split()[0] != PROMPT_SHA:
            raise RetryError("prompt SHA-256 mismatch")
        free_worker = int(ssh(NIMO1, "df", "-B1", "--output=avail", "/var/tmp").stdout.splitlines()[-1])
        if free_worker < 2_000_000_000:
            raise RetryError("worker free space below 2 GB gate")
        ssh(NIMO2, "rm", "-rf", "--", REMOTE_EVIDENCE, COORDINATOR_ROOT)
        ssh(NIMO2, "install", "-d", "-m", "700", REMOTE_EVIDENCE, COORDINATOR_ROOT)
        ssh(NIMO2, "bash", "-c", f"umask 077; openssl rand -hex 64 | fold -w 64 > {CONTROL}")
        ssh(NIMO1, "rm", "-rf", "--", WORKER_ROOT)
        ssh(NIMO1, "install", "-d", "-m", "700", WORKER_ROOT)
        run(["scp", f"{NIMO2}:{CONTROL}", f"{NIMO1}:/var/tmp/halofpx-l13r-primary-control.key"])
        ssh(NIMO1, "chmod", "600", "/var/tmp/halofpx-l13r-primary-control.key")

        (root / "diskstats-nimo1-before.txt").write_text(ssh(NIMO1, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        (root / "diskstats-nimo2-before.txt").write_text(ssh(NIMO2, "cat", "/proc/diskstats").stdout, encoding="utf-8")

        unit1 = "halofpx-l13r-primary-worker-capture-20260721"
        local_units.append(unit1)
        pid_capture = start_worker(True, unit1)
        capture = canary("capture")
        write_log(root, "capture.log", capture)
        results["capture"] = output_fields(capture.stdout)
        suffixes["capture"] = fetch_suffix(root, "capture", "capture")
        capture_journal = worker_journal(unit1)
        (root / "worker-capture.log").write_text(capture_journal, encoding="utf-8")
        stop_worker(unit1)

        unit2 = "halofpx-l13r-primary-worker-restore-20260721"
        local_units.append(unit2)
        pid_restore = start_worker(True, unit2)

        cold = canary("cold")
        write_log(root, "cold.log", cold)
        results["cold"] = output_fields(cold.stdout)
        suffixes["cold"] = fetch_suffix(root, "cold", "cold")

        restore = canary("restore")
        write_log(root, "restore.log", restore)
        results["restore"] = output_fields(restore.stdout)
        suffixes["restore"] = fetch_suffix(root, "restore", "restore")

        objects = ssh(NIMO1, "find", WORKER_ROOT + "/objects", "-type", "f", "-name", "*.hfx").stdout.splitlines()
        if len(objects) != 1:
            raise RetryError(f"expected one worker object, found {len(objects)}")
        object_path = objects[0]
        object_bytes = int(ssh(NIMO1, "stat", "-c", "%s", object_path).stdout.strip())
        object_sha = ssh(NIMO1, "sha256sum", object_path).stdout.split()[0]
        ssh(NIMO1, "mv", "--", object_path, object_path + ".missing")
        try:
            missing = canary("restore")
        finally:
            ssh(NIMO1, "mv", "--", object_path + ".missing", object_path, check=False)
        write_log(root, "missing-object.log", missing)
        results["missing_object"] = output_fields(missing.stdout)
        suffixes["missing_object"] = fetch_suffix(root, "restore", "missing-object")
        if results["missing_object"].get("fallback") != "cold":
            raise RetryError("missing object did not cold fallback")

        mismatch = canary("restore", plan="f" * 64)
        write_log(root, "plan-mismatch.log", mismatch)
        results["plan_mismatch"] = output_fields(mismatch.stdout)
        suffixes["plan_mismatch"] = fetch_suffix(root, "restore", "plan-mismatch")
        if results["plan_mismatch"].get("fallback") != "cold":
            raise RetryError("plan mismatch did not cold fallback")

        restore_journal = worker_journal(unit2)
        (root / "worker-restore.log").write_text(restore_journal, encoding="utf-8")
        capture_window, restore_window = state_windows(capture_journal, restore_journal)
        (root / "capture-state-window.log").write_text("\n".join(capture_window) + "\n", encoding="utf-8")
        (root / "restore-state-window.log").write_text("\n".join(restore_window) + "\n", encoding="utf-8")
        stop_worker(unit2)

        unit3 = "halofpx-l13r-primary-worker-runtime-off-20260721"
        local_units.append(unit3)
        pid_runtime_off = start_worker(False, unit3)
        runtime_off = canary("cold")
        write_log(root, "runtime-off-cold.log", runtime_off)
        results["runtime_off"] = output_fields(runtime_off.stdout)
        suffixes["runtime_off"] = fetch_suffix(root, "cold", "runtime-off-cold")

        if len(set(suffixes.values())) != 1:
            raise RetryError(f"suffix mismatch: {suffixes}")
        cold_ms = float(results["cold"]["prompt_ms"]) + float(results["cold"]["generation_ms"])
        off_ms = float(results["runtime_off"]["prompt_ms"]) + float(results["runtime_off"]["generation_ms"])
        if cold_ms > max(off_ms * 1.5, off_ms + 50.0):
            raise RetryError(f"obvious retained cold slowdown: enabled={cold_ms} off={off_ms}")

        summary = {
            "schema": "halofpx.l13r.primary-result.v1",
            "model_sha256": MODEL_SHA,
            "model_bytes": MODEL_BYTES,
            "pids": {"capture": pid_capture, "restore": pid_restore, "runtime_off": pid_runtime_off},
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
        print(f"primary retry failed: {exc}", file=sys.stderr)
        return 1
    finally:
        cleanup_errors = []
        for unit in reversed(local_units):
            try:
                stop_worker(unit)
            except Exception as exc:
                cleanup_errors.append(f"{unit}: {exc}")
        if cleanup_errors:
            raise RetryError("; ".join(cleanup_errors))


if __name__ == "__main__":
    raise SystemExit(main())
