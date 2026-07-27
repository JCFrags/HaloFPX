from __future__ import annotations

import hashlib
import hmac
import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
VERIFIER = ROOT / "scripts" / "halofpx_rpc_response_boundary.py"
DOMAIN = b"halofpx.rpc-response-boundary.v1"


def record(key: bytes, event: int = 1, *, side: str = "client",
           phase: str = "response_header", expected: int = 8,
           actual: int = 8, rc: int = 1, eof: int = 0,
           parent_uid: int | None = None) -> str:
    if parent_uid is None:
        parent_uid = 26 if side == "client" else 0
    canonical = (
        f"domain={DOMAIN.decode()}|version=1|event={event}|side={side}|phase={phase}"
        f"|opcode=25|parent_uid={parent_uid}|split_uid=27|exec_sequence=1|backend_ordinal=0"
        f"|attempt={'11' * 32}|connection_epoch={'22' * 32}"
        f"|expected={expected}|actual={actual}|rc={rc}|errno=0|eof={eof}|status=0"
        "|wall_ns=100|mono_ns=50"
    )
    tag = hmac.new(key, DOMAIN + canonical.encode("ascii"), hashlib.sha256).hexdigest()
    return f"{canonical}|tag={tag}\n"


def streams(key: bytes) -> tuple[str, str]:
    client_spec = [
        ("request_opcode", 1), ("request_header", 8), ("request_body", 16),
        ("response_header", 8), ("response_body", 32),
        ("client_decode", 0), ("client_receipt_validation", 0),
    ]
    server_spec = [
        ("handler_entry", 0), ("handler_validation", 0),
        ("backend_complete", 0), ("receipt_construction", 0), ("handler_exit", 0),
        ("response_header_publish", 8), ("response_body_publish", 32),
    ]
    client = "".join(
        record(key, i, phase=phase, expected=size, actual=size)
        for i, (phase, size) in enumerate(client_spec, 1))
    server = "".join(
        record(key, i, side="server", phase=phase, expected=size, actual=size)
        for i, (phase, size) in enumerate(server_spec, 1))
    return client, server


def invoke(tmp_path: Path, contents: tuple[str, ...]) -> subprocess.CompletedProcess[str]:
    key = bytes(range(32))
    key_path = tmp_path / "key"
    key_path.write_bytes(
        (key.hex() + "\n" + hashlib.sha256(key).hexdigest() + "\n").encode("ascii"))
    paths = []
    for index, content in enumerate(contents):
        evidence = tmp_path / f"records-{index}"
        evidence.write_text(content, encoding="ascii")
        paths.append(evidence)
    if os.name != "nt":
        key_path.chmod(0o600)
    return subprocess.run(
        [sys.executable, str(VERIFIER), "--key-file", str(key_path.resolve()),
         *sum((["--record-file", str(path.resolve())] for path in paths), [])],
        text=True, capture_output=True, check=False)


def test_accepts_bounded_authenticated_record(tmp_path: Path) -> None:
    key = bytes(range(32))
    client, server = streams(key)
    result = invoke(tmp_path, (client, server))
    assert result.returncode == 0, result.stderr
    parsed = json.loads(result.stdout)
    assert parsed["client"][3]["phase"] == "response_header"
    assert parsed["server"][-1]["phase"] == "response_body_publish"


def test_rejects_tamper_and_sequence_errors(tmp_path: Path) -> None:
    key = bytes(range(32))
    client, server = streams(key)
    assert invoke(tmp_path, (client.replace("actual=8", "actual=7", 1), server)).returncode != 0
    assert invoke(tmp_path, (client, client)).returncode != 0
    assert invoke(tmp_path, (client.splitlines(keepends=True)[1], server)).returncode != 0
    assert invoke(tmp_path, (client.replace("client_decode", "unknown"), server)).returncode != 0


def test_accepts_terminal_receipt_construction_failure(tmp_path: Path) -> None:
    key = bytes(range(32))
    client_spec = [
        ("request_opcode", 1, 0), ("request_header", 8, 0),
        ("request_body", 16, 0), ("response_header", 0, 1),
    ]
    server_spec = [
        ("handler_entry", 1), ("handler_validation", 1),
        ("backend_complete", 1), ("receipt_construction", 0),
    ]
    client = "".join(
        record(
            key, i, phase=phase, expected=8 if phase == "response_header" else actual,
            actual=actual, rc=0 if eof else 1, eof=eof)
        for i, (phase, actual, eof) in enumerate(client_spec, 1))
    server = "".join(
        record(key, i, side="server", phase=phase, expected=0, actual=0, rc=rc)
        for i, (phase, rc) in enumerate(server_spec, 1))
    result = invoke(tmp_path, (client, server))
    assert result.returncode == 0, result.stderr
    parsed = json.loads(result.stdout)
    assert parsed["server"][-1]["phase"] == "receipt_construction"
    assert parsed["server"][-1]["rc"] == 0
