import hashlib
import importlib.util
import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]
VERIFIER = ROOT / "scripts" / "halofpx_l48_composed_result.py"
RUNNER = ROOT / "scripts" / "halofpx-l13-primary-retry.py"
L101_EVIDENCE = ROOT / "docs" / "halofpx" / "evidence" / "l101-attempt-final" / "child"
SPEC = importlib.util.spec_from_file_location("halofpx_l48_result", VERIFIER)
assert SPEC and SPEC.loader
result_authority = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(result_authority)
RUNNER_SPEC = importlib.util.spec_from_file_location("halofpx_l48_runner", RUNNER)
assert RUNNER_SPEC and RUNNER_SPEC.loader
runner = importlib.util.module_from_spec(RUNNER_SPEC)
RUNNER_SPEC.loader.exec_module(runner)


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
        "split_mapping_root": digest,
        "scheduler_root": digest,
        "scheduler_tag": digest,
        "graph_entries": 4,
        "splits": 2,
        "rpc_split_count": 1,
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
        "parent_uid": ordinal + 1,
        "split_ordinal": 0,
        "split_uid": ordinal + 101,
        "reconcile_status": 1,
        "rpc_splits": [{
            "backend_ordinal": 0,
            "parent_uid": ordinal + 1,
            "split_ordinal": 0,
            "split_uid": ordinal + 101,
            "reconcile_status": 1,
            "graph_status": 2,
            "graph_sequence": sequence,
            "graph_digest": digest,
            "graph_transcript_root": digest,
            "graph_receipt_tag": digest,
        }],
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

    def authority_args(self):
        if os.name != "nt":
            import pwd
            expected_owner = pwd.getpwuid(self.key.stat().st_uid).pw_name
        else:
            expected_owner = os.environ.get("USERNAME", "connorb")
        return (
            "--expected-key-sha256", hashlib.sha256(self.key.read_bytes()).hexdigest(),
            "--expected-owner", expected_owner,
        )

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

    def test_zero_legacy_hash_activity_with_authenticated_sets_is_accepted(self):
        value = payload()
        for record in value["capture"] + value["restore"]:
            record["set_hash_hit"] = 0
            record["set_hash_miss"] = 0
        self.assertEqual(result_authority.validate(value), value)

        if os.name == "nt":
            return
        canonical = json.dumps(value, sort_keys=True, separators=(",", ":"))
        signed = self.command(
            "sign", "--key-file", str(self.key), *self.authority_args(),
            stdin=canonical)
        self.assertEqual(signed.returncode, 0, signed.stderr)
        record = self.root / "result.json"
        record.write_text(json.dumps({
            "payload": value, "auth_tag": signed.stdout.strip(),
        }, sort_keys=True, separators=(",", ":")) + "\n", encoding="ascii")
        os.chmod(record, 0o600)
        verified = self.command(
            "verify", "--key-file", str(self.key), "--record", str(record),
            *self.authority_args())
        self.assertEqual(verified.returncode, 0, verified.stderr)
        self.assertEqual(json.loads(verified.stdout), value)

    def test_retained_l101_composed_executions_accept_zero_hash_activity(self):
        value = payload()
        for phase, expected_count in (("capture", 4), ("restore", 1)):
            log = (L101_EVIDENCE / f"{phase}.log").read_text(encoding="utf-8")
            line = next(
                item for item in log.splitlines()
                if item.startswith(f"[halofpx-composed-authority] phase={phase}|"))
            body = line.split(f"phase={phase}|", 1)[1].rsplit("|auth_tag=", 1)[0]
            markers = list(re.finditer(r"(?:^|\|)(prompt_[0-9]+|replay)=", body))
            records = []
            for ordinal, marker in enumerate(markers):
                end = markers[ordinal + 1].start() if ordinal + 1 < len(markers) else len(body)
                records.append(runner._composed_execution(
                    body[marker.end():end].strip("|"), phase, ordinal))
            self.assertEqual(len(records), expected_count)
            self.assertTrue(all(record["set"] == 7 for record in records))
            self.assertTrue(all(
                record["set_hash_hit"] == 0 and record["set_hash_miss"] == 0
                for record in records))
            for ordinal, record in enumerate(records):
                result_authority._execution(record, phase, ordinal)

    def test_zero_set_and_partial_mutable_authority_refuse(self):
        zero_set = payload()
        zero_set["capture"][0]["set"] = 0
        with self.assertRaisesRegex(
                result_authority.ResultError, "RPC mutable authority is incomplete"):
            result_authority.validate(zero_set)

        missing = payload()
        del missing["restore"][0]["mutation_root"]
        with self.assertRaisesRegex(
                result_authority.ResultError, "execution field set mismatch"):
            result_authority.validate(missing)

    def test_multiple_split_receipts_are_ordered_and_closed(self):
        value = payload()
        record = value["capture"][0]
        second = dict(record["rpc_splits"][0])
        second["split_ordinal"] = 2
        second["split_uid"] += 100
        second["graph_digest"] = hashlib.sha256(b"second split").hexdigest()
        record["rpc_splits"].append(second)
        record["rpc_split_count"] = 2
        self.assertEqual(result_authority.validate(value), value)
        record["rpc_splits"].reverse()
        with self.assertRaises(result_authority.ResultError):
            result_authority.validate(value)

    def test_missing_untyped_and_phase_divergent_split_refuse(self):
        missing = payload()
        missing["capture"][0]["rpc_split_count"] = 2
        with self.assertRaises(result_authority.ResultError):
            result_authority.validate(missing)
        untyped = payload()
        untyped["capture"][0]["rpc_splits"][0]["backend_ordinal"] = "0"
        with self.assertRaises(result_authority.ResultError):
            result_authority.validate(untyped)
        boolean = payload()
        boolean["capture"][0]["rpc_splits"][0]["backend_ordinal"] = True
        with self.assertRaises(result_authority.ResultError):
            result_authority.validate(boolean)
        divergent = payload()
        divergent["restore"][0]["rpc_splits"][0]["graph_digest"] = "ab" * 32
        divergent["restore"][0]["graph_digest"] = "ab" * 32
        with self.assertRaises(result_authority.ResultError):
            result_authority.validate(divergent)

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
