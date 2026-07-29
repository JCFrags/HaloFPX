#!/usr/bin/env python3
"""Focused L96 relocatable staged-runtime package qualification."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import os
import subprocess
import sys
from pathlib import Path
from unittest import mock

import pytest


ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "scripts" / "halofpx_staged_runtime_gate.py"
spec = importlib.util.spec_from_file_location("l96_gate", MODULE_PATH)
assert spec is not None and spec.loader is not None
gate = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = gate
spec.loader.exec_module(gate)
controller_spec = importlib.util.spec_from_file_location(
    "l96_controller", ROOT / "scripts" / "halofpx-production-transition.py")
assert controller_spec is not None and controller_spec.loader is not None
controller = importlib.util.module_from_spec(controller_spec)
sys.modules[controller_spec.name] = controller
controller_spec.loader.exec_module(controller)


EXACT_DYNAMIC = """
 0x0000000000000001 (NEEDED) Shared library: [libllama-common.so.0]
 0x000000000000001d (RUNPATH) Library runpath: [$ORIGIN:/opt/rocm/lib]
"""


def test_exact_origin_rocm_runpath_and_needed_accept():
    assert gate.validate_runpath(EXACT_DYNAMIC) == ["$ORIGIN", "/opt/rocm/lib"]
    assert gate.parse_needed(EXACT_DYNAMIC) == ["libllama-common.so.0"]


@pytest.mark.parametrize("runpath", [
    "/var/tmp/halofpx-l48-source-nimo1/build-l48/bin:/opt/rocm/lib",
    "$ORIGIN/../outside:/opt/rocm/lib",
    "$ORIGIN:/tmp",
    "$ORIGIN",
    "$ORIGIN:/opt/rocm/lib:/usr/local/lib",
])
def test_wrong_absolute_traversing_or_expanded_runpath_refuses(runpath):
    dynamic = (
        "0x1 (NEEDED) Shared library: [libx.so.0]\n"
        f"0x1d (RUNPATH) Library runpath: [{runpath}]\n")
    with pytest.raises(gate.GateError):
        gate.validate_runpath(dynamic)


def test_dt_rpath_is_refused_even_with_exact_text():
    dynamic = (
        "0x1 (NEEDED) Shared library: [libx.so.0]\n"
        "0xf (RPATH) Library rpath: [$ORIGIN:/opt/rocm/lib]\n")
    with pytest.raises(gate.GateError, match="RPATH"):
        gate.validate_runpath(dynamic)


def test_canary_provenance_is_exactly_bound():
    source_root = "a" * 64
    build_id = "b" * 64
    line = (
        "schema=halofpx.l57.binary-provenance.v1"
        f"|source_root={source_root}|build_id={build_id}|binary=canary\n")
    assert gate.parse_provenance(line, "", source_root, build_id) == {
        "schema": "halofpx.l57.binary-provenance.v1",
        "source_root": source_root,
        "build_id": build_id,
        "binary": "canary",
    }
    for stdout, stderr in (
        ("", ""),
        (line + line, ""),
        (line, "warning"),
        (line.replace(source_root, "c" * 64), ""),
        (line.replace("binary=canary", "binary=other"), ""),
        (line.replace("schema=", "extra=x|schema="), ""),
    ):
        with pytest.raises(gate.GateError):
            gate.parse_provenance(stdout, stderr, source_root, build_id)


def test_unresolved_or_omitted_dependency_refuses():
    with pytest.raises(gate.GateError, match="omitted"):
        gate.parse_ldd(
            "libother.so => /usr/lib/libother.so (0x1234)\n",
            ["librequired.so"])
    with pytest.raises(gate.GateError):
        gate.parse_ldd(
            "librequired.so => not found\n", ["librequired.so"])


def test_relative_symlink_chain_accepts_and_escape_refuses(
        tmp_path, monkeypatch):
    monkeypatch.setattr(gate, "owner_name", lambda _uid: "connorb")
    staged = tmp_path / "bin"
    staged.mkdir()
    final = staged / "libx.so.1.2"
    final.write_bytes(b"library")
    final.chmod(0o755)
    (staged / "libx.so.1").symlink_to("libx.so.1.2")
    (staged / "libx.so").symlink_to("libx.so.1")
    receipt = gate.symlink_receipt(staged / "libx.so", staged)
    assert receipt["final"] == str(final)
    assert [item["target"] for item in receipt["links"]] == [
        "libx.so.1", "libx.so.1.2"]
    assert receipt["sha256"] == hashlib.sha256(b"library").hexdigest()
    (staged / "escape.so").symlink_to("../outside.so")
    with pytest.raises(gate.GateError, match="escapes"):
        gate.symlink_receipt(staged / "escape.so", staged)


def test_missing_and_tampered_binary_refuse_before_dynamic_inspection(
        tmp_path, monkeypatch):
    staged = tmp_path / "bin"
    staged.mkdir()
    missing = staged / "missing"
    with pytest.raises((FileNotFoundError, gate.GateError)):
        gate.validate_elf("missing", missing, "0" * 64, staged)
    binary = staged / "binary"
    binary.write_bytes(b"tampered")
    binary.chmod(0o755)
    monkeypatch.setattr(gate, "owner_name", lambda _uid: "connorb")
    with pytest.raises(gate.GateError, match="identity mismatch"):
        gate.validate_elf("binary", binary, "0" * 64, staged)


def test_sanitized_probe_environment_is_closed(monkeypatch):
    observed = {}

    def fake_run(argv, **kwargs):
        observed.update(kwargs)
        return subprocess.CompletedProcess(argv, 0, "ok", "")

    monkeypatch.setattr(gate.subprocess, "run", fake_run)
    gate.run_checked(["/bin/true"])
    assert observed["env"] == {
        "PATH": "/usr/bin:/bin",
        "HOME": "/nonexistent",
        "LC_ALL": "C",
    }


def test_relocatable_build_option_is_default_off_and_exact_when_enabled():
    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    assert (
        'option(HALOFPX_RELOCATABLE_STAGED_RUNTIME\n'
        '    "Use origin-relative build lookup for closed staged HaloFPX runtimes" OFF)'
        in cmake)
    enabled = cmake[cmake.index(
        "if (HALOFPX_RELOCATABLE_STAGED_RUNTIME)"):cmake.index(
        "endif()", cmake.index("if (HALOFPX_RELOCATABLE_STAGED_RUNTIME)"))]
    assert "set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)" in enabled
    assert 'set(CMAKE_INSTALL_RPATH "$ORIGIN;/opt/rocm/lib")' in enabled


def test_controller_requires_exact_gate_receipt_before_shutdown(tmp_path):
    manifest = json.loads(
        (ROOT / "scripts" / "halofpx-l77-primary-manifest.json").read_text())
    gate_sha = hashlib.sha256(MODULE_PATH.read_bytes()).hexdigest()
    manifest["source_identity"]["files"][
        "scripts/halofpx_staged_runtime_gate.py"] = gate_sha
    manifest_path = tmp_path / "manifest.json"
    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
    manifest_sha = hashlib.sha256(manifest_path.read_bytes()).hexdigest()
    archive_sha = "a" * 64
    provenance = manifest["authority_contract"]["provenance"]
    receipt = {
        "schema": "halofpx.l96.staged-runtime.v1",
        "status": "pass",
        "root": "/var/tmp/halofpx-l48-source-nimo2",
        "archive_sha256": archive_sha,
        "manifest_sha256": manifest_sha,
        "gate_sha256": gate_sha,
        "source_root": provenance["source_root"],
        "build_id": provenance["build_id"],
        "binaries": {"worker": {}, "canary": {}, "placement": {}},
        "probes": {
            "canary_provenance": {"record": {
                "schema": "halofpx.l57.binary-provenance.v1",
                "source_root": provenance["source_root"],
                "build_id": provenance["build_id"],
                "binary": "canary",
            }},
            "canary_help": {},
            "worker_help": {},
        },
    }

    class Runner:
        mutations = []

        def run(self, _host, argv, *, operation="command"):
            if operation in {"service-mutation", "recovery-mutation"}:
                self.mutations.append(argv)
                raise AssertionError("gate attempted production mutation")
            if argv[:2] == ["sha256sum", "--"] and argv[2].endswith(
                    "halofpx-l77-primary-manifest.json"):
                return controller.CommandResult(0, f"{manifest_sha}  x\n", "")
            if argv[:2] == ["sha256sum", "--"]:
                return controller.CommandResult(0, f"{archive_sha}  x\n", "")
            if argv[0] == "python3":
                return controller.CommandResult(0, "", "")
            if argv[:2] == ["cat", "--"]:
                return controller.CommandResult(0, json.dumps(receipt), "")
            raise AssertionError(argv)

    evidence = tmp_path / "evidence"
    evidence.mkdir()
    actual = controller.run_l96_staged_runtime_gate(
        manifest, manifest_path, evidence, Runner())
    assert actual == receipt
    assert json.loads(
        (evidence / "l96-staged-runtime.json").read_text()) == receipt
    source = Path(controller.__file__).read_text(encoding="utf-8")
    assert source.rindex("run_l96_staged_runtime_gate(") < source.index(
        "controller.shutdown()")
