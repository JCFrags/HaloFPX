#!/usr/bin/env python3
"""Fail-closed controller for the fixed HaloFPX production service pair."""

from __future__ import annotations

import argparse
import atexit
import ctypes
import hashlib
import json
import os
import re
import secrets
import shlex
import shutil
import signal
import subprocess
import sys
import threading
import time
from datetime import datetime, timezone
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Protocol, Sequence


COORDINATOR_HOST = "nimo-1"
COORDINATOR_UNIT = "minimax-m27-q6-server.service"
COORDINATOR_EXEC = "/opt/llm-usb4-cluster/bin/run-minimax-m27-q6-server.sh"
COORDINATOR_PROCESS = "/opt/llm-usb4-cluster/llama/llama-server"
COORDINATOR_MODEL = (
    "/opt/llm-usb4-cluster/models/unsloth_MiniMax-M2.7-GGUF/UD-Q6_K_XL/"
    "MiniMax-M2.7-UD-Q6_K_XL-00001-of-00006.gguf"
)
COORDINATOR_PORT = 8081

WORKER_HOST = "nimo-2"
WORKER_UNIT = "minimax-m27-rpc-worker.service"
WORKER_EXEC = "/opt/llm-usb4-cluster/bin/run-minimax-m27-rpc-worker.sh"
WORKER_PROCESS = "/opt/llm-usb4-cluster/llama/ggml-rpc-server"
WORKER_PORT = 50052

DISPOSABLE_HOST = "nimo-1"
DISPOSABLE_PORT = 50184
DISPOSABLE_WORKER_UNITS = (
    "halofpx-l24-primary-worker-capture.service",
    "halofpx-l24-primary-worker-restore.service",
)
DISPOSABLE_CANARY_HOST = "nimo-2"
DISPOSABLE_CANARY_BIN = "/var/tmp/halofpx-l24-source-nimo2/build-l24/bin/test-halofpx-distributed-state-canary"
DISPOSABLE_CANARY_UNITS = ("halofpx-l24-primary-canary-diagnostic.service",)

CHANNEL_KEY_OWNER = "connorb"
CHANNEL_KEY_BYTES = 130
CHANNEL_KEY_PATHS = {
    "nimo-1": "/var/tmp/halofpx-l24-primary-control.key",
    "nimo-2": "/var/tmp/halofpx-l24-primary-control.key",
}
DISPOSABLE_PATHS = {
    "nimo-1": (
        "/var/tmp/halofpx-l24-source-nimo1.tar",
        "/var/tmp/halofpx-l24-source-nimo1",
        "/var/tmp/halofpx-l24-primary-worker",
    ),
    "nimo-2": (
        "/var/tmp/halofpx-l24-source-nimo2.tar",
        "/var/tmp/halofpx-l24-source-nimo2",
        "/var/tmp/halofpx-l24-primary-evidence",
        "/var/tmp/halofpx-l24-primary-coordinator",
        "/var/tmp/halofpx-l24-primary-rendezvous",
    ),
}
L28_KEY_PATHS = {
    "nimo-1": "/var/tmp/halofpx-l28-control.key",
    "nimo-2": "/var/tmp/halofpx-l28-control.key",
}
L28_EXECUTABLES = {
    "worker": "/var/tmp/halofpx-l28-source-nimo1/build-l28/bin/rpc-server",
    "canary": "/var/tmp/halofpx-l28-source-nimo2/build-l28/bin/test-halofpx-distributed-state-canary",
    "readiness": "/var/tmp/halofpx-l28-source-nimo2/scripts/halofpx_rpc_readiness.py",
    "placement": "/var/tmp/halofpx-l28-source-nimo2/build-l28/bin/test-halofpx-placement-probe",
    "epoch_receipt": "/var/tmp/halofpx-l28-source-nimo2/scripts/halofpx_epoch_receipt.py",
}
L29_KEY_PATHS = {
    "nimo-1": "/var/tmp/halofpx-l29-control.key",
    "nimo-2": "/var/tmp/halofpx-l29-control.key",
}
L29_EXECUTABLES = {
    "worker": "/var/tmp/halofpx-l29-source-nimo1/build-l29/bin/rpc-server",
    "canary": "/var/tmp/halofpx-l29-source-nimo2/build-l29/bin/test-halofpx-distributed-state-canary",
    "readiness": "/var/tmp/halofpx-l29-source-nimo2/scripts/halofpx_rpc_readiness.py",
    "placement": "/var/tmp/halofpx-l29-source-nimo2/build-l29/bin/test-halofpx-placement-probe",
    "epoch_receipt": "/var/tmp/halofpx-l29-source-nimo2/scripts/halofpx_epoch_receipt.py",
}
L31_KEY_PATHS = {
    "nimo-1": "/var/tmp/halofpx-l31-control.key",
    "nimo-2": "/var/tmp/halofpx-l31-control.key",
}
L31_EXECUTABLES = {
    "worker": "/var/tmp/halofpx-l31-source-nimo1/build-l31/bin/rpc-server",
    "canary": "/var/tmp/halofpx-l31-source-nimo2/build-l31/bin/test-halofpx-distributed-state-canary",
    "readiness": "/var/tmp/halofpx-l31-source-nimo2/scripts/halofpx_rpc_readiness.py",
    "placement": "/var/tmp/halofpx-l31-source-nimo2/build-l31/bin/test-halofpx-placement-probe",
    "epoch_receipt": "/var/tmp/halofpx-l31-source-nimo2/scripts/halofpx_epoch_receipt.py",
    "component_diagnostics": "/var/tmp/halofpx-l31-source-nimo1/scripts/halofpx_state_component_diagnostics.py",
}
L33_KEY_PATHS = {
    "nimo-1": "/var/tmp/halofpx-l33-control.key",
    "nimo-2": "/var/tmp/halofpx-l33-control.key",
}
L33_EXECUTABLES = {
    "worker": "/var/tmp/halofpx-l33-source-nimo1/build-l33/bin/rpc-server",
    "canary": "/var/tmp/halofpx-l33-source-nimo2/build-l33/bin/test-halofpx-distributed-state-canary",
    "readiness": "/var/tmp/halofpx-l33-source-nimo2/scripts/halofpx_rpc_readiness.py",
    "placement": "/var/tmp/halofpx-l33-source-nimo2/build-l33/bin/test-halofpx-placement-probe",
    "epoch_receipt": "/var/tmp/halofpx-l33-source-nimo2/scripts/halofpx_epoch_receipt.py",
    "component_diagnostics": "/var/tmp/halofpx-l33-source-nimo1/scripts/halofpx_state_component_diagnostics.py",
}
L36_KEY_PATHS = {
    "nimo-1": "/var/tmp/halofpx-l36-control.key",
    "nimo-2": "/var/tmp/halofpx-l36-control.key",
}
L36_EXECUTABLES = {
    "worker": "/var/tmp/halofpx-l36-source-nimo1/build-l36/bin/rpc-server",
    "canary": "/var/tmp/halofpx-l36-source-nimo2/build-l36/bin/test-halofpx-distributed-state-canary",
    "readiness": "/var/tmp/halofpx-l36-source-nimo2/scripts/halofpx_rpc_readiness.py",
    "placement": "/var/tmp/halofpx-l36-source-nimo2/build-l36/bin/test-halofpx-placement-probe",
    "epoch_receipt": "/var/tmp/halofpx-l36-source-nimo2/scripts/halofpx_epoch_receipt.py",
    "component_diagnostics": "/var/tmp/halofpx-l36-source-nimo1/scripts/halofpx_state_component_diagnostics.py",
    "semantic_verifier": "/var/tmp/halofpx-l36-source-nimo2/scripts/halofpx_semantic_provenance.py",
    "replay_authority_verifier": "/var/tmp/halofpx-l36-source-nimo2/scripts/halofpx_replay_authority.py",
}
L48_KEY_PATHS = {
    "nimo-1": "/var/tmp/halofpx-l48-control.key",
    "nimo-2": "/var/tmp/halofpx-l48-control.key",
}
L48_EXECUTABLES = {
    "worker": "/var/tmp/halofpx-l48-source-nimo1/build-l48/bin/rpc-server",
    "canary": "/var/tmp/halofpx-l48-source-nimo2/build-l48/bin/test-halofpx-distributed-state-canary",
    "readiness": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_rpc_readiness.py",
    "placement": "/var/tmp/halofpx-l48-source-nimo2/build-l48/bin/test-halofpx-placement-probe",
    "epoch_receipt": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_epoch_receipt.py",
    "component_diagnostics": "/var/tmp/halofpx-l48-source-nimo1/scripts/halofpx_state_component_diagnostics.py",
    "semantic_verifier": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_semantic_provenance.py",
    "replay_authority_verifier": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_replay_authority.py",
    "result_authority_verifier": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_result_authority.py",
    "composed_result_verifier": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_l48_composed_result.py",
    "device_receipt": "/var/tmp/halofpx-l48-source-nimo1/scripts/halofpx_l50_device_receipt.py",
    "status_verifier": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_l55_status.py",
    "response_boundary_verifier": "/var/tmp/halofpx-l48-source-nimo2/scripts/halofpx_rpc_response_boundary.py",
}
L29_MODEL = (
    "/opt/llm-usb4-cluster/models/rcmorano_saricles-minimax-m2.7-reap-172b-a10b-rocmfpx/"
    "dba517197f2854f3d362529e13abddcdcad6c10b/"
    "saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf"
)
L29_MODEL_BYTES = 159873097824
L29_MODEL_SHA256 = "96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6"
L48_SOURCE_FILES = (
    "CMakeLists.txt",
    "ggml/include/ggml-backend.h",
    "ggml/include/ggml-rpc.h",
    "ggml/src/ggml-backend.cpp",
    "ggml/src/ggml-rpc/ggml-rpc.cpp",
    "ggml/src/ggml-rpc/transport.cpp",
    "ggml/src/ggml-rpc/transport.h",
    "include/llama.h",
    "src/llama-context.cpp",
    "src/llama-context.h",
    "src/llama-graph.cpp",
    "src/llama-graph.h",
    "tests/test-halofpx-distributed-state-canary.cpp",
    "tests/test-halofpx-rpc-mutable-authority.cpp",
    "tests/test-halofpx-scheduler-authority.cpp",
    "tests/test_halofpx_rpc_response_boundary.py",
    "tools/rpc/rpc-server.cpp",
    "scripts/halofpx-l13-primary-retry.py",
    "scripts/halofpx_l48_composed_result.py",
    "scripts/halofpx_l50_device_receipt.py",
    "scripts/halofpx_l55_status.py",
    "scripts/halofpx_rpc_response_boundary.py",
)


class TransitionError(RuntimeError):
    pass


@dataclass(frozen=True)
class CommandResult:
    returncode: int
    stdout: str
    stderr: str = ""


class Runner(Protocol):
    def run(self, host: str, argv: Sequence[str], *, operation: str = "command") -> CommandResult: ...
    def run_stdin(
        self, host: str, argv: Sequence[str], stdin: bytes, *, operation: str = "command"
    ) -> CommandResult: ...


SSH_OPERATION_DEADLINES = {
    "host-key": 10.0,
    "connect": 15.0,
    "authentication": 15.0,
    "command": 30.0,
    "service-mutation": 45.0,
    "service-readiness": 30.0,
    "hfxcap2-readiness": 150.0,
    "recovery-probe": 30.0,
    "recovery-mutation": 45.0,
    "cleanup": 60.0,
    "hash": 120.0,
    "evidence": 60.0,
    "model-session": 1800.0,
}
HFXCAP2_READINESS_INNER_SECONDS = 120.0
HFXCAP2_READINESS_TEARDOWN_MARGIN_SECONDS = 30.0
HFXCAP2_READINESS_ARGV = (
    "python3", L48_EXECUTABLES["readiness"],
    "--endpoint", "10.44.0.1:50248",
    "--timeout-seconds", "120",
    "--attempt-timeout-seconds", "2",
    "--initial-backoff-seconds", "0.1",
    "--maximum-backoff-seconds", "1",
)
HFXCAP2_READINESS_FEATURE_ON_ARGV = HFXCAP2_READINESS_ARGV + (
    "--logical-rank", "1", "--world-size", "2", "--key-generation", "7",
    "--expected-channel-key-file", "/var/tmp/halofpx-l48-control.key",
)
HFXCAP2_READINESS_FEATURE_OFF_ARGV = HFXCAP2_READINESS_ARGV + (
    "--expect-feature-off",
)
HFXCAP2_DEVICE_GATE_ARGV = (
    "python3",
    "/var/tmp/halofpx-l48-source-nimo1/scripts/halofpx_rpc_readiness.py",
    "--endpoint", "127.0.0.1:50249",
    "--timeout-seconds", "120",
    "--attempt-timeout-seconds", "2",
    "--initial-backoff-seconds", "0.1",
    "--maximum-backoff-seconds", "1",
    "--logical-rank", "1", "--world-size", "2", "--key-generation", "7",
    "--expected-channel-key-file", "/var/tmp/halofpx-l48-control.key",
)
SSH_TERMINATE_GRACE_SECONDS = 2.0
SSH_EVIDENCE_LIMIT = 65536


class SshTimeoutError(RuntimeError):
    def __init__(self, host: str, operation: str, timeout: float):
        super().__init__(f"{host}: SSH {operation} timed out after {timeout:.3f}s")
        self.host = host
        self.operation = operation
        self.timeout = timeout


class SshSetupError(RuntimeError):
    def __init__(self, host: str, operation: str, detail: str):
        super().__init__(f"{host}: SSH {operation} process-group setup failed: {detail}")
        self.host = host
        self.operation = operation


class SshRunner:
    def __init__(
        self,
        evidence_root: Path,
        *,
        deadlines: dict[str, float] | None = None,
        popen_factory=subprocess.Popen,
    ):
        if not evidence_root.is_absolute():
            raise ValueError("SSH evidence root must be absolute")
        self.evidence_path = evidence_root / "ssh-operations.jsonl"
        self.deadlines = dict(SSH_OPERATION_DEADLINES if deadlines is None else deadlines)
        if set(self.deadlines) != set(SSH_OPERATION_DEADLINES):
            raise ValueError("SSH deadline operation classes are not closed")
        if any(value <= 0 for value in self.deadlines.values()):
            raise ValueError("SSH deadlines must be positive")
        self.popen_factory = popen_factory
        self._lock = threading.Lock()
        self._sequence = 0

    @staticmethod
    def _classify_failure(stderr: str) -> str:
        lowered = stderr.lower()
        if "host key verification failed" in lowered:
            return "host-key"
        if "connection timed out" in lowered or "connection refused" in lowered:
            return "connect"
        if "permission denied" in lowered or "authentication" in lowered:
            return "authentication"
        return "command"

    def _validate_operation_authority(
        self, argv: Sequence[str], operation: str
    ) -> float | None:
        try:
            remote = shlex.split(argv[0]) if len(argv) == 1 else list(argv)
        except ValueError as exc:
            raise ValueError("malformed SSH remote argv") from exc
        is_readiness = (
            len(remote) >= 2 and remote[0] == "python3"
            and Path(remote[1]).name == "halofpx_rpc_readiness.py")
        if operation != "hfxcap2-readiness":
            if is_readiness:
                raise ValueError(
                    "HFXCAP2 readiness command requires hfxcap2-readiness operation class")
            return None
        if tuple(remote) not in {
            HFXCAP2_READINESS_FEATURE_ON_ARGV,
            HFXCAP2_READINESS_FEATURE_OFF_ARGV,
            HFXCAP2_DEVICE_GATE_ARGV,
        }:
            raise ValueError("HFXCAP2 readiness argv is outside closed authority")
        outer = self.deadlines[operation]
        if (
            outer != 150.0
            or HFXCAP2_READINESS_INNER_SECONDS >= outer
            or outer - HFXCAP2_READINESS_INNER_SECONDS !=
                HFXCAP2_READINESS_TEARDOWN_MARGIN_SECONDS
        ):
            raise ValueError("HFXCAP2 readiness deadline nesting mismatch")
        return HFXCAP2_READINESS_INNER_SECONDS

    def _record(self, value: dict[str, object]) -> None:
        self.evidence_path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
        with self._lock:
            with self.evidence_path.open("a", encoding="utf-8", newline="\n") as output:
                output.write(json.dumps(value, sort_keys=True) + "\n")
                output.flush()
                os.fsync(output.fileno())

    @staticmethod
    def _create_windows_job(process: subprocess.Popen[bytes]) -> int | None:
        if os.name != "nt" or not hasattr(process, "_handle"):
            return None

        class IO_COUNTERS(ctypes.Structure):
            _fields_ = [
                ("ReadOperationCount", ctypes.c_ulonglong),
                ("WriteOperationCount", ctypes.c_ulonglong),
                ("OtherOperationCount", ctypes.c_ulonglong),
                ("ReadTransferCount", ctypes.c_ulonglong),
                ("WriteTransferCount", ctypes.c_ulonglong),
                ("OtherTransferCount", ctypes.c_ulonglong),
            ]

        class BASIC_LIMIT(ctypes.Structure):
            _fields_ = [
                ("PerProcessUserTimeLimit", ctypes.c_longlong),
                ("PerJobUserTimeLimit", ctypes.c_longlong),
                ("LimitFlags", ctypes.c_uint32),
                ("MinimumWorkingSetSize", ctypes.c_size_t),
                ("MaximumWorkingSetSize", ctypes.c_size_t),
                ("ActiveProcessLimit", ctypes.c_uint32),
                ("Affinity", ctypes.c_size_t),
                ("PriorityClass", ctypes.c_uint32),
                ("SchedulingClass", ctypes.c_uint32),
            ]

        class EXTENDED_LIMIT(ctypes.Structure):
            _fields_ = [
                ("BasicLimitInformation", BASIC_LIMIT),
                ("IoInfo", IO_COUNTERS),
                ("ProcessMemoryLimit", ctypes.c_size_t),
                ("JobMemoryLimit", ctypes.c_size_t),
                ("PeakProcessMemoryUsed", ctypes.c_size_t),
                ("PeakJobMemoryUsed", ctypes.c_size_t),
            ]

        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.CreateJobObjectW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p]
        kernel32.CreateJobObjectW.restype = ctypes.c_void_p
        kernel32.SetInformationJobObject.argtypes = [
            ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32]
        kernel32.SetInformationJobObject.restype = ctypes.c_int
        kernel32.AssignProcessToJobObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        kernel32.AssignProcessToJobObject.restype = ctypes.c_int
        kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
        kernel32.CloseHandle.restype = ctypes.c_int
        job = kernel32.CreateJobObjectW(None, None)
        if not job:
            raise OSError(ctypes.get_last_error(), "CreateJobObjectW failed")
        info = EXTENDED_LIMIT()
        info.BasicLimitInformation.LimitFlags = 0x00002000  # KILL_ON_JOB_CLOSE
        if not kernel32.SetInformationJobObject(
            job, 9, ctypes.byref(info), ctypes.sizeof(info)
        ):
            error = ctypes.get_last_error()
            kernel32.CloseHandle(job)
            raise OSError(error, "SetInformationJobObject failed")
        if not kernel32.AssignProcessToJobObject(
            ctypes.c_void_p(job), ctypes.c_void_p(int(process._handle))
        ):
            error = ctypes.get_last_error()
            kernel32.CloseHandle(job)
            raise OSError(error, "AssignProcessToJobObject failed")
        return int(job)

    @staticmethod
    def _close_windows_job(job_handle: int | None) -> None:
        if job_handle is not None:
            kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
            kernel32.CloseHandle.argtypes = [ctypes.c_void_p]
            kernel32.CloseHandle.restype = ctypes.c_int
            kernel32.CloseHandle(ctypes.c_void_p(job_handle))

    @staticmethod
    def _terminate_group(
        process: subprocess.Popen[bytes], job_handle: int | None
    ) -> tuple[bool, bool]:
        terminated = False
        killed = False
        if process.poll() is not None:
            SshRunner._close_windows_job(job_handle)
            return terminated, killed
        if os.name == "nt":
            try:
                os.kill(process.pid, signal.CTRL_BREAK_EVENT)
            except OSError:
                process.terminate()
        else:
            os.killpg(process.pid, signal.SIGTERM)
        terminated = True
        try:
            process.wait(timeout=SSH_TERMINATE_GRACE_SECONDS)
        except subprocess.TimeoutExpired:
            if os.name == "nt":
                SshRunner._close_windows_job(job_handle)
                job_handle = None
            else:
                os.killpg(process.pid, signal.SIGKILL)
            killed = True
            process.wait()
        if os.name == "nt" and job_handle is not None:
            # Even if ssh itself honored CTRL_BREAK, kill-on-close guarantees
            # no descendant assigned to its job can survive the timeout.
            SshRunner._close_windows_job(job_handle)
            killed = True
        return terminated, killed

    @staticmethod
    def _cleanup_setup_failure(
        process: subprocess.Popen[bytes],
    ) -> tuple[bool, bool, str]:
        terminated = False
        killed = False
        cleanup_detail = ""
        if os.name == "nt":
            try:
                tree_kill = subprocess.run(
                    ["taskkill", "/PID", str(process.pid), "/T", "/F"],
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    check=False, timeout=5,
                )
                killed = tree_kill.returncode == 0
                cleanup_detail = (
                    f"taskkill_rc={tree_kill.returncode} "
                    f"taskkill_stderr="
                    f"{tree_kill.stderr.decode('utf-8', errors='replace')!r}"
                )
            except Exception as cleanup_exc:
                cleanup_detail = (
                    f"taskkill_error={type(cleanup_exc).__name__}:{cleanup_exc}"
                )
            finally:
                if process.poll() is None:
                    process.terminate()
                    terminated = True
                try:
                    process.wait(timeout=SSH_TERMINATE_GRACE_SECONDS)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            if not killed:
                cleanup_detail += " descendant_cleanup_unproven"
        else:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
            killed = True
        return terminated, killed, cleanup_detail

    def _execute(
        self,
        host: str,
        argv: Sequence[str],
        *,
        operation: str,
        stdin: bytes | None,
    ) -> CommandResult:
        if operation not in self.deadlines:
            raise ValueError(f"unknown SSH operation class: {operation}")
        inner_budget = self._validate_operation_authority(argv, operation)
        timeout = self.deadlines[operation]
        remote_command = " ".join(shlex.quote(str(value)) for value in argv)
        command = [
            "ssh", "-o", "BatchMode=yes", "-o", "ConnectTimeout=10",
            "-o", "ConnectionAttempts=1", host, remote_command,
        ]
        started_wall = datetime.now(timezone.utc).isoformat()
        started_mono = time.monotonic()
        creationflags = subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0
        process = self.popen_factory(
            command,
            stdin=subprocess.PIPE if stdin is not None else subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            start_new_session=os.name != "nt",
            creationflags=creationflags,
        )
        try:
            job_handle = self._create_windows_job(process)
        except Exception as exc:
            terminated, killed, cleanup_detail = self._cleanup_setup_failure(process)
            stdout, stderr = process.communicate()
            ended_mono = time.monotonic()
            record = {
                "schema": "halofpx.ssh-operation.v1",
                "sequence": self._sequence + 1,
                "host": host,
                "operation": operation,
                "argv": list(argv),
                "started_at": started_wall,
                "ended_at": datetime.now(timezone.utc).isoformat(),
                "duration_seconds": round(ended_mono - started_mono, 6),
                "deadline_seconds": timeout,
                "pid": process.pid,
                "returncode": process.returncode,
                "timed_out": False,
                "term_sent": terminated,
                "kill_sent": killed,
                "failure_class": "process-group-setup",
                "stdout": stdout.decode("utf-8", errors="replace")[-SSH_EVIDENCE_LIMIT:],
                "stderr": (
                    stderr.decode("utf-8", errors="replace") +
                    f"\n{type(exc).__name__}: {exc}; {cleanup_detail}"
                )[-SSH_EVIDENCE_LIMIT:],
            }
            self._sequence += 1
            self._record(record)
            raise SshSetupError(host, operation, str(exc)) from exc
        timed_out = False
        terminated = False
        killed = False
        stdout = b""
        stderr = b""
        try:
            stdout, stderr = process.communicate(input=stdin, timeout=timeout)
        except subprocess.TimeoutExpired as exc:
            timed_out = True
            stdout = (exc.output or b"")[-SSH_EVIDENCE_LIMIT:]
            stderr = (exc.stderr or b"")[-SSH_EVIDENCE_LIMIT:]
            terminated, killed = self._terminate_group(process, job_handle)
            job_handle = None
            remaining_out, remaining_err = process.communicate()
            stdout = (stdout + remaining_out)[-SSH_EVIDENCE_LIMIT:]
            stderr = (stderr + remaining_err)[-SSH_EVIDENCE_LIMIT:]
        ended_mono = time.monotonic()
        if job_handle is not None:
            self._close_windows_job(job_handle)
        ended_wall = datetime.now(timezone.utc).isoformat()
        decoded_out = stdout.decode("utf-8", errors="replace")
        decoded_err = stderr.decode("utf-8", errors="replace")
        self._sequence += 1
        record = {
            "schema": "halofpx.ssh-operation.v1",
            "sequence": self._sequence,
            "host": host,
            "operation": operation,
            "argv": list(argv),
            "started_at": started_wall,
            "ended_at": ended_wall,
            "duration_seconds": round(ended_mono - started_mono, 6),
            "deadline_seconds": timeout,
            "pid": process.pid,
            "returncode": process.returncode,
            "timed_out": timed_out,
            "term_sent": terminated,
            "kill_sent": killed,
            "failure_class": (
                "timeout" if timed_out else
                None if process.returncode == 0 else self._classify_failure(decoded_err)
            ),
            "stdout": decoded_out[-SSH_EVIDENCE_LIMIT:],
            "stderr": decoded_err[-SSH_EVIDENCE_LIMIT:],
        }
        if inner_budget is not None:
            record["inner_budget_seconds"] = inner_budget
        self._record(record)
        if timed_out:
            raise SshTimeoutError(host, operation, timeout)
        return CommandResult(process.returncode, decoded_out, decoded_err)

    def run(
        self, host: str, argv: Sequence[str], *, operation: str = "command"
    ) -> CommandResult:
        return self._execute(host, argv, operation=operation, stdin=None)

    def run_stdin(
        self, host: str, argv: Sequence[str], stdin: bytes, *, operation: str = "command"
    ) -> CommandResult:
        return self._execute(host, argv, operation=operation, stdin=stdin)


@dataclass(frozen=True)
class RoleSpec:
    role: str
    host: str
    unit: str
    exec_start: str
    process: str
    port: int
    expected_process_tokens: tuple[str, ...]


@dataclass
class RoleSnapshot:
    role: str
    host: str
    hostname: str
    unit: str
    load_state: str
    active_state: str
    sub_state: str
    main_pid: int
    exec_start: str
    fragment_path: str
    process_command: str
    listener_pid: int
    listener_line: str
    start_timestamp: str
    nrestarts: int
    http_status: int | None


COORDINATOR = RoleSpec(
    role="coordinator",
    host=COORDINATOR_HOST,
    unit=COORDINATOR_UNIT,
    exec_start=COORDINATOR_EXEC,
    process=COORDINATOR_PROCESS,
    port=COORDINATOR_PORT,
    expected_process_tokens=(
        COORDINATOR_PROCESS,
        "--model", COORDINATOR_MODEL,
        "--alias", "minimax-m2.7-ud-q6-k-xl",
        "--rpc", "10.44.0.2:50052",
        "--split-mode", "layer",
        "--tensor-split", "1,1",
        "--fit", "off",
        "--no-mmap",
        "--direct-io",
        "--n-gpu-layers", "99",
        "--flash-attn", "on",
        "--ctx-size", "131072",
        "--batch-size", "4096",
        "--ubatch-size", "4096",
        "--cache-type-k", "q8_0",
        "--cache-type-v", "q8_0",
        "--parallel", "2",
        "--threads", "16",
        "--threads-batch", "16",
        "--poll", "100",
        "--prio", "2",
        "--prio-batch", "2",
        "--cache-prompt",
        "--cache-reuse", "256",
        "--cache-ram", "12288",
        "--cache-idle-slots",
        "--ctx-checkpoints", "256",
        "--checkpoint-min-step", "512",
        "--temp", "1.0",
        "--top-p", "0.95",
        "--top-k", "40",
        "--host", "0.0.0.0",
        "--port", "8081",
        "--jinja",
        "--metrics",
    ),
)
WORKER = RoleSpec(
    role="worker",
    host=WORKER_HOST,
    unit=WORKER_UNIT,
    exec_start=WORKER_EXEC,
    process=WORKER_PROCESS,
    port=WORKER_PORT,
    expected_process_tokens=(
        WORKER_PROCESS, "--host", "0.0.0.0", "--port", "50052",
    ),
)


def _parse_properties(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            result[key] = value
    return result


def validate_milestone_manifest(path: Path, runner: Runner) -> dict[str, object]:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise TransitionError(f"L22 manifest unreadable: {exc}") from exc
    expected_keys = {
        "schema", "milestone", "worker_host", "canary_host", "worker_port",
        "worker_units", "canary_units", "key_paths", "disposable_paths",
        "executables", "executable_sha256", "child_argv", "child_evidence_subdir",
    }
    l29 = isinstance(raw, dict) and raw.get("schema") == "halofpx.l29.primary-manifest.v1"
    l31 = isinstance(raw, dict) and raw.get("schema") == "halofpx.l31.primary-manifest.v1"
    l33 = isinstance(raw, dict) and raw.get("schema") == "halofpx.l33.primary-manifest.v1"
    l36 = isinstance(raw, dict) and raw.get("schema") == "halofpx.l36.primary-manifest.v1"
    l48 = isinstance(raw, dict) and raw.get("schema") == "halofpx.l48.fixture-manifest.v1"
    l48_provenance = l48 and raw.get("milestone") in {
        "l55-first-armed-prompt-discriminator",
        "l57-parent-split-identity-qualification",
        "l58-rpc-response-boundary-discriminator",
    }
    primary = l29 or l31 or l33 or l36
    if l48:
        expected_keys |= {"authority_contract", "source_identity", "build_authority"}
    if primary:
        expected_keys |= {"artifact", "allocation_plan"}
    if not isinstance(raw, dict) or set(raw) != expected_keys:
        raise TransitionError("L22 manifest field set mismatch")
    l28 = raw.get("schema") == "halofpx.l28.fixture-manifest.v1"
    if l28 or l48 or primary:
        child_path = (Path(__file__).parent / "halofpx-l13-primary-retry.py").resolve()
        interpreter_path = Path(sys.executable).resolve()
        expected_exec = {
            **(L48_EXECUTABLES if l48 else L36_EXECUTABLES if l36 else L33_EXECUTABLES if l33 else L31_EXECUTABLES if l31 else L29_EXECUTABLES if l29 else L28_EXECUTABLES),
            "interpreter": str(interpreter_path),
            "child": str(child_path),
        }
        if l48:
            expected_exec["controller"] = str(Path(__file__).resolve())
        expected_child_argv = [
            str(interpreter_path), str(child_path), "--evidence-dir",
            "{evidence_root}/child",
            "--l48-fixture" if l48 else "--l36-primary" if l36 else "--l33-primary" if l33 else "--l31-primary" if l31 else "--l29-primary" if l29 else "--l28-fixture",
        ]
        if l48:
            if raw["milestone"] in {
                    "l55-first-armed-prompt-discriminator",
                    "l58-rpc-response-boundary-discriminator"}:
                expected_child_argv += ["--l55-first-chunk"]
            expected_child_argv += ["--authority-key-file", L48_KEY_PATHS["nimo-2"]]
        prefix = "halofpx-l48" if l48 else "halofpx-l36-primary" if l36 else "halofpx-l33-primary" if l33 else "halofpx-l31-primary" if l31 else "halofpx-l29-primary" if l29 else "halofpx-l28"
        port = 50248 if l48 else 50236 if l36 else 50233 if l33 else 50191 if l31 else 50189 if l29 else 50188
        key_paths = L48_KEY_PATHS if l48 else L36_KEY_PATHS if l36 else L33_KEY_PATHS if l33 else L31_KEY_PATHS if l31 else L29_KEY_PATHS if l29 else L28_KEY_PATHS
        source_tag = "l48" if l48 else "l36" if l36 else "l33" if l33 else "l31" if l31 else "l29" if l29 else "l28"
        expected_milestone = (
            raw["milestone"] if l48 and raw["milestone"] in {
                "l50-rocm-device-admission",
                "l55-first-armed-prompt-discriminator",
                "l57-parent-split-identity-qualification",
                "l58-rpc-response-boundary-discriminator",
            }
            else "l36-primary-replay-authority-discriminator" if l36
            else "l33-primary-live-state-discriminator" if l33
            else "l31-primary-corrected-restore-confirmation" if l31
            else "l29-primary-fresh-residency-discriminator" if l29
            else "l28-fresh-residency-fixture"
        )
        if (
            raw["milestone"] != expected_milestone
            or raw["worker_host"] != "nimo-1"
            or raw["canary_host"] != "nimo-2"
            or raw["worker_port"] != port
            or tuple(raw["worker_units"]) != (
                *(("halofpx-l50-device-gate.service",) if l48 else ()),
                f"{prefix}-worker-capture.service",
                f"{prefix}-worker-restore.service",
            )
            or tuple(raw["canary_units"]) != (
                f"{prefix}-canary-capture.service",
                f"{prefix}-canary-restore.service",
            )
            or raw["key_paths"] != key_paths
            or raw["disposable_paths"] != {
                "nimo-1": [
                    f"/var/tmp/halofpx-{source_tag}-source-nimo1.tar",
                    f"/var/tmp/halofpx-{source_tag}-source-nimo1",
                    f"/var/tmp/halofpx-{source_tag}-worker",
                    *(["/var/tmp/halofpx-l50-device-gate"] if l48 else []),
                ],
                "nimo-2": [
                    f"/var/tmp/halofpx-{source_tag}-source-nimo2.tar",
                    f"/var/tmp/halofpx-{source_tag}-source-nimo2",
                    f"/var/tmp/halofpx-{source_tag}-evidence",
                    f"/var/tmp/halofpx-{source_tag}-coordinator",
                    f"/var/tmp/halofpx-{source_tag}-rendezvous",
                    *(["/var/tmp/halofpx-l50-device-gate-verify"] if l48 else []),
                ],
            }
            or raw["executables"] != expected_exec
            or raw["child_evidence_subdir"] != "child"
            or raw["child_argv"] != expected_child_argv
        ):
            raise TransitionError("L28 manifest identity/executable authority mismatch")
        if l48 and raw["authority_contract"] != {
            "features": {
                "rpc_graph": 1, "scheduler": 2, "mutable_session": 1,
                "composition": 1,
                **({"response_boundary": 1}
                   if raw["milestone"] == "l58-rpc-response-boundary-discriminator"
                   else {}),
            },
            **({
                "provenance": {
                    "schema": "halofpx.l57.binary-provenance.v1",
                    "source_root": raw["authority_contract"].get(
                        "provenance", {}).get("source_root"),
                    "build_id": raw["authority_contract"].get(
                        "provenance", {}).get("build_id"),
                },
            } if l48_provenance else {}),
            "result_schema": "halofpx.l48.composed-result.v1",
            "result_path": "/var/tmp/halofpx-l48-evidence/l48-composed-result.json",
            "expected_capture_executions": 4,
            "expected_restore_executions": 1,
            "expected_prompt_chunks": [512, 512, 104],
            "expected_replay_count": 1,
            "transport": {
                "hfxcap2_readiness_operation": "hfxcap2-readiness",
                "inner_budget_seconds": 120,
                "outer_deadline_seconds": 150,
                "teardown_evidence_margin_seconds": 30,
            },
            "evidence_publication": {
                "host": "nimo-2",
                "directory": "/var/tmp/halofpx-l48-evidence",
                "owner": "connorb",
                "directory_mode": "0700",
                "temporary_name": ".device-admission.pending",
                "final_name": "device-admission.json",
                "file_mode": "0600",
            },
        }:
            raise TransitionError("L48 authority contract mismatch")
        if l48_provenance:
            provenance = raw["authority_contract"]["provenance"]
            if (
                provenance["schema"] != "halofpx.l57.binary-provenance.v1"
                or re.fullmatch(r"[0-9a-f]{64}", provenance["source_root"]) is None
                or re.fullmatch(r"[0-9a-f]{64}", provenance["build_id"]) is None
            ):
                raise TransitionError("L55/L57 binary provenance contract mismatch")
        if l48:
            source_identity = raw["source_identity"]
            if (
                not isinstance(source_identity, dict)
                or set(source_identity) != {"schema", "files", "root_sha256"}
                or source_identity["schema"] != "halofpx.l48.source-identity.v1"
                or not isinstance(source_identity["files"], dict)
                or tuple(sorted(source_identity["files"])) != tuple(sorted(L48_SOURCE_FILES))
                or any(
                    not isinstance(value, str) or re.fullmatch(r"[0-9a-f]{64}", value) is None
                    for value in source_identity["files"].values())
            ):
                raise TransitionError("L48 source identity is malformed")
            source_root = Path(__file__).parents[1]
            canonical = bytearray()
            for relative in sorted(L48_SOURCE_FILES):
                local = source_root / relative
                if (
                    not local.is_file()
                    or hashlib.sha256(local.read_bytes()).hexdigest() !=
                        source_identity["files"][relative]
                ):
                    raise TransitionError(f"L48 source hash mismatch: {relative}")
                canonical.extend(relative.encode("utf-8") + b"\0")
                canonical.extend(source_identity["files"][relative].encode("ascii") + b"\0")
                for host, remote_root in (
                    ("nimo-1", "/var/tmp/halofpx-l48-source-nimo1"),
                    ("nimo-2", "/var/tmp/halofpx-l48-source-nimo2"),
                ):
                    remote = runner.run(
                        host, ["sha256sum", "--", f"{remote_root}/{relative}"],
                        operation="hash")
                    remote_hash = (
                        remote.stdout.split()[0]
                        if remote.returncode == 0 and remote.stdout.split() else "")
                    if remote_hash != source_identity["files"][relative]:
                        raise TransitionError(
                            f"L48 staged source hash mismatch: {host}:{relative}")
            if (
                re.fullmatch(r"[0-9a-f]{64}", str(source_identity["root_sha256"])) is None
                or hashlib.sha256(canonical).hexdigest() != source_identity["root_sha256"]
            ):
                raise TransitionError("L48 source identity root mismatch")
            build = raw["build_authority"]
            required_build = {
                "schema", "worker_configure_argv", "cmake_cache_sha256",
                "worker_binary_sha256", "ldd_sha256", "device_inventory_sha256",
                "compiler_sha256", "hipcc_sha256", "backend", "device", "gfx",
            }
            if (
                not isinstance(build, dict) or set(build) != required_build
                or build["schema"] != "halofpx.l50.rocm-build-authority.v1"
                or build["worker_configure_argv"] != [
                    "cmake", "-S", ".", "-B", "build-l48", "-DGGML_RPC=ON",
                    "-DGGML_RPC_HALOFPX_LOCAL_STATE=ON", "-DLLAMA_BUILD_TESTS=ON",
                    "-DGGML_HIP=ON", "-DAMDGPU_TARGETS=gfx1151",
                    "-DCMAKE_BUILD_TYPE=Release",
                    *([
                        "-DHALOFPX_PROVENANCE_SOURCE_ROOT=" +
                            raw["authority_contract"]["provenance"]["source_root"],
                        "-DHALOFPX_PROVENANCE_BUILD_ID=" +
                            raw["authority_contract"]["provenance"]["build_id"],
                    ] if l48_provenance
                    else []),
                ]
                or (build["backend"], build["device"], build["gfx"]) !=
                    ("ROCm", "ROCm0", "gfx1151")
                or any(
                    re.fullmatch(r"[0-9a-f]{64}", str(build[name])) is None
                    for name in (
                        "cmake_cache_sha256", "worker_binary_sha256", "ldd_sha256",
                        "device_inventory_sha256", "compiler_sha256", "hipcc_sha256"))
            ):
                raise TransitionError("L50 ROCm build authority is malformed")
            checks = (
                ("cmake_cache_sha256", ["sha256sum", "--",
                    "/var/tmp/halofpx-l48-source-nimo1/build-l48/CMakeCache.txt"], True),
                ("worker_binary_sha256", ["sha256sum", "--", L48_EXECUTABLES["worker"]], True),
                ("ldd_sha256", ["ldd", L48_EXECUTABLES["worker"]], False),
                ("device_inventory_sha256", [L48_EXECUTABLES["worker"], "--help"], False),
                ("compiler_sha256", ["c++", "--version"], False),
                ("hipcc_sha256", ["/opt/rocm/bin/hipcc", "--version"], False),
            )
            for name, command, take_first in checks:
                result = runner.run("nimo-1", command, operation="hash")
                if result.returncode != 0:
                    raise TransitionError(f"L50 {name} command failed")
                actual = (
                    result.stdout.split()[0] if take_first else
                    hashlib.sha256((
                        re.sub(
                            r"\(0x[0-9a-fA-F]+\)", "(0xADDR)", result.stdout)
                        if name == "ldd_sha256" else result.stdout + result.stderr
                    ).encode()).hexdigest()
                )
                if actual != build[name]:
                    raise TransitionError(f"L50 {name} mismatch")
            inventory = runner.run(
                "nimo-1", [L48_EXECUTABLES["worker"], "--help"],
                operation="hash")
            if (
                "found 1 ROCm devices" not in inventory.stdout + inventory.stderr
                or "Device 0:" not in inventory.stdout + inventory.stderr
                or "gfx1151" not in inventory.stdout + inventory.stderr
            ):
                raise TransitionError("L50 exact ROCm device-0/gfx1151 inventory is absent")
        if primary:
            if raw["artifact"] != {
                "host": "nimo-2", "path": L29_MODEL,
                "bytes": L29_MODEL_BYTES, "sha256": L29_MODEL_SHA256,
            } or raw["allocation_plan"] != {
                "rpc_request_bytes": 80950550528,
                "rocm_device_request_bytes": 78280456704,
                "rocm_host_request_bytes": 633802752,
                "rpc_required_bytes": 106643119104,
                "rocm_required_bytes": 104737304935,
                "reported_total_each_bytes": 133143986176,
                "minimum_rpc_margin_bytes": 26500867072,
                "minimum_rocm_margin_bytes": 28406681241,
            }:
                raise TransitionError("L29 artifact/allocation authority mismatch")
    elif (
        raw["schema"] != "halofpx.l24.primary-manifest.v1"
        or raw["milestone"] != "l24-primary-diagnostic"
        or raw["worker_host"] != DISPOSABLE_HOST
        or raw["canary_host"] != DISPOSABLE_CANARY_HOST
        or raw["worker_port"] != DISPOSABLE_PORT
        or tuple(raw["worker_units"]) != DISPOSABLE_WORKER_UNITS
        or tuple(raw["canary_units"]) != DISPOSABLE_CANARY_UNITS
        or raw["key_paths"] != CHANNEL_KEY_PATHS
        or {host: tuple(values) for host, values in raw["disposable_paths"].items()} != DISPOSABLE_PATHS
    ):
        raise TransitionError("L22 manifest identity/cleanup authority mismatch")
    if not (l28 or l48 or primary):
        child_path = (Path(__file__).parent / "halofpx-l13-primary-retry.py").resolve()
        interpreter_path = Path(sys.executable).resolve()
        expected_exec = {
            "worker": "/var/tmp/halofpx-l24-source-nimo1/build-l24/bin/rpc-server",
            "canary": DISPOSABLE_CANARY_BIN,
            "readiness": "/var/tmp/halofpx-l24-source-nimo2/scripts/halofpx_rpc_readiness.py",
            "placement": "/var/tmp/halofpx-l24-source-nimo2/build-l24/bin/test-halofpx-placement-probe",
            "interpreter": str(interpreter_path),
            "child": str(child_path),
        }
        if (
            raw["executables"] != expected_exec
            or raw["child_evidence_subdir"] != "child"
            or raw["child_argv"] != [
                str(interpreter_path), str(child_path), "--evidence-dir", "{evidence_root}/child",
            ]
        ):
            raise TransitionError("L22 manifest executable/child authority mismatch")
    hashes = raw["executable_sha256"]
    if not isinstance(hashes, dict) or set(hashes) != set(expected_exec) or any(
        not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value)
        for value in hashes.values()
    ):
        raise TransitionError("L22 manifest executable hash set malformed")
    host_for = {
        "worker": DISPOSABLE_HOST, "canary": DISPOSABLE_CANARY_HOST,
        "readiness": DISPOSABLE_CANARY_HOST, "placement": DISPOSABLE_CANARY_HOST,
    }
    if l28 or l48 or primary:
        host_for["epoch_receipt"] = DISPOSABLE_CANARY_HOST
    if l31 or l33 or l36 or l48:
        host_for["component_diagnostics"] = DISPOSABLE_HOST
    if l36 or l48:
        host_for["semantic_verifier"] = DISPOSABLE_CANARY_HOST
        host_for["replay_authority_verifier"] = DISPOSABLE_CANARY_HOST
    if l48:
        host_for["result_authority_verifier"] = DISPOSABLE_CANARY_HOST
        host_for["composed_result_verifier"] = DISPOSABLE_CANARY_HOST
        host_for["device_receipt"] = DISPOSABLE_HOST
        host_for["status_verifier"] = DISPOSABLE_CANARY_HOST
        host_for["response_boundary_verifier"] = DISPOSABLE_CANARY_HOST
    for name, host in host_for.items():
        result = runner.run(
            host, ["sha256sum", "--", expected_exec[name]], operation="hash")
        actual = result.stdout.split()[0] if result.returncode == 0 and result.stdout.split() else ""
        if actual != hashes[name]:
            raise TransitionError(f"L22 manifest {name} executable hash mismatch")
    for name in ("interpreter", "child", *(("controller",) if l48 else ())):
        executable = Path(expected_exec[name])
        if not executable.is_file() or hashlib.sha256(executable.read_bytes()).hexdigest() != hashes[name]:
            raise TransitionError(f"L22 manifest {name} hash mismatch")
    if primary:
        artifact = raw["artifact"]
        size_result = runner.run(
            artifact["host"], ["stat", "-c", "%s", "--", artifact["path"]],
            operation="hash")
        if size_result.returncode != 0 or size_result.stdout.strip() != str(L29_MODEL_BYTES):
            raise TransitionError("L29 primary artifact size mismatch")
        hash_result = runner.run(
            artifact["host"], ["sha256sum", "--", artifact["path"]],
            operation="hash")
        actual = hash_result.stdout.split()[0] if hash_result.returncode == 0 and hash_result.stdout.split() else ""
        if actual != L29_MODEL_SHA256:
            raise TransitionError("L29 primary artifact hash mismatch")
    return raw


def bind_maintenance_command(
        manifest: dict[str, object], evidence_root: Path,
        caller_command: Sequence[str]) -> list[str]:
    """Return the exact argv admitted for Popen or refuse before mutation."""
    if not evidence_root.is_absolute():
        raise TransitionError("controller evidence root is not absolute")
    if any(part == ".." for part in evidence_root.parts):
        raise TransitionError("controller evidence root contains parent traversal")
    resolved_root = evidence_root.resolve()
    if os.path.normcase(os.path.normpath(str(evidence_root))) != os.path.normcase(str(resolved_root)):
        raise TransitionError("controller evidence root resolves through a symlink")

    executables = manifest["executables"]
    assert isinstance(executables, dict)
    interpreter = Path(str(executables["interpreter"]))
    child = Path(str(executables["child"]))
    expected_child_root = resolved_root / str(manifest["child_evidence_subdir"])
    template = list(manifest["child_argv"])
    expected = [
        str(expected_child_root) if value == "{evidence_root}/child" else str(value)
        for value in template
    ]
    actual = list(caller_command)
    if actual and actual[0] == "--":
        actual.pop(0)
    if len(actual) != len(expected) or actual[2:3] != ["--evidence-dir"]:
        raise TransitionError("maintenance argv shape does not match the closed manifest")

    normalized = list(actual)
    for index, expected_path in ((0, interpreter), (1, child), (3, expected_child_root)):
        supplied = Path(actual[index])
        if not supplied.is_absolute() or any(part == ".." for part in supplied.parts):
            raise TransitionError("maintenance argv contains a relative or traversing path")
        if index in (0, 1):
            try:
                resolved = supplied.resolve(strict=True)
            except OSError as exc:
                raise TransitionError(f"maintenance executable is unavailable: {exc}") from exc
            if os.path.normcase(os.path.normpath(str(supplied))) != os.path.normcase(str(resolved)):
                raise TransitionError("maintenance executable resolves through a symlink")
        else:
            resolved = supplied.resolve()
        if os.path.normcase(str(resolved)) != os.path.normcase(str(expected_path)):
            raise TransitionError("maintenance argv path is outside manifest authority")
        normalized[index] = str(resolved)
    if normalized != expected:
        raise TransitionError("maintenance argv does not exactly match the closed manifest")
    return normalized


def prepare_l52_evidence_directories(
        manifest: dict[str, object], evidence_root: Path,
        runner: Runner) -> dict[str, object]:
    """Create the closed evidence namespace before the disposable child runs."""
    if manifest.get("schema") != "halofpx.l48.fixture-manifest.v1":
        return {}
    resolved_root = evidence_root.resolve()
    child = resolved_root / str(manifest["child_evidence_subdir"])
    if child.exists() or child.is_symlink():
        raise TransitionError("L52 child evidence directory already exists")
    child.mkdir(mode=0o700, parents=False, exist_ok=False)
    os.chmod(child, 0o700)
    if child.is_symlink() or not child.is_dir() or any(child.iterdir()):
        raise TransitionError("L52 local evidence directory admission failed")
    if os.name != "nt" and (child.stat().st_mode & 0o777) != 0o700:
        raise TransitionError("L52 local evidence directory mode mismatch")

    contract = manifest["authority_contract"]
    assert isinstance(contract, dict)
    evidence = contract.get("evidence_publication")
    expected = {
        "host": "nimo-2",
        "directory": "/var/tmp/halofpx-l48-evidence",
        "owner": "connorb",
        "directory_mode": "0700",
        "temporary_name": ".device-admission.pending",
        "final_name": "device-admission.json",
        "file_mode": "0600",
    }
    if evidence != expected:
        raise TransitionError("L52 evidence publication authority mismatch")
    remote_dir = str(expected["directory"])
    disposable = manifest["disposable_paths"]
    if remote_dir not in disposable.get("nimo-2", []):
        raise TransitionError("L52 remote evidence directory is outside cleanup authority")
    created = False
    try:
        absent = runner.run(
            "nimo-2", ["stat", "-c", "%F", "--", remote_dir], operation="command")
        if absent.returncode != 1:
            raise TransitionError("L52 remote evidence directory is preexisting")
        made = runner.run(
            "nimo-2", ["install", "-d", "-m", "0700", "--", remote_dir],
            operation="command")
        if made.returncode != 0:
            raise TransitionError("L52 remote evidence directory creation failed")
        created = True
        authority = runner.run(
            "nimo-2", ["stat", "-c", "%F|%U|%a", "--", remote_dir],
            operation="command")
        if authority.returncode != 0 or authority.stdout.strip() != "directory|connorb|700":
            raise TransitionError("L52 remote evidence directory authority mismatch")
        contents = runner.run(
            "nimo-2", ["find", remote_dir, "-mindepth", "1", "-maxdepth", "1",
                       "-print", "-quit"], operation="command")
        if contents.returncode != 0 or contents.stdout.strip():
            raise TransitionError("L52 remote evidence directory is not empty")
    except BaseException:
        if created:
            runner.run(
                "nimo-2", ["rm", "-rf", "--", remote_dir], operation="cleanup")
        try:
            child.rmdir()
        except OSError:
            pass
        raise
    record = {
        "schema": "halofpx.l52.evidence-directory-admission.v1",
        "local": {"path": str(child), "type": "directory", "mode": "0700",
                  "empty": True, "cleanup_owned": True},
        "remote": {"host": "nimo-2", "path": remote_dir, "owner": "connorb",
                   "type": "directory", "mode": "0700", "empty": True,
                   "cleanup_owned": True},
    }
    _atomic_json(resolved_root / "evidence-directory-admission.json", record)
    return record


def _pid_from_listener(line: str) -> int:
    marker = "pid="
    at = line.find(marker)
    if at < 0:
        raise TransitionError("listener has no visible PID")
    digits = []
    for char in line[at + len(marker):]:
        if not char.isdigit():
            break
        digits.append(char)
    if not digits:
        raise TransitionError("listener PID is malformed")
    return int("".join(digits))


def _command_tokens(command: str) -> list[str]:
    try:
        return shlex.split(command)
    except ValueError as exc:
        raise TransitionError(f"malformed process command: {exc}") from exc


class Controller:
    def __init__(
            self, runner: Runner, wait_seconds: float = 1.0, timeout_seconds: float = 300.0,
            disposable_paths: dict[str, tuple[str, ...]] | None = None,
            key_paths: dict[str, str] | None = None,
            disposable_authority: dict[str, object] | None = None):
        self.runner = runner
        self.wait_seconds = wait_seconds
        self.timeout_seconds = timeout_seconds
        self.snapshot: dict[str, RoleSnapshot] | None = None
        self.first_mutation = False
        self.recovery_complete = False
        self.key_digest: str | None = None
        self.disposable_paths = disposable_paths
        self.key_paths = key_paths or CHANNEL_KEY_PATHS
        self.disposable_authority = disposable_authority or {
            "worker_host": DISPOSABLE_HOST,
            "canary_host": DISPOSABLE_CANARY_HOST,
            "worker_port": DISPOSABLE_PORT,
            "worker_units": DISPOSABLE_WORKER_UNITS,
            "canary_units": DISPOSABLE_CANARY_UNITS,
            "canary_bin": DISPOSABLE_CANARY_BIN,
        }
        self.in_recovery = False

    def _operation(self, argv: Sequence[str]) -> str:
        if self.in_recovery:
            return "recovery-mutation" if list(argv[:3]) == ["sudo", "-n", "systemctl"] else "recovery-probe"
        if argv and argv[0] in {"rm", "stat"}:
            return "cleanup"
        if argv and argv[0] == "sha256sum":
            return "hash"
        if list(argv[:3]) == ["sudo", "-n", "systemctl"]:
            return "service-mutation"
        if argv and argv[0] in {"systemctl", "ss", "ps", "curl"}:
            return "service-readiness"
        if argv and argv[0] == "hostname":
            return "connect"
        return "command"

    def _run(self, host: str, argv: Sequence[str], *, allow_failure: bool = False) -> CommandResult:
        operation = self._operation(argv)
        try:
            result = self.runner.run(host, argv, operation=operation)
        except SshTimeoutError as exc:
            raise TransitionError(
                f"{host}: typed SSH timeout operation={operation} deadline={exc.timeout:.3f}s"
            ) from exc
        if result.returncode != 0 and not allow_failure:
            rendered = " ".join(argv)
            raise TransitionError(
                f"{host}: command failed ({result.returncode}): {rendered}: "
                f"{result.stderr.strip()}"
            )
        return result

    def _run_stdin(self, host: str, argv: Sequence[str], stdin: bytes) -> CommandResult:
        try:
            result = self.runner.run_stdin(host, argv, stdin, operation="command")
        except SshTimeoutError as exc:
            raise TransitionError(
                f"{host}: typed SSH timeout operation=command deadline={exc.timeout:.3f}s"
            ) from exc
        if result.returncode != 0:
            rendered = " ".join(argv)
            raise TransitionError(
                f"{host}: stdin command failed ({result.returncode}): {rendered}: "
                f"{result.stderr.strip()}"
            )
        return result

    def _validate_key(self, host: str, path: str, expected_digest: str) -> dict[str, object]:
        stat = self._run(
            host, ["stat", "-c", "%F:%U:%a:%s", "--", path], allow_failure=True
        )
        if stat.returncode != 0:
            raise TransitionError(f"{host}: channel key is missing")
        fields = stat.stdout.strip().split(":")
        if fields != ["regular file", CHANNEL_KEY_OWNER, "600", str(CHANNEL_KEY_BYTES)]:
            raise TransitionError(
                f"{host}: channel key type/owner/mode/size mismatch: {fields!r}"
            )
        digest_result = self._run(host, ["sha256sum", "--", path], allow_failure=True)
        digest = digest_result.stdout.split()[0] if digest_result.returncode == 0 and digest_result.stdout.split() else ""
        if digest != expected_digest:
            raise TransitionError(f"{host}: channel key digest mismatch")
        return {
            "host": host,
            "path": path,
            "type": fields[0],
            "owner": fields[1],
            "mode": fields[2],
            "bytes": int(fields[3]),
            "sha256": digest,
        }

    def validate_keys(self) -> dict[str, dict[str, object]]:
        if self.key_digest is None:
            raise TransitionError("channel key identity is not prepared")
        return {
            host: self._validate_key(host, path, self.key_digest)
            for host, path in self.key_paths.items()
        }

    def cleanup_keys(self) -> None:
        failures = []
        for host, path in self.key_paths.items():
            removed = self._run(host, ["rm", "-f", "--", path], allow_failure=True)
            if removed.returncode != 0:
                failures.append(f"{host}: remove failed ({removed.returncode})")
                continue
            remaining = self._run(
                host, ["stat", "-c", "%F", "--", path], allow_failure=True
            )
            if remaining.returncode == 0:
                failures.append(f"{host}: path remains")
            elif remaining.returncode != 1:
                failures.append(f"{host}: absence check failed ({remaining.returncode})")
        if failures:
            raise TransitionError(f"channel key cleanup not proven on {failures}")
        self.key_digest = None

    def prepare_keys(self) -> dict[str, object]:
        raw = secrets.token_hex(64)
        key_bytes = (raw[:64] + "\n" + raw[64:] + "\n").encode("ascii")
        if len(key_bytes) != CHANNEL_KEY_BYTES:
            raise TransitionError("internal channel key format mismatch")
        digest = hashlib.sha256(key_bytes).hexdigest()
        try:
            for host, path in self.key_paths.items():
                existing = self._run(
                    host, ["stat", "-c", "%F", "--", path], allow_failure=True
                )
                if existing.returncode == 0:
                    raise TransitionError(f"{host}: channel key path already exists")
                if existing.returncode != 1:
                    raise TransitionError(
                        f"{host}: channel key freshness check failed ({existing.returncode})"
                    )
                self._run_stdin(host, ["install", "-m", "600", "/dev/stdin", path], key_bytes)
            self.key_digest = digest
            hosts = self.validate_keys()
        except Exception:
            self.cleanup_keys()
            self.key_digest = None
            raise
        return {
            "schema": "halofpx.channel-key-preparation.v1",
            "format": "two-lowercase-hex-lines-v1",
            "bytes": CHANNEL_KEY_BYTES,
            "sha256": digest,
            "hosts": hosts,
        }

    def _listener(self, spec: RoleSpec, required: bool) -> tuple[int, str]:
        result = self._run(spec.host, ["ss", "-H", "-ltnp"])
        matches = []
        suffix = f":{spec.port}"
        for line in result.stdout.splitlines():
            fields = line.split()
            if len(fields) >= 4 and fields[3].endswith(suffix):
                matches.append(line)
        if not matches:
            if required:
                raise TransitionError(f"{spec.host}: expected port {spec.port} is not listening")
            return 0, ""
        if len(matches) != 1:
            raise TransitionError(f"{spec.host}: port {spec.port} has {len(matches)} listeners")
        pid = _pid_from_listener(matches[0])
        return pid, matches[0]

    def inspect(self, spec: RoleSpec, *, require_active: bool, require_http: bool = False) -> RoleSnapshot:
        hostname = self._run(spec.host, ["hostname"]).stdout.strip()
        if hostname != spec.host:
            raise TransitionError(
                f"host binding mismatch: SSH target {spec.host!r} reports {hostname!r}"
            )
        props = _parse_properties(self._run(spec.host, [
            "systemctl", "--system", "show", spec.unit,
            "-p", "Id", "-p", "LoadState", "-p", "ActiveState", "-p", "SubState",
            "-p", "MainPID", "-p", "ExecStart", "-p", "FragmentPath",
            "-p", "NRestarts", "-p", "ExecMainStartTimestamp",
        ]).stdout)
        required_keys = {
            "Id", "LoadState", "ActiveState", "SubState", "MainPID", "ExecStart",
            "FragmentPath", "NRestarts", "ExecMainStartTimestamp",
        }
        if not required_keys.issubset(props):
            missing = sorted(required_keys - set(props))
            raise TransitionError(f"{spec.host}: partial preflight, missing {missing}")
        if props["Id"] != spec.unit or props["LoadState"] != "loaded":
            raise TransitionError(f"{spec.host}: unit identity/load mismatch")
        expected_exec_prefix = f"{{ path={spec.exec_start} ; argv[]={spec.exec_start} ;"
        if not props["ExecStart"].startswith(expected_exec_prefix):
            raise TransitionError(f"{spec.host}: ExecStart mismatch for {spec.unit}")
        main_pid = int(props["MainPID"] or "0")
        active = props["ActiveState"] == "active" and props["SubState"] == "running"
        if require_active and (not active or main_pid <= 0):
            raise TransitionError(f"{spec.host}: {spec.unit} is not active/running")
        if not require_active and (
            props["ActiveState"] != "inactive"
            or props["SubState"] != "dead"
            or main_pid != 0
        ):
            raise TransitionError(
                f"{spec.host}: {spec.unit} is not exactly inactive/dead with MainPID=0"
            )

        process_command = ""
        listener_pid = 0
        listener_line = ""
        if require_active:
            process_command = self._run(
                spec.host, ["ps", "-p", str(main_pid), "-o", "args="],
            ).stdout.strip()
            tokens = _command_tokens(process_command)
            if tokens != list(spec.expected_process_tokens):
                raise TransitionError(f"{spec.host}: exact process command mismatch")
            listener_pid, listener_line = self._listener(spec, required=True)
            if listener_pid != main_pid:
                raise TransitionError(
                    f"{spec.host}: listener PID {listener_pid} != MainPID {main_pid}"
                )
        else:
            listener_pid, listener_line = self._listener(spec, required=False)
            if listener_pid:
                raise TransitionError(f"{spec.host}: port {spec.port} is still open")

        http_status: int | None = None
        if require_http:
            result = self._run(spec.host, [
                "curl", "-sS", "-o", "/dev/null", "-w", "%{http_code}",
                f"http://127.0.0.1:{spec.port}/health",
            ], allow_failure=True)
            try:
                http_status = int(result.stdout.strip() or "0")
            except ValueError:
                http_status = 0
            if http_status != 200:
                raise TransitionError(f"{spec.host}: HTTP health is {http_status}, expected 200")

        return RoleSnapshot(
            role=spec.role,
            host=spec.host,
            hostname=hostname,
            unit=props["Id"],
            load_state=props["LoadState"],
            active_state=props["ActiveState"],
            sub_state=props["SubState"],
            main_pid=main_pid,
            exec_start=props["ExecStart"],
            fragment_path=props["FragmentPath"],
            process_command=process_command,
            listener_pid=listener_pid,
            listener_line=listener_line,
            start_timestamp=props["ExecMainStartTimestamp"],
            nrestarts=int(props["NRestarts"]),
            http_status=http_status,
        )

    def preflight(self) -> dict[str, RoleSnapshot]:
        # Both inspections must finish before snapshot publication or mutation.
        coordinator = self.inspect(COORDINATOR, require_active=True, require_http=True)
        worker = self.inspect(WORKER, require_active=True)
        self.snapshot = {"coordinator": coordinator, "worker": worker}
        return self.snapshot

    def _mutate(self, spec: RoleSpec, verb: str) -> None:
        self.first_mutation = True
        self._run(spec.host, ["sudo", "-n", "systemctl", "--system", verb, spec.unit])

    def shutdown(self) -> None:
        if self.snapshot is None:
            raise TransitionError("shutdown requires a complete preflight snapshot")
        # This is the last executable key check before the first production mutation.
        self.validate_keys()
        self._mutate(COORDINATOR, "stop")
        self.inspect(COORDINATOR, require_active=False)
        # This second check is the executable authorization to touch the worker.
        if self._listener(COORDINATOR, required=False)[0] != 0:
            raise TransitionError("coordinator listener remains open; worker stop forbidden")
        self._mutate(WORKER, "stop")
        self.inspect(WORKER, require_active=False)

    def _wait_active(self, spec: RoleSpec, require_http: bool) -> RoleSnapshot:
        deadline = time.monotonic() + self.timeout_seconds
        last_error: Exception | None = None
        while time.monotonic() < deadline:
            try:
                return self.inspect(spec, require_active=True, require_http=require_http)
            except (TransitionError, ValueError) as exc:
                last_error = exc
                time.sleep(self.wait_seconds)
        raise TransitionError(f"timed out waiting for {spec.role}: {last_error}")

    def cleanup_disposable(self) -> None:
        worker_host = str(self.disposable_authority["worker_host"])
        canary_host = str(self.disposable_authority["canary_host"])
        worker_port = int(self.disposable_authority["worker_port"])
        worker_units = tuple(self.disposable_authority["worker_units"])
        canary_units = tuple(self.disposable_authority["canary_units"])
        canary_bin = str(self.disposable_authority["canary_bin"])
        for host in (worker_host, canary_host):
            hostname = self._run(host, ["hostname"]).stdout.strip()
            if hostname != host:
                raise TransitionError(
                    f"disposable host binding mismatch: expected {host!r}, got {hostname!r}"
                )
        unit_groups = (
            (worker_host, worker_units),
            (canary_host, canary_units),
        )
        for host, units in unit_groups:
            for unit in units:
                self._run(
                    host,
                    ["systemctl", "--user", "stop", unit],
                    allow_failure=True,
                )
        deadline = time.monotonic() + self.timeout_seconds
        last = ""
        while time.monotonic() < deadline:
            clean = True
            details = []
            for host, units in unit_groups:
                for unit in units:
                    props = _parse_properties(self._run(
                        host,
                        [
                            "systemctl", "--user", "show", unit,
                            "-p", "LoadState", "-p", "ActiveState", "-p", "SubState", "-p", "MainPID",
                        ],
                        allow_failure=True,
                    ).stdout)
                    exact = (
                        props.get("LoadState") in {"loaded", "not-found"}
                        and props.get("ActiveState") == "inactive"
                        and props.get("SubState") == "dead"
                        and props.get("MainPID") == "0"
                    )
                    clean = clean and exact
                    details.append(f"{host}/{unit}:{props}")
            listeners = self._run(worker_host, ["ss", "-H", "-ltnp"]).stdout
            port_closed = not any(
                len(line.split()) >= 4 and line.split()[3].endswith(f":{worker_port}")
                for line in listeners.splitlines()
            )
            processes = self._run(
                canary_host, ["ps", "-eo", "pid=,args="]
            ).stdout
            canary_absent = True
            for line in processes.splitlines():
                fields = line.strip().split(maxsplit=1)
                if len(fields) == 2 and _command_tokens(fields[1])[:1] == [canary_bin]:
                    canary_absent = False
                    break
            if clean and port_closed and canary_absent:
                break
            last = "; ".join(details) + (
                f"; port_closed={port_closed}; canary_absent={canary_absent}"
            )
            time.sleep(self.wait_seconds)
        else:
            raise TransitionError(f"disposable L22 cleanup not proven before recovery: {last}")
        path_errors = []
        for host, paths in (self.disposable_paths or {}).items():
            for path in paths:
                removed = self._run(host, ["rm", "-rf", "--", path], allow_failure=True)
                absent = self._run(host, ["stat", "-c", "%F", "--", path], allow_failure=True)
                if removed.returncode != 0 or absent.returncode != 1:
                    path_errors.append(f"{host}:{path}")
        if path_errors:
            raise TransitionError(f"disposable L22 path cleanup failed: {path_errors}")

    def recover(self) -> dict[str, RoleSnapshot]:
        if self.snapshot is None:
            raise TransitionError("recovery requires the preserved preflight snapshot")
        # The maintenance child can be interrupted while a systemd-run unit survives it.
        # Production recovery is forbidden until every exact L22 unit, port, and path is clean.
        self.in_recovery = True
        self.cleanup_disposable()
        self.cleanup_keys()
        self._mutate(WORKER, "start")
        worker = self._wait_active(WORKER, require_http=False)
        self._mutate(COORDINATOR, "start")
        coordinator = self._wait_active(COORDINATOR, require_http=True)
        for role, current in (("worker", worker), ("coordinator", coordinator)):
            before = self.snapshot[role]
            if current.nrestarts != before.nrestarts:
                raise TransitionError(
                    f"{role}: NRestarts changed {before.nrestarts} -> {current.nrestarts}"
                )
        self.recovery_complete = True
        return {"coordinator": coordinator, "worker": worker}


def _atomic_json(path: Path, value: object) -> None:
    if not path.is_absolute():
        raise TransitionError("snapshot/evidence path must be absolute")
    path.parent.mkdir(mode=0o700, parents=True, exist_ok=True)
    if path.exists():
        raise TransitionError(f"refusing to overwrite preserved evidence: {path}")
    temporary = path.with_name(path.name + ".tmp")
    with temporary.open("x", encoding="utf-8", newline="\n") as output:
        json.dump(value, output, indent=2, sort_keys=True)
        output.write("\n")
        output.flush()
        os.fsync(output.fileno())
    if path.exists():
        temporary.unlink(missing_ok=True)
        raise TransitionError(f"refusing to overwrite preserved evidence: {path}")
    os.rename(temporary, path)


def _snapshot_dict(snapshot: dict[str, RoleSnapshot]) -> dict[str, object]:
    return {
        "schema": "halofpx.production-transition.snapshot.v1",
        "roles": {role: asdict(value) for role, value in snapshot.items()},
    }


def child_environment(
        prepared: dict[str, object], manifest: dict[str, object]) -> dict[str, str]:
    hashes = manifest["executable_sha256"]
    if not isinstance(hashes, dict):
        raise TransitionError("validated manifest executable hashes are unavailable")
    required = ("worker", "canary", "readiness", "placement", "epoch_receipt")
    if manifest.get("schema") == "halofpx.l24.primary-manifest.v1":
        required = required[:-1]
    l48 = manifest.get("schema") == "halofpx.l48.fixture-manifest.v1"
    if manifest.get("schema") in {
        "halofpx.l31.primary-manifest.v1", "halofpx.l33.primary-manifest.v1",
        "halofpx.l36.primary-manifest.v1",
    } or l48:
        required += ("component_diagnostics",)
    if manifest.get("schema") == "halofpx.l36.primary-manifest.v1" or l48:
        required += ("semantic_verifier", "replay_authority_verifier")
    if l48:
        required += (
            "result_authority_verifier", "composed_result_verifier",
            "device_receipt")
    if any(not re.fullmatch(r"[0-9a-f]{64}", str(hashes.get(name, ""))) for name in required):
        raise TransitionError("validated manifest child hash authority is incomplete")
    environment = os.environ.copy()
    environment["HALOFPX_CHANNEL_KEY_SHA256"] = str(prepared["sha256"])
    environment["HALOFPX_L28_WORKER_SHA256"] = str(hashes["worker"])
    environment["HALOFPX_L28_CANARY_SHA256"] = str(hashes["canary"])
    environment["HALOFPX_L28_READINESS_SHA256"] = str(hashes["readiness"])
    environment["HALOFPX_L28_PLACEMENT_SHA256"] = str(hashes["placement"])
    if "epoch_receipt" in required:
        environment["HALOFPX_L28_EPOCH_RECEIPT_SHA256"] = str(hashes["epoch_receipt"])
    if "component_diagnostics" in required:
        environment[
            "HALOFPX_L36_COMPONENT_DIAGNOSTICS_SHA256"
            if manifest.get("schema") == "halofpx.l36.primary-manifest.v1"
            else "HALOFPX_L33_COMPONENT_DIAGNOSTICS_SHA256"
            if manifest.get("schema") == "halofpx.l33.primary-manifest.v1"
            else "HALOFPX_L31_COMPONENT_DIAGNOSTICS_SHA256"
        ] = str(hashes["component_diagnostics"])
    if "semantic_verifier" in required:
        prefix = "HALOFPX_L37" if l48 else "HALOFPX_L36"
        environment[f"{prefix}_SEMANTIC_VERIFIER_SHA256"] = str(hashes["semantic_verifier"])
        environment[f"{prefix}_REPLAY_AUTHORITY_VERIFIER_SHA256"] = str(
            hashes["replay_authority_verifier"])
        environment["HALOFPX_SEMANTIC_DIAGNOSTICS"] = "1"
    if l48:
        provenance = manifest["authority_contract"]["provenance"]
        environment["HALOFPX_PROVENANCE_SOURCE_ROOT"] = str(
            provenance["source_root"])
        environment["HALOFPX_PROVENANCE_BUILD_ID"] = str(
            provenance["build_id"])
        environment["HALOFPX_L37_COMPONENT_DIAGNOSTICS_SHA256"] = str(
            hashes["component_diagnostics"])
        environment["HALOFPX_L37_RESULT_AUTHORITY_VERIFIER_SHA256"] = str(
            hashes["result_authority_verifier"])
        environment["HALOFPX_COMPOSED_AUTHORITY"] = "1"
        environment["HALOFPX_RPC_GRAPH_AUTH"] = "1"
        environment["HALOFPX_RPC_MUTABLE_AUTH"] = "1"
        if manifest.get("milestone") == "l58-rpc-response-boundary-discriminator":
            environment["HALOFPX_RPC_RESPONSE_DIAGNOSTICS"] = "1"
        environment["HALOFPX_L50_DEVICE_RECEIPT_SHA256"] = str(
            hashes["device_receipt"])
    return environment


def verify_l48_child_result(
        manifest: dict[str, object], prepared: dict[str, object],
        runner: Runner) -> dict[str, object]:
    if manifest.get("schema") != "halofpx.l48.fixture-manifest.v1":
        return {}
    authority = manifest["authority_contract"]
    executables = manifest["executables"]
    result = runner.run(
        str(manifest["canary_host"]),
        [
            "python3", str(executables["composed_result_verifier"]), "verify",
            "--key-file", str(manifest["key_paths"]["nimo-2"]),
            "--record", str(authority["result_path"]),
            "--expected-key-sha256", str(prepared["sha256"]),
            "--expected-owner", CHANNEL_KEY_OWNER,
        ],
        operation="evidence",
    )
    if result.returncode != 0:
        raise TransitionError("L48 child result verification failed")
    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as exc:
        raise TransitionError("L48 child result verifier output malformed") from exc
    if (
        not isinstance(payload, dict)
        or payload.get("schema") != authority["result_schema"]
        or len(payload.get("capture", [])) != authority["expected_capture_executions"]
        or len(payload.get("restore", [])) != authority["expected_restore_executions"]
        or payload.get("prompt_chunks") != authority["expected_prompt_chunks"]
        or payload.get("replay_count") != authority["expected_replay_count"]
    ):
        raise TransitionError("L48 child result contract mismatch")
    publication = authority["evidence_publication"]
    published = f"{publication['directory']}/{publication['final_name']}"
    temporary = f"{publication['directory']}/{publication['temporary_name']}"
    published_stat = runner.run(
        str(publication["host"]),
        ["stat", "-c", "%F|%U|%a|%s", "--", published],
        operation="evidence")
    temporary_stat = runner.run(
        str(publication["host"]),
        ["stat", "-c", "%F", "--", temporary],
        operation="evidence")
    published_hash = runner.run(
        str(publication["host"]), ["sha256sum", "--", published],
        operation="evidence")
    if (
        published_stat.returncode != 0
        or not published_stat.stdout.startswith(
            f"regular file|{publication['owner']}|600|")
        or temporary_stat.returncode != 1
        or published_hash.returncode != 0
        or not published_hash.stdout.split()
    ):
        raise TransitionError("L52 device receipt publication authority mismatch")
    payload["controller_device_publication"] = {
        "schema": "halofpx.l52.controller-publication-verification.v1",
        "host": publication["host"], "path": published,
        "stat": published_stat.stdout.strip(),
        "sha256": published_hash.stdout.split()[0],
        "temporary_absent": True,
    }
    return payload


def l29_capacity_preflight(
        manifest: dict[str, object], runner: Runner, evidence_root: Path) -> dict[str, object]:
    if manifest.get("schema") not in {
        "halofpx.l29.primary-manifest.v1", "halofpx.l31.primary-manifest.v1",
        "halofpx.l33.primary-manifest.v1", "halofpx.l36.primary-manifest.v1",
    }:
        raise TransitionError("primary capacity preflight requires exact manifest authority")
    plan = manifest["allocation_plan"]
    total = int(plan["reported_total_each_bytes"])
    rpc_required = int(plan["rpc_required_bytes"])
    rocm_required = int(plan["rocm_required_bytes"])
    rpc_margin = int(plan["minimum_rpc_margin_bytes"])
    rocm_margin = int(plan["minimum_rocm_margin_bytes"])
    if rpc_required + rpc_margin != total or rocm_required + rocm_margin != total:
        raise TransitionError("L29 allocation margin arithmetic mismatch")

    hosts: dict[str, object] = {}
    for host, required in (("nimo-1", rpc_required), ("nimo-2", rocm_required)):
        memory = runner.run(
            host, ["grep", "^MemTotal:", "/proc/meminfo"], operation="service-readiness")
        match = re.fullmatch(r"MemTotal:\s+(\d+)\s+kB\s*", memory.stdout)
        if memory.returncode != 0 or match is None:
            raise TransitionError(f"{host}: current memory capacity is unavailable")
        memory_bytes = int(match.group(1)) * 1024
        if memory_bytes < total or required > memory_bytes - (
                rpc_margin if host == "nimo-1" else rocm_margin):
            raise TransitionError(f"{host}: current memory lacks frozen allocation margin")
        disk = runner.run(
            host, ["df", "-B1", "--output=avail,itotal,iavail", "/var/tmp"],
            operation="service-readiness")
        fields = disk.stdout.splitlines()[-1].split() if disk.returncode == 0 else []
        if len(fields) != 3:
            raise TransitionError(f"{host}: disk/inode capacity is unavailable")
        available, inode_total, inode_free = map(int, fields)
        if available < 8 * 1024**3:
            raise TransitionError(f"{host}: disposable disk capacity below 8 GiB")
        if inode_total > 0 and inode_free < 1000:
            raise TransitionError(f"{host}: disposable inode capacity below 1000")
        hosts[host] = {
            "mem_total_bytes": memory_bytes,
            "allocation_required_bytes": required,
            "allocation_margin_bytes": memory_bytes - required,
            "var_tmp_available_bytes": available,
            "inode_total": inode_total,
            "inode_free": inode_free,
            "inode_accounting": "fixed_pool" if inode_total > 0 else "not_reported_by_filesystem",
        }
    local = shutil.disk_usage(evidence_root.parent)
    if local.free < 2 * 1024**3:
        raise TransitionError("local evidence capacity below 2 GiB")
    return {
        "schema": "halofpx.primary.capacity-preflight.v1",
        "frozen_plan": plan,
        "hosts": hosts,
        "local_evidence_free_bytes": local.free,
    }


def _validate_snapshot_role(snapshot: RoleSnapshot, spec: RoleSpec) -> None:
    if (
        snapshot.role != spec.role
        or snapshot.host != spec.host
        or snapshot.hostname != spec.host
        or snapshot.unit != spec.unit
        or snapshot.load_state != "loaded"
        or snapshot.active_state != "active"
        or snapshot.sub_state != "running"
        or snapshot.main_pid <= 0
        or snapshot.listener_pid != snapshot.main_pid
        or not snapshot.listener_line
        or snapshot.http_status != (200 if spec.role == "coordinator" else None)
    ):
        raise TransitionError(f"preserved {spec.role} snapshot binding mismatch")
    expected_exec_prefix = f"{{ path={spec.exec_start} ; argv[]={spec.exec_start} ;"
    if not snapshot.exec_start.startswith(expected_exec_prefix):
        raise TransitionError(f"preserved {spec.role} ExecStart mismatch")
    if _command_tokens(snapshot.process_command) != list(spec.expected_process_tokens):
        raise TransitionError(f"preserved {spec.role} process command mismatch")


def _load_snapshot(path: Path) -> dict[str, RoleSnapshot]:
    raw = json.loads(path.read_text(encoding="utf-8"))
    if set(raw) != {"schema", "roles"} or raw["schema"] != "halofpx.production-transition.snapshot.v1":
        raise TransitionError("preserved snapshot schema mismatch")
    roles = raw["roles"]
    if not isinstance(roles, dict) or set(roles) != {"coordinator", "worker"}:
        raise TransitionError("preserved snapshot role set mismatch")
    snapshot = {role: RoleSnapshot(**value) for role, value in roles.items()}
    _validate_snapshot_role(snapshot["coordinator"], COORDINATOR)
    _validate_snapshot_role(snapshot["worker"], WORKER)
    return snapshot


def main(argv: Sequence[str] | None = None, *, runner: Runner | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", required=True, type=Path)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--milestone-manifest", type=Path)
    parser.add_argument("--timeout-seconds", type=float, default=300.0)
    parser.add_argument(
        "command", choices=("preflight", "prepare", "disposable", "maintenance", "recover"))
    parser.add_argument("maintenance_command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    if not args.evidence_dir.is_absolute():
        parser.error("--evidence-dir must be absolute")

    selected_runner = runner or SshRunner(args.evidence_dir)
    manifest = None
    maintenance_command = list(args.maintenance_command)
    if args.milestone_manifest is None:
        if runner is None:
            parser.error("--milestone-manifest is required for real execution")
    else:
        manifest = validate_milestone_manifest(args.milestone_manifest, selected_runner)
        if args.command in {"maintenance", "disposable"}:
            maintenance_command = bind_maintenance_command(
                manifest, args.evidence_dir, maintenance_command,
            )
    controller = Controller(
        selected_runner, timeout_seconds=args.timeout_seconds,
        disposable_paths=(
            {host: tuple(paths) for host, paths in manifest["disposable_paths"].items()}
            if manifest is not None else None),
        key_paths=(dict(manifest["key_paths"]) if manifest is not None else None),
        disposable_authority=({
            "worker_host": manifest["worker_host"],
            "canary_host": manifest["canary_host"],
            "worker_port": manifest["worker_port"],
            "worker_units": tuple(manifest["worker_units"]),
            "canary_units": tuple(manifest["canary_units"]),
            "canary_bin": manifest["executables"]["canary"],
        } if manifest is not None else None),
    )
    snapshot_path = args.evidence_dir / "production-preflight.json"
    final_path = args.evidence_dir / "production-final.json"
    recovery_running = False

    if manifest is not None and manifest.get("schema") in {
        "halofpx.l29.primary-manifest.v1", "halofpx.l31.primary-manifest.v1",
        "halofpx.l33.primary-manifest.v1", "halofpx.l36.primary-manifest.v1",
    }:
        capacity = l29_capacity_preflight(manifest, selected_runner, args.evidence_dir)
        _atomic_json(args.evidence_dir / "capacity-preflight.json", capacity)

    def emergency_recover() -> None:
        nonlocal recovery_running
        if (
            recovery_running
            or not controller.first_mutation
            or controller.recovery_complete
            or controller.snapshot is None
            or args.dry_run
        ):
            return
        recovery_running = True
        try:
            final = controller.recover()
            _atomic_json(final_path, _snapshot_dict(final))
        except Exception as exc:  # best-effort trap; original failure remains visible
            print(f"EMERGENCY RECOVERY FAILED: {exc}", file=sys.stderr)
        finally:
            recovery_running = False

    atexit.register(emergency_recover)

    def signal_handler(signum: int, _frame: object) -> None:
        raise TransitionError(f"received signal {signum}")

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    try:
        if args.command == "recover":
            if not snapshot_path.is_file():
                raise TransitionError("recover requires production-preflight.json")
            controller.snapshot = _load_snapshot(snapshot_path)
            final = controller.recover()
            _atomic_json(final_path, _snapshot_dict(final))
            return 0

        snapshot = controller.preflight()
        if args.dry_run:
            _atomic_json(args.evidence_dir / "production-dry-run.json", _snapshot_dict(snapshot))
            print(json.dumps(_snapshot_dict(snapshot), indent=2, sort_keys=True))
        else:
            _atomic_json(snapshot_path, _snapshot_dict(snapshot))
        if args.command == "preflight" or args.dry_run:
            return 0
        if args.command == "prepare":
            prepared = controller.prepare_keys()
            _atomic_json(args.evidence_dir / "key-preparation.json", prepared)
            print(json.dumps(prepared, indent=2, sort_keys=True))
            controller.cleanup_keys()
            return 0
        if args.command == "disposable":
            if manifest is None:
                raise TransitionError("disposable execution requires a closed manifest")
            cleanup_failures = []
            returncode = -1
            execution_failure = None
            child = None
            prepared = None
            try:
                prepare_l52_evidence_directories(
                    manifest, args.evidence_dir, selected_runner)
                prepared = controller.prepare_keys()
                _atomic_json(args.evidence_dir / "key-preparation.json", prepared)
                child_env = child_environment(prepared, manifest)
                child = subprocess.Popen(maintenance_command, env=child_env)
                returncode = child.wait()
                if returncode == 0:
                    verified_child = verify_l48_child_result(
                        manifest, prepared, selected_runner)
                    if verified_child:
                        _atomic_json(
                            args.evidence_dir / "verified-child-result.json",
                            verified_child)
            except BaseException as exc:
                execution_failure = exc
            finally:
                if child is not None and child.poll() is None:
                    child.terminate()
                    try:
                        child.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        child.kill()
                        child.wait()
                if controller.key_digest is not None:
                    try:
                        controller.cleanup_keys()
                    except Exception as exc:
                        cleanup_failures.append(f"keys: {exc}")
                for host, paths in controller.disposable_paths.items():
                    for path in paths:
                        try:
                            removed = controller._run(
                                host, ["rm", "-rf", "--", path], allow_failure=True)
                            absent = controller._run(
                                host, ["stat", "-c", "%F", "--", path], allow_failure=True)
                            if removed.returncode != 0 or absent.returncode != 1:
                                cleanup_failures.append(f"{host}:{path}")
                        except Exception as exc:
                            cleanup_failures.append(f"{host}:{path}: {exc}")
                try:
                    final = controller.preflight()
                    if final != snapshot:
                        cleanup_failures.append(
                            "production snapshot changed during disposable execution")
                    _atomic_json(
                        args.evidence_dir / "production-final.json",
                        _snapshot_dict(final))
                except Exception as exc:
                    cleanup_failures.append(f"production-final: {exc}")
            if returncode != 0 or execution_failure is not None or cleanup_failures:
                raise TransitionError(
                    f"disposable child={returncode} execution={execution_failure} "
                    f"cleanup={cleanup_failures}")
            return 0
        if manifest is None and maintenance_command and maintenance_command[0] == "--":
            maintenance_command.pop(0)
        if not maintenance_command:
            raise TransitionError("maintenance requires a command after --")
        prepared = controller.prepare_keys()
        _atomic_json(args.evidence_dir / "key-preparation.json", prepared)
        # Bind the exact child environment from the already validated manifest
        # before shutdown/first production mutation.
        if manifest is not None:
            child_env = child_environment(prepared, manifest)
        else:
            child_env = os.environ.copy()
            child_env["HALOFPX_CHANNEL_KEY_SHA256"] = str(prepared["sha256"])
        controller.shutdown()
        child = subprocess.Popen(maintenance_command, env=child_env)
        try:
            returncode = child.wait()
        except BaseException:
            child.terminate()
            try:
                child.wait(timeout=10)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
            raise
        if returncode != 0:
            raise TransitionError(f"maintenance command exited {returncode}")
        final = controller.recover()
        _atomic_json(final_path, _snapshot_dict(final))
        return 0
    except Exception as exc:
        print(f"transition refused/failed: {exc}", file=sys.stderr)
        if not controller.first_mutation and controller.key_digest is not None:
            try:
                controller.cleanup_keys()
            except Exception as cleanup_exc:
                print(f"PRE-MUTATION KEY CLEANUP FAILED: {cleanup_exc}", file=sys.stderr)
        emergency_recover()
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
