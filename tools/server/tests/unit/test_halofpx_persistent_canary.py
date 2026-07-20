"""Focused opt-in Linux qualification for the HaloFPX direct-session canary.

This test is skipped unless HALOFPX_CANARY_SERVER and HALOFPX_CANARY_MODEL are
set. It intentionally proves only the first admitted profile: authenticated
direct ID, single slot, transformer target state, and greedy continuation.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import platform
import secrets
import shutil
import subprocess
import tempfile
import time

import pytest
import requests


SERVER = os.environ.get("HALOFPX_CANARY_SERVER")
MODEL = os.environ.get("HALOFPX_CANARY_MODEL")
PORT = int(os.environ.get("HALOFPX_CANARY_PORT", "18081"))
MODE = os.environ.get("HALOFPX_CANARY_MODE", "direct-rw")
API_KEY_A = "halofpx-canary-principal-a"
API_KEY_B = "halofpx-canary-principal-b"
SESSION = "4f" * 32
PROMPT = "The quick brown fox crossed the quiet valley because"


pytestmark = pytest.mark.skipif(
    not SERVER or not MODEL,
    reason="set HALOFPX_CANARY_SERVER and HALOFPX_CANARY_MODEL for the opt-in canary",
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
        candidate = fields[2] if len(fields) >= 3 and fields[1] == "=>" else (fields[0] if fields else "")
        if candidate.startswith("/") and Path(candidate).is_file():
            paths.add(str(Path(candidate).resolve()))
    return {Path(path).name: _sha256(path) for path in sorted(paths)}


def _request(method: str, path: str, key: str, body: dict | None = None) -> requests.Response:
    return requests.request(
        method,
        f"http://127.0.0.1:{PORT}{path}",
        headers={"Authorization": f"Bearer {key}"},
        json=body,
        timeout=60,
    )


def _start(
    root: Path,
    key_file: Path,
    compatibility: str,
    compatibility_components: list[str] | None,
    log_file: Path,
    anchor_root: Path | None = None,
    store_uuid: str | None = None,
) -> subprocess.Popen:
    args = [
        SERVER,
        "--model", MODEL,
        "--host", "127.0.0.1",
        "--port", str(PORT),
        "--parallel", "1",
        "--ctx-size", "128",
        "--n-gpu-layers", "0",
        "--fit", "off",
        "--temp", "0",
        "--samplers", "temperature",
        "--cache-ram", "0",
        "--no-cache-idle-slots",
        "--no-webui",
        "--no-jinja",
        "--reasoning-format", "none",
        "--offline",
        "--api-key", f"{API_KEY_A},{API_KEY_B}",
        "--halofpx-context-store-mode", MODE,
        "--halofpx-context-store-root", str(root),
        "--halofpx-context-store-key-file", str(key_file),
        "--halofpx-context-store-quota", "512",
        "--halofpx-context-store-reserve", os.environ.get("HALOFPX_CANARY_RESERVE_MIB", "1024"),
        "--halofpx-context-store-max-entries", "4",
    ]
    if compatibility_components is None:
        args.extend(["--halofpx-context-store-compatibility-root", compatibility])
    else:
        args.extend([
            "--halofpx-context-store-compatibility-component",
            ",".join(compatibility_components),
        ])
    if MODE == "protected-rw-canary":
        assert anchor_root is not None and store_uuid is not None
        args.extend([
            "--halofpx-context-store-anchor-root", str(anchor_root),
            "--halofpx-context-store-uuid", store_uuid,
        ])
    output = open(log_file, "ab", buffering=0)
    process = subprocess.Popen(args, stdout=output, stderr=subprocess.STDOUT)
    process._halofpx_output = output  # type: ignore[attr-defined]
    deadline = time.monotonic() + 60
    while time.monotonic() < deadline:
        if process.poll() is not None:
            output.close()
            raise AssertionError(f"server exited {process.returncode}; see {log_file}")
        try:
            if requests.get(f"http://127.0.0.1:{PORT}/health", timeout=1).status_code == 200:
                return process
        except requests.RequestException:
            pass
        time.sleep(0.1)
    process.terminate()
    process.wait(timeout=10)
    output.close()
    raise AssertionError(f"server did not become healthy; see {log_file}")


def _stop(process: subprocess.Popen) -> None:
    process.terminate()
    try:
        process.wait(timeout=20)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=10)
    process._halofpx_output.close()  # type: ignore[attr-defined]


def _completion(n_predict: int, overrides: dict | None = None) -> dict:
    body = {
        "prompt": PROMPT,
        "id_slot": 0,
        "cache_prompt": True,
        "temperature": 0,
        "n_predict": n_predict,
        "return_tokens": True,
    }
    if overrides:
        body.update(overrides)
    response = _request("POST", "/completion", API_KEY_A, body)
    assert response.status_code == 200, response.text
    return response.json()


def test_restart_hit_corruption_recomputes_and_scope_isolation() -> None:
    assert SERVER is not None and MODEL is not None
    evidence_parent = os.environ.get("HALOFPX_CANARY_EVIDENCE_DIR")
    workspace = Path(tempfile.mkdtemp(prefix="halofpx-canary-", dir=evidence_parent))
    root = workspace / "store"
    root.mkdir(mode=0o700)
    anchor_root = workspace / "anchors"
    if MODE == "protected-rw-canary":
        anchor_root.mkdir(mode=0o700)
    store_uuid = secrets.token_hex(16)
    key_file = workspace / "authority.key"
    descriptor = os.open(key_file, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    os.write(descriptor, secrets.token_bytes(32))
    os.close(descriptor)

    tuple_record = {
        "format": "halofpx-protected-canary-v1" if MODE == "protected-rw-canary"
        else "halofpx-direct-canary-v1",
        "runtime_files": _runtime_hashes(SERVER),
        "model_sha256": _sha256(MODEL),
        "model_bytes": Path(MODEL).stat().st_size,
        "platform": {
            "node": platform.node(),
            "system": platform.system(),
            "release": platform.release(),
            "machine": platform.machine(),
        },
        "backend": "cpu",
        "context": 128,
        "kv_k": "f16",
        "kv_v": "f16",
        "parallel": 1,
        "sampling": "greedy-memoryless",
        "topology": "world-1-rank-0-target-only",
        "api": "native-completion-no-template-no-grammar-no-tools",
        "adapters": [],
        "draft_speculative_mtp": False,
    }
    help_text = subprocess.check_output([SERVER, "--help"], text=True, stderr=subprocess.STDOUT)
    canonical_compatibility = (
        MODE == "protected-rw-canary"
        and "--halofpx-context-store-compatibility-component" in help_text
    )
    component_facts = {
        "model_bytes_and_shards": {
            "bytes": tuple_record["model_bytes"],
            "sha256": tuple_record["model_sha256"],
        },
        "model_metadata": {"model_sha256": tuple_record["model_sha256"]},
        "tokenizer_bytes_and_policy": {"model_sha256": tuple_record["model_sha256"]},
        "chat_template_bytes_renderer_and_rendered_output": {
            "api": tuple_record["api"], "template": "disabled",
        },
        "system_and_tool_context": {"system": "empty", "tools": "disabled"},
        "adapter_projector_set_and_order": tuple_record["adapters"],
        "runtime_abi_and_build": tuple_record["runtime_files"],
        "backend_and_device_abi": {
            "backend": tuple_record["backend"], "platform": tuple_record["platform"],
        },
        "quantization_and_kv_layout": {
            "kv_k": tuple_record["kv_k"], "kv_v": tuple_record["kv_v"],
        },
        "context_rope_window_and_position": {"context": tuple_record["context"]},
        "sampler_and_logits_processors": tuple_record["sampling"],
        "grammar_parser_and_tool_state": {"grammar": "disabled", "tools": "disabled"},
        "rng_state_and_counter": {"rng": "irrelevant-greedy-memoryless"},
        "target_draft_mtp_speculative_state": tuple_record["draft_speculative_mtp"],
        "topology_plan_rank_world_placement_epoch": tuple_record["topology"],
        "security_domain_and_scope_policy": {
            "scope": "authenticated-private", "issuer": "llama-server-api-key",
        },
    }
    compatibility_components = None
    component_record = None
    if canonical_compatibility:
        digests = []
        component_record = []
        for label, fact in component_facts.items():
            encoded = json.dumps(fact, sort_keys=True, separators=(",", ":")).encode("utf-8")
            digest = hashlib.sha256(
                b"halofpx.compat-component.v1\0"
                + len(label).to_bytes(2, "big")
                + label.encode("ascii")
                + encoded
            ).digest()
            digests.append(digest)
            component_record.append({"label": label, "digest": digest.hex(), "fact": fact})
        compatibility = hashlib.sha256(
            b"halofpx.compat.v1\0"
            + b"\xb0"
            + b"".join(bytes([index, 0x58, 0x20]) + digest for index, digest in enumerate(digests))
        ).hexdigest()
        compatibility_components = [
            f"{entry['label']}={entry['digest']}" for entry in component_record
        ]
    else:
        compatibility = hashlib.sha256(
            json.dumps(tuple_record, sort_keys=True, separators=(",", ":")).encode("utf-8")
        ).hexdigest()
    (workspace / "compatibility.json").write_text(
        json.dumps({
            **tuple_record,
            "compatibility_root": compatibility,
            "canonical_component_authority": canonical_compatibility,
            "components": component_record,
        }, indent=2) + "\n",
        encoding="utf-8",
    )

    process = _start(root, key_file, compatibility, compatibility_components,
                     workspace / "server-1.log", anchor_root, store_uuid)
    try:
        miss = _request("POST", "/slots/0?action=halofpx-restore", API_KEY_A, {"session": SESSION})
        assert miss.status_code == 200, miss.text
        assert miss.json()["hit"] is False
        assert miss.json()["status"] == "miss-not-found"

        cold = _completion(8)
        _completion(0, {"parse_tool_calls": True})
        rejected_profile = _request(
            "POST", "/slots/0?action=halofpx-publish", API_KEY_A, {"session": "50" * 32}
        )
        assert rejected_profile.status_code != 200, rejected_profile.text
        _completion(0)
        published = _request("POST", "/slots/0?action=halofpx-publish", API_KEY_A, {"session": SESSION})
        assert published.status_code == 200, published.text
        assert published.json()["published"] is True
        assert published.json()["status"] == "published"
    finally:
        _stop(process)

    changed_component_miss = False
    if compatibility_components is not None:
        changed_components = list(compatibility_components)
        label, digest = changed_components[2].split("=", 1)
        changed_components[2] = f"{label}={digest[:-1]}{'0' if digest[-1] != '0' else '1'}"
        process = _start(root, key_file, compatibility, changed_components,
                         workspace / "server-component-mismatch.log", anchor_root, store_uuid)
        try:
            incompatible = _request(
                "POST", "/slots/0?action=halofpx-restore", API_KEY_A, {"session": SESSION}
            )
            assert incompatible.status_code == 200, incompatible.text
            assert incompatible.json()["hit"] is False
            assert incompatible.json()["status"] == "miss-not-found"
            changed_component_miss = True
        finally:
            _stop(process)

    process = _start(root, key_file, compatibility, compatibility_components,
                     workspace / "server-2.log", anchor_root, store_uuid)
    try:
        isolated = _request("POST", "/slots/0?action=halofpx-restore", API_KEY_B, {"session": SESSION})
        assert isolated.status_code == 200, isolated.text
        assert isolated.json()["hit"] is False
        assert isolated.json()["status"] == "miss-not-found"

        restored = _request("POST", "/slots/0?action=halofpx-restore", API_KEY_A, {"session": SESSION})
        assert restored.status_code == 200, restored.text
        assert restored.json()["hit"] is True
        assert restored.json()["status"] == "hit"
        warm = _completion(8)
        assert warm["content"] == cold["content"]
        if "tokens" in cold and "tokens" in warm:
            assert warm["tokens"] == cold["tokens"]
    finally:
        _stop(process)

    if MODE == "protected-rw-canary":
        corruption_files = [path for path in anchor_root.rglob("*.anchor") if ".staging" not in path.parts]
    else:
        corruption_files = [path for path in root.rglob("state") if ".staging" not in path.parts]
    assert len(corruption_files) == 1
    with corruption_files[0].open("r+b") as state:
        first = state.read(1)
        assert first
        state.seek(0)
        state.write(bytes([first[0] ^ 0x01]))
        state.flush()
        os.fsync(state.fileno())

    process = _start(root, key_file, compatibility, compatibility_components,
                     workspace / "server-3.log", anchor_root, store_uuid)
    try:
        corrupt = _request("POST", "/slots/0?action=halofpx-restore", API_KEY_A, {"session": SESSION})
        assert corrupt.status_code == 200, corrupt.text
        assert corrupt.json()["hit"] is False
        assert corrupt.json()["status"] == "miss-corrupt"
        recomputed = _completion(8)
        assert recomputed["content"] == cold["content"]
        if "tokens" in cold and "tokens" in recomputed:
            assert recomputed["tokens"] == cold["tokens"]
    finally:
        _stop(process)

    unexpected = root / "unexpected-root-entry"
    descriptor = os.open(unexpected, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    os.write(descriptor, b"reject this root")
    os.fsync(descriptor)
    os.close(descriptor)

    process = _start(root, key_file, compatibility, compatibility_components,
                     workspace / "server-4.log", anchor_root, store_uuid)
    try:
        unavailable = _request("POST", "/slots/0?action=halofpx-restore", API_KEY_A, {"session": SESSION})
        assert unavailable.status_code != 200, unavailable.text
        startup_cold = _completion(8)
        assert startup_cold["content"] == cold["content"]
        if "tokens" in cold and "tokens" in startup_cold:
            assert startup_cold["tokens"] == cold["tokens"]
    finally:
        _stop(process)

    summary = {
        "compatibility_root": compatibility,
        "mode": MODE,
        "store_uuid": store_uuid if MODE == "protected-rw-canary" else None,
        "cold_content_sha256": hashlib.sha256(cold["content"].encode()).hexdigest(),
        "restart_hit": True,
        "wrong_scope_miss": True,
        "corruption_miss": True,
        "safe_recomputation_equal": True,
        "changed_component_miss": changed_component_miss,
        "unsupported_profile_rejected": True,
        "startup_store_rejection_continues_cold": True,
    }
    (workspace / "result.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    if not evidence_parent:
        shutil.rmtree(workspace)
