from __future__ import annotations

import hashlib
import hmac
import importlib.util
import struct
import base64
import json
import sys
import stat
from pathlib import Path

import pytest


ROOT = Path(__file__).parents[1]
SPEC = importlib.util.spec_from_file_location(
    "server_harvest", ROOT / "scripts" / "halofpx_server_authority_harvest.py")
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)
CONTROLLER_SPEC = importlib.util.spec_from_file_location(
    "controller", ROOT / "scripts" / "halofpx-production-transition.py")
assert CONTROLLER_SPEC and CONTROLLER_SPEC.loader
CONTROLLER = importlib.util.module_from_spec(CONTROLLER_SPEC)
sys.modules[CONTROLLER_SPEC.name] = CONTROLLER
CONTROLLER_SPEC.loader.exec_module(CONTROLLER)

KEY_FILE = b"01" * 32 + b"\n" + b"02" * 32 + b"\n"
GRAPH_KEY = bytes([1]) * 32


def authority(key: bytes = GRAPH_KEY, *, terminal=(1, 3, 4, 5, 6, 7)) -> tuple[bytes, dict[str, object]]:
    attempt = bytes(range(32))
    admission = bytes(range(32, 64))
    expected_digest = bytes([65]) * 32
    graph_digest = bytes([66]) * 32
    receipt = bytes([67]) * 32
    prior = bytes(32)
    lines = []
    for index, event in enumerate(terminal, 1):
        raw = struct.pack(
            "<HHIIIIQQQQQQQIII",
            1, 0, 2, event, 0, 1, index, 9, 11, 13, 17, 19, 23,
            29, 31, 1,
        ) + attempt + admission + expected_digest + graph_digest + receipt + prior
        tag = hmac.new(key, MODULE.DOMAIN + raw, hashlib.sha256).digest()
        lines.append(
            b"domain=" + MODULE.DOMAIN +
            b"|role=server|grammar=1.0|record=" + raw.hex().encode() +
            b"|tag=" + tag.hex().encode() + b"\n")
        prior = tag
    data = b"".join(lines)
    expected = {
        "attempt": attempt.hex(), "admission": admission.hex(),
        "sequence": 11, "split_uid": 17, "split_ordinal": 31,
        "backend": 29, "sha256": hashlib.sha256(data).hexdigest(),
    }
    return data, expected


def test_server_success_authority_authenticates_and_cross_binds() -> None:
    data, expected = authority()
    verified = MODULE._verify(data, MODULE._decode_graph_key(KEY_FILE), expected)
    assert verified["terminal_branch"] == 1
    assert verified["records"] == 6
    assert verified["execute_receipt"] == bytes([67] * 32).hex()


def test_tamper_and_wrong_journal_binding_fail_closed() -> None:
    key = MODULE._decode_graph_key(KEY_FILE)
    data, expected = authority()
    tampered = bytearray(data)
    tampered[data.index(b"record=") + len(b"record=") + 10] ^= 1
    try:
        MODULE._verify(bytes(tampered), key, expected)
    except (ValueError, UnicodeError):
        pass
    else:
        raise AssertionError("tampered server authority accepted")
    wrong = dict(expected)
    wrong["backend"] = 30
    try:
        MODULE._verify(data, key, wrong)
    except ValueError as exc:
        assert str(exc) == "journal_cross_binding"
    else:
        raise AssertionError("wrong journal binding accepted")


def test_source_absence_is_proven_only_at_initial_open(tmp_path: Path) -> None:
    source = tmp_path / "server.authority"
    try:
        MODULE._open_source(source)
    except MODULE.SourceAbsent:
        pass
    else:
        raise AssertionError("missing source was not classified at source open")
    source.write_bytes(b"authority")
    descriptor = MODULE._open_source(source)
    MODULE.os.close(descriptor)
    source.unlink()
    try:
        MODULE.os.open(tmp_path / "missing-key", MODULE.os.O_RDONLY)
    except FileNotFoundError as exc:
        assert not isinstance(exc, MODULE.SourceAbsent)
    else:
        raise AssertionError("missing key unexpectedly opened")


def test_key_mode_accepts_only_controller_canonical_0600(tmp_path: Path) -> None:
    key = tmp_path / "control.key"
    key.write_bytes(KEY_FILE)
    uid = key.stat().st_uid
    observed = list(key.stat())
    def metadata(mode: int):
        values = list(observed)
        values[0] = stat.S_IFREG | mode
        return MODULE.os.stat_result(values)
    assert MODULE._key_authority_valid(metadata(0o600), uid)
    assert not MODULE._key_authority_valid(metadata(0o400), uid)
    assert not MODULE._key_authority_valid(metadata(0o644), uid)
    wrong_size = list(metadata(0o600))
    wrong_size[6] = 131
    assert not MODULE._key_authority_valid(
        MODULE.os.stat_result(wrong_size), uid)


def test_key_append_after_metadata_is_refused(tmp_path: Path) -> None:
    key = tmp_path / "control.key"
    key.write_bytes(KEY_FILE)
    descriptor = MODULE.os.open(key, MODULE.os.O_RDONLY)
    try:
        with key.open("ab") as output:
            output.write(b"x")
        try:
            MODULE._read_exact_unchanged(descriptor, 130)
        except ValueError as exc:
            assert str(exc) == "key_short_or_changed"
        else:
            raise AssertionError("appended key file accepted")
    finally:
        MODULE.os.close(descriptor)


def test_graph_key_format_matches_server_and_refuses_other_material() -> None:
    digest = hashlib.sha256(KEY_FILE).hexdigest()
    decoded = MODULE._validated_graph_key(KEY_FILE, digest)
    assert decoded == GRAPH_KEY
    data, expected = authority()
    assert MODULE._verify(data, decoded, expected)["terminal_branch"] == 1
    for wrong_key in (KEY_FILE, bytes([2]) * 32):
        try:
            MODULE._verify(data, wrong_key, expected)
        except ValueError as exc:
            assert str(exc).endswith(":hmac")
        else:
            raise AssertionError("wrong HMAC key material accepted")
    malformed = (
        KEY_FILE[:-1],
        KEY_FILE + b"\n",
        KEY_FILE.replace(b"\n", b"", 1),
        b"gg" + KEY_FILE[2:],
        b"AA" + KEY_FILE[2:],
        b"00" * 32 + KEY_FILE[64:],
    )
    for candidate in malformed:
        try:
            MODULE._decode_graph_key(candidate)
        except ValueError as exc:
            assert str(exc) == "key_format"
        else:
            raise AssertionError("malformed key file accepted")
    try:
        MODULE._validated_graph_key(
            KEY_FILE, hashlib.sha256(KEY_FILE + b"x").hexdigest())
    except ValueError as exc:
        assert str(exc) == "key_digest"
    else:
        raise AssertionError("wrong full-file digest accepted")


def test_missing_reordered_duplicate_and_post_terminal_refuse() -> None:
    key = MODULE._decode_graph_key(KEY_FILE)
    for production in (
        (1, 3, 5, 6, 7),
        (1, 4, 3, 5, 6, 7),
        (1, 3, 3, 4, 5, 6, 7),
        (1, 3, 4, 5, 6, 7, 8),
    ):
        data, expected = authority(terminal=production)
        try:
            MODULE._verify(data, key, expected)
        except ValueError:
            continue
        raise AssertionError(f"invalid server production accepted: {production}")


class FakeRunner:
    def __init__(self, data: bytes):
        self.data = data
        self.calls: list[list[str]] = []

    def run(self, _host: str, argv: list[str], *, operation: str = "command"):
        self.calls.append(list(argv))
        if argv[0] == "systemctl":
            return CONTROLLER.CommandResult(
                0, "ActiveState=inactive\nSubState=dead\nMainPID=0\n")
        if argv[0] == "sha256sum":
            digest = hashlib.sha256(
                (ROOT / "scripts" / "halofpx_server_authority_harvest.py").read_bytes()
            ).hexdigest()
            return CONTROLLER.CommandResult(0, f"{digest}  {argv[-1]}\n")
        if argv[0] == "python3":
            return CONTROLLER.CommandResult(0, json.dumps({
                "schema": "halofpx.server-authority-harvest.v1",
                "status": "present", "reason": "authenticated",
            }))
        if argv[0] == "base64":
            return CONTROLLER.CommandResult(0, base64.b64encode(self.data).decode())
        if argv[0] == "rm":
            return CONTROLLER.CommandResult(0, "")
        raise AssertionError((operation, argv))

    def run_stdin(self, *_args, **_kwargs):
        raise AssertionError("unexpected stdin")


class BadHelperRunner(FakeRunner):
    def __init__(self, data: bytes, missing: bool):
        super().__init__(data)
        self.missing = missing

    def run(self, host: str, argv: list[str], *, operation: str = "command"):
        if argv[0] == "sha256sum":
            self.calls.append(list(argv))
            return CONTROLLER.CommandResult(
                1 if self.missing else 0,
                "" if self.missing else f"{'0' * 64}  {argv[-1]}\n",
            )
        return super().run(host, argv, operation=operation)


def journal_line(expected: dict[str, object], status: str = "present") -> str:
    return (
        "[halofpx-preexecute-server-publication] "
        f"status={status} attempt={expected['attempt']} "
        f"admission={expected['admission']} sequence={expected['sequence']} "
        f"split_uid={expected['split_uid']} "
        f"split_ordinal={expected['split_ordinal']} "
        f"backend={expected['backend']} "
        f"path=/var/tmp/halofpx-l48-worker/{expected['attempt']}-9-19-server.authority "
        f"sha256={expected['sha256']} errno={'0' if status == 'present' else '5'}\n"
    )


def test_controller_harvest_precedes_remote_cleanup(tmp_path: Path) -> None:
    data, expected = authority()
    child = tmp_path / "child"
    child.mkdir()
    (child / "worker-journal.txt").write_text(journal_line(expected))
    runner = FakeRunner(data)
    manifest = {
        "worker_host": "nimo-1", "worker_units": ["worker.service"],
        "executables": {
            "server_authority_harvester":
                "/var/tmp/halofpx-l48-source-nimo1/scripts/"
                "halofpx_server_authority_harvest.py",
        },
        "executable_sha256": {
            "server_authority_harvester": hashlib.sha256(
                (ROOT / "scripts" /
                 "halofpx_server_authority_harvest.py").read_bytes()
            ).hexdigest(),
        },
        "key_paths": {"nimo-1": "/var/tmp/key"},
    }
    result = CONTROLLER.harvest_server_authority_finally(
        manifest, {"sha256": "unused"}, tmp_path, runner)
    assert result["status"] == "present", repr(result)
    commands = [call[0] for call in runner.calls]
    assert commands == ["systemctl", "sha256sum", "python3", "base64", "rm"]
    helper_argv = next(call for call in runner.calls if call[0] == "python3")
    staging = helper_argv[helper_argv.index("--staging") + 1]
    assert staging.startswith("/var/tmp/halofpx-l48-worker/.")
    assert staging.endswith("-server.authority.harvest")
    assert "\\" not in staging
    retained = next((tmp_path / "server-authority").iterdir())
    assert retained.read_bytes() == data
    assert json.loads(
        (tmp_path / "server-authority-harvest.json").read_text()
    )["status"] == "present"


@pytest.mark.parametrize("missing", [True, False])
def test_remote_helper_missing_or_hash_mismatch_refuses(
        tmp_path: Path, missing: bool) -> None:
    data, expected = authority()
    child = tmp_path / "child"
    child.mkdir()
    (child / "worker-journal.txt").write_text(journal_line(expected))
    manifest = {
        "worker_host": "nimo-1", "worker_units": [],
        "executables": {
            "server_authority_harvester":
                "/var/tmp/halofpx-l48-source-nimo1/scripts/"
                "halofpx_server_authority_harvest.py",
        },
        "executable_sha256": {
            "server_authority_harvester": hashlib.sha256(
                (ROOT / "scripts" /
                 "halofpx_server_authority_harvest.py").read_bytes()
            ).hexdigest(),
        },
        "key_paths": {"nimo-1": "/var/tmp/key"},
    }
    result = CONTROLLER.harvest_server_authority_finally(
        manifest, {}, tmp_path, BadHelperRunner(data, missing))
    assert result["status"] == "error"
    assert "source identity mismatch" in result["reason"]


def test_publication_failure_is_durable_and_cleanup_can_continue(tmp_path: Path) -> None:
    data, expected = authority()
    child = tmp_path / "child"
    child.mkdir()
    (child / "worker-journal.txt").write_text(journal_line(expected, "error"))
    runner = FakeRunner(data)
    manifest = {
        "worker_host": "nimo-1", "worker_units": ["worker.service"],
        "executables": {
            "server_authority_harvester":
                "/var/tmp/halofpx-l48-source-nimo1/scripts/"
                "halofpx_server_authority_harvest.py",
        },
        "executable_sha256": {
            "server_authority_harvester": hashlib.sha256(
                (ROOT / "scripts" /
                 "halofpx_server_authority_harvest.py").read_bytes()
            ).hexdigest(),
        },
        "key_paths": {"nimo-1": "/var/tmp/key"},
    }
    result = CONTROLLER.harvest_server_authority_finally(
        manifest, {}, tmp_path, runner)
    assert result["status"] == "error"
    assert [call[0] for call in runner.calls] == ["systemctl", "sha256sum"]
    durable = json.loads(
        (tmp_path / "server-authority-harvest.json").read_text())
    assert durable["attempts"][0]["reason"] == "publication_failed"


def test_retained_copy_race_refuses_without_overwrite(
        tmp_path: Path, monkeypatch) -> None:
    data, expected = authority()
    child = tmp_path / "child"
    child.mkdir()
    (child / "worker-journal.txt").write_text(journal_line(expected))
    runner = FakeRunner(data)
    manifest = {
        "worker_host": "nimo-1", "worker_units": ["worker.service"],
        "executables": {
            "server_authority_harvester":
                "/var/tmp/halofpx-l48-source-nimo1/scripts/"
                "halofpx_server_authority_harvest.py",
        },
        "executable_sha256": {
            "server_authority_harvester": hashlib.sha256(
                (ROOT / "scripts" /
                 "halofpx_server_authority_harvest.py").read_bytes()
            ).hexdigest(),
        },
        "key_paths": {"nimo-1": "/var/tmp/key"},
    }
    original_link = CONTROLLER.os.link

    def collide(source, destination):
        Path(destination).write_bytes(b"preexisting")
        return original_link(source, destination)

    monkeypatch.setattr(CONTROLLER.os, "link", collide)
    result = CONTROLLER.harvest_server_authority_finally(
        manifest, {"sha256": "unused"}, tmp_path, runner)
    assert result["status"] == "error"
    retained = next((tmp_path / "server-authority").iterdir())
    assert retained.read_bytes() == b"preexisting"
    assert not any(call[0] == "rm" for call in runner.calls)


def test_status_publication_failure_is_converted_for_cleanup(monkeypatch) -> None:
    def fail(*_args, **_kwargs):
        raise OSError("injected evidence fsync failure")

    monkeypatch.setattr(CONTROLLER, "harvest_server_authority_finally", fail)
    failure = CONTROLLER.server_authority_cleanup_boundary(
        {}, {}, Path("unused"), FakeRunner(b""))
    assert failure == "server-authority-record:injected evidence fsync failure"


def test_remote_publication_path_refuses_traversal_and_non_posix(
        tmp_path: Path) -> None:
    _, expected = authority()
    valid = journal_line(expected)
    marker = "/var/tmp/halofpx-l48-worker/"
    malformed = (
        valid.replace(marker, marker + "../"),
        valid.replace(marker, r"\var\tmp\halofpx-l48-worker\\"),
        valid.replace("-server.authority", "/nested-server.authority"),
    )
    for index, line in enumerate(malformed):
        root = tmp_path / str(index) / "child"
        root.mkdir(parents=True)
        (root / "worker-journal.txt").write_text(line)
        try:
            CONTROLLER._server_publications(root.parent)
        except CONTROLLER.TransitionError:
            continue
        raise AssertionError(f"malformed remote path accepted: {line}")
