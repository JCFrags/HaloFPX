#!/usr/bin/env python3
"""Create a fail-closed, identity-complete host ngram-simple parity receipt.

This tool is intentionally Linux-only because it reads ``/proc/<pid>`` to
record the executable, argv, environment, and every mapped file. It makes no
performance measurement or claim.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import sys
import time
from typing import Any
from urllib.error import URLError
from urllib.request import Request, urlopen


REQUEST_BYTES = (
    b'{"prompt":"I believe the meaning of life is","temperature":0.0,'
    b'"samplers":["temperature"],"n_predict":96,"return_tokens":true,'
    b'"cache_prompt":false,"seed":42}'
)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_json(value: Any) -> str:
    return sha256_bytes(
        json.dumps(value, separators=(",", ":"), sort_keys=True).encode("utf-8")
    )


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def write_receipt_manifest(output_dir: Path) -> dict[str, Any]:
    manifest_path = output_dir / "MANIFEST.sha256"
    entries = []
    for path in sorted(output_dir.rglob("*"), key=lambda value: value.as_posix()):
        if not path.is_file() or path == manifest_path:
            continue
        relative = path.relative_to(output_dir).as_posix()
        entries.append({
            "path": relative,
            "sha256": sha256_file(path),
            "size_bytes": path.stat().st_size,
        })
    manifest_text = "".join(
        f"{entry['sha256']}  {entry['path']}\n" for entry in entries
    )
    manifest_path.write_text(manifest_text, encoding="utf-8")
    return {
        "path": manifest_path.name,
        "entry_count": len(entries),
        "sha256": sha256_file(manifest_path),
        "size_bytes": manifest_path.stat().st_size,
        "payload_size_bytes": sum(entry["size_bytes"] for entry in entries),
    }


def run_capture(command: list[str], *, cwd: Path | None = None) -> dict[str, Any]:
    completed = subprocess.run(
        command,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "argv": command,
        "returncode": completed.returncode,
        "stdout": completed.stdout.decode("utf-8", errors="replace"),
        "stderr": completed.stderr.decode("utf-8", errors="replace"),
    }


def git_command(source_root: Path, arguments: list[str]) -> dict[str, Any]:
    result = run_capture(["git", "-C", str(source_root), *arguments])
    if result["returncode"] == 0 or not shutil.which("git.exe") or not shutil.which("wslpath"):
        return result

    windows_root = run_capture(["wslpath", "-w", str(source_root)])
    if windows_root["returncode"] != 0:
        return result
    root = windows_root["stdout"].strip()
    fallback = run_capture(["git.exe", "-C", root, *arguments])
    fallback["fallback_from"] = result
    return fallback


def exported_source_manifest(source_root: Path) -> list[dict[str, Any]]:
    manifest = []
    for path in sorted(source_root.rglob("*"), key=lambda value: value.as_posix()):
        relative = path.relative_to(source_root).as_posix()
        if path.is_symlink():
            target = os.readlink(path)
            manifest.append({
                "path": relative,
                "kind": "symlink",
                "target": target,
                "sha256": sha256_bytes(target.encode("utf-8")),
            })
        elif path.is_file():
            manifest.append({
                "path": relative,
                "kind": "file",
                "size_bytes": path.stat().st_size,
                "sha256": sha256_file(path),
            })
    return manifest


def source_state(
    source_root: Path,
    *,
    source_commit: str | None = None,
    source_tree: str | None = None,
    source_archive: Path | None = None,
) -> tuple[dict[str, Any], bytes]:
    if source_archive is not None:
        if not source_commit or not source_tree:
            raise ValueError("exported source identity requires commit and tree")
        manifest = exported_source_manifest(source_root)
        manifest_bytes = (
            json.dumps(manifest, indent=2, sort_keys=True) + "\n"
        ).encode("utf-8")
        return {
            "kind": "clean_git_archive_export",
            "source_root": str(source_root.resolve()),
            "commit": source_commit,
            "tree": source_tree,
            "archive_path": str(source_archive.resolve()),
            "archive_sha256": sha256_file(source_archive),
            "archive_size_bytes": source_archive.stat().st_size,
            "manifest_sha256": sha256_bytes(manifest_bytes),
            "manifest_size_bytes": len(manifest_bytes),
            "file_count": len(manifest),
            "total_regular_file_bytes": sum(
                entry.get("size_bytes", 0) for entry in manifest
            ),
            "git_metadata_present": (source_root / ".git").exists(),
        }, manifest_bytes

    commands = {
        "head": ["rev-parse", "HEAD"],
        "head_tree": ["show", "-s", "--format=%T", "HEAD"],
        "index_tree": ["write-tree"],
        "status": ["status", "--porcelain=v2", "--untracked-files=all"],
        "diff": ["diff", "--binary", "HEAD"],
    }
    results = {name: git_command(source_root, arguments) for name, arguments in commands.items()}
    for name, result in results.items():
        if result["returncode"] != 0:
            raise RuntimeError(f"git {name} identity command failed: {result['stderr']}")

    diff_bytes = results["diff"]["stdout"].encode("utf-8")
    return {
        "source_root": str(source_root.resolve()),
        "head": results["head"]["stdout"].strip(),
        "head_tree": results["head_tree"]["stdout"].strip(),
        "index_tree": results["index_tree"]["stdout"].strip(),
        "status_porcelain_v2": results["status"]["stdout"].splitlines(),
        "diff_sha256": sha256_bytes(diff_bytes),
        "diff_size_bytes": len(diff_bytes),
        "git_implementations": {
            name: result["argv"][0] for name, result in results.items()
        },
    }, diff_bytes


def source_identity(
    source_root: Path,
    output_dir: Path,
    *,
    source_commit: str | None = None,
    source_tree: str | None = None,
    source_archive: Path | None = None,
) -> dict[str, Any]:
    state, evidence_bytes = source_state(
        source_root,
        source_commit=source_commit,
        source_tree=source_tree,
        source_archive=source_archive,
    )
    evidence_name = "source-manifest.json" if source_archive is not None else "source.diff"
    (output_dir / evidence_name).write_bytes(evidence_bytes)
    return state


def retain_build_provenance(
    *,
    build_receipt_dir: Path,
    output_dir: Path,
    source_root: Path,
    server: Path,
) -> dict[str, Any]:
    required = [
        "configure-command.json",
        "configure.log",
        "build-command.json",
        "build.log",
        "test-arg-parser-offline-command.json",
        "test-arg-parser-offline.log",
        "pytest-host-command.json",
        "pytest-host.log",
        "CMakeCache.txt",
        "build.ninja",
    ]
    missing = [name for name in required if not (build_receipt_dir / name).is_file()]
    if missing:
        raise FileNotFoundError(f"build provenance is incomplete: {missing}")

    command_receipts = {}
    for name in (
        "configure-command.json",
        "build-command.json",
        "test-arg-parser-offline-command.json",
        "pytest-host-command.json",
    ):
        receipt = json.loads((build_receipt_dir / name).read_text(encoding="utf-8"))
        if receipt.get("returncode") != 0 or not isinstance(receipt.get("argv"), list):
            raise RuntimeError(f"unsuccessful or malformed command receipt: {name}")
        command_receipts[name] = receipt

    cache_path = build_receipt_dir / "CMakeCache.txt"
    cache_text = cache_path.read_text(encoding="utf-8", errors="replace")
    source_binding = f"CMAKE_HOME_DIRECTORY:INTERNAL={source_root.resolve()}"
    if source_binding not in cache_text:
        raise RuntimeError("CMake cache does not bind the exported source root")
    build_root = cache_path.parent.resolve()
    if not server.is_relative_to(build_root):
        raise RuntimeError("server executable is not under the retained CMake build root")
    configure_argv = command_receipts["configure-command.json"]["argv"]
    build_argv = command_receipts["build-command.json"]["argv"]
    if str(source_root.resolve()) not in configure_argv or str(build_root) not in configure_argv:
        raise RuntimeError("configure command does not bind the source and build roots")
    if str(build_root) not in build_argv or "llama-server" not in build_argv:
        raise RuntimeError("build command does not bind the server to the build root")
    parser_receipt = command_receipts["test-arg-parser-offline-command.json"]
    if parser_receipt.get("environment", {}).get("LLAMA_TEST_SKIP_NETWORK") != "1":
        raise RuntimeError("parser test receipt does not prove the offline network filter")
    pytest_argv = command_receipts["pytest-host-command.json"]["argv"]
    if "unit/test_ngram_simple_qualification.py" not in pytest_argv:
        raise RuntimeError("pytest receipt does not name the isolated ngram host module")
    if "unit/test_speculative.py" in pytest_argv:
        raise RuntimeError("pytest receipt unexpectedly invokes the external-draft module")

    retained_dir = output_dir / "build-provenance"
    retained_dir.mkdir()
    retained_files = []
    for name in required:
        source = build_receipt_dir / name
        destination = retained_dir / name
        shutil.copy2(source, destination)
        retained_files.append({
            "path": f"build-provenance/{name}",
            "sha256": sha256_file(destination),
            "size_bytes": destination.stat().st_size,
        })

    return {
        "build_root": str(build_root),
        "server_relative_to_build_root": server.relative_to(build_root).as_posix(),
        "cmake_source_binding": source_binding,
        "required_command_returncodes_zero": True,
        "offline_parser_filter": command_receipts[
            "test-arg-parser-offline-command.json"
        ].get("environment", {}).get("LLAMA_TEST_SKIP_NETWORK"),
        "files": retained_files,
        "files_sha256": sha256_json(retained_files),
    }


def port_has_listener(host: str, port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.settimeout(0.25)
        return probe.connect_ex((host, port)) == 0


def wait_ready(host: str, port: int, process: subprocess.Popen[bytes], timeout: float) -> None:
    deadline = time.monotonic() + timeout
    url = f"http://{host}:{port}/health"
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise RuntimeError(f"server exited before health check: returncode={process.returncode}")
        try:
            with urlopen(url, timeout=1.0) as response:
                if response.status == 200:
                    return
        except (OSError, URLError):
            time.sleep(0.1)
    raise TimeoutError(f"server did not become ready within {timeout} seconds")


def http_bytes(method: str, url: str, body: bytes | None = None) -> bytes:
    headers = {"Content-Type": "application/json"} if body is not None else {}
    request = Request(url, data=body, headers=headers, method=method)
    with urlopen(request, timeout=120.0) as response:
        if response.status != 200:
            raise RuntimeError(f"unexpected HTTP status {response.status} from {url}")
        return response.read()


def proc_bytes(pid: int, name: str) -> bytes:
    return Path(f"/proc/{pid}/{name}").read_bytes()


def mapped_file_paths(pid: int) -> list[Path]:
    paths: set[Path] = set()
    for line in Path(f"/proc/{pid}/maps").read_text(encoding="utf-8").splitlines():
        columns = line.split(maxsplit=5)
        if len(columns) < 6 or not columns[5].startswith("/"):
            continue
        raw_path = columns[5].removesuffix(" (deleted)")
        candidate = Path(raw_path)
        if candidate.is_file():
            paths.add(candidate.resolve())
    return sorted(paths, key=str)


def process_identity(pid: int, project_root: Path) -> dict[str, Any]:
    argv = [
        item.decode("utf-8", errors="surrogateescape")
        for item in proc_bytes(pid, "cmdline").split(b"\0")
        if item
    ]
    environ = {}
    for item in proc_bytes(pid, "environ").split(b"\0"):
        if not item or b"=" not in item:
            continue
        key, value = item.split(b"=", 1)
        environ[key.decode("utf-8", errors="replace")] = value.decode("utf-8", errors="replace")

    files = []
    project_files = []
    for path in mapped_file_paths(pid):
        stat = path.stat()
        entry = {
            "path": str(path),
            "sha256": sha256_file(path),
            "size_bytes": stat.st_size,
        }
        files.append(entry)
        if path.is_relative_to(project_root):
            project_files.append(entry)

    project_basenames = {Path(entry["path"]).name for entry in project_files}
    for required_prefix in ("libllama-common.so.", "libllama.so."):
        if not any(name.startswith(required_prefix) for name in project_basenames):
            raise RuntimeError(
                f"mapped project files do not include required {required_prefix} DSO"
            )

    executable = Path(f"/proc/{pid}/exe").resolve()
    return {
        "pid": pid,
        "proc_exe": str(executable),
        "proc_exe_sha256": sha256_file(executable),
        "proc_argv": argv,
        "environment": {
            "LD_LIBRARY_PATH": environ.get("LD_LIBRARY_PATH"),
            "LLAMA_CACHE": environ.get("LLAMA_CACHE"),
            "HF_HUB_OFFLINE": environ.get("HF_HUB_OFFLINE"),
        },
        "mapped_files": files,
        "mapped_files_sha256": sha256_json(files),
        "project_mapped_files": project_files,
        "project_mapped_files_sha256": sha256_json(project_files),
    }


def stop_exact(process: subprocess.Popen[bytes], expected_server: Path) -> dict[str, Any]:
    receipt: dict[str, Any] = {
        "pid": process.pid,
        "poll_before": process.poll(),
        "signal": None,
    }
    if process.poll() is None:
        proc_exe = Path(f"/proc/{process.pid}/exe").resolve()
        receipt["proc_exe_before"] = str(proc_exe)
        receipt["proc_exe_matches_expected"] = proc_exe == expected_server.resolve()
        if not receipt["proc_exe_matches_expected"]:
            raise RuntimeError(
                f"refusing to stop pid {process.pid}: {proc_exe} != {expected_server.resolve()}"
            )
        os.killpg(process.pid, signal.SIGTERM)
        receipt["signal"] = "SIGTERM"
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            receipt["signal"] = "SIGKILL"
            process.wait(timeout=10)
    receipt["returncode"] = process.returncode
    receipt["proc_pid_absent_after"] = not Path(f"/proc/{process.pid}").exists()
    return receipt


def server_argv(server: Path, model: Path, host: str, port: int, ubatch: int, speculative: bool) -> list[str]:
    argv = [
        str(server),
        "--model", str(model),
        "--host", host,
        "--port", str(port),
        "--threads", "4",
        "--ctx-size", "512",
        "--parallel", "1",
        "--batch-size", "256",
        "--ubatch-size", str(ubatch),
        "--n-gpu-layers", "0",
        "--flash-attn", "off",
        "--cont-batching",
        "--cache-ram", "0",
        "--no-webui",
        "--no-warmup",
        "--offline",
        "--temp", "0",
        "--seed", "42",
        "--verbose",
    ]
    if speculative:
        argv.extend([
            "--spec-type", "ngram-simple",
            "--spec-draft-n-min", "1",
            "--spec-draft-n-max", "16",
            "--spec-ngram-simple-size-n", "3",
            "--spec-ngram-simple-size-m", "16",
            "--spec-ngram-simple-min-hits", "1",
        ])
    else:
        argv.extend(["--spec-type", "none"])
    return argv


def first_difference(left: list[int], right: list[int]) -> int | None:
    for index, (left_token, right_token) in enumerate(zip(left, right)):
        if left_token != right_token:
            return index
    return None if len(left) == len(right) else min(len(left), len(right))


def token_sha256(tokens: list[int]) -> str:
    encoded = json.dumps(tokens, separators=(",", ":")).encode("ascii")
    return sha256_bytes(encoded)


def effective_batch_identity(log_path: Path, *, expected_ubatch: int) -> dict[str, Any]:
    log_text = log_path.read_text(encoding="utf-8", errors="replace")
    batch_values = [
        int(value) for value in re.findall(r"llama_context: n_batch\s*=\s*(\d+)", log_text)
    ]
    ubatch_values = [
        int(value) for value in re.findall(r"llama_context: n_ubatch\s*=\s*(\d+)", log_text)
    ]
    prompt_cache_disabled_count = log_text.count("prompt cache is disabled")
    if not batch_values or set(batch_values) != {256}:
        raise AssertionError(f"unexpected effective n_batch values: {batch_values}")
    if not ubatch_values or set(ubatch_values) != {expected_ubatch}:
        raise AssertionError(f"unexpected effective n_ubatch values: {ubatch_values}")
    if prompt_cache_disabled_count < 1:
        raise AssertionError("startup log did not confirm disabled prompt cache")
    return {
        "n_batch_observations": batch_values,
        "n_batch_unique": sorted(set(batch_values)),
        "n_ubatch_observations": ubatch_values,
        "n_ubatch_unique": sorted(set(ubatch_values)),
        "prompt_cache_disabled_log_count": prompt_cache_disabled_count,
    }


def run_mode(
    *,
    server: Path,
    model: Path,
    output_dir: Path,
    source_root: Path,
    source_commit: str | None,
    source_tree: str | None,
    source_archive: Path | None,
    host: str,
    port: int,
    ubatch: int,
    mode: str,
    timeout: float,
) -> dict[str, Any]:
    speculative = mode == "speculative"
    prefix = f"ubatch-{ubatch}-{mode}"
    log_path = output_dir / f"{prefix}.log"
    argv = server_argv(server, model, host, port, ubatch, speculative)
    if port_has_listener(host, port):
        raise RuntimeError(f"refusing to use occupied port {host}:{port}")

    cache_path = output_dir / "llama-cache-empty"
    if cache_path.exists():
        raise RuntimeError(f"expected fresh absent cache path: {cache_path}")
    source_before, _ = source_state(
        source_root,
        source_commit=source_commit,
        source_tree=source_tree,
        source_archive=source_archive,
    )
    environment = os.environ.copy()
    environment["LLAMA_CACHE"] = str(cache_path)
    environment["HF_HUB_OFFLINE"] = "1"
    process: subprocess.Popen[bytes] | None = None
    cleanup: dict[str, Any] = {}
    result: dict[str, Any] | None = None
    try:
        with log_path.open("wb") as log:
            process = subprocess.Popen(
                argv,
                stdout=log,
                stderr=subprocess.STDOUT,
                env=environment,
                start_new_session=True,
            )
            wait_ready(host, port, process, timeout)
            identity_before = process_identity(process.pid, server.parent)
            if identity_before["proc_argv"] != argv:
                raise AssertionError(f"{prefix}: /proc argv differs from planned argv")
            props_raw = http_bytes("GET", f"http://{host}:{port}/props")
            response_raw = http_bytes(
                "POST", f"http://{host}:{port}/completion", REQUEST_BYTES
            )
            identity_after = process_identity(process.pid, server.parent)
            if identity_before["project_mapped_files_sha256"] != identity_after["project_mapped_files_sha256"]:
                raise AssertionError(f"{prefix}: project DSO map changed during request")

        (output_dir / f"{prefix}-props.json").write_bytes(props_raw)
        (output_dir / f"{prefix}-response.json").write_bytes(response_raw)
        response = json.loads(response_raw)
        props = json.loads(props_raw)
        params = props["default_generation_settings"]["params"]
        expected_type = "ngram-simple" if speculative else "none"
        expected_min = 1 if speculative else 0
        if params["speculative.types"] != expected_type:
            raise AssertionError(f"{prefix}: unexpected speculative.types")
        if type(params["speculative.n_min"]) is not int or params["speculative.n_min"] != expected_min:
            raise AssertionError(f"{prefix}: unexpected speculative.n_min")
        if type(params["speculative.n_max"]) is not int or params["speculative.n_max"] != 16:
            raise AssertionError(f"{prefix}: unexpected speculative.n_max")
        if response["tokens_predicted"] != 96 or len(response["tokens"]) != 96:
            raise AssertionError(f"{prefix}: expected exactly 96 generated tokens")
        if not all(type(token) is int for token in response["tokens"]):
            raise AssertionError(f"{prefix}: response tokens must be integers")

        timings = response["timings"]
        if speculative:
            if type(timings.get("draft_n")) is not int or timings["draft_n"] <= 0:
                raise AssertionError(f"{prefix}: missing authoritative draft_n")
            if type(timings.get("draft_n_accepted")) is not int:
                raise AssertionError(f"{prefix}: missing authoritative draft_n_accepted")
            if not 0 <= timings["draft_n_accepted"] < timings["draft_n"]:
                raise AssertionError(f"{prefix}: expected at least one rejected draft")
        elif "draft_n" in timings or "draft_n_accepted" in timings:
            raise AssertionError(f"{prefix}: target-only response exposed draft counters")

        result = {
            "mode": mode,
            "ubatch": ubatch,
            "port": port,
            "planned_argv": argv,
            "identity_before_request": identity_before,
            "identity_after_request": identity_after,
            "source_state_before_process": source_before,
            "props_sha256": sha256_bytes(props_raw),
            "response_sha256": sha256_bytes(response_raw),
            "tokens": response["tokens"],
            "token_array_sha256": token_sha256(response["tokens"]),
            "content_sha256": sha256_bytes(response["content"].encode("utf-8")),
            "draft_n": timings.get("draft_n"),
            "draft_n_accepted": timings.get("draft_n_accepted"),
        }
    finally:
        if process is not None:
            cleanup = stop_exact(process, server)
        cleanup["listener_absent_after"] = not port_has_listener(host, port)
        cleanup["cache_path_absent_after"] = not cache_path.exists()
        write_json(output_dir / f"{prefix}-cleanup.json", cleanup)

    if result is None:
        raise RuntimeError(f"{prefix}: run completed without a result")
    source_after, _ = source_state(
        source_root,
        source_commit=source_commit,
        source_tree=source_tree,
        source_archive=source_archive,
    )
    result["source_state_after_process"] = source_after
    result["source_state_unchanged"] = source_before == source_after
    result["effective_batch_from_log"] = effective_batch_identity(
        log_path, expected_ubatch=ubatch
    )
    result["cleanup"] = cleanup
    if not result["source_state_unchanged"]:
        raise AssertionError(f"{prefix}: source state changed during process")
    if not cleanup["listener_absent_after"] or not cleanup["proc_pid_absent_after"]:
        raise AssertionError(f"{prefix}: server cleanup was incomplete")
    if not cleanup["cache_path_absent_after"]:
        raise AssertionError(f"{prefix}: cache path was created unexpectedly")
    write_json(output_dir / f"{prefix}-runtime.json", result)
    return result


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--source-commit")
    parser.add_argument("--source-tree")
    parser.add_argument("--source-archive", type=Path)
    parser.add_argument("--build-receipt-dir", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--base-port", type=int, default=19320)
    parser.add_argument("--timeout", type=float, default=60.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    server = args.server.resolve()
    model = args.model.resolve()
    source_root = args.source_root.resolve()
    source_archive = args.source_archive.resolve() if args.source_archive else None
    build_receipt_dir = (
        args.build_receipt_dir.resolve() if args.build_receipt_dir else None
    )
    output_dir = args.output_dir.resolve()
    if sys.platform != "linux" or not Path("/proc/self/maps").is_file():
        raise RuntimeError("this qualification tool requires Linux /proc")
    if not server.is_file() or not os.access(server, os.X_OK):
        raise FileNotFoundError(f"server is not executable: {server}")
    if not model.is_file():
        raise FileNotFoundError(f"model does not exist: {model}")
    exported_values = (args.source_commit, args.source_tree, source_archive)
    if any(value is not None for value in exported_values) and not all(
        value is not None for value in exported_values
    ):
        raise ValueError(
            "--source-commit, --source-tree, and --source-archive are all required together"
        )
    if source_archive is not None and build_receipt_dir is None:
        raise ValueError("exported qualification requires --build-receipt-dir")
    output_dir.mkdir(parents=True, exist_ok=False)

    identity = source_identity(
        source_root,
        output_dir,
        source_commit=args.source_commit,
        source_tree=args.source_tree,
        source_archive=source_archive,
    )
    initial_source_state = dict(identity)
    if source_archive is not None:
        retained_archive = output_dir / "source.tar"
        shutil.copy2(source_archive, retained_archive)
        if sha256_file(retained_archive) != identity["archive_sha256"]:
            raise RuntimeError("retained source archive hash changed during copy")
        identity["retained_source_archive"] = {
            "path": retained_archive.name,
            "sha256": sha256_file(retained_archive),
            "size_bytes": retained_archive.stat().st_size,
        }
        identity["build_provenance"] = retain_build_provenance(
            build_receipt_dir=build_receipt_dir,
            output_dir=output_dir,
            source_root=source_root,
            server=server,
        )
    identity.update({
        "server_path": str(server),
        "server_sha256_before": sha256_file(server),
        "model_path": str(model),
        "model_size_bytes": model.stat().st_size,
        "model_sha256_before": sha256_file(model),
        "request_bytes_utf8": REQUEST_BYTES.decode("utf-8"),
        "request_sha256": sha256_bytes(REQUEST_BYTES),
        "cache_state": {
            "startup": "--cache-ram 0",
            "request": "cache_prompt=false",
            "llama_cache": str(output_dir / "llama-cache-empty"),
        },
        "batch_size": 256,
        "ubatch_values": [1, 2, 256],
        "n_predict": 96,
        "claim_boundary": "correctness and runtime identity only; not performance evidence",
    })
    (output_dir / "request.json").write_bytes(REQUEST_BYTES)
    write_json(output_dir / "run-identity.json", identity)

    runs: dict[int, dict[str, dict[str, Any]]] = {}
    for index, ubatch in enumerate((1, 2, 256)):
        runs[ubatch] = {}
        for mode_index, mode in enumerate(("target", "speculative")):
            port = args.base_port + index * 2 + mode_index
            runs[ubatch][mode] = run_mode(
                server=server,
                model=model,
                output_dir=output_dir,
                source_root=source_root,
                source_commit=args.source_commit,
                source_tree=args.source_tree,
                source_archive=source_archive,
                host=args.host,
                port=port,
                ubatch=ubatch,
                mode=mode,
                timeout=args.timeout,
            )

    comparisons = []
    all_pass = True
    mapped_files_hashes = []
    project_mapped_files_hashes = []
    for ubatch, pair in runs.items():
        target = pair["target"]
        speculative = pair["speculative"]
        difference = first_difference(target["tokens"], speculative["tokens"])
        passed = difference is None and target["content_sha256"] == speculative["content_sha256"]
        all_pass &= passed
        mapped_files_hashes.extend(
            run["identity_after_request"]["mapped_files_sha256"] for run in pair.values()
        )
        project_mapped_files_hashes.extend(
            run["identity_after_request"]["project_mapped_files_sha256"]
            for run in pair.values()
        )
        comparisons.append({
            "ubatch": ubatch,
            "passed": passed,
            "first_token_difference": difference,
            "target_token_at_difference": (
                target["tokens"][difference] if difference is not None else None
            ),
            "speculative_token_at_difference": (
                speculative["tokens"][difference] if difference is not None else None
            ),
            "target_token_array_sha256": target["token_array_sha256"],
            "speculative_token_array_sha256": speculative["token_array_sha256"],
            "target_content_sha256": target["content_sha256"],
            "speculative_content_sha256": speculative["content_sha256"],
            "draft_n": speculative["draft_n"],
            "draft_n_accepted": speculative["draft_n_accepted"],
        })

    summary = {
        "qualification": "PASS" if all_pass else "REFUSE",
        "comparisons": comparisons,
        "server_sha256_after": sha256_file(server),
        "model_sha256_after": sha256_file(model),
        "server_unchanged": sha256_file(server) == identity["server_sha256_before"],
        "model_unchanged": sha256_file(model) == identity["model_sha256_before"],
        "all_ports_clear": all(
            not port_has_listener(args.host, args.base_port + offset) for offset in range(6)
        ),
        "mapped_files_identical_across_runs": len(set(mapped_files_hashes)) == 1,
        "project_mapped_files_identical_across_runs": (
            len(set(project_mapped_files_hashes)) == 1
        ),
        "mapped_files_sha256": sorted(set(mapped_files_hashes)),
        "project_mapped_files_sha256": sorted(set(project_mapped_files_hashes)),
    }
    final_source_state, final_evidence = source_state(
        source_root,
        source_commit=args.source_commit,
        source_tree=args.source_tree,
        source_archive=source_archive,
    )
    summary["source_state_after_matrix"] = final_source_state
    summary["source_evidence_sha256_after_matrix"] = sha256_bytes(final_evidence)
    summary["source_state_unchanged"] = final_source_state == initial_source_state
    summary["qualification"] = (
        "PASS"
        if all_pass
        and summary["all_ports_clear"]
        and summary["server_unchanged"]
        and summary["model_unchanged"]
        and summary["source_state_unchanged"]
        and summary["mapped_files_identical_across_runs"]
        and summary["project_mapped_files_identical_across_runs"]
        else "REFUSE"
    )
    write_json(output_dir / "summary.json", summary)
    final_output = {
        "summary": summary,
        "receipt_manifest": write_receipt_manifest(output_dir),
    }
    print(json.dumps(final_output, indent=2, sort_keys=True))
    return 0 if summary["qualification"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
