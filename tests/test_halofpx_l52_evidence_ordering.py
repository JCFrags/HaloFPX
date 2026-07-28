#!/usr/bin/env python3
"""Focused L52 evidence-directory admission and publication tests."""

from __future__ import annotations

import importlib.util
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def load(name: str, relative: str):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


controller = load("l52_controller", "scripts/halofpx-production-transition.py")
runner_module = load("l52_runner", "scripts/halofpx-l13-primary-retry.py")


def manifest(
        schema: str = "halofpx.l48.fixture-manifest.v1") -> dict[str, object]:
    return {
        "schema": schema,
        "child_evidence_subdir": "child",
        "disposable_paths": {
            "nimo-1": [],
            "nimo-2": ["/var/tmp/halofpx-l48-evidence"],
        },
        "authority_contract": {
            "evidence_publication": {
                "host": "nimo-2",
                "directory": "/var/tmp/halofpx-l48-evidence",
                "owner": "connorb",
                "directory_mode": "0700",
                "temporary_name": ".device-admission.pending",
                "final_name": "device-admission.json",
                "file_mode": "0600",
            },
        },
    }


class DirectoryRunner:
    def __init__(self, *, existing: bool = False, wrong_mode: bool = False,
                 fail_find: bool = False):
        self.existing = existing
        self.wrong_mode = wrong_mode
        self.fail_find = fail_find
        self.calls: list[tuple[str, tuple[str, ...], str]] = []

    def run(self, host, argv, operation="command"):
        argv = tuple(argv)
        self.calls.append((host, argv, operation))
        command = argv[0]
        if command == "stat" and not self.existing:
            return subprocess.CompletedProcess(argv, 1, "", "")
        if command == "install":
            self.existing = True
            return subprocess.CompletedProcess(argv, 0, "", "")
        if command == "stat":
            mode = "755" if self.wrong_mode else "700"
            return subprocess.CompletedProcess(argv, 0, f"directory|connorb|{mode}\n", "")
        if command == "find":
            return subprocess.CompletedProcess(argv, 1 if self.fail_find else 0, "", "")
        if command == "rm":
            self.existing = False
            return subprocess.CompletedProcess(argv, 0, "", "")
        raise AssertionError(argv)


class EvidenceDirectoryTests(unittest.TestCase):
    def test_exact_l48_and_l77_schemas_are_admitted(self):
        for schema in (
                "halofpx.l48.fixture-manifest.v1",
                "halofpx.l77.primary-manifest.v1"):
            with self.subTest(schema=schema), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                record = controller.prepare_l52_evidence_directories(
                    manifest(schema), root, DirectoryRunner())
                self.assertEqual(record["local"]["path"], str(root / "child"))

    def test_unknown_and_near_match_schemas_refuse(self):
        for schema in (
                "halofpx.l78.primary-manifest.v1",
                "halofpx.l77.primary-manifest.v0",
                "halofpx.l77.primary-manifest.v1.extra",
                "halofpx.l48.fixture-manifest.v1 "):
            with self.subTest(schema=schema), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                with self.assertRaisesRegex(
                        controller.TransitionError, "schema is not admitted"):
                    controller.prepare_l52_evidence_directories(
                        manifest(schema), root, DirectoryRunner())
                self.assertFalse((root / "child").exists())

    def test_create_precedes_publish_and_records_authority(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            remote = DirectoryRunner()
            record = controller.prepare_l52_evidence_directories(
                manifest(), root, remote)
            self.assertEqual(record["remote"]["mode"], "0700")
            self.assertTrue((root / "child").is_dir())
            stored = json.loads(
                (root / "evidence-directory-admission.json").read_text())
            self.assertEqual(stored, record)
            commands = [call[1][0] for call in remote.calls]
            self.assertEqual(commands[:4], ["stat", "install", "stat", "find"])

    def test_preexisting_remote_refuses_before_create(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            remote = DirectoryRunner(existing=True)
            with self.assertRaisesRegex(
                    controller.TransitionError, "preexisting"):
                controller.prepare_l52_evidence_directories(
                    manifest(), root, remote)
            self.assertFalse((root / "child").exists())
            self.assertNotIn("install", [call[1][0] for call in remote.calls])

    def test_wrong_mode_refuses_and_cleans_partial_directories(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            remote = DirectoryRunner(wrong_mode=True)
            with self.assertRaisesRegex(
                    controller.TransitionError, "authority mismatch"):
                controller.prepare_l52_evidence_directories(
                    manifest(), root, remote)
            self.assertFalse((root / "child").exists())
            self.assertFalse(remote.existing)

    def test_copy_phase_failure_cleanup_authority_is_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            remote = DirectoryRunner(fail_find=True)
            with self.assertRaisesRegex(
                    controller.TransitionError, "not empty"):
                controller.prepare_l52_evidence_directories(
                    manifest(), root, remote)
            self.assertFalse((root / "child").exists())
            self.assertEqual(remote.calls[-1][1][0], "rm")


class PublicationTests(unittest.TestCase):
    def test_publish_before_admitted_directory_refuses(self):
        original_ssh = runner_module.ssh
        original_run = runner_module.run
        try:
            runner_module.ssh = lambda *args, **kwargs: subprocess.CompletedProcess(
                args, 0 if args[1:3] == ("test", "-e") else 1, "", "")
            runner_module.run = lambda *args, **kwargs: (_ for _ in ()).throw(
                AssertionError("copy must not occur"))
            with tempfile.TemporaryDirectory() as temporary:
                receipt = Path(temporary) / "receipt.json"
                receipt.write_text("{}\n", encoding="utf-8")
                with self.assertRaisesRegex(
                        runner_module.CanaryError, "collision"):
                    runner_module.publish_device_receipt(Path(temporary), receipt)
        finally:
            runner_module.ssh = original_ssh
            runner_module.run = original_run

    def test_successful_publication_uses_no_replace_and_fsync(self):
        original_ssh = runner_module.ssh
        original_run = runner_module.run
        original_remote = runner_module.REMOTE_EVIDENCE
        calls: list[tuple[object, ...]] = []
        try:
            runner_module.REMOTE_EVIDENCE = "/var/tmp/halofpx-l48-evidence"
            def fake_ssh(*args, **kwargs):
                calls.append(args)
                command = args[1]
                if command == "test":
                    return subprocess.CompletedProcess(args, 1, "", "")
                if command == "chmod":
                    return subprocess.CompletedProcess(args, 0, "", "")
                if command == "python3":
                    size = int(args[-2])
                    digest = args[-1]
                    return subprocess.CompletedProcess(
                        args, 0, json.dumps({
                            "bytes": size, "sha256": digest,
                            "no_replace": True, "directory_fsynced": True,
                        }), "")
                if command == "stat":
                    size = len(b'{"device":"ROCm0"}\n')
                    return subprocess.CompletedProcess(
                        args, 0, f"regular file|connorb|600|{size}\n", "")
                if command == "sha256sum":
                    digest = hashlib.sha256(b'{"device":"ROCm0"}\n').hexdigest()
                    return subprocess.CompletedProcess(args, 0, f"{digest}  receipt\n", "")
                raise AssertionError(args)

            runner_module.ssh = fake_ssh
            runner_module.run = lambda *args, **kwargs: subprocess.CompletedProcess(
                args, 0, "", "")
            with tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                receipt = root / "receipt.json"
                receipt.write_bytes(b'{"device":"ROCm0"}\n')
                result = runner_module.publish_device_receipt(root, receipt)
                self.assertEqual(
                    result, "/var/tmp/halofpx-l48-evidence/device-admission.json")
                record = json.loads((root / "device-publication.json").read_text())
                self.assertTrue(record["no_replace"])
                publish_call = next(call for call in calls if call[1] == "python3")
                self.assertIn("renameat2", publish_call[3])
                self.assertNotIn("os.replace", publish_call[3])
        finally:
            runner_module.ssh = original_ssh
            runner_module.run = original_run
            runner_module.REMOTE_EVIDENCE = original_remote

    def test_tampered_temporary_refuses_without_final_acceptance(self):
        original_ssh = runner_module.ssh
        original_run = runner_module.run
        original_remote = runner_module.REMOTE_EVIDENCE
        try:
            runner_module.REMOTE_EVIDENCE = "/var/tmp/halofpx-l48-evidence"
            def fake_ssh(*args, **kwargs):
                if args[1] == "test":
                    return subprocess.CompletedProcess(args, 1, "", "")
                if args[1] == "chmod":
                    return subprocess.CompletedProcess(args, 0, "", "")
                if args[1] == "python3":
                    return subprocess.CompletedProcess(
                        args, 1, "", "temporary digest mismatch")
                raise AssertionError(args)

            runner_module.ssh = fake_ssh
            runner_module.run = lambda *args, **kwargs: subprocess.CompletedProcess(
                args, 0, "", "")
            with tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                receipt = root / "receipt.json"
                receipt.write_text("{}\n", encoding="utf-8")
                with self.assertRaisesRegex(
                        runner_module.CanaryError, "atomic publication failed"):
                    runner_module.publish_device_receipt(root, receipt)
                self.assertFalse((root / "device-publication.json").exists())
        finally:
            runner_module.ssh = original_ssh
            runner_module.run = original_run
            runner_module.REMOTE_EVIDENCE = original_remote


class TransportCompositionTests(unittest.TestCase):
    def test_child_passes_structured_argv_without_double_quoting(self):
        class FakeTransport:
            def __init__(self):
                self.call = None

            def run(self, host, argv, operation):
                self.call = (host, argv, operation)
                return subprocess.CompletedProcess(argv, 0, "ok", "")

        original = runner_module.SSH_TRANSPORT
        transport = FakeTransport()
        try:
            runner_module.SSH_TRANSPORT = transport
            result = runner_module.ssh(
                "nimo-2", "stat", "-c", "%F|%U|%a", "--",
                "/var/tmp/path with space")
            self.assertEqual(result.returncode, 0)
            self.assertEqual(transport.call, (
                "nimo-2",
                ["stat", "-c", "%F|%U|%a", "--", "/var/tmp/path with space"],
                "cleanup",
            ))
        finally:
            runner_module.SSH_TRANSPORT = original


if __name__ == "__main__":
    unittest.main()
