"""Focused process proof for the default-off L10d exact-key catalog canary."""

from __future__ import annotations

import hashlib
import importlib.util
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
PORT = int(os.environ.get("HALOFPX_CANARY_PORT", "18084"))
API_KEY = "halofpx-exact-catalog-principal"
PROMPTS = (
    "The quick brown fox crossed the quiet valley because",
    "A patient astronomer mapped the silent winter sky while",
    "The third uncached prompt proves bounded admission remains cold",
)

pytestmark = pytest.mark.skipif(
    not SERVER or not MODEL,
    reason="set HALOFPX_CANARY_SERVER and HALOFPX_CANARY_MODEL",
)


def _create_lock(path: Path) -> None:
    fd = os.open(path, os.O_RDWR | os.O_CREAT | os.O_EXCL, 0o600)
    os.fsync(fd)
    os.close(fd)


def _prepare(workspace: Path) -> tuple[Path, Path, Path]:
    data = workspace / "data"
    anchor = workspace / "anchor"
    data.mkdir(mode=0o700)
    anchor.mkdir(mode=0o700)
    _create_lock(anchor / "writer.lock")
    for ordinal in range(2):
        child_data = data / f"slot-{ordinal:02d}"
        child_anchor = anchor / f"slot-{ordinal:02d}"
        child_data.mkdir(mode=0o700)
        child_anchor.mkdir(mode=0o700)
        for name in ("staging", "manifests", "objects"):
            (child_data / name).mkdir(mode=0o700)
        _create_lock(child_data / "writer.lock")
        _create_lock(child_anchor / "writer.lock")
    key = workspace / "operator.key"
    fd = os.open(key, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    os.write(fd, secrets.token_bytes(32))
    os.fsync(fd)
    os.close(fd)
    return data, anchor, key


def _start(data: Path, anchor: Path, key: Path, store_uuid: str,
           components: list[str], log: Path) -> subprocess.Popen:
    assert SERVER is not None and MODEL is not None
    args = [
        SERVER, "--model", MODEL, "--host", "127.0.0.1", "--port", str(PORT),
        "--parallel", "1", "--ctx-size", "128", "--n-gpu-layers", "0", "--fit", "off",
        "--temp", "0", "--samplers", "temperature", "--cache-ram", "0",
        "--no-cache-idle-slots", "--no-webui", "--no-jinja", "--reasoning-format", "none",
        "--offline", "--api-key", API_KEY,
        "--halofpx-context-store-mode", "full-v1-exact-key-catalog-canary",
        "--halofpx-context-store-root", str(data),
        "--halofpx-context-store-anchor-root", str(anchor),
        "--halofpx-context-store-key-file", str(key),
        "--halofpx-context-store-uuid", store_uuid,
        "--halofpx-context-store-compatibility-component", ",".join(components),
        "--halofpx-context-store-quota", "128",
        "--halofpx-context-store-reserve", "0",
        "--halofpx-context-store-max-entries", "2",
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


def _completion(prompt: str) -> dict:
    response = requests.post(
        f"http://127.0.0.1:{PORT}/completion",
        headers={"Authorization": f"Bearer {API_KEY}"},
        json={"prompt": prompt, "stream": False, "cache_prompt": True,
              "temperature": 0, "samplers": ["temperature"],
              "n_predict": 8, "return_tokens": True},
        timeout=60,
    )
    assert response.status_code == 200, response.text
    return response.json()


def _tree(root: Path) -> list[tuple[str, int]]:
    return sorted((str(path.relative_to(root)), path.stat().st_size)
                  for path in root.rglob("*") if path.is_file())


def test_two_entries_restart_hit_and_capacity_is_nonmutating() -> None:
    assert SERVER is not None and MODEL is not None
    workspace = Path(tempfile.mkdtemp(prefix="halofpx-exact-catalog-"))
    data, anchor, key = _prepare(workspace)
    components, tuple_record = _FIXTURE._components(SERVER, MODEL)
    store_uuid = secrets.token_hex(16)
    (workspace / "tuple.json").write_text(
        json.dumps({**tuple_record, "store_uuid": store_uuid}, indent=2) + "\n",
        encoding="utf-8",
    )
    cold: list[dict] = []
    for index, prompt in enumerate(PROMPTS[:2]):
        process = _start(data, anchor, key, store_uuid, components,
                         workspace / f"cold-{index}.log")
        try:
            value = _completion(prompt)
            assert value["timings"]["prompt_n"] > 1
            cold.append(value)
        finally:
            _stop(process)

    hits: list[dict] = []
    for index, prompt in enumerate(PROMPTS[:2]):
        process = _start(data, anchor, key, store_uuid, components,
                         workspace / f"hit-{index}.log")
        try:
            value = _completion(prompt)
            assert value["timings"]["prompt_n"] <= 1
            assert value["content"] == cold[index]["content"]
            assert value.get("tokens") == cold[index].get("tokens")
            hits.append(value)
        finally:
            _stop(process)

    before = {"data": _tree(data), "anchor": _tree(anchor)}
    process = _start(data, anchor, key, store_uuid, components, workspace / "capacity.log")
    try:
        capacity = _completion(PROMPTS[2])
        assert capacity["timings"]["prompt_n"] > 1
    finally:
        _stop(process)
    after = {"data": _tree(data), "anchor": _tree(anchor)}
    assert before == after

    record = {
        "workspace": str(workspace),
        "prompt_n": {"cold": [v["timings"]["prompt_n"] for v in cold],
                     "restart_hits": [v["timings"]["prompt_n"] for v in hits],
                     "capacity_cold": capacity["timings"]["prompt_n"]},
        "content_sha256": [hashlib.sha256(v["content"].encode()).hexdigest() for v in cold],
        "tree_unchanged_at_capacity": before == after,
    }
    (workspace / "results.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(record, sort_keys=True))


if __name__ == "__main__":
    test_two_entries_restart_hit_and_capacity_is_nonmutating()
