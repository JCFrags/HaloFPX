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


def test_exact_key_miss_publish_restart_hit_and_different_prompt_is_cold() -> None:
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
        cold = _completion()
        assert cold["timings"]["prompt_n"] > 1
    finally:
        _stop(process)

    process = _start(data, anchor, key, store_uuid, components, workspace / "hit.log")
    try:
        hit = _completion()
        assert hit["content"] == cold["content"]
        assert hit.get("tokens") == cold.get("tokens")
        assert hit["timings"]["prompt_n"] <= 1
    finally:
        _stop(process)

    # A fresh process excludes inherited in-memory LCP reuse as an explanation
    # for the different-key control.
    process = _start(data, anchor, key, store_uuid, components, workspace / "different.log")
    try:
        different = _completion(PROMPT + " suddenly")
        assert different["timings"]["prompt_n"] > 1
    finally:
        _stop(process)

    process = _start(data, anchor, key, store_uuid, components, workspace / "hit-again.log")
    try:
        retained = _completion()
        assert retained["content"] == cold["content"]
        assert retained.get("tokens") == cold.get("tokens")
        assert retained["timings"]["prompt_n"] <= 1
    finally:
        _stop(process)

    result_record = {
        "workspace": str(workspace),
        "prompt_n": {
            "cold": cold["timings"]["prompt_n"],
            "restart_hit": hit["timings"]["prompt_n"],
            "different_key_fresh_process": different["timings"]["prompt_n"],
            "retained_hit": retained["timings"]["prompt_n"],
        },
        "content_sha256": hashlib.sha256(cold["content"].encode()).hexdigest(),
        "tokens_sha256": hashlib.sha256(
            json.dumps(cold.get("tokens"), separators=(",", ":")).encode()
        ).hexdigest(),
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
    test_exact_key_miss_publish_restart_hit_and_different_prompt_is_cold()
    test_exact_key_reserve_exhaustion_does_not_publish()
