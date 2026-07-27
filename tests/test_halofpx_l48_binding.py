import hashlib
import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
VERIFIER = ROOT / "scripts" / "halofpx_l48_composed_result.py"
RUNNER = ROOT / "scripts" / "halofpx-l13-primary-retry.py"
SPEC = importlib.util.spec_from_file_location("halofpx_l48_result", VERIFIER)
assert SPEC and SPEC.loader
result_authority = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(result_authority)


def execution(phase: str, ordinal: int, sequence: int) -> dict[str, object]:
    identity = "replay" if phase == "restore" or ordinal == 3 else f"{phase}:{ordinal}"
    digest = hashlib.sha256(identity.encode()).hexdigest()
    return {
        "phase": phase,
        "ordinal": ordinal,
        "execution_sequence": sequence,
        "graph_uid": ordinal + 1,
        "prepared_status": 1,
        "final_status": 1,
        "prepared_root": digest,
        "scheduler_root": digest,
        "scheduler_tag": digest,
        "graph_entries": 4,
        "splits": 2,
        "copies": 1,
        "local": 2,
        "rpc": 2,
        "mutable_sessions": 1,
        "mutable_status": 1,
        "mutable_census": 3,
        "set": 1,
        "set_hash_hit": 1,
        "set_hash_miss": 0,
        "mutation_root": digest,
        "semantic_root": digest,
        "census_root": digest,
        "receipt_tag": digest,
        "graph_status": 2,
        "graph_sequence": sequence,
        "graph_digest": digest,
        "graph_transcript_root": digest,
        "graph_receipt_tag": digest,
    }


def payload() -> dict[str, object]:
    logits = hashlib.sha256(b"logits").hexdigest()
    return {
        "schema": "halofpx.l48.composed-result.v1",
        "attempt": hashlib.sha256(("11" * 16 + "|" + "22" * 16).encode()).hexdigest(),
        "lineage": {
            "capture_worker_invocation": "11" * 16,
            "restore_worker_invocation": "22" * 16,
        },
        "features": {
            "rpc_graph": 1, "scheduler": 2, "mutable_session": 1, "composition": 1,
        },
        "feature_off": False,
        "capture": [execution("capture", i, i + 1) for i in range(4)],
        "restore": [execution("restore", 0, 1)],
        "prompt_chunks": [512, 512, 104],
        "replay_count": 1,
        "tokens": {"capture": 4245, "restore": 4245},
        "logits": {"capture": logits, "restore": logits},
        "legacy_state_get_set": 0,
    }


class L48ResultTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.key = self.root / "control.key"
        self.key.write_text("11" * 32 + "\n" + "22" * 32 + "\n", encoding="ascii")
        os.chmod(self.key, 0o600)

    def tearDown(self):
        self.temp.cleanup()

    def command(self, *args, stdin=None):
        return subprocess.run(
            [sys.executable, str(VERIFIER), *args],
            input=stdin, text=True, capture_output=True, check=False)

    def signed_record(self, value):
        canonical = json.dumps(value, sort_keys=True, separators=(",", ":"))
        signed = self.command("sign", "--key-file", str(self.key), stdin=canonical)
        self.assertEqual(signed.returncode, 0, signed.stderr)
        record = self.root / "result.json"
        record.write_text(json.dumps({
            "payload": value, "auth_tag": signed.stdout.strip(),
        }, sort_keys=True, separators=(",", ":")) + "\n", encoding="ascii")
        os.chmod(record, 0o600)
        return record

    def test_exact_result_is_accepted(self):
        self.assertEqual(result_authority.validate(payload()), payload())

    def test_wrong_mode_key_refuses(self):
        os.chmod(self.key, 0o644)
        result = self.command(
            "sign", "--key-file", str(self.key),
            "--expected-key-sha256", hashlib.sha256(self.key.read_bytes()).hexdigest(),
            "--expected-owner", "connorb", stdin="{}")
        self.assertNotEqual(result.returncode, 0)

    def test_incomplete_duplicate_reordered_and_tampered_refuse(self):
        cases = []
        missing = payload()
        missing["capture"] = missing["capture"][:-1]
        cases.append(missing)
        duplicate = payload()
        duplicate["capture"][1]["execution_sequence"] = 1
        cases.append(duplicate)
        reordered = payload()
        reordered["capture"][0], reordered["capture"][1] = (
            reordered["capture"][1], reordered["capture"][0])
        cases.append(reordered)
        wrong_attempt = payload()
        wrong_attempt["attempt"] = "00" * 32
        cases.append(wrong_attempt)
        for value in cases:
            with self.assertRaises(result_authority.ResultError):
                result_authority.validate(value)
        tampered = payload()
        tampered["tokens"]["restore"] = 1
        with self.assertRaises(result_authority.ResultError):
            result_authority.validate(tampered)

    def test_runner_requires_exact_key_option(self):
        result = subprocess.run(
            [sys.executable, str(RUNNER), "--evidence-dir", str(self.root / "e"),
             "--l48-fixture", "--authority-key-file", "/tmp/wrong"],
            text=True, capture_output=True, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("exact manifest-owned authority key file", result.stderr)


if __name__ == "__main__":
    unittest.main()
