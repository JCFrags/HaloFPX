import hashlib
import hmac
import os
import subprocess
import sys


def test_exact_scheduler_failure_record(tmp_path):
    key = bytes(range(32))
    key_path = tmp_path / "control.key"
    key_path.write_bytes(key.hex().encode() + b"\n" + bytes(reversed(key)).hex().encode() + b"\n")
    os.chmod(key_path, 0o600)
    canonical = (
        "phase=capture-chunk|decode_status=-3|authority=version=1|status=failed"
        "|branch=scheduler_graph_compute_failed_authority_2"
        "|execution_sequence=1|pending=1|ggml_status=-1"
    )
    tag = hmac.new(
        key, b"halofpx.l55.first-chunk.v1" + canonical.encode(),
        hashlib.sha256).hexdigest()
    record = f"[halofpx-l55-status] {canonical}|auth_tag={tag}"
    result = subprocess.run(
        [
            sys.executable, "scripts/halofpx_l55_status.py",
            "--key-file", str(key_path.resolve()), "--record", record,
        ],
        text=True, capture_output=True, check=False,
    )
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == canonical


def test_tamper_refuses(tmp_path):
    key = bytes(range(32))
    key_path = tmp_path / "control.key"
    key_path.write_bytes(key.hex().encode() + b"\n" + bytes(reversed(key)).hex().encode() + b"\n")
    os.chmod(key_path, 0o600)
    record = (
        "[halofpx-l55-status] phase=capture-chunk|decode_status=-3"
        "|authority=version=1|status=failed"
        "|branch=scheduler_graph_compute_failed_authority_2"
        "|execution_sequence=1|pending=1|ggml_status=-1"
        "|auth_tag=" + "0" * 64
    )
    result = subprocess.run(
        [
            sys.executable, "scripts/halofpx_l55_status.py",
            "--key-file", str(key_path.resolve()), "--record", record,
        ],
        text=True, capture_output=True, check=False,
    )
    assert result.returncode != 0


def test_exact_l40_subreason_record(tmp_path):
    key = bytes(range(32))
    key_path = tmp_path / "control.key"
    key_path.write_bytes(key.hex().encode() + b"\n" + bytes(reversed(key)).hex().encode() + b"\n")
    os.chmod(key_path, 0o600)
    canonical = (
        "phase=capture-chunk|decode_status=-3|authority=version=1|status=failed"
        "|branch=l40_graph_result_reconcile|subreason=graph_uid_mismatch"
        "|expected_graph_uid=26|actual_graph_uid=27"
        "|expected_execution_sequence=1|actual_execution_sequence=1"
        "|backend_ordinal=0|receipt_reason=0"
        "|execution_sequence=1|pending=1|ggml_status=-1"
    )
    tag = hmac.new(
        key, b"halofpx.l55.first-chunk.v1" + canonical.encode(),
        hashlib.sha256).hexdigest()
    result = subprocess.run(
        [
            sys.executable, "scripts/halofpx_l55_status.py",
            "--key-file", str(key_path.resolve()),
            "--record", f"[halofpx-l55-status] {canonical}|auth_tag={tag}",
        ],
        text=True, capture_output=True, check=False,
    )
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == canonical


def test_unknown_or_incomplete_l40_subreason_refuses(tmp_path):
    key = bytes(range(32))
    key_path = tmp_path / "control.key"
    key_path.write_bytes(key.hex().encode() + b"\n" + bytes(reversed(key)).hex().encode() + b"\n")
    os.chmod(key_path, 0o600)
    for detail in (
        "|subreason=other|expected_graph_uid=26|actual_graph_uid=27"
        "|expected_execution_sequence=1|actual_execution_sequence=1"
        "|backend_ordinal=0|receipt_reason=0",
        "|subreason=graph_uid_mismatch|expected_graph_uid=26|actual_graph_uid=27",
    ):
        canonical = (
            "phase=capture-chunk|decode_status=-3|authority=version=1|status=failed"
            "|branch=l40_graph_result_reconcile" + detail +
            "|execution_sequence=1|pending=1|ggml_status=-1"
        )
        tag = hmac.new(
            key, b"halofpx.l55.first-chunk.v1" + canonical.encode(),
            hashlib.sha256).hexdigest()
        result = subprocess.run(
            [
                sys.executable, "scripts/halofpx_l55_status.py",
                "--key-file", str(key_path.resolve()),
                "--record", f"[halofpx-l55-status] {canonical}|auth_tag={tag}",
            ],
            text=True, capture_output=True, check=False,
        )
        assert result.returncode != 0

    wrong_branch = (
        "phase=capture-chunk|decode_status=-3|authority=version=1|status=failed"
        "|branch=l42_scheduler_finalize|subreason=graph_uid_mismatch"
        "|expected_graph_uid=26|actual_graph_uid=27"
        "|expected_execution_sequence=1|actual_execution_sequence=1"
        "|backend_ordinal=0|receipt_reason=0"
        "|execution_sequence=1|pending=1|ggml_status=-1"
    )
    tag = hmac.new(
        key, b"halofpx.l55.first-chunk.v1" + wrong_branch.encode(),
        hashlib.sha256).hexdigest()
    result = subprocess.run(
        [
            sys.executable, "scripts/halofpx_l55_status.py",
            "--key-file", str(key_path.resolve()),
            "--record", f"[halofpx-l55-status] {wrong_branch}|auth_tag={tag}",
        ],
        text=True, capture_output=True, check=False,
    )
    assert result.returncode != 0
