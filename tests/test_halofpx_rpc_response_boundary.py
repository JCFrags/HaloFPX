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
HARVESTER = ROOT / "scripts" / "halofpx_rpc_response_harvest.py"
DOMAIN = b"halofpx.rpc-response-boundary.v1"


def record(key: bytes, event: int = 1, *, side: str = "client",
           phase: str = "response_header", expected: int = 8,
           actual: int = 8, rc: int = 1, eof: int = 0,
           parent_uid: int | None = None, status: int = 0,
           opcode: int = 25, split_uid: int = 27,
           exec_sequence: int = 1, backend_ordinal: int = 0,
           attempt: str = "11" * 32,
           connection_epoch: str = "22" * 32) -> str:
    if parent_uid is None:
        parent_uid = 26 if side == "client" else 0
    canonical = (
        f"domain={DOMAIN.decode()}|version=1|event={event}|side={side}|phase={phase}"
        f"|opcode={opcode}|parent_uid={parent_uid}|split_uid={split_uid}"
        f"|exec_sequence={exec_sequence}|backend_ordinal={backend_ordinal}"
        f"|attempt={attempt}|connection_epoch={connection_epoch}"
        f"|expected={expected}|actual={actual}|rc={rc}|errno=0|eof={eof}|status={status}"
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


def late_semantic_streams(key: bytes) -> tuple[str, str]:
    client = "".join([
        record(key, 1, phase="client_decode", expected=0, actual=0, status=1),
        record(
            key, 2, phase="client_receipt_validation",
            expected=0, actual=0, status=2),
    ])
    server_spec = [
        ("handler_entry", 0, 0),
        ("handler_validation", 0, 1),
        ("backend_complete", 0, 0),
        ("receipt_construction", 0, 1),
        ("handler_exit", 0, 2),
        ("response_header_publish", 8, 0),
        ("response_body_publish", 264, 0),
    ]
    server = "".join(
        record(
            key, i, side="server", phase=phase,
            expected=size, actual=size, status=status)
        for i, (phase, size, status) in enumerate(server_spec, 1))
    return client, server


def invoke(
        tmp_path: Path, contents: tuple[str, ...], *,
        allow_prefix: bool = False) -> subprocess.CompletedProcess[str]:
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
    command = [sys.executable, str(VERIFIER), "--key-file", str(key_path.resolve())]
    if allow_prefix:
        command.append("--allow-prefix")
    return subprocess.run(
        [*command,
         *sum((["--record-file", str(path.resolve())] for path in paths), [])],
        text=True, capture_output=True, check=False)


def resign_stream(stream: str, key: bytes) -> str:
    result = []
    for line in stream.splitlines():
        canonical, separator, supplied_tag = line.rpartition("|tag=")
        assert separator and len(supplied_tag) == 64
        tag = hmac.new(
            key, DOMAIN + canonical.encode("ascii"), hashlib.sha256).hexdigest()
        result.append(f"{canonical}|tag={tag}\n")
    return "".join(result)


def test_accepts_bounded_authenticated_record(tmp_path: Path) -> None:
    key = bytes(range(32))
    client, server = streams(key)
    result = invoke(tmp_path, (client, server))
    assert result.returncode == 0, result.stderr
    parsed = json.loads(result.stdout)
    assert parsed["client"][3]["phase"] == "response_header"
    assert parsed["server"][-1]["phase"] == "response_body_publish"


def test_accepts_exact_paired_late_client_semantics(tmp_path: Path) -> None:
    key = bytes(range(32))
    client, server = late_semantic_streams(key)
    result = invoke(tmp_path, (client, server))
    assert result.returncode == 0, result.stderr
    parsed = json.loads(result.stdout)
    assert [r["phase"] for r in parsed["client"]] == [
        "client_decode", "client_receipt_validation"]
    assert parsed["server"][-1]["phase"] == "response_body_publish"


def test_rejects_unpaired_or_noncanonical_late_client_semantics(
        tmp_path: Path) -> None:
    key = bytes(range(32))
    client, server = late_semantic_streams(key)
    assert invoke(tmp_path, (client,), allow_prefix=True).returncode != 0
    assert invoke(tmp_path, (client, "\n".join(server.splitlines()[:-1]) + "\n"),
                  allow_prefix=True).returncode != 0
    reordered = "".join(reversed(client.splitlines(keepends=True)))
    assert invoke(tmp_path, (reordered, server), allow_prefix=True).returncode != 0
    extra = client + record(
        key, 3, phase="client_receipt_validation",
        expected=0, actual=0, status=2)
    assert invoke(tmp_path, (extra, server), allow_prefix=True).returncode != 0
    failed = "".join([
        record(
            key, 1, phase="client_decode", expected=0, actual=0,
            rc=0, status=0),
        record(
            key, 2, phase="client_receipt_validation",
            expected=0, actual=0, status=2),
    ])
    assert invoke(tmp_path, (failed, server), allow_prefix=True).returncode != 0
    wrong_opcode = "".join([
        record(
            key, 1, phase="client_decode", expected=0, actual=0,
            status=1, opcode=24),
        record(
            key, 2, phase="client_receipt_validation", expected=0, actual=0,
            status=2, opcode=24),
    ])
    assert invoke(tmp_path, (wrong_opcode, server), allow_prefix=True).returncode != 0


def test_rejects_late_client_cross_binding_mismatch(tmp_path: Path) -> None:
    key = bytes(range(32))
    client, server = late_semantic_streams(key)
    mutations = [
        {"attempt": "33" * 32},
        {"connection_epoch": "44" * 32},
        {"split_uid": 28},
        {"exec_sequence": 2},
        {"backend_ordinal": 1},
        {"opcode": 24},
    ]
    server_spec = [
        ("handler_entry", 0, 0),
        ("handler_validation", 0, 1),
        ("backend_complete", 0, 0),
        ("receipt_construction", 0, 1),
        ("handler_exit", 0, 2),
        ("response_header_publish", 8, 0),
        ("response_body_publish", 264, 0),
    ]
    for mutation in mutations:
        mismatched = "".join(
            record(
                key, i, side="server", phase=phase,
                expected=size, actual=size, status=status, **mutation)
            for i, (phase, size, status) in enumerate(server_spec, 1))
        assert invoke(
            tmp_path, (client, mismatched), allow_prefix=True).returncode != 0
    assert server


def test_replays_l88_late_semantic_pair(tmp_path: Path) -> None:
    key = bytes(range(32))
    evidence = ROOT / "docs" / "halofpx" / "evidence" / "l88-transition" / "child"
    client = resign_stream(
        (evidence / "rpc-response-client.jsonl").read_text(encoding="ascii"), key)
    server = resign_stream(
        (evidence / "rpc-response-worker.jsonl").read_text(encoding="ascii"), key)
    result = invoke(tmp_path, (client, server), allow_prefix=True)
    assert result.returncode == 0, result.stderr
    parsed = json.loads(result.stdout)
    assert [record["phase"] for record in parsed["client"]] == [
        "client_decode", "client_receipt_validation"]
    assert len(parsed["server"]) == 7


def test_accepts_multiple_exact_attempt_groups(tmp_path: Path) -> None:
    key = bytes(range(32))
    client_a, server_a = late_semantic_streams(key)
    client_b = "".join(
        record(
            key, index, phase=phase, expected=0, actual=0, status=status,
            attempt="11" * 32, connection_epoch="22" * 32,
            split_uid=31, exec_sequence=2, parent_uid=29)
        for index, (phase, status) in enumerate((
            ("client_decode", 1), ("client_receipt_validation", 2)), 1))
    server_spec = [
        ("handler_entry", 0, 0), ("handler_validation", 0, 1),
        ("backend_complete", 0, 0), ("receipt_construction", 0, 1),
        ("handler_exit", 0, 2), ("response_header_publish", 8, 0),
        ("response_body_publish", 264, 0),
    ]
    server_b = "".join(
        record(
            key, index, side="server", phase=phase, expected=size,
            actual=size, status=status, attempt="11" * 32,
            connection_epoch="22" * 32, split_uid=31, exec_sequence=2)
        for index, (phase, size, status) in enumerate(server_spec, 1))
    result = invoke(
        tmp_path, (client_a + client_b, server_a + server_b))
    assert result.returncode == 0, result.stderr
    parsed = json.loads(result.stdout)
    assert parsed["schema"] == "halofpx.rpc-response-boundary.grouped.v1"
    assert len(parsed["attempts"]) == 2


def test_grouping_rejects_cross_attempt_mix_replay_and_interleave(
        tmp_path: Path) -> None:
    key = bytes(range(32))
    client_a, server_a = late_semantic_streams(key)
    client_b = "".join([
        record(
            key, 1, phase="client_decode", expected=0, actual=0, status=1,
            attempt="33" * 32, connection_epoch="44" * 32,
            split_uid=31, exec_sequence=2, parent_uid=29),
        record(
            key, 2, phase="client_receipt_validation", expected=0, actual=0,
            status=2, attempt="33" * 32, connection_epoch="44" * 32,
            split_uid=31, exec_sequence=2, parent_uid=29),
    ])
    assert invoke(
        tmp_path, (client_a + client_b, server_a)).returncode != 0
    assert invoke(
        tmp_path, (client_a + client_a, server_a),
        allow_prefix=True).returncode != 0
    client_lines = client_a.splitlines(keepends=True)
    interleaved = client_lines[0] + client_b + client_lines[1]
    assert invoke(
        tmp_path, (interleaved, server_a),
        allow_prefix=True).returncode != 0


def test_grouping_rejects_whole_group_gap_reorder_and_unmatched_side(
        tmp_path: Path) -> None:
    key = bytes(range(32))
    client_a, server_a = late_semantic_streams(key)
    client_c = "".join(
        record(
            key, index, phase=phase, expected=0, actual=0, status=status,
            split_uid=35, exec_sequence=3, parent_uid=29)
        for index, (phase, status) in enumerate((
            ("client_decode", 1), ("client_receipt_validation", 2)), 1))
    server_c = "".join(
        record(
            key, index, side="server", phase=phase, expected=size,
            actual=size, status=status, split_uid=35, exec_sequence=3)
        for index, (phase, size, status) in enumerate((
            ("handler_entry", 0, 0), ("handler_validation", 0, 1),
            ("backend_complete", 0, 0), ("receipt_construction", 0, 1),
            ("handler_exit", 0, 2), ("response_header_publish", 8, 0),
            ("response_body_publish", 264, 0)), 1))
    assert invoke(tmp_path, (client_a + client_c, server_a + server_c)).returncode != 0
    assert invoke(tmp_path, (client_c + client_a, server_c + server_a)).returncode != 0
    assert invoke(
        tmp_path, (client_a, server_a + server_c),
        allow_prefix=True).returncode != 0


def test_replays_grouped_l98_streams(tmp_path: Path) -> None:
    key = bytes(range(32))
    evidence = ROOT / "docs" / "halofpx" / "evidence" / "l98-attempt" / "child"
    client = resign_stream(
        (evidence / "rpc-response-client.jsonl").read_text(encoding="ascii"), key)
    server = resign_stream(
        (evidence / "rpc-response-worker.jsonl").read_text(encoding="ascii"), key)
    result = invoke(tmp_path, (client, server))
    assert result.returncode == 0, result.stderr
    parsed = json.loads(result.stdout)
    assert parsed["schema"] == "halofpx.rpc-response-boundary.grouped.v1"
    assert len(parsed["attempts"]) == 5
    assert all(
        len(attempt["client"]) == 2 and len(attempt["server"]) == 7
        for attempt in parsed["attempts"])


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


def test_accepts_single_authenticated_crash_prefix_and_rejects_tamper(
        tmp_path: Path) -> None:
    key = bytes(range(32))
    prefix = "".join([
        record(key, 1, side="server", phase="handler_entry", expected=0, actual=0),
        record(key, 2, side="server", phase="handler_validation", expected=0, actual=0),
    ])
    accepted = invoke(tmp_path, (prefix,), allow_prefix=True)
    assert accepted.returncode == 0, accepted.stderr
    assert set(json.loads(accepted.stdout)) == {"server"}
    refused = invoke(
        tmp_path, (prefix.replace("actual=0", "actual=1", 1),), allow_prefix=True)
    assert refused.returncode != 0


def test_harvester_missing_and_present_contract(tmp_path: Path) -> None:
    if os.name == "nt":
        return
    import getpass

    staging_root = tmp_path / "stage"
    staging_root.mkdir(mode=0o700)
    missing = subprocess.run(
        [sys.executable, str(HARVESTER), "--source", str((tmp_path / "missing").resolve()),
         "--staging", str((staging_root / "missing-copy").resolve()),
         "--expected-owner", getpass.getuser()],
        text=True, capture_output=True, check=False)
    assert missing.returncode == 0
    assert json.loads(missing.stdout)["status"] == "missing"

    source = tmp_path / "source"
    source.write_text("authenticated-prefix\n", encoding="ascii")
    source.chmod(0o600)
    present = subprocess.run(
        [sys.executable, str(HARVESTER), "--source", str(source.resolve()),
         "--staging", str((staging_root / "copy").resolve()),
         "--expected-owner", getpass.getuser()],
        text=True, capture_output=True, check=False)
    assert present.returncode == 0, present.stderr
    metadata = json.loads(present.stdout)
    assert metadata["status"] == "present"
    assert (staging_root / "copy").read_bytes() == source.read_bytes()
