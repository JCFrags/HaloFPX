"""Focused opt-in process-restart qualification for the HaloFPX full-v1 canary.

Set HALOFPX_CANARY_SERVER and HALOFPX_CANARY_MODEL. This deliberately covers
one explicit-handle miss/publish/restart/hit path and one corruption-to-cold-
recomputation path; broader matrices remain separate gates.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import platform
import secrets
import subprocess
import tempfile
import time

import requests

if __name__ != "__main__":
    import pytest


SERVER = os.environ.get("HALOFPX_CANARY_SERVER")
MODEL = os.environ.get("HALOFPX_CANARY_MODEL")
PORT = int(os.environ.get("HALOFPX_CANARY_PORT", "18082"))
QUOTA_MIB = int(os.environ.get("HALOFPX_CANARY_QUOTA_MIB", "64"))
RESERVE_MIB = int(os.environ.get("HALOFPX_CANARY_RESERVE_MIB", "64"))
API_KEY = "halofpx-full-v1-principal"
SESSION = "6f" * 32
PROMPT = "The quick brown fox crossed the quiet valley because"

if __name__ != "__main__":
    pytestmark = pytest.mark.skipif(
        not SERVER or not MODEL,
        reason="set HALOFPX_CANARY_SERVER and HALOFPX_CANARY_MODEL",
    )


def _sha256(path: str) -> str:
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _runtime_hashes(executable: str) -> dict[str, str]:
    paths = {str(Path(executable).resolve())}
    for line in subprocess.check_output(["ldd", executable], text=True).splitlines():
        fields = line.strip().split()
        candidate = fields[2] if len(fields) >= 3 and fields[1] == "=>" else ""
        if candidate.startswith("/") and Path(candidate).is_file():
            paths.add(str(Path(candidate).resolve()))
    return {Path(path).name: _sha256(path) for path in sorted(paths)}


def _request(method: str, path: str, body: dict | None = None) -> requests.Response:
    return requests.request(
        method,
        f"http://127.0.0.1:{PORT}{path}",
        headers={"Authorization": f"Bearer {API_KEY}"},
        json=body,
        timeout=60,
    )


def _components(executable: str, model: str) -> tuple[list[str], dict]:
    runtime = _runtime_hashes(executable)
    model_sha = _sha256(model)
    facts = {
        "model_bytes_and_shards": {"bytes": Path(model).stat().st_size, "sha256": model_sha},
        "model_metadata": {"model_sha256": model_sha},
        "tokenizer_bytes_and_policy": {"model_sha256": model_sha},
        "chat_template_bytes_renderer_and_rendered_output": {
            "api": "native-completion-no-template", "template": "disabled"
        },
        "system_and_tool_context": {"system": "empty", "tools": "disabled"},
        "adapter_projector_set_and_order": [],
        "runtime_abi_and_build": runtime,
        "backend_and_device_abi": {
            "backend": "cpu",
            "platform": {
                "node": platform.node(), "release": platform.release(),
                "machine": platform.machine(),
            },
        },
        "quantization_and_kv_layout": {"kv_k": "f16", "kv_v": "f16"},
        "context_rope_window_and_position": {"context": 128},
        "sampler_and_logits_processors": "greedy-memoryless",
        "grammar_parser_and_tool_state": {"grammar": "disabled", "tools": "disabled"},
        "rng_state_and_counter": {"rng": "irrelevant-greedy-memoryless"},
        "target_draft_mtp_speculative_state": False,
        "topology_plan_rank_world_placement_epoch": "world-1-rank-0-target-only-epoch-1",
        "security_domain_and_scope_policy": {
            "scope": "authenticated-private", "issuer": "llama-server-api-key"
        },
    }
    encoded = []
    record = []
    for label, fact in facts.items():
        value = json.dumps(fact, sort_keys=True, separators=(",", ":")).encode()
        digest = hashlib.sha256(
            b"halofpx.compat-component.v1\0"
            + len(label).to_bytes(2, "big") + label.encode("ascii") + value
        ).hexdigest()
        encoded.append(f"{label}={digest}")
        record.append({"label": label, "digest": digest, "fact": fact})
    return encoded, {"components": record, "model_sha256": model_sha, "runtime": runtime}


def _prepare_roots(workspace: Path) -> tuple[Path, Path, Path]:
    data = workspace / "data"
    anchor = workspace / "anchor"
    data.mkdir(mode=0o700)
    anchor.mkdir(mode=0o700)
    for name in ("staging", "manifests", "objects"):
        (data / name).mkdir(mode=0o700)
    for root in (data, anchor):
        descriptor = os.open(root / "writer.lock", os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        os.fsync(descriptor)
        os.close(descriptor)
    key = workspace / "operator.key"
    descriptor = os.open(key, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    os.write(descriptor, secrets.token_bytes(32))
    os.fsync(descriptor)
    os.close(descriptor)
    return data, anchor, key


def _start(data: Path, anchor: Path, key: Path, store_uuid: str,
           components: list[str], log: Path) -> subprocess.Popen:
    args = [
        SERVER, "--model", MODEL, "--host", "127.0.0.1", "--port", str(PORT),
        "--parallel", "1", "--ctx-size", "128", "--n-gpu-layers", "0", "--fit", "off",
        "--temp", "0", "--samplers", "temperature", "--cache-ram", "0",
        "--no-cache-idle-slots", "--no-webui", "--no-jinja", "--reasoning-format", "none",
        "--offline", "--api-key", API_KEY,
        "--halofpx-context-store-mode", "full-v1-rw-canary",
        "--halofpx-context-store-root", str(data),
        "--halofpx-context-store-anchor-root", str(anchor),
        "--halofpx-context-store-key-file", str(key),
        "--halofpx-context-store-uuid", store_uuid,
        "--halofpx-context-store-compatibility-component", ",".join(components),
        "--halofpx-context-store-quota", str(QUOTA_MIB),
        "--halofpx-context-store-reserve", str(RESERVE_MIB),
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
    process.terminate()
    process.wait(timeout=15)
    output.close()
    raise AssertionError(f"server did not become healthy; see {log}")


def _stop(process: subprocess.Popen) -> None:
    process.terminate()
    try:
        process.wait(timeout=30)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=10)
    process._halofpx_output.close()  # type: ignore[attr-defined]


def _completion(n_predict: int) -> dict:
    response = _request("POST", "/completion", {
        "prompt": PROMPT, "id_slot": 0, "cache_prompt": True,
        "temperature": 0, "n_predict": n_predict, "return_tokens": True,
    })
    assert response.status_code == 200, response.text
    return response.json()


def test_full_v1_restart_hit_and_corruption_recomputes() -> None:
    assert SERVER is not None and MODEL is not None
    assert QUOTA_MIB > 0 and RESERVE_MIB >= 0
    evidence_parent = os.environ.get("HALOFPX_CANARY_EVIDENCE_DIR")
    workspace = Path(tempfile.mkdtemp(prefix="halofpx-full-v1-", dir=evidence_parent))
    data, anchor, key = _prepare_roots(workspace)
    store_uuid = secrets.token_hex(16)
    components, tuple_record = _components(SERVER, MODEL)
    (workspace / "tuple.json").write_text(
        json.dumps({
            **tuple_record,
            "store_uuid": store_uuid,
            "lifecycle_limits": {
                "quota_mib": QUOTA_MIB,
                "reserve_mib": RESERVE_MIB,
                "max_entries": 1,
            },
        }, indent=2) + "\n",
        encoding="utf-8",
    )

    process = _start(data, anchor, key, store_uuid, components, workspace / "cold.log")
    try:
        tokenized = _request("POST", "/tokenize", {"content": PROMPT, "add_special": True})
        assert tokenized.status_code == 200, tokenized.text
        tokens = tokenized.json()["tokens"]
        absent = _request("POST", "/slots/0?action=halofpx-restore", {
            "session": SESSION, "selected_manifest": "7f" * 32, "tokens": tokens,
        })
        assert absent.status_code == 200, absent.text
        assert absent.json()["hit"] is False
        assert absent.json()["status"] == "miss-not-found"
        cold = _completion(8)
    finally:
        _stop(process)

    process = _start(data, anchor, key, store_uuid, components, workspace / "publish.log")
    try:
        _completion(0)
        published = _request(
            "POST", "/slots/0?action=halofpx-publish", {"session": SESSION}
        )
        assert published.status_code == 200, published.text
        assert published.json()["published"] is True
        selected = published.json()["selected_manifest"]
        assert len(selected) == 64 and selected != "0" * 64
    finally:
        _stop(process)

    process = _start(data, anchor, key, store_uuid, components, workspace / "restart.log")
    try:
        restored = _request("POST", "/slots/0?action=halofpx-restore", {
            "session": SESSION, "selected_manifest": selected, "tokens": tokens,
        })
        assert restored.status_code == 200, restored.text
        assert restored.json()["hit"] is True
        warm = _completion(8)
        assert warm["content"] == cold["content"]
        if "tokens" in warm and "tokens" in cold:
            assert warm["tokens"] == cold["tokens"]
    finally:
        _stop(process)

    with (anchor / "anchor.v1").open("r+b") as target:
        first = target.read(1)
        assert first
        target.seek(0)
        target.write(bytes([first[0] ^ 1]))
        target.flush()
        os.fsync(target.fileno())

    process = _start(data, anchor, key, store_uuid, components, workspace / "corrupt.log")
    try:
        corrupt = _request("POST", "/slots/0?action=halofpx-restore", {
            "session": SESSION, "selected_manifest": selected, "tokens": tokens,
        })
        assert corrupt.status_code == 200, corrupt.text
        assert corrupt.json()["hit"] is False
        assert corrupt.json()["status"] == "miss-corrupt"
        recomputed = _completion(8)
        assert recomputed["content"] == cold["content"]
        if "tokens" in recomputed and "tokens" in cold:
            assert recomputed["tokens"] == cold["tokens"]
    finally:
        _stop(process)

    operator_key_sha256 = _sha256(str(key))
    (workspace / "result.json").write_text(json.dumps({
        "selected_manifest": selected,
        "restart_hit": True,
        "corruption_miss": True,
        "safe_recomputation_equal": True,
        "cold_content_sha256": hashlib.sha256(cold["content"].encode()).hexdigest(),
        "operator_key_sha256": operator_key_sha256,
    }, indent=2) + "\n", encoding="utf-8")
    key.unlink()


if __name__ == "__main__":
    test_full_v1_restart_hit_and_corruption_recomputes()
    print("HaloFPX full-v1 process-restart canary passed")
