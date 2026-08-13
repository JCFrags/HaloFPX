from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
import subprocess
import tempfile
import unittest
from datetime import datetime, timezone
from pathlib import Path
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPO_ROOT / "scripts" / "materialize-rocmfpx-fixture.py"
SPEC = importlib.util.spec_from_file_location(
    "materialize_rocmfpx_fixture", SCRIPT_PATH
)
assert SPEC is not None and SPEC.loader is not None
fixture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fixture)


class MaterializeROCmFPXFixtureTests(unittest.TestCase):
    def setUp(self) -> None:
        self.registry_path = fixture.DEFAULT_REGISTRY
        self.registry = json.loads(self.registry_path.read_text(encoding="utf-8"))

    def test_default_registry_is_valid_offline(self) -> None:
        fixture.validate_registry(self.registry, self.registry_path)

    def test_evidence_checksum_ledger_covers_every_companion(self) -> None:
        evidence = (
            REPO_ROOT
            / "docs"
            / "halofpx"
            / "evidence"
            / "2026-08-12-qwen3-0.6b-rocmfpx-fixture"
        )
        ledger_path = evidence / "SHA256SUMS"
        entries: dict[str, str] = {}
        for line in ledger_path.read_text(encoding="utf-8").splitlines():
            digest, name = line.split("  ", maxsplit=1)
            entries[name] = digest
        companions = {
            path.name
            for path in evidence.iterdir()
            if path.is_file() and path.name != ledger_path.name
        }
        self.assertEqual(set(entries), companions)
        for name, expected in entries.items():
            actual = hashlib.sha256((evidence / name).read_bytes()).hexdigest()
            self.assertEqual(actual, expected, name)

    def test_unknown_registry_key_is_rejected(self) -> None:
        changed = copy.deepcopy(self.registry)
        changed["unexpected"] = True
        with self.assertRaisesRegex(fixture.FixtureError, "unknown"):
            fixture.validate_registry(changed, self.registry_path)

    def test_artifact_path_cannot_escape_root(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(fixture.FixtureError, "escapes root"):
                fixture.safe_artifact_path(Path(directory), "../outside")

    def test_artifact_root_cannot_be_inside_checkout(self) -> None:
        args = argparse.Namespace(artifact_root=REPO_ROOT / "local-fixture")
        with self.assertRaisesRegex(fixture.FixtureError, "outside the Git checkout"):
            fixture.resolve_artifact_root(args, self.registry)

    def test_artifact_root_cannot_contain_checkout(self) -> None:
        args = argparse.Namespace(artifact_root=REPO_ROOT.parent)
        with self.assertRaisesRegex(fixture.FixtureError, "outside the Git checkout"):
            fixture.resolve_artifact_root(args, self.registry)

    def test_wrong_existing_download_is_not_overwritten(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = root / "source" / "fixture.bin"
            target.parent.mkdir()
            target.write_bytes(b"wrong")
            expected = b"right"
            record = {
                "relative_path": "source/fixture.bin",
                "url": "https://huggingface.co/example/resolve/pin/fixture.bin",
                "size_bytes": len(expected),
                "sha256": hashlib.sha256(expected).hexdigest(),
            }
            with self.assertRaisesRegex(fixture.FixtureError, "wrong SHA-256"):
                fixture.download_one(root, record, "test fixture")
            self.assertEqual(target.read_bytes(), b"wrong")

    def test_unresolved_argv_placeholder_is_rejected(self) -> None:
        with self.assertRaisesRegex(fixture.FixtureError, "unresolved placeholder"):
            fixture.render_argv(["{missing}"], {}, "test arguments")

    def test_rebuilt_binary_identity_is_recorded_but_not_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / "binary"
            binary.write_bytes(b"actual")
            expected = hashlib.sha256(b"expected").hexdigest()
            record = fixture.inspect_binary_identity(binary, expected, "test binary")
            self.assertFalse(record["matches_recorded_observation"])
            self.assertEqual(record["sha256"], hashlib.sha256(b"actual").hexdigest())
            with self.assertRaisesRegex(fixture.FixtureError, "recorded evidence"):
                fixture.inspect_binary_identity(
                    binary, expected, "test binary", require_recorded=True
                )

    def test_reported_commit_must_match_source_commit(self) -> None:
        expected = "6c88472bf5f567a1064f27f4d8a90fc8e2b47a02"
        reported = fixture.require_reported_commit(
            "llama_print_build_info: build = 400 (6c88472b)", expected, "tool"
        )
        self.assertEqual(reported, "6c88472b")
        with self.assertRaisesRegex(fixture.FixtureError, "source/binary mismatch"):
            fixture.require_reported_commit("version: 612 (b77f2bce)", expected, "tool")

    def test_attempt_log_is_reserved_exclusively(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            fixed = datetime(2026, 8, 13, 3, 30, tzinfo=timezone.utc)
            with mock.patch.object(fixture, "datetime") as mocked_datetime:
                mocked_datetime.now.return_value = fixed
                first, handle = fixture.create_attempt_log(root, "smoke", "q3")
                with handle:
                    handle.write(b"first\n")
                with self.assertRaisesRegex(fixture.FixtureError, "cannot allocate"):
                    fixture.create_attempt_log(root, "smoke", "q3")
            self.assertEqual(first.read_bytes(), b"first\n")

    def test_exact_git_source_must_be_clean(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            repository = Path(directory)
            subprocess.run(
                ["git", "init", str(repository)], check=True, capture_output=True
            )
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(repository),
                    "config",
                    "user.email",
                    "fixture@example.invalid",
                ],
                check=True,
            )
            subprocess.run(
                ["git", "-C", str(repository), "config", "user.name", "Fixture Test"],
                check=True,
            )
            tracked = repository / "tracked.txt"
            tracked.write_text("clean\n", encoding="utf-8")
            subprocess.run(
                ["git", "-C", str(repository), "add", "tracked.txt"], check=True
            )
            subprocess.run(
                ["git", "-C", str(repository), "commit", "-m", "fixture"],
                check=True,
                capture_output=True,
            )
            head = fixture.git_head(repository, "test source")
            fixture.require_exact_git_head(repository, head, "test source")
            tracked.write_text("dirty\n", encoding="utf-8")
            with self.assertRaisesRegex(fixture.FixtureError, "must be clean"):
                fixture.require_exact_git_head(repository, head, "test source")


if __name__ == "__main__":
    unittest.main()
