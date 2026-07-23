#!/usr/bin/env python3
"""Focused refusal tests for the closed L21 execution/evidence contract."""

from __future__ import annotations

import copy
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))

from halofpx_l21_contract import (  # noqa: E402
    ContractError,
    EvidenceCollector,
    Result,
    SshRunner,
    UnitIdentity,
    _journal_cursor,
    load_manifest,
)


CHILD = Path(__file__).resolve().parents[1] / "scripts" / "halofpx-l21-refusal.py"
ZERO_HASH = "0" * 64


def valid_raw() -> dict:
    prefix = "/var/tmp/halofpx-l21-small-"
    return {
        "schema": "halofpx.l21.manifest.v1",
        "milestone": "l21-small",
        "worker_host": "nimo-1",
        "canary_host": "nimo-2",
        "worker_port": 50198,
        "worker_units": ["halofpx-l21-small-worker-refusal.service"],
        "canary_units": ["halofpx-l21-small-canary-refusal.service"],
        "executables": {
            "worker": prefix + "build-nimo1/bin/rpc-server",
            "canary": prefix + "build-nimo2/bin/test-halofpx-rpc-allocation-refusal",
            "readiness": prefix + "source-nimo2/scripts/halofpx_rpc_readiness.py",
            "child": "scripts/halofpx-l21-refusal.py",
        },
        "executable_sha256": {
            "worker": ZERO_HASH, "canary": ZERO_HASH, "readiness": ZERO_HASH,
            "child": hashlib.sha256(CHILD.read_bytes()).hexdigest(),
        },
        "child_argv": [
            "python", "scripts/halofpx-l21-refusal.py", "--manifest",
            "scripts/halofpx-l21-small-manifest.json",
        ],
        "paths": {
            "nimo-1": {
                "source_archive": prefix + "source-nimo1.tar",
                "source_root": prefix + "source-nimo1",
                "build_root": prefix + "build-nimo1",
                "state_root": prefix + "state-nimo1",
                "key": prefix + "key-nimo1",
            },
            "nimo-2": {
                "source_archive": prefix + "source-nimo2.tar",
                "source_root": prefix + "source-nimo2",
                "build_root": prefix + "build-nimo2",
                "state_root": prefix + "state-nimo2",
                "key": prefix + "key-nimo2",
                "evidence_root": prefix + "evidence-nimo2",
                "archive_stage": prefix + "archive-stage-nimo2.tar",
                "cleanup_receipt": prefix + "cleanup-receipt-nimo2.json",
                "model": "/var/tmp/halofpx-fixture/tiny.gguf",
                "prompt": "/var/tmp/halofpx-fixture/prompt-1129.txt",
            },
        },
        "cleanup_paths": {},
        "prior_archives": [],
        "retained_archive": prefix + "evidence-v1.tar.zst",
    }


def complete_cleanup(raw: dict) -> None:
    raw["cleanup_paths"] = {
        host: [value for key, value in values.items() if key not in {"model", "prompt"}]
        for host, values in raw["paths"].items()
    }


def manifest_from(raw: dict):
    complete_cleanup(raw)
    temporary = tempfile.NamedTemporaryFile("w", suffix=".json", delete=False)
    try:
        json.dump(raw, temporary)
        temporary.close()
        return load_manifest(Path(temporary.name))
    finally:
        Path(temporary.name).unlink(missing_ok=True)


class ScriptedRunner:
    def __init__(self):
        self.calls: list[tuple[str, tuple[str, ...]]] = []
        self.fail_token: str | None = None
        self.cleanup_fail = False

    def run(self, host, argv, *, stdin=None):
        self.calls.append((host, tuple(argv)))
        joined = " ".join(argv)
        if self.fail_token and self.fail_token in joined:
            return Result(9, "", "injected")
        if argv[:2] == ["systemctl", "--user"] and "show" in argv:
            return Result(0, "LoadState=not-found\nActiveState=inactive\nMainPID=0\n")
        if argv[:3] == ["ss", "-H", "-ltnp"]:
            return Result(0, "")
        if argv[0] == "rm" and self.cleanup_fail:
            return Result(1, "", "injected")
        if argv[0] == "stat":
            return Result(0 if self.cleanup_fail else 1, "")
        return Result(0, "")


class ContractTests(unittest.TestCase):
    def test_remote_argv_is_one_shell_quoted_command(self):
        completed = type("Completed", (), {
            "returncode": 0, "stdout": b"", "stderr": b"",
        })()
        with patch("halofpx_l21_contract.subprocess.run", return_value=completed) as run:
            SshRunner().run("nimo-2", [
                "journalctl", "--after-cursor",
                "s=abc;i=1;b=2;m=3;t=4;x=5", "_SYSTEMD_INVOCATION_ID=a" * 2,
            ])
        argv = run.call_args.args[0]
        self.assertEqual(argv[:5], ["ssh", "-o", "BatchMode=yes", "nimo-2", argv[4]])
        self.assertEqual(len(argv), 5)
        self.assertIn("'s=abc;i=1;b=2;m=3;t=4;x=5'", argv[4])

    def test_cursor_is_exact_opaque_token_and_ambiguous_is_rejected(self):
        token = "s=abc;i=1;b=2;m=3;t=4;x=5"
        self.assertEqual(_journal_cursor(f"-- No entries --\n-- cursor: {token}\n"), token)
        for value in ("", "-- cursor: a\n-- cursor: b\n", "-- cursor: has spaces\n"):
            with self.subTest(value=value), self.assertRaises(ContractError):
                _journal_cursor(value)

    def test_closed_manifest_accepts_exact_authority(self):
        manifest = manifest_from(valid_raw())
        self.assertEqual(manifest.worker_host, "nimo-1")
        self.assertEqual(manifest.worker_port, 50198)

    def test_manifest_rejects_unknown_missing_duplicate_swapped_and_relative(self):
        cases = []
        raw = valid_raw()
        raw["extra"] = 1
        cases.append(raw)
        raw = valid_raw()
        del raw["worker_port"]
        cases.append(raw)
        raw = valid_raw()
        raw["worker_units"].append(raw["worker_units"][0])
        cases.append(raw)
        raw = valid_raw()
        raw["worker_host"], raw["canary_host"] = raw["canary_host"], raw["worker_host"]
        cases.append(raw)
        raw = valid_raw()
        raw["paths"]["nimo-1"]["state_root"] = "relative"
        cases.append(raw)
        for case in cases:
            with self.subTest(case=cases.index(case)), self.assertRaises(ContractError):
                manifest_from(case)

    def test_manifest_rejects_partial_cleanup_and_overlapping_identity(self):
        raw = valid_raw()
        complete_cleanup(raw)
        raw["cleanup_paths"]["nimo-1"].pop()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "manifest.json"
            path.write_text(json.dumps(raw), encoding="utf-8")
            with self.assertRaises(ContractError):
                load_manifest(path)
        raw = valid_raw()
        raw["paths"]["nimo-2"]["state_root"] = raw["paths"]["nimo-2"]["build_root"]
        with self.assertRaises(ContractError):
            manifest_from(raw)

    def test_invocation_or_pid_mismatch_refuses_collection(self):
        manifest = manifest_from(valid_raw())

        class MismatchRunner(ScriptedRunner):
            def run(self, host, argv, *, stdin=None):
                if argv[:3] == ["systemctl", "--user", "show"]:
                    return Result(0, "ExecMainPID=42\nInvocationID=" + "b" * 32 +
                                  "\nExecMainStatus=7\nResult=exit-code\n"
                                  "ExecMainExitTimestampMonotonic=12\n")
                return super().run(host, argv, stdin=stdin)

        identity = UnitIdentity("nimo-2", manifest.canary_units[0], 41, "a" * 32, 10, "s=1", 1, 1)
        with self.assertRaises(ContractError):
            EvidenceCollector(MismatchRunner(), manifest).finish_unit(identity)

    def test_abnormal_exit_is_retained_with_pid_bound_refusal(self):
        manifest = manifest_from(valid_raw())

        class ExitRunner(ScriptedRunner):
            last_size = 0

            def run(self, host, argv, *, stdin=None):
                if argv[:3] == ["systemctl", "--user", "show"]:
                    return Result(0, "ExecMainPID=41\nInvocationID=" + "a" * 32 +
                                  "\nExecMainStatus=7\nResult=exit-code\n"
                                  "ExecMainExitTimestampMonotonic=12\n")
                if argv[0] == "journalctl":
                    return Result(0, "1.0 host test[41]: allocating buffer: cudaMalloc failed\n")
                if argv[0] == "cat":
                    return Result(0, "disk\n")
                if argv[0] == "install":
                    self.last_size = len(stdin or b"")
                    return Result(0, "")
                if argv[0] == "stat":
                    return Result(0, f"regular file:600:{self.last_size}\n")
                return super().run(host, argv, stdin=stdin)

        identity = UnitIdentity("nimo-2", manifest.canary_units[0], 41, "a" * 32, 10, "s=1", 1, 1)
        result = EvidenceCollector(ExitRunner(), manifest).finish_unit(identity, require_refusal=True)
        self.assertEqual(result["status"], 7)
        self.assertEqual(result["refusal_lines"], 1)

    def test_timeout_fails_closed(self):
        manifest = manifest_from(valid_raw())

        class ActiveRunner(ScriptedRunner):
            def run(self, host, argv, *, stdin=None):
                if argv[:3] == ["systemctl", "--user", "show"]:
                    return Result(0, "ActiveState=active\nExecMainPID=41\nInvocationID=" + "a" * 32 + "\n")
                return super().run(host, argv, stdin=stdin)

        identity = UnitIdentity("nimo-2", manifest.canary_units[0], 41, "a" * 32, 10, "s=1", 1, 1)
        with self.assertRaisesRegex(ContractError, "timeout"):
            EvidenceCollector(ActiveRunner(), manifest).wait_unit(identity, 0.001)

    def test_evidence_command_failure_is_fatal(self):
        runner = ScriptedRunner()
        runner.fail_token = "/proc/diskstats"
        collector = EvidenceCollector(runner, manifest_from(valid_raw()))
        with self.assertRaisesRegex(ContractError, "evidence command failed"):
            collector.required("nimo-2", ["cat", "/proc/diskstats"])

    def test_path_probe_failure_is_fatal_not_silently_absent(self):
        runner = ScriptedRunner()
        runner.fail_token = "stat -c"
        with self.assertRaisesRegex(ContractError, "mandatory path probe failed"):
            EvidenceCollector(runner, manifest_from(valid_raw())).path_exists(
                "nimo-2", "/var/tmp/halofpx-l21-small-evidence-nimo2"
            )

    def test_archive_failure_retains_evidence_root(self):
        manifest = manifest_from(valid_raw())
        runner = ScriptedRunner()
        runner.fail_token = "tar -cf"
        collector = EvidenceCollector(runner, manifest)
        with patch.object(collector, "path_exists", return_value=True):
            with self.assertRaises(ContractError):
                collector.finalize_archive({"outcome": "non_promotable"})
        root = manifest.paths["nimo-2"]["evidence_root"]
        self.assertFalse(any(argv[:4] == ("rm", "-rf", "--", root) for _, argv in runner.calls))

    def test_cleanup_failure_is_fatal_and_all_targets_are_attempted(self):
        manifest = manifest_from(valid_raw())
        runner = ScriptedRunner()
        runner.cleanup_fail = True
        with self.assertRaisesRegex(ContractError, "cleanup verification failed"):
            EvidenceCollector(runner, manifest).cleanup()
        removed = {argv[-1] for _, argv in runner.calls if argv and argv[0] == "rm"}
        packaging = {
            manifest.paths["nimo-2"]["evidence_root"],
            manifest.paths["nimo-2"]["archive_stage"],
            manifest.paths["nimo-2"]["cleanup_receipt"],
        }
        expected = {
            value for values in manifest.cleanup_paths.values()
            for value in values if value not in packaging
        }
        self.assertEqual(removed, expected)

    def test_loaded_unit_rejects_stop_and_reset_not_found_exit(self):
        manifest = manifest_from(valid_raw())

        class LoadedRunner(ScriptedRunner):
            def run(self, host, argv, *, stdin=None):
                if argv[:3] in (
                    ["systemctl", "--user", "stop"],
                    ["systemctl", "--user", "reset-failed"],
                ):
                    return Result(5, "", "not found")
                if argv[:3] == ["systemctl", "--user", "show"]:
                    return Result(0, "LoadState=loaded\nActiveState=inactive\nMainPID=0\n")
                return super().run(host, argv, stdin=stdin)

        receipt = EvidenceCollector(LoadedRunner(), manifest).cleanup_runtime()
        self.assertTrue(any(error.endswith(": stop") for error in receipt["errors"]))
        self.assertTrue(any(error.endswith(": reset-failed") for error in receipt["errors"]))


if __name__ == "__main__":
    unittest.main()
