#!/usr/bin/env python3
"""Closed-path L79 composed-controller integration qualification."""

from __future__ import annotations

import ast
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from collections import defaultdict, deque
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
CONTROLLER_PATH = ROOT / "scripts" / "halofpx-production-transition.py"
MANIFEST_PATH = ROOT / "scripts" / "halofpx-l77-primary-manifest.json"
PREFLIGHT_LOG = (
    ROOT / "docs" / "halofpx" / "evidence" /
    "l78-preflight-2f5252a" / "ssh-operations.jsonl")


def load(name: str, relative: str):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


controller = load(
    "l79_controller", "scripts/halofpx-production-transition.py")


class ReplayRunner:
    """Replay accepted read-only preflight responses without host access."""

    def __init__(self):
        self.responses = defaultdict(deque)
        for line in PREFLIGHT_LOG.read_text(encoding="utf-8").splitlines():
            record = json.loads(line)
            key = (
                record["host"], tuple(record["argv"]), record["operation"])
            self.responses[key].append(controller.CommandResult(
                record["returncode"], record["stdout"], record["stderr"]))
        self.mutations = []

    def run(self, host, argv, *, operation="command"):
        key = (host, tuple(str(value) for value in argv), operation)
        if operation in {"service-mutation", "recovery-mutation"}:
            self.mutations.append(key)
            raise AssertionError("closed validation attempted host mutation")
        if not self.responses[key]:
            raise AssertionError(f"unrecorded closed-path operation: {key!r}")
        return self.responses[key].popleft()


class DirectoryRunner:
    def __init__(self):
        self.existing = False

    def run(self, host, argv, *, operation="command"):
        command = argv[0]
        if command == "stat" and not self.existing:
            return controller.CommandResult(1, "", "")
        if command == "install":
            self.existing = True
            return controller.CommandResult(0, "", "")
        if command == "stat":
            return controller.CommandResult(0, "directory|connorb|700\n", "")
        if command == "find":
            return controller.CommandResult(0, "", "")
        if command == "rm":
            self.existing = False
            return controller.CommandResult(0, "", "")
        raise AssertionError(argv)


class ComposedControllerClosureTests(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))

    def test_schema_family_is_exact_and_static_scan_has_no_l48_only_gate(self):
        self.assertEqual(
            controller.COMPOSED_MANIFEST_SCHEMAS,
            frozenset({
                "halofpx.l48.fixture-manifest.v1",
                "halofpx.l77.primary-manifest.v1",
            }))
        tree = ast.parse(CONTROLLER_PATH.read_text(encoding="utf-8"))
        l48_literals = [
            node for node in ast.walk(tree)
            if isinstance(node, ast.Constant)
            and node.value == "halofpx.l48.fixture-manifest.v1"]
        self.assertEqual(
            len(l48_literals), 1,
            "L48 schema must occur only in the canonical composed family")

    def test_exact_l77_manifest_validates_without_host_mutation(self):
        runner = ReplayRunner()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "manifest.json"
            path.write_text(
                json.dumps(self.manifest), encoding="utf-8")
            validated = controller.validate_milestone_manifest(path, runner)
        self.assertEqual(validated, self.manifest)
        self.assertEqual(runner.mutations, [])

    def test_near_match_and_unknown_manifests_refuse(self):
        for schema in (
                "halofpx.l77.primary-manifest.v0",
                "halofpx.l77.primary-manifest.v1.extra",
                "halofpx.l79.primary-manifest.v1"):
            with self.subTest(schema=schema), tempfile.TemporaryDirectory() as temporary:
                altered = json.loads(json.dumps(self.manifest))
                altered["schema"] = schema
                path = Path(temporary) / "manifest.json"
                path.write_text(json.dumps(altered), encoding="utf-8")
                runner = ReplayRunner()
                with self.assertRaises(controller.TransitionError):
                    controller.validate_milestone_manifest(path, runner)
                self.assertEqual(runner.mutations, [])

    def test_directory_environment_and_child_admission_close_before_ssh(self):
        prepared = {"sha256": "a" * 64}
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            record = controller.prepare_l52_evidence_directories(
                self.manifest, root, DirectoryRunner())
            self.assertEqual(record["local"]["path"], str(root / "child"))
            environment = controller.child_environment(
                prepared, self.manifest)

            hashes = self.manifest["executable_sha256"]
            authority = self.manifest["authority_contract"]
            expected = {
                "HALOFPX_CHANNEL_KEY_SHA256": prepared["sha256"],
                "HALOFPX_L28_WORKER_SHA256": hashes["worker"],
                "HALOFPX_L28_CANARY_SHA256": hashes["canary"],
                "HALOFPX_L28_READINESS_SHA256": hashes["readiness"],
                "HALOFPX_L28_PLACEMENT_SHA256": hashes["placement"],
                "HALOFPX_L28_EPOCH_RECEIPT_SHA256": hashes["epoch_receipt"],
                "HALOFPX_L37_COMPONENT_DIAGNOSTICS_SHA256":
                    hashes["component_diagnostics"],
                "HALOFPX_L37_SEMANTIC_VERIFIER_SHA256":
                    hashes["semantic_verifier"],
                "HALOFPX_L37_REPLAY_AUTHORITY_VERIFIER_SHA256":
                    hashes["replay_authority_verifier"],
                "HALOFPX_L37_RESULT_AUTHORITY_VERIFIER_SHA256":
                    hashes["result_authority_verifier"],
                "HALOFPX_SEMANTIC_DIAGNOSTICS": "1",
                "HALOFPX_PROVENANCE_SOURCE_ROOT":
                    authority["provenance"]["source_root"],
                "HALOFPX_PROVENANCE_BUILD_ID":
                    authority["provenance"]["build_id"],
                "HALOFPX_COMPOSED_AUTHORITY": "1",
                "HALOFPX_RPC_GRAPH_AUTH": "1",
                "HALOFPX_RPC_MUTABLE_AUTH": "1",
                "HALOFPX_RPC_RESPONSE_DIAGNOSTICS": "1",
                "HALOFPX_RPC_RESPONSE_HARVESTER_WORKER_PATH":
                    self.manifest["executables"]["response_harvester_worker"],
                "HALOFPX_RPC_RESPONSE_HARVESTER_WORKER_SHA256":
                    hashes["response_harvester_worker"],
                "HALOFPX_RPC_RESPONSE_HARVESTER_CLIENT_PATH":
                    self.manifest["executables"]["response_harvester_client"],
                "HALOFPX_RPC_RESPONSE_HARVESTER_CLIENT_SHA256":
                    hashes["response_harvester_client"],
                "HALOFPX_L61_HOST_BOUND_HARVEST": "1",
                "HALOFPX_L50_DEVICE_RECEIPT_SHA256":
                    hashes["device_receipt"],
            }
            self.assertEqual(
                {name: environment.get(name) for name in expected}, expected)

            child = load(
                "l79_child_closed",
                "scripts/halofpx-l13-primary-retry.py")
            sentinel = RuntimeError("SSH boundary reached")
            argv = [
                str(self.manifest["executables"]["child"]),
                "--evidence-dir", str(root / "child"),
                "--l77-primary", "--authority-key-file",
                "/var/tmp/halofpx-l48-control.key",
            ]
            with (
                mock.patch.dict(os.environ, environment, clear=True),
                mock.patch.object(sys, "argv", argv),
                mock.patch.object(
                    child, "initialize_ssh_transport",
                    side_effect=sentinel) as initialize,
                self.assertRaisesRegex(RuntimeError, "SSH boundary reached"),
            ):
                child.main()
            initialize.assert_called_once_with(root / "child")


if __name__ == "__main__":
    unittest.main()
