"""Focused normal-completion restart harness for the L10c exact-key canary.

This intentionally reuses the reviewed root and compatibility-fixture helpers
from the explicit-handle harness. Set HALOFPX_CANARY_SERVER and
HALOFPX_CANARY_MODEL to run it on Linux.
"""

from __future__ import annotations

import importlib.util
import hashlib
import json
import os
from pathlib import Path
import secrets
import subprocess
import tempfile
import time

import pytest
import requests


_HERE = Path(__file__).resolve().parent
_SPEC = importlib.util.spec_from_file_location(
    "halofpx_full_v1_fixture", _HERE / "test_halofpx_full_v1_canary.py"
)
assert _SPEC is not None and _SPEC.loader is not None
_FIXTURE = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(_FIXTURE)

SERVER = os.environ.get("HALOFPX_CANARY_SERVER")
MODEL = os.environ.get("HALOFPX_CANARY_MODEL")
PORT = int(os.environ.get("HALOFPX_CANARY_PORT", "18083"))
API_KEY = "halofpx-exact-key-principal"
PROMPT = "The quick brown fox crossed the quiet valley because"

pytestmark = pytest.mark.skipif(
    not SERVER or not MODEL,
    reason="set HALOFPX_CANARY_SERVER and HALOFPX_CANARY_MODEL",
)


def _start(data: Path, anchor: Path, key: Path, store_uuid: str,
           components: list[str], log: Path, reserve_mib: int = 0) -> subprocess.Popen:
    args = [
        SERVER, "--model", MODEL, "--host", "127.0.0.1", "--port", str(PORT),
        "--parallel", "1", "--ctx-size", "128", "--n-gpu-layers", "0", "--fit", "off",
        "--temp", "0", "--samplers", "temperature", "--cache-ram", "0",
        "--no-cache-idle-slots", "--no-webui", "--no-jinja", "--reasoning-format", "none",
        "--offline", "--api-key", API_KEY,
        "--halofpx-context-store-mode", "full-v1-exact-key-canary",
        "--halofpx-context-store-root", str(data),
        "--halofpx-context-store-anchor-root", str(anchor),
        "--halofpx-context-store-key-file", str(key),
        "--halofpx-context-store-uuid", store_uuid,
        "--halofpx-context-store-compatibility-component", ",".join(components),
        "--halofpx-context-store-quota", "64",
        "--halofpx-context-store-reserve", str(reserve_mib),
        "--halofpx-context-store-max-entries", "1",
    ]
    output = open(log, "ab", buffering=0)
    process = subprocess.Popen(args, stdout=output, stderr=subprocess.STDOUT)
    process._halofpx_output = output  # type: ignore[attr-defined]
    deadline = time.monotonic() + 90
    while time.monotonic() < deadline:
        if process.poll() is not None:
            output.close()
            raise AssertionError(f"server exited {process.returncode}; see {log}")
        try:
            if requests.get(f"http://127.0.0.1:{PORT}/health", timeout=1).status_code == 200:
                return process
        except requests.RequestException:
            pass
        time.sleep(0.1)
    _stop(process)
    raise AssertionError(f"server did not become healthy; see {log}")


def _stop(process: subprocess.Popen) -> None:
    process.terminate()
    try:
        process.wait(timeout=30)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=10)
    process._halofpx_output.close()  # type: ignore[attr-defined]


def _completion(prompt: str = PROMPT) -> dict:
    response = requests.post(
        f"http://127.0.0.1:{PORT}/completion",
        headers={"Authorization": f"Bearer {API_KEY}"},
        json={
            "prompt": prompt, "stream": False, "cache_prompt": True,
            "temperature": 0, "samplers": ["temperature"],
            "n_predict": 8, "return_tokens": True,
        },
        timeout=60,
    )
    assert response.status_code == 200, response.text
    value = response.json()
    serialized = json.dumps(value, sort_keys=True)
    for forbidden in ("session_id", "scope_namespace", "selected_manifest", "anchor.v1"):
        assert forbidden not in serialized
    return value


def _timed_completion() -> tuple[dict, float]:
    started = time.perf_counter_ns()
    value = _completion()
    elapsed_ms = (time.perf_counter_ns() - started) / 1_000_000
    return value, elapsed_ms


def _changed_component(components: list[str], label: str) -> list[str]:
    changed = list(components)
    prefix = f"{label}="
    matches = [index for index, value in enumerate(changed) if value.startswith(prefix)]
    assert len(matches) == 1
    index = matches[0]
    digest = changed[index][len(prefix):]
    assert len(digest) == 64 and digest == digest.lower()
    replacement = format(int(digest[0], 16) ^ 1, "x") + digest[1:]
    assert replacement != digest and int(replacement, 16) != 0
    changed[index] = prefix + replacement
    return changed


def _store_snapshot(data: Path, anchor: Path) -> list[dict]:
    result = []
    for root in (data, anchor):
        for path in sorted(value for value in root.rglob("*") if value.is_file()):
            metadata = path.stat()
            result.append({
                "path": f"{root.name}/{path.relative_to(root).as_posix()}",
                "size": metadata.st_size,
                "sha256": _FIXTURE._sha256(str(path)),
                "inode": metadata.st_ino,
                "mtime_ns": metadata.st_mtime_ns,
            })
    return result


def _largest_state_object(data: Path) -> Path:
    objects = sorted(path for path in (data / "objects").iterdir() if path.is_file())
    assert len(objects) == 2
    selected = max(objects, key=lambda path: path.stat().st_size)
    assert selected.stat().st_size > 0
    return selected


def _flip_same_size(path: Path) -> tuple[int, str, str]:
    size = path.stat().st_size
    before = _FIXTURE._sha256(str(path))
    with path.open("r+b") as target:
        offset = size // 2
        target.seek(offset)
        original = target.read(1)
        assert len(original) == 1
        target.seek(offset)
        target.write(bytes([original[0] ^ 1]))
        target.flush()
        os.fsync(target.fileno())
    after = _FIXTURE._sha256(str(path))
    assert path.stat().st_size == size and after != before
    return size, before, after


def _assert_same_continuation(actual: dict, expected: dict) -> None:
    assert "tokens" in actual and "tokens" in expected
    assert actual["content"] == expected["content"]
    assert actual["tokens"] == expected["tokens"]


def test_exact_key_restart_hit_mismatch_and_corruption_recompute() -> None:
    assert SERVER is not None and MODEL is not None
    workspace = Path(tempfile.mkdtemp(prefix="halofpx-exact-key-"))
    data, anchor, key = _FIXTURE._prepare_roots(workspace)
    components, tuple_record = _FIXTURE._components(SERVER, MODEL)
    store_uuid = secrets.token_hex(16)
    (workspace / "tuple.json").write_text(
        json.dumps({**tuple_record, "store_uuid": store_uuid}, indent=2) + "\n",
        encoding="utf-8",
    )

    process = _start(data, anchor, key, store_uuid, components, workspace / "cold.log")
    try:
        cold, cold_wall_ms = _timed_completion()
        assert cold["timings"]["prompt_n"] > 1
        assert "tokens" in cold
    finally:
        _stop(process)
    assert len(list((data / "manifests").iterdir())) == 1
    assert len(list((data / "objects").iterdir())) == 2
    assert (anchor / "anchor.v1").is_file()
    clean_tree = _store_snapshot(data, anchor)

    process = _start(data, anchor, key, store_uuid, components, workspace / "hit.log")
    try:
        hit, hit_wall_ms = _timed_completion()
        _assert_same_continuation(hit, cold)
        assert hit["timings"]["prompt_n"] <= 1
    finally:
        _stop(process)
    assert _store_snapshot(data, anchor) == clean_tree

    changed_label = "runtime_abi_and_build"
    changed_components = _changed_component(components, changed_label)
    process = _start(
        data, anchor, key, store_uuid, changed_components,
        workspace / "compatibility-miss.log",
    )
    try:
        incompatible, incompatible_wall_ms = _timed_completion()
        _assert_same_continuation(incompatible, cold)
        assert incompatible["timings"]["prompt_n"] > 1
    finally:
        _stop(process)
    assert _store_snapshot(data, anchor) == clean_tree

    corrupted_object = _largest_state_object(data)
    corrupt_size, clean_sha256, corrupt_sha256 = _flip_same_size(corrupted_object)
    corrupted_tree = _store_snapshot(data, anchor)

    process = _start(data, anchor, key, store_uuid, components, workspace / "corrupt.log")
    try:
        recomputed, recomputed_wall_ms = _timed_completion()
        _assert_same_continuation(recomputed, cold)
        assert recomputed["timings"]["prompt_n"] > 1
    finally:
        _stop(process)
    assert _store_snapshot(data, anchor) == corrupted_tree

    result_record = {
        "workspace": str(workspace),
        "prompt_n": {
            "cold": cold["timings"]["prompt_n"],
            "restart_hit": hit["timings"]["prompt_n"],
            "compatibility_miss": incompatible["timings"]["prompt_n"],
            "corruption_recompute": recomputed["timings"]["prompt_n"],
        },
        "client_wall_ms": {
            "cold": cold_wall_ms,
            "restart_hit": hit_wall_ms,
            "compatibility_miss": incompatible_wall_ms,
            "corruption_recompute": recomputed_wall_ms,
        },
        "server_timings": {
            "cold": cold["timings"],
            "restart_hit": hit["timings"],
            "compatibility_miss": incompatible["timings"],
            "corruption_recompute": recomputed["timings"],
        },
        "clean_store_logical_bytes": sum(entry["size"] for entry in clean_tree),
        "corrupt_store_logical_bytes": sum(entry["size"] for entry in corrupted_tree),
        "content_sha256": hashlib.sha256(cold["content"].encode()).hexdigest(),
        "tokens_sha256": hashlib.sha256(
            json.dumps(cold["tokens"], separators=(",", ":")).encode()
        ).hexdigest(),
        "changed_compatibility_component": changed_label,
        "corrupted_object": {
            "path": corrupted_object.relative_to(data).as_posix(),
            "size": corrupt_size,
            "clean_sha256": clean_sha256,
            "corrupt_sha256": corrupt_sha256,
        },
        "restart_hit": True,
        "compatibility_cold_miss": True,
        "corruption_cold_recomputation": True,
        "clean_tree_immutable": True,
        "corrupt_tree_immutable": True,
    }
    (workspace / "results.json").write_text(
        json.dumps(result_record, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(result_record, sort_keys=True))


def test_exact_key_reserve_exhaustion_does_not_publish() -> None:
    assert SERVER is not None and MODEL is not None
    workspace = Path(tempfile.mkdtemp(prefix="halofpx-exact-key-reserve-"))
    data, anchor, key = _FIXTURE._prepare_roots(workspace)
    components, _ = _FIXTURE._components(SERVER, MODEL)
    store_uuid = secrets.token_hex(16)
    reserve_mib = (os.statvfs(workspace).f_bavail * os.statvfs(workspace).f_frsize) // (1024 * 1024) + 1024

    prompt_counts = []
    for name in ("reserve-cold.log", "reserve-restart.log"):
        process = _start(data, anchor, key, store_uuid, components, workspace / name, reserve_mib)
        try:
            prompt_counts.append(_completion()["timings"]["prompt_n"])
        finally:
            _stop(process)
    assert all(value > 1 for value in prompt_counts)
    assert list((data / "manifests").iterdir()) == []
    assert list((data / "objects").iterdir()) == []
    assert [entry.name for entry in anchor.iterdir() if entry.name != "writer.lock"] == []
    record = {"workspace": str(workspace), "prompt_n": prompt_counts, "reserve_mib": reserve_mib}
    (workspace / "results.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(record, sort_keys=True))


if __name__ == "__main__":
    test_exact_key_restart_hit_mismatch_and_corruption_recompute()
    test_exact_key_reserve_exhaustion_does_not_publish()
