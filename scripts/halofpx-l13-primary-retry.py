#!/usr/bin/env python3
"""One-shot bounded L22 three-residency primary canary; controller child only."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
import time
from pathlib import Path


NIMO1 = "nimo-1"
NIMO2 = "nimo-2"
PORT = 50180
WORKER_BIN = "/var/tmp/halofpx-l22-source-nimo1/build-l22/bin/rpc-server"
CANARY_BIN = "/var/tmp/halofpx-l22-source-nimo2/build-l22/bin/test-halofpx-distributed-state-canary"
READINESS_PROBE = "/var/tmp/halofpx-l22-source-nimo2/scripts/halofpx_rpc_readiness.py"
PLACEMENT_PROBE = "/var/tmp/halofpx-l22-source-nimo2/build-l22/bin/test-halofpx-placement-probe"
PLACEMENT_PROBE_SHA = "171d41b9a2cbe6cc25589ed7afed4457e015952130d503b08c8db72b0d438d2a"
READINESS_PROBE_SHA = "f2db27e26567b33a4d4e69c5cb248cf61b63dfa3765aa218d09668225905c980"
MODEL = (
    "/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/"
    "dba517197f2854f3d362529e13abddcdcad6c10b/"
    "saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf"
)
MODEL_SHA = "96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6"
MODEL_BYTES = 159873097824
CANARY_SHA = "9ff1908f1ba402bb9de7ed78ea1705fbb3c9ee33011ccd580e094eebbfcd03e1"
WORKER_SHA = "83273c7aa7070071d6d0f64dd398e8007a97fc2f8543b2e83af06375e75a944e"
PROMPT_SHA = "f20c7c7a4137de98b991f1bfe6de27e194a93c7257d9496944e312085923143f"
PROMPT = "/var/tmp/halofpx-l13-primary-20260721/prompt.txt"
REMOTE_EVIDENCE = "/var/tmp/halofpx-l22-primary-evidence"
COORDINATOR_ROOT = "/var/tmp/halofpx-l22-primary-coordinator"
WORKER_ROOT = "/var/tmp/halofpx-l22-primary-worker"
RENDEZVOUS_ROOT = "/var/tmp/halofpx-l22-primary-rendezvous"
CONTROL = "/var/tmp/halofpx-l22-primary-control.key"
WORKER_CONTROL = CONTROL
CHANNEL_KEY_OWNER = "connorb"
CHANNEL_KEY_BYTES = 130
CHANNEL_KEY_DIGEST_ENV = "HALOFPX_CHANNEL_KEY_SHA256"
CHECKPOINT = "421016c41e1af022aa65feef9c7b9329fdc1b49ff0b1c4df4aaad10cf13bf816"
ARTIFACT_DIR = f"{COORDINATOR_ROOT}/{CHECKPOINT}"
CACHE_TYPE_K = "q8_0"
CACHE_TYPE_V = "q8_0"
FLASH_ATTN = "on"
FIXTURE_QUALIFICATION = False

MODEL_DIGEST = MODEL_SHA
COMPATIBILITY = "a8f921ae8742823eac2942004094d1d11f47962bae0607c4b2fce6ce5a81c36f"
PLAN = "0268cc6071a8d78983f6351fe45d510e767d8cd26618a8bdffc972b6655f7967"
TOPOLOGY = "09b71fe40ae05c841a5be563f6e2b27ad2529d893b9420412e5280541ae53e1f"
PLACEMENT = "d4aa0d3c14a3bec4ba5de733e00b6447f79f94d5dbeda6e3593be74ce84f917e"


class CanaryError(RuntimeError):
    pass


def run(argv, *, timeout=900, check=True):
    result = subprocess.run(
        argv, text=True, encoding="utf-8", errors="replace",
        capture_output=True, timeout=timeout, check=False,
    )
    if check and result.returncode != 0:
        raise CanaryError(
            f"command failed ({result.returncode}): {argv!r}\n{result.stdout}\n{result.stderr}"
        )
    return result


def ssh(host, *argv, timeout=900, check=True):
    remote_command = " ".join(shlex.quote(str(value)) for value in argv)
    return run(
        ["ssh", "-o", "BatchMode=yes", host, remote_command],
        timeout=timeout, check=check,
    )


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


def validate_provisioned_keys() -> str:
    expected = os.environ.get(CHANNEL_KEY_DIGEST_ENV, "")
    if not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise CanaryError("controller channel key identity is absent or malformed")
    for host, path in ((NIMO1, WORKER_CONTROL), (NIMO2, CONTROL)):
        stat = ssh(host, "stat", "-c", "%F:%U:%a:%s", "--", path, check=False)
        if stat.returncode != 0:
            raise CanaryError(f"{host}: provisioned channel key is missing")
        fields = stat.stdout.strip().split(":")
        if fields != ["regular file", CHANNEL_KEY_OWNER, "600", str(CHANNEL_KEY_BYTES)]:
            raise CanaryError(f"{host}: provisioned channel key type/owner/mode/size mismatch")
        digest = ssh(host, "sha256sum", "--", path, check=False)
        actual = digest.stdout.split()[0] if digest.returncode == 0 and digest.stdout.split() else ""
        if actual != expected:
            raise CanaryError(f"{host}: provisioned channel key digest mismatch")
    return expected


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
    placement = ssh(
        NIMO2, PLACEMENT_PROBE,
        "--hfx-expected-rpc-endpoint", f"10.44.0.1:{PORT}",
        "--rpc", f"10.44.0.1:{PORT}",
        "--device", "RPC0,ROCm0",
        "--split-mode", "layer",
        "--tensor-split", "1,1",
        "--n-gpu-layers", "999",
        timeout=30, check=False,
    )
    if placement.returncode != 0:
        raise CanaryError(f"worker {unit} failed pre-allocation placement authority: {placement.stdout}{placement.stderr}")
    try:
        placement_result = json.loads(placement.stdout)
    except json.JSONDecodeError as exc:
        raise CanaryError(f"worker {unit} returned malformed placement evidence") from exc
    if placement_result.get("admitted") is not True or placement_result.get("endpoint") != f"10.44.0.1:{PORT}":
        raise CanaryError(f"worker {unit} returned mismatched placement evidence")
    if evidence_root is not None:
        (evidence_root / f"{unit}-readiness.json").write_text(
            json.dumps(readiness_result, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n"
        )
        (evidence_root / f"{unit}-placement.json").write_text(
            json.dumps(placement_result, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n"
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
    ssh(NIMO1, "systemctl", "--user", "stop", f"{unit}.service", check=False)
    ssh(NIMO1, "systemctl", "--user", "reset-failed", f"{unit}.service", check=False)
    deadline = time.monotonic() + 30
    last = ""
    while time.monotonic() < deadline:
        is_stopped, last = stopped()
        if is_stopped:
            return
        time.sleep(1)
    raise CanaryError(f"disposable worker cleanup not verified for {unit}: {last}")


def canary_sequence(sequence: str, unit_label: str, rendezvous: bool = False):
    canary_command = [
        CANARY_BIN,
        "--hfx-mode", "capture",
        "--hfx-sequence", sequence,
        "--hfx-rendezvous-root", RENDEZVOUS_ROOT,
        "--hfx-artifact-root", COORDINATOR_ROOT,
        "--hfx-model-digest", MODEL_DIGEST,
        "--hfx-compatibility-root", COMPATIBILITY,
        "--hfx-plan-digest", PLAN,
        "--hfx-topology-digest", TOPOLOGY,
        "--hfx-placement-digest", PLACEMENT,
        "--hfx-checkpoint-digest", CHECKPOINT,
        "--hfx-control-file", CONTROL,
        "--hfx-expected-prompt-tokens", "1129",
        "--model", MODEL,
        "--rpc", f"10.44.0.1:{PORT}",
        "--device", "RPC0,ROCm0",
        "--split-mode", "layer",
        "--tensor-split", "1,1",
        "--n-gpu-layers", "999",
        "--fit", "off",
        "--no-mmap",
        "--direct-io",
        "--flash-attn", FLASH_ATTN,
        "--ctx-size", "4096",
        "--batch-size", "512",
        "--ubatch-size", "512",
        "--cache-type-k", CACHE_TYPE_K,
        "--cache-type-v", CACHE_TYPE_V,
        "--parallel", "1",
        "--threads", "16",
        "--threads-batch", "16",
        "--file", PROMPT,
        "--n-predict", "128",
        "--seed", "1234",
        "--temp", "0",
    ]
    unit = f"halofpx-l22-primary-canary-{unit_label}"
    command = [
        "systemd-run", "--user", f"--unit={unit}", "--property=RuntimeMaxSec=20min",
        "--wait", "--collect", "--pipe", *canary_command,
    ]
    invocation = "invocation=" + " ".join(canary_command) + "\ntransient_unit=" + unit + "\n"
    if not rendezvous:
        result = ssh(NIMO2, *command, timeout=1800, check=False)
    else:
        process = subprocess.Popen(
            [
                "ssh", "-o", "BatchMode=yes", NIMO2,
                " ".join(shlex.quote(str(value)) for value in command),
            ],
            text=True, encoding="utf-8", errors="replace",
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        )
        object_path = ""
        missing_path = ""
        try:
            wait_remote_file(f"{RENDEZVOUS_ROOT}/restore-ready", 1200)
            objects = ssh(NIMO1, "find", WORKER_ROOT + "/objects", "-type", "f", "-name", "*.hfx").stdout.splitlines()
            if len(objects) != 1:
                raise CanaryError(f"expected one worker object at restore rendezvous, found {len(objects)}")
            object_path = objects[0]
            missing_path = object_path + ".missing"
            ssh(NIMO1, "mv", "--", object_path, missing_path)
            ssh(NIMO2, "touch", f"{RENDEZVOUS_ROOT}/worker-object-missing")
            wait_remote_file(f"{RENDEZVOUS_ROOT}/missing-done", 1200)
            ssh(NIMO1, "mv", "--", missing_path, object_path)
            missing_path = ""
            ssh(NIMO2, "touch", f"{RENDEZVOUS_ROOT}/worker-object-restored")
            stdout, stderr = process.communicate(timeout=1800)
        except BaseException:
            if missing_path:
                ssh(NIMO1, "mv", "--", missing_path, object_path, check=False)
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            raise
        result = subprocess.CompletedProcess(command, process.returncode, stdout, stderr)
    result.stdout = invocation + result.stdout
    if result.returncode != 0:
        raise CanaryError(result.stdout + result.stderr)
    return result


def wait_remote_file(path: str, timeout_seconds: float) -> None:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        if ssh(NIMO2, "test", "-f", path, check=False).returncode == 0:
            return
        time.sleep(1)
    raise CanaryError(f"timed out waiting for residency rendezvous {path}")


def output_fields(text: str) -> dict[str, str]:
    line = next((line for line in reversed(text.splitlines()) if line.startswith("mode=")), "")
    if not line:
        raise CanaryError("canary output has no result line")
    fields = {}
    for match in re.finditer(r"(?:^| )([a-z_]+)=([^ ]+)", line):
        fields[match.group(1)] = match.group(2)
    return fields


def output_sequence(text: str) -> dict[str, dict[str, str]]:
    result = {}
    for line in text.splitlines():
        if not line.startswith("mode="):
            continue
        fields = {}
        for match in re.finditer(r"(?:^| )([a-z_]+)=([^ ]+)", line):
            fields[match.group(1)] = match.group(2)
        label = fields.get("label", "")
        if not label or label in result:
            raise CanaryError("sequence result labels are absent or duplicate")
        result[label] = fields
    return result


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
    def is_state_allocation(line: str, payload_bytes: int) -> bool:
        match = re.search(r"\[alloc_buffer\].* size: (\d+)", line)
        if not match:
            return False
        allocated = int(match.group(1))
        return payload_bytes <= allocated < payload_bytes + 65536

    capture_lines = capture.splitlines()
    stored = next(i for i, line in enumerate(capture_lines) if "[halofpx-state] stored" in line)
    stored_bytes = re.search(r"bytes=(\d+)", capture_lines[stored])
    if not stored_bytes:
        raise CanaryError("capture state byte count is absent")
    capture_payload_bytes = int(stored_bytes.group(1))
    capture_start = next(
        i for i in range(stored - 1, -1, -1)
        if is_state_allocation(capture_lines[i], capture_payload_bytes)
    )
    capture_window = capture_lines[capture_start:stored + 1]

    restore_lines = restore.splitlines()
    ready = next(i for i, line in enumerate(restore_lines) if "[halofpx-state] ready" in line)
    applied = next(i for i, line in enumerate(restore_lines[ready + 1:], ready + 1) if "[halofpx-state] apply" in line)
    ready_bytes = re.search(r"bytes=(\d+)", restore_lines[ready])
    if not ready_bytes:
        raise CanaryError("restore state byte count is absent")
    restore_payload_bytes = int(ready_bytes.group(1))
    restore_start = next(
        i for i in range(ready - 1, -1, -1)
        if is_state_allocation(restore_lines[i], restore_payload_bytes)
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
        if FIXTURE_QUALIFICATION:
            if ssh(NIMO1, "systemctl", "is-active", "minimax-m27-q6-server.service", check=False).stdout.strip() != "active":
                raise CanaryError("fixture qualification requires the production coordinator to remain active")
            if ssh(NIMO2, "systemctl", "is-active", "minimax-m27-rpc-worker.service", check=False).stdout.strip() != "active":
                raise CanaryError("fixture qualification requires the production worker to remain active")
            health = ssh(NIMO1, "curl", "-fsS", "-o", "/dev/null", "-w", "%{http_code}", "http://127.0.0.1:8081/health")
            if health.stdout.strip() != "200":
                raise CanaryError("fixture qualification requires production HTTP 200")
        else:
            if ssh(NIMO1, "systemctl", "is-active", "minimax-m27-q6-server.service", check=False).stdout.strip() != "inactive":
                raise CanaryError("production coordinator is not inactive")
            if ssh(NIMO2, "systemctl", "is-active", "minimax-m27-rpc-worker.service", check=False).stdout.strip() != "inactive":
                raise CanaryError("production worker is not inactive")
            if f":8081" in ssh(NIMO1, "ss", "-H", "-ltnp").stdout or f":50052" in ssh(NIMO2, "ss", "-H", "-ltnp").stdout:
                raise CanaryError("production listener remains open")
        channel_key_sha = validate_provisioned_keys()
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
        if ssh(NIMO2, "sha256sum", PLACEMENT_PROBE).stdout.split()[0] != PLACEMENT_PROBE_SHA:
            raise CanaryError("placement probe mismatch")
        if ssh(NIMO2, "sha256sum", PROMPT).stdout.split()[0] != PROMPT_SHA:
            raise CanaryError("prompt SHA-256 mismatch")
        free_worker = int(ssh(NIMO1, "df", "-B1", "--output=avail", "/var/tmp").stdout.splitlines()[-1])
        if free_worker < 2_000_000_000:
            raise CanaryError("worker free space below 2 GB gate")
        ssh(NIMO2, "rm", "-rf", "--", REMOTE_EVIDENCE, COORDINATOR_ROOT, RENDEZVOUS_ROOT)
        ssh(NIMO2, "install", "-d", "-m", "700", REMOTE_EVIDENCE, COORDINATOR_ROOT, RENDEZVOUS_ROOT)
        ssh(NIMO1, "rm", "-rf", "--", WORKER_ROOT)
        ssh(NIMO1, "install", "-d", "-m", "700", WORKER_ROOT)

        (root / "diskstats-nimo1-before.txt").write_text(ssh(NIMO1, "cat", "/proc/diskstats").stdout, encoding="utf-8")
        (root / "diskstats-nimo2-before.txt").write_text(ssh(NIMO2, "cat", "/proc/diskstats").stdout, encoding="utf-8")

        unit1 = "halofpx-l22-primary-worker-capture"
        local_units.append(unit1)
        pid_capture, invocation_capture, readiness_capture = start_worker(True, unit1, root)
        residency1 = canary_sequence("residency1", "residency1")
        write_log(root, "residency1.log", residency1)
        residency1_results = output_sequence(residency1.stdout)
        if set(residency1_results) != {"capture", "cold"}:
            raise CanaryError(f"residency1 result set mismatch: {residency1_results.keys()}")
        results.update(residency1_results)
        require_result(results["capture"], "capture", require_worker_state=True)
        require_result(results["cold"], "cold")
        suffixes["capture"] = fetch_suffix(root, "capture", "capture")
        suffixes["cold"] = fetch_suffix(root, "cold", "cold")
        capture_journal = worker_journal(unit1, invocation_capture, pid_capture)
        (root / "worker-capture.log").write_text(capture_journal, encoding="utf-8")
        stop_worker(unit1)

        unit2 = "halofpx-l22-primary-worker-restore"
        local_units.append(unit2)
        pid_restore, invocation_restore, readiness_restore = start_worker(True, unit2, root)

        objects = ssh(NIMO1, "find", WORKER_ROOT + "/objects", "-type", "f", "-name", "*.hfx").stdout.splitlines()
        if len(objects) != 1:
            raise CanaryError(f"expected one worker object after capture, found {len(objects)}")
        object_path = objects[0]
        object_bytes = int(ssh(NIMO1, "stat", "-c", "%s", object_path).stdout.strip())
        object_sha = ssh(NIMO1, "sha256sum", object_path).stdout.split()[0]

        residency2 = canary_sequence("residency2", "residency2", rendezvous=True)
        write_log(root, "residency2.log", residency2)
        residency2_results = output_sequence(residency2.stdout)
        if set(residency2_results) != {"restore", "missing", "plan-mismatch"}:
            raise CanaryError(f"residency2 result set mismatch: {residency2_results.keys()}")
        results["restore"] = residency2_results["restore"]
        results["missing_object"] = residency2_results["missing"]
        results["plan_mismatch"] = residency2_results["plan-mismatch"]
        require_result(results["restore"], "restore", require_worker_state=True)
        suffixes["restore"] = fetch_suffix(root, "restore", "restore")
        suffixes["missing_object"] = fetch_suffix(root, "missing", "missing-object")
        require_result(results["missing_object"], "restore", fallback_reason="worker-stage")
        suffixes["plan_mismatch"] = fetch_suffix(root, "plan-mismatch", "plan-mismatch")
        require_result(results["plan_mismatch"], "restore", fallback_reason="coordinator-artifact")

        restore_journal = worker_journal(unit2, invocation_restore, pid_restore)
        (root / "worker-restore.log").write_text(restore_journal, encoding="utf-8")
        capture_window, restore_window = state_windows(capture_journal, restore_journal)
        (root / "capture-state-window.log").write_text("\n".join(capture_window) + "\n", encoding="utf-8")
        (root / "restore-state-window.log").write_text("\n".join(restore_window) + "\n", encoding="utf-8")
        stop_worker(unit2)

        unit3 = "halofpx-l22-primary-worker-runtime-off"
        local_units.append(unit3)
        pid_runtime_off, invocation_runtime_off, readiness_runtime_off = start_worker(False, unit3, root)
        residency3 = canary_sequence("residency3", "residency3")
        write_log(root, "residency3.log", residency3)
        residency3_results = output_sequence(residency3.stdout)
        if set(residency3_results) != {"runtime-off"}:
            raise CanaryError(f"residency3 result set mismatch: {residency3_results.keys()}")
        results["runtime_off"] = residency3_results["runtime-off"]
        require_result(results["runtime_off"], "cold")
        suffixes["runtime_off"] = fetch_suffix(root, "runtime-off", "runtime-off-cold")

        if len(set(suffixes.values())) != 1:
            raise CanaryError(f"suffix mismatch: {suffixes}")
        cold_ms = float(results["cold"]["prompt_ms"]) + float(results["cold"]["generation_ms"])
        off_ms = float(results["runtime_off"]["prompt_ms"]) + float(results["runtime_off"]["generation_ms"])
        if cold_ms > max(off_ms * 1.5, off_ms + 50.0):
            raise CanaryError(f"obvious retained cold slowdown: enabled={cold_ms} off={off_ms}")

        summary = {
            "schema": "halofpx.l22.primary-result.v1",
            "fixture_qualification": FIXTURE_QUALIFICATION,
            "fixture_nonrepresentative_primary_kv_kernel": FIXTURE_QUALIFICATION,
            "channel_key_sha256": channel_key_sha,
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
        print(f"L22 primary canary failed: {exc}", file=sys.stderr)
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
