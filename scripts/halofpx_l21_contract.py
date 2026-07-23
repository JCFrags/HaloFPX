#!/usr/bin/env python3
"""Closed no-production disposable execution/evidence contract."""

from __future__ import annotations

import hashlib
import json
import re
import shlex
import subprocess
import time
from dataclasses import asdict, dataclass
from pathlib import Path, PurePosixPath
from typing import Protocol, Sequence


class ContractError(RuntimeError):
    pass


@dataclass(frozen=True)
class Result:
    returncode: int
    stdout: str
    stderr: str = ""


class Runner(Protocol):
    def run(self, host: str, argv: Sequence[str], *, stdin: bytes | None = None) -> Result: ...


class SshRunner:
    def run(self, host: str, argv: Sequence[str], *, stdin: bytes | None = None) -> Result:
        result = subprocess.run(
            ["ssh", "-o", "BatchMode=yes", host, shlex.join(argv)],
            input=stdin, capture_output=True, check=False,
        )
        return Result(
            result.returncode,
            result.stdout.decode("utf-8", errors="replace"),
            result.stderr.decode("utf-8", errors="replace"),
        )


@dataclass(frozen=True)
class Manifest:
    milestone: str
    worker_host: str
    canary_host: str
    worker_port: int
    worker_units: tuple[str, ...]
    canary_units: tuple[str, ...]
    executables: dict[str, str]
    executable_sha256: dict[str, str]
    child_argv: tuple[str, ...]
    paths: dict[str, dict[str, str]]
    cleanup_paths: dict[str, tuple[str, ...]]
    prior_archives: tuple[str, ...]
    retained_archive: str


@dataclass(frozen=True)
class UnitIdentity:
    host: str
    unit: str
    pid: int
    invocation_id: str
    start_monotonic: int
    cursor_before: str
    wall_before_ns: int
    monotonic_before_ns: int


TOP_KEYS = {
    "schema", "milestone", "worker_host", "canary_host", "worker_port",
    "worker_units", "canary_units", "executables", "executable_sha256",
    "child_argv", "paths", "cleanup_paths", "retained_archive",
    "prior_archives",
}
EXEC_KEYS = {"worker", "canary", "readiness", "child"}
PATH_HOSTS = {"nimo-1", "nimo-2"}
PATH_KEYS = {
    "nimo-1": {"source_archive", "source_root", "build_root", "state_root", "key"},
    "nimo-2": {
        "source_archive", "source_root", "build_root", "state_root", "key",
        "evidence_root", "archive_stage", "cleanup_receipt", "model", "prompt",
    },
}


def _private_path(value: object, milestone: str, *, fixture: bool = False) -> str:
    if not isinstance(value, str) or not value.startswith("/var/tmp/"):
        raise ContractError("manifest path is not absolute/private")
    path = PurePosixPath(value)
    if ".." in path.parts or str(path) != value or value.endswith("/"):
        raise ContractError("manifest path is not canonical")
    if not fixture and not (
        value.startswith(f"/var/tmp/halofpx-{milestone}-") or
        value.startswith(f"/var/tmp/{milestone}-")
    ):
        raise ContractError("manifest path is outside milestone allowlist")
    return value


def _units(value: object, milestone: str, role: str) -> tuple[str, ...]:
    if not isinstance(value, list) or not value or len(value) != len(set(value)):
        raise ContractError(f"{role} unit set is missing or duplicate")
    prefix = f"halofpx-{milestone}-{role}-"
    result = tuple(value)
    if any(not isinstance(v, str) or not v.startswith(prefix) or not v.endswith(".service") for v in result):
        raise ContractError(f"{role} unit outside allowlist")
    return result


def load_manifest(path: Path) -> Manifest:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ContractError(f"manifest unreadable: {exc}") from exc
    if not isinstance(raw, dict) or set(raw) != TOP_KEYS or raw["schema"] != "halofpx.l21.manifest.v1":
        raise ContractError("manifest schema/field set mismatch")
    milestone = raw["milestone"]
    if milestone != "l21-small":
        raise ContractError("manifest milestone outside allowlist")
    if raw["worker_host"] != "nimo-1" or raw["canary_host"] != "nimo-2":
        raise ContractError("manifest host roles are swapped or unknown")
    if raw["worker_port"] != 50198:
        raise ContractError("manifest port outside allowlist")
    worker_units = _units(raw["worker_units"], milestone, "worker")
    canary_units = _units(raw["canary_units"], milestone, "canary")
    if set(worker_units) & set(canary_units):
        raise ContractError("manifest unit identities overlap")
    executables = raw["executables"]
    hashes = raw["executable_sha256"]
    if not isinstance(executables, dict) or set(executables) != EXEC_KEYS:
        raise ContractError("manifest executable set mismatch")
    if not isinstance(hashes, dict) or set(hashes) != EXEC_KEYS:
        raise ContractError("manifest executable hash set mismatch")
    checked_exec = {k: _private_path(v, milestone) for k, v in executables.items() if k != "child"}
    child = executables["child"]
    if child != "scripts/halofpx-l21-refusal.py":
        raise ContractError("manifest child outside allowlist")
    checked_exec["child"] = child
    if any(not isinstance(v, str) or not re.fullmatch(r"[0-9a-f]{64}", v) for v in hashes.values()):
        raise ContractError("manifest executable hash invalid")
    argv = raw["child_argv"]
    if not isinstance(argv, list) or tuple(argv) != (
        "python", "scripts/halofpx-l21-refusal.py", "--manifest",
        "scripts/halofpx-l21-small-manifest.json",
    ):
        raise ContractError("manifest child argv mismatch")
    paths = raw["paths"]
    if not isinstance(paths, dict) or set(paths) != PATH_HOSTS:
        raise ContractError("manifest path host set mismatch")
    checked_paths: dict[str, dict[str, str]] = {}
    all_mutable: list[str] = []
    for host in PATH_HOSTS:
        values = paths[host]
        if not isinstance(values, dict) or set(values) != PATH_KEYS[host]:
            raise ContractError(f"manifest {host} path set mismatch")
        checked_paths[host] = {}
        for name, value in values.items():
            fixture = name in {"model", "prompt"}
            checked = _private_path(value, milestone, fixture=fixture)
            checked_paths[host][name] = checked
            if not fixture:
                all_mutable.append(f"{host}:{checked}")
    if len(all_mutable) != len(set(all_mutable)):
        raise ContractError("manifest mutable paths overlap")
    cleanup = raw["cleanup_paths"]
    if not isinstance(cleanup, dict) or set(cleanup) != PATH_HOSTS:
        raise ContractError("manifest cleanup host set mismatch")
    checked_cleanup: dict[str, tuple[str, ...]] = {}
    for host in PATH_HOSTS:
        values = cleanup[host]
        if not isinstance(values, list) or len(values) != len(set(values)):
            raise ContractError("manifest cleanup path set duplicate")
        checked_cleanup[host] = tuple(_private_path(v, milestone) for v in values)
        required = {
            value for name, value in checked_paths[host].items()
            if name not in {"model", "prompt"}
        }
        if set(checked_cleanup[host]) != required:
            raise ContractError("manifest cleanup paths do not exactly cover mutable roots")
    archive = _private_path(raw["retained_archive"], milestone)
    prior_raw = raw["prior_archives"]
    if not isinstance(prior_raw, list) or len(prior_raw) != len(set(prior_raw)):
        raise ContractError("prior archive set is malformed")
    prior = tuple(_private_path(v, milestone) for v in prior_raw)
    archives = (*prior, archive)
    if len(archives) != len(set(archives)):
        raise ContractError("retained archive identities overlap")
    if any(v in {p for values in checked_cleanup.values() for p in values} for v in archives):
        raise ContractError("retained archive is a cleanup target")
    return Manifest(
        milestone, raw["worker_host"], raw["canary_host"], raw["worker_port"],
        worker_units, canary_units, checked_exec, dict(hashes), tuple(argv),
        checked_paths, checked_cleanup, prior, archive,
    )


def _props(text: str) -> dict[str, str]:
    return dict(line.split("=", 1) for line in text.splitlines() if "=" in line)


def _journal_cursor(text: str) -> str:
    matches = re.findall(r"^-- cursor: (\S+)$", text, re.MULTILINE)
    if len(matches) != 1:
        raise ContractError("journal cursor output is malformed or ambiguous")
    return matches[0]


class EvidenceCollector:
    def __init__(self, runner: Runner, manifest: Manifest):
        self.runner = runner
        self.manifest = manifest
        self.failures: list[str] = []

    def required(self, host: str, argv: Sequence[str], *, stdin: bytes | None = None) -> Result:
        result = self.runner.run(host, argv, stdin=stdin)
        if result.returncode != 0:
            raise ContractError(f"{host}: evidence command failed {list(argv)!r}: {result.stderr}")
        return result

    def write(self, name: str, data: bytes) -> None:
        root = self.manifest.paths[self.manifest.canary_host]["evidence_root"]
        target = f"{root}/{name}"
        self.required(self.manifest.canary_host, ["install", "-m", "600", "/dev/stdin", target], stdin=data)
        stat = self.required(self.manifest.canary_host, ["stat", "-c", "%F:%a:%s", "--", target]).stdout.strip()
        expected = f"regular file:600:{len(data)}"
        if stat != expected:
            raise ContractError(f"evidence write verification mismatch for {name}")

    def begin_unit(self, host: str, unit: str, command: Sequence[str]) -> UnitIdentity:
        allowed = self.manifest.worker_units if host == self.manifest.worker_host else self.manifest.canary_units
        if unit not in allowed:
            raise ContractError("unit outside manifest authority")
        clock = self.required(host, [
            "python3", "-c",
            "import time; print(f'{time.time_ns()} {time.monotonic_ns()}')",
        ]).stdout.split()
        if len(clock) != 2 or not all(value.isdigit() for value in clock):
            raise ContractError("remote clock lower bound is malformed")
        wall, mono = map(int, clock)
        cursor_output = self.required(
            host, ["journalctl", "--user", "-n", "0", "--show-cursor", "--no-pager"]
        ).stdout
        cursor = _journal_cursor(cursor_output)
        disk = self.required(host, ["cat", "/proc/diskstats"]).stdout
        self.write(f"{unit}.cursor-before.txt", (cursor + "\n").encode())
        self.write(f"{unit}.disk-before.txt", disk.encode())
        start = self.required(host, ["systemd-run", "--user", f"--unit={unit.removesuffix('.service')}", "--", *command])
        self.write(f"{unit}.start.txt", (start.stdout + start.stderr).encode())
        deadline = time.monotonic() + 10
        last = {}
        while time.monotonic() < deadline:
            show = self.required(host, [
                "systemctl", "--user", "show", unit, "-p", "ExecMainPID",
                "-p", "InvocationID", "-p", "ExecMainStartTimestampMonotonic",
            ])
            last = _props(show.stdout)
            pid = int(last.get("ExecMainPID", "0") or "0")
            invocation = last.get("InvocationID", "")
            start_mono = int(last.get("ExecMainStartTimestampMonotonic", "0") or "0")
            if pid > 0 and re.fullmatch(r"[0-9a-fA-F]{32}", invocation) and start_mono > 0:
                identity = UnitIdentity(host, unit, pid, invocation.lower(), start_mono, cursor, wall, mono)
                self.write(f"{unit}.identity.json", (json.dumps(asdict(identity), sort_keys=True) + "\n").encode())
                return identity
            time.sleep(0.05)
        raise ContractError(f"unit identity was not captured before collection: {last}")

    def finish_unit(self, identity: UnitIdentity, *, require_refusal: bool = False) -> dict[str, object]:
        show = self.required(identity.host, [
            "systemctl", "--user", "show", identity.unit, "-p", "ExecMainPID",
            "-p", "InvocationID", "-p", "ExecMainStatus", "-p", "Result",
            "-p", "ExecMainExitTimestampMonotonic",
        ])
        props = _props(show.stdout)
        if int(props.get("ExecMainPID", "0") or "0") != identity.pid:
            raise ContractError("unit PID changed before evidence capture")
        if props.get("InvocationID", "").lower() != identity.invocation_id:
            raise ContractError("unit InvocationID changed before evidence capture")
        exit_mono = int(props.get("ExecMainExitTimestampMonotonic", "0") or "0")
        if exit_mono and exit_mono < identity.start_monotonic:
            raise ContractError("unit exit monotonic timestamp predates start")
        journal = self.required(identity.host, [
            "journalctl", "--user", "-u", identity.unit,
            f"_SYSTEMD_INVOCATION_ID={identity.invocation_id}", "--after-cursor", identity.cursor_before,
            "--no-pager", "-o", "short-monotonic",
        ]).stdout
        if not journal.strip():
            raise ContractError("InvocationID/cursor-bound journal is empty")
        relevant = [line for line in journal.splitlines() if
                    "[alloc_buffer]" in line.lower() or "failed to allocate" in line.lower() or
                    "allocation refusal" in line.lower() or
                    ("allocating" in line.lower() and "failed" in line.lower())]
        disk = self.required(identity.host, ["cat", "/proc/diskstats"]).stdout
        self.write(f"{identity.unit}.journal.txt", journal.encode())
        self.write(f"{identity.unit}.disk-after.txt", disk.encode())
        if require_refusal and (not relevant or not all(re.search(rf"\[{identity.pid}\]:", line) for line in relevant)):
            raise ContractError("allocation refusal journal is not PID-bound")
        result = {
            "pid": identity.pid, "invocation_id": identity.invocation_id,
            "status": int(props.get("ExecMainStatus", "-1") or "-1"),
            "result": props.get("Result", ""), "refusal_lines": len(relevant),
            "wall_lower_bound_ns": identity.wall_before_ns,
            "monotonic_lower_bound_ns": identity.monotonic_before_ns,
            "unit_start_monotonic_us": identity.start_monotonic,
            "unit_exit_monotonic_us": exit_mono,
        }
        self.write(f"{identity.unit}.exit.json", (json.dumps(result, sort_keys=True) + "\n").encode())
        return result

    def wait_unit(self, identity: UnitIdentity, timeout_seconds: float) -> None:
        deadline = time.monotonic() + timeout_seconds
        while time.monotonic() < deadline:
            show = self.required(identity.host, [
                "systemctl", "--user", "show", identity.unit, "-p", "ActiveState",
                "-p", "ExecMainPID", "-p", "InvocationID",
            ])
            props = _props(show.stdout)
            if int(props.get("ExecMainPID", "0") or "0") != identity.pid or \
                    props.get("InvocationID", "").lower() != identity.invocation_id:
                raise ContractError("unit identity changed while waiting")
            if props.get("ActiveState") in {"inactive", "failed"}:
                return
            time.sleep(0.05)
        raise ContractError("unit timeout")

    def validate_executables(self) -> None:
        hosts = {
            "worker": self.manifest.worker_host,
            "canary": self.manifest.canary_host,
            "readiness": self.manifest.canary_host,
        }
        for name, host in hosts.items():
            path = self.manifest.executables[name]
            digest = self.required(host, ["sha256sum", "--", path]).stdout.split()
            stat = self.required(host, ["stat", "-c", "%F:%a", "--", path]).stdout.strip()
            if not digest or digest[0] != self.manifest.executable_sha256[name] or \
                    stat.split(":")[0] != "regular file":
                raise ContractError(f"{name} executable identity mismatch")
        child = Path(self.manifest.executables["child"])
        if not child.is_file() or hashlib.sha256(child.read_bytes()).hexdigest() != self.manifest.executable_sha256["child"]:
            raise ContractError("maintenance child identity mismatch")

    def cleanup_runtime(self) -> dict[str, object]:
        errors = []
        for host, units in (
            (self.manifest.worker_host, self.manifest.worker_units),
            (self.manifest.canary_host, self.manifest.canary_units),
        ):
            for unit in units:
                result = self.runner.run(host, ["systemctl", "--user", "stop", unit])
                reset = self.runner.run(host, ["systemctl", "--user", "reset-failed", unit])
                show = self.runner.run(host, [
                    "systemctl", "--user", "show", unit, "-p", "LoadState",
                    "-p", "ActiveState", "-p", "MainPID",
                ])
                props = _props(show.stdout)
                if show.returncode != 0 or props.get("ActiveState") != "inactive" or props.get("MainPID") != "0":
                    errors.append(f"{host}/{unit}: remains")
                if result.returncode != 0 and props.get("LoadState") != "not-found":
                    errors.append(f"{host}/{unit}: stop")
                if reset.returncode != 0 and props.get("LoadState") != "not-found":
                    errors.append(f"{host}/{unit}: reset-failed")
        listener = self.runner.run(self.manifest.worker_host, ["ss", "-H", "-ltnp"])
        if listener.returncode != 0 or any(
            len(line.split()) >= 4 and line.split()[3].endswith(f":{self.manifest.worker_port}")
            for line in listener.stdout.splitlines()
        ):
            errors.append("disposable port remains")
        packaging = {
            self.manifest.paths[self.manifest.canary_host]["evidence_root"],
            self.manifest.paths[self.manifest.canary_host]["archive_stage"],
            self.manifest.paths[self.manifest.canary_host]["cleanup_receipt"],
        }
        for host, paths in self.manifest.cleanup_paths.items():
            for path in paths:
                if path in packaging:
                    continue
                removed = self.runner.run(host, ["rm", "-rf", "--", path])
                absent = self.runner.run(host, ["stat", "-c", "%F", "--", path])
                if removed.returncode != 0 or absent.returncode != 1:
                    errors.append(f"{host}/{path}: cleanup")
        return {
            "schema": "halofpx.l21.runtime-cleanup.v1",
            "errors": errors,
            "units_inactive": not any("remains" in error for error in errors),
            "port_closed": "disposable port remains" not in errors,
            "runtime_paths_absent": not any(error.endswith(": cleanup") for error in errors),
        }

    def cleanup(self) -> None:
        receipt = self.cleanup_runtime()
        if receipt["errors"]:
            raise ContractError(f"cleanup verification failed: {receipt['errors']}")

    def path_exists(self, host: str, path: str) -> bool:
        result = self.runner.run(host, ["stat", "-c", "%F", "--", path])
        if result.returncode not in {0, 1}:
            raise ContractError(f"{host}: mandatory path probe failed for {path}: {result.stderr}")
        return result.returncode == 0

    def finalize_archive(self, final_receipt: dict[str, object]) -> dict[str, object]:
        host = self.manifest.canary_host
        root = self.manifest.paths[host]["evidence_root"]
        stage = self.manifest.paths[host]["archive_stage"]
        cleanup_receipt = self.manifest.paths[host]["cleanup_receipt"]
        archive = self.manifest.retained_archive
        parent = str(PurePosixPath(root).parent)
        name = PurePosixPath(root).name
        if not self.path_exists(host, root):
            raise ContractError("evidence root is absent before finalization")
        self.required(host, ["tar", "-cf", stage, "-C", parent, name])
        self.required(host, ["chmod", "600", stage])
        self.required(host, ["rm", "-rf", "--", root])
        if self.path_exists(host, root):
            raise ContractError("evidence root remains after cleanup")
        final_receipt = dict(final_receipt)
        final_receipt.update({
            "schema": "halofpx.l21.finalization.v1",
            "evidence_root_absent": True,
            "archive_stage_planned_absent": True,
            "cleanup_receipt_planned_absent": True,
        })
        payload = (json.dumps(final_receipt, indent=2, sort_keys=True) + "\n").encode()
        self.required(host, ["install", "-m", "600", "/dev/stdin", cleanup_receipt], stdin=payload)
        self.required(host, [
            "tar", "-rf", stage, "-C", str(PurePosixPath(cleanup_receipt).parent),
            PurePosixPath(cleanup_receipt).name,
        ])
        self.required(host, ["zstd", "-q", "-f", stage, "-o", archive])
        self.required(host, ["chmod", "600", archive])
        self.required(host, ["rm", "-f", "--", stage, cleanup_receipt])
        if self.path_exists(host, stage) or self.path_exists(host, cleanup_receipt):
            raise ContractError("archive packaging intermediates remain")
        digest_line = self.required(host, ["sha256sum", "--", archive]).stdout.split()
        stat = self.required(host, ["stat", "-c", "%F:%a:%s", "--", archive]).stdout.strip().split(":")
        if len(digest_line) < 1 or not re.fullmatch(r"[0-9a-f]{64}", digest_line[0]):
            raise ContractError("archive hash output invalid")
        if len(stat) != 3 or stat[0] != "regular file" or stat[1] != "600" or int(stat[2]) <= 0:
            raise ContractError("archive integrity/mode mismatch")
        return {"path": archive, "sha256": digest_line[0], "bytes": int(stat[2]), "mode": "600"}


def production_snapshot(runner: Runner) -> dict[str, object]:
    specs = {
        "coordinator": ("nimo-1", "minimax-m27-q6-server.service", 8081),
        "worker": ("nimo-2", "minimax-m27-rpc-worker.service", 50052),
    }
    result: dict[str, object] = {}
    for role, (host, unit, port) in specs.items():
        hostname = runner.run(host, ["hostname"])
        show = runner.run(host, [
            "systemctl", "show", unit, "-p", "Id", "-p", "ActiveState", "-p", "SubState",
            "-p", "MainPID", "-p", "NRestarts", "-p", "ExecMainStartTimestamp", "-p", "ExecStart",
        ])
        listeners = runner.run(host, ["ss", "-H", "-ltnp"])
        if hostname.returncode or show.returncode or listeners.returncode or hostname.stdout.strip() != host:
            raise ContractError("production snapshot command/hostname failure")
        props = _props(show.stdout)
        if props.get("Id") != unit or props.get("ActiveState") != "active" or props.get("SubState") != "running":
            raise ContractError("production unit is not active/running")
        pid = int(props.get("MainPID", "0") or "0")
        matches = [line for line in listeners.stdout.splitlines()
                   if len(line.split()) >= 4 and line.split()[3].endswith(f":{port}")]
        if pid <= 0 or len(matches) != 1 or f"pid={pid}," not in matches[0]:
            raise ContractError("production listener/PID mismatch")
        command = runner.run(host, ["ps", "-p", str(pid), "-o", "args="])
        if command.returncode or not command.stdout.strip():
            raise ContractError("production process command unavailable")
        http = None
        if role == "coordinator":
            health = runner.run(host, ["curl", "-sS", "-o", "/dev/null", "-w", "%{http_code}",
                                       "http://127.0.0.1:8081/health"])
            if health.returncode or health.stdout.strip() != "200":
                raise ContractError("production HTTP health mismatch")
            http = 200
        result[role] = {
            "host": host, "unit": unit, "pid": pid, "port": port,
            "nrestarts": int(props.get("NRestarts", "-1")),
            "start": props.get("ExecMainStartTimestamp", ""),
            "exec_start": props.get("ExecStart", ""),
            "command": command.stdout.strip(),
            "listener": " ".join(matches[0].split()),
            "http": http,
        }
    return result


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()
