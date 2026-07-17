from __future__ import annotations
from dataclasses import dataclass
import os
import signal
import subprocess
import time
from pathlib import Path
from typing import Mapping, Sequence

@dataclass
class CommandResult:
    status: str
    exit_code: int | None
    duration_seconds: float
    stdout_path: str
    stderr_path: str

def run_command(
    command: Sequence[str],
    *,
    output_dir: str | Path,
    timeout_seconds: float,
    cwd: str | Path | None = None,
    env: Mapping[str, str] | None = None,
) -> CommandResult:
    if timeout_seconds <= 0:
        raise ValueError("timeout_seconds must be positive")
    outdir = Path(output_dir)
    outdir.mkdir(parents=True, exist_ok=True)
    stdout_path = outdir / "stdout.log"
    stderr_path = outdir / "stderr.log"
    merged_env = os.environ.copy()
    if env:
        merged_env.update(env)

    start = time.monotonic()
    with stdout_path.open("wb") as stdout, stderr_path.open("wb") as stderr:
        proc = subprocess.Popen(
            list(command),
            cwd=str(cwd) if cwd else None,
            env=merged_env,
            stdout=stdout,
            stderr=stderr,
            start_new_session=True,
        )
        try:
            exit_code = proc.wait(timeout=timeout_seconds)
            status = "pass" if exit_code == 0 else "fail"
        except subprocess.TimeoutExpired:
            status = "timeout"
            exit_code = None
            try:
                os.killpg(proc.pid, signal.SIGTERM)
                proc.wait(timeout=5)
            except (ProcessLookupError, subprocess.TimeoutExpired):
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                proc.wait()
    return CommandResult(
        status=status,
        exit_code=exit_code,
        duration_seconds=time.monotonic() - start,
        stdout_path=str(stdout_path),
        stderr_path=str(stderr_path),
    )
