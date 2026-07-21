#!/usr/bin/env python3
"""Fail-closed controller for the fixed HaloFPX production service pair."""

from __future__ import annotations

import argparse
import atexit
import json
import os
import shlex
import signal
import subprocess
import sys
import time
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


class TransitionError(RuntimeError):
    pass


@dataclass(frozen=True)
class CommandResult:
    returncode: int
    stdout: str
    stderr: str = ""


class Runner(Protocol):
    def run(self, host: str, argv: Sequence[str]) -> CommandResult: ...


class SshRunner:
    def run(self, host: str, argv: Sequence[str]) -> CommandResult:
        result = subprocess.run(
            ["ssh", "-o", "BatchMode=yes", host, *argv],
            text=True,
            capture_output=True,
            check=False,
        )
        return CommandResult(result.returncode, result.stdout, result.stderr)


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
    def __init__(self, runner: Runner, wait_seconds: float = 1.0, timeout_seconds: float = 300.0):
        self.runner = runner
        self.wait_seconds = wait_seconds
        self.timeout_seconds = timeout_seconds
        self.snapshot: dict[str, RoleSnapshot] | None = None
        self.first_mutation = False
        self.recovery_complete = False

    def _run(self, host: str, argv: Sequence[str], *, allow_failure: bool = False) -> CommandResult:
        result = self.runner.run(host, argv)
        if result.returncode != 0 and not allow_failure:
            rendered = " ".join(argv)
            raise TransitionError(
                f"{host}: command failed ({result.returncode}): {rendered}: "
                f"{result.stderr.strip()}"
            )
        return result

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
            "systemctl", "show", spec.unit,
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
        self._run(spec.host, ["sudo", "-n", "systemctl", verb, spec.unit])

    def shutdown(self) -> None:
        if self.snapshot is None:
            raise TransitionError("shutdown requires a complete preflight snapshot")
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

    def recover(self) -> dict[str, RoleSnapshot]:
        if self.snapshot is None:
            raise TransitionError("recovery requires the preserved preflight snapshot")
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
    parser.add_argument("--timeout-seconds", type=float, default=300.0)
    parser.add_argument("command", choices=("preflight", "maintenance", "recover"))
    parser.add_argument("maintenance_command", nargs=argparse.REMAINDER)
    args = parser.parse_args(argv)
    if not args.evidence_dir.is_absolute():
        parser.error("--evidence-dir must be absolute")

    controller = Controller(runner or SshRunner(), timeout_seconds=args.timeout_seconds)
    snapshot_path = args.evidence_dir / "production-preflight.json"
    final_path = args.evidence_dir / "production-final.json"
    recovery_running = False

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
        maintenance_command = list(args.maintenance_command)
        if maintenance_command and maintenance_command[0] == "--":
            maintenance_command.pop(0)
        if not maintenance_command:
            raise TransitionError("maintenance requires a command after --")
        controller.shutdown()
        child = subprocess.run(maintenance_command, check=False)
        if child.returncode != 0:
            raise TransitionError(f"maintenance command exited {child.returncode}")
        final = controller.recover()
        _atomic_json(final_path, _snapshot_dict(final))
        return 0
    except Exception as exc:
        print(f"transition refused/failed: {exc}", file=sys.stderr)
        emergency_recover()
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
