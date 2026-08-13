from __future__ import annotations

import ast
import copy
import datetime as dt
import importlib.util
import json
import subprocess
import sys
import unittest
from pathlib import Path
from typing import Any


REPO = Path(__file__).parents[1]
SOURCE = REPO / "scripts" / "halofpx_strix_nonce_protocol.py"
SPEC = importlib.util.spec_from_file_location("halofpx_strix_nonce_protocol", SOURCE)
assert SPEC is not None and SPEC.loader is not None
protocol = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = protocol
SPEC.loader.exec_module(protocol)

NOW = dt.datetime(2026, 8, 13, 7, 0, tzinfo=dt.timezone.utc)


class NonceProtocolTests(unittest.TestCase):
    def binding(self) -> dict[str, Any]:
        return copy.deepcopy(protocol.example_binding())

    def simulate(
        self,
        scenario: str = "success",
        *,
        binding: dict[str, Any] | None = None,
        nodes: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        return protocol.simulate_pair(
            self.binding() if binding is None else binding,
            observed_at=NOW,
            scenario=scenario,
            nodes=nodes,
        )

    def verify(self, transcript: Any) -> dict[str, Any]:
        result = protocol.verify_transcript(transcript)
        self.assertFalse(result["authorized"])
        self.assertFalse(result["simulation_qualified"])
        self.assertFalse(result["target_execution_qualified"])
        self.assertFalse(result["distributed_atomicity_proven"])
        return result

    def assert_refused(self, transcript: Any) -> dict[str, Any]:
        result = self.verify(transcript)
        self.assertEqual(result["classification"], "REFUSED")
        self.assertFalse(result["pair_committed"])
        self.assertFalse(result["transcript_accepted"])
        return result

    def assert_in_doubt(self, transcript: Any) -> dict[str, Any]:
        result = self.verify(transcript)
        self.assertEqual(result["classification"], "IN_DOUBT")
        self.assertFalse(result["pair_committed"])
        self.assertFalse(result["transcript_accepted"])
        return result

    def rewrite_role_binding(
        self,
        transcript: dict[str, Any],
        role: str,
        mutate: Any,
    ) -> None:
        """Create an internally coherent node journal for a divergent binding."""

        ledger = transcript["nodes"][role]
        mutate(ledger["binding"])
        parsed = protocol.parse_binding(ledger["binding"], "rewritten binding")
        ledger["binding_sha256"] = parsed.digest
        ledger["after"] = {
            "schema": protocol.STATE_SCHEMA,
            "epoch_high_water": parsed.epoch,
            "burned_nonce_commitments": sorted(
                set(ledger["before"]["burned_nonce_commitments"])
                | {parsed.nonce_commitment}
            ),
            "quarantine_transaction_id": protocol.ZERO_HASH,
        }
        phase_digests: dict[str, str] = {}
        previous = protocol.ZERO_HASH
        for entry in ledger["records"]:
            record = entry["record"]
            record["binding_sha256"] = parsed.digest
            record["transaction_id"] = parsed.transaction_id
            record["authorization_sha256"] = parsed.raw["authorization_sha256"]
            record["nonce_commitment"] = parsed.nonce_commitment
            record["epoch"] = parsed.epoch
            record["previous_record_sha256"] = previous
            digest = protocol._record_digest(record)
            entry["record_sha256"] = digest
            phase_digests[record["phase"]] = digest
            previous = digest

        previous = protocol.ZERO_HASH
        for entry in transcript["events"]:
            event = entry["event"]
            if event["role"] == role:
                event["binding_sha256"] = parsed.digest
                if event["kind"].endswith("_RESPONSE") and event["outcome"] == "ACK":
                    phase = event["kind"].removesuffix("_RESPONSE")
                    event["record_sha256"] = phase_digests[
                        {"PREPARE": "PREPARED", "ABORT": "ABORTED"}[phase]
                    ]
            event["previous_event_sha256"] = previous
            digest = protocol.domain_digest(protocol.DOMAIN_EVENT, event)
            entry["event_sha256"] = digest
            previous = digest

    def rehash_events(self, transcript: dict[str, Any]) -> None:
        previous = protocol.ZERO_HASH
        for sequence, entry in enumerate(transcript["events"], start=1):
            event = entry["event"]
            event["sequence"] = sequence
            event["previous_event_sha256"] = previous
            digest = protocol.domain_digest(protocol.DOMAIN_EVENT, event)
            entry["event_sha256"] = digest
            previous = digest

    def rewrite_commit_timestamp(
        self, transcript: dict[str, Any], role: str, observed_at_utc: str
    ) -> None:
        commit = transcript["nodes"][role]["records"][1]
        commit["record"]["observed_at_utc"] = observed_at_utc
        commit_digest = protocol._record_digest(commit["record"])
        commit["record_sha256"] = commit_digest
        for entry in transcript["events"]:
            event = entry["event"]
            if event["role"] == role and event["kind"] == "COMMIT_RESPONSE":
                event["record_sha256"] = commit_digest
        certificate = transcript["pair_certificate"]
        certificate["node_commit_record_sha256"][role] = commit_digest
        unhashed = copy.deepcopy(certificate)
        del unhashed["certificate_sha256"]
        certificate["certificate_sha256"] = protocol.domain_digest(
            protocol.DOMAIN_CERTIFICATE, unhashed
        )
        transcript["terminal"]["pair_certificate_sha256"] = certificate[
            "certificate_sha256"
        ]
        self.rehash_events(transcript)

    def clear_pair_quarantine(self, transcript: dict[str, Any]) -> None:
        for role in protocol.ROLES:
            transcript["nodes"][role]["after"][
                "quarantine_transaction_id"
            ] = protocol.ZERO_HASH

    def run_verify_cli(self, content: bytes) -> tuple[int, dict[str, Any]]:
        completed = subprocess.run(
            [sys.executable, "-X", "utf8", "-B", str(SOURCE), "verify"],
            input=content,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=10,
        )
        self.assertEqual(completed.stderr, b"")
        return completed.returncode, json.loads(completed.stdout.decode("utf-8"))

    def test_success_is_only_a_non_authorizing_simulated_complete(self) -> None:
        transcript = self.simulate()
        result = self.verify(transcript)
        self.assertEqual(result["classification"], "SIMULATED_COMPLETE")
        self.assertTrue(result["pair_committed"])
        self.assertTrue(result["transcript_accepted"])
        self.assertEqual(result["issues"], [])
        self.assertFalse(transcript["target_execution_enabled"])
        self.assertEqual(transcript["transport"], "in-memory-fake-only")
        self.assertEqual(transcript["durability"], "simulated-append-only")

    def test_success_has_prepare_then_commit_on_both_nodes(self) -> None:
        transcript = self.simulate()
        for role in protocol.ROLES:
            records = transcript["nodes"][role]["records"]
            self.assertEqual(
                [item["record"]["phase"] for item in records],
                ["PREPARED", "COMMITTED"],
            )
            self.assertTrue(all(item["simulated_durable"] for item in records))
            self.assertEqual(records[0]["record"]["previous_record_sha256"], protocol.ZERO_HASH)
            self.assertEqual(
                records[1]["record"]["previous_record_sha256"],
                records[0]["record_sha256"],
            )

    def test_pair_certificate_is_permanently_non_authorizing(self) -> None:
        certificate = self.simulate()["pair_certificate"]
        self.assertEqual(certificate["pair_state"], "SIMULATED_COMMITTED")
        for field in (
            "authorized",
            "simulation_qualified",
            "target_execution_qualified",
            "distributed_atomicity_proven",
        ):
            self.assertIs(certificate[field], False)

    def test_pair_certificate_binds_exact_commit_records(self) -> None:
        transcript = self.simulate()
        certificate = transcript["pair_certificate"]
        for role in protocol.ROLES:
            self.assertEqual(
                certificate["node_commit_record_sha256"][role],
                transcript["nodes"][role]["records"][-1]["record_sha256"],
            )

    def test_policy_abort_is_durable_and_refused(self) -> None:
        transcript = self.simulate("policy-abort")
        self.assert_refused(transcript)
        self.assertIsNone(transcript["pair_certificate"])
        for role in protocol.ROLES:
            self.assertEqual(
                [entry["record"]["phase"] for entry in transcript["nodes"][role]["records"]],
                ["PREPARED", "ABORTED"],
            )

    def test_abort_burns_nonce_and_epoch(self) -> None:
        transcript = self.simulate("policy-abort")
        nonce = protocol.parse_binding(transcript["binding"]).nonce_commitment
        for role in protocol.ROLES:
            before = transcript["nodes"][role]["before"]
            after = transcript["nodes"][role]["after"]
            self.assertEqual(before["epoch_high_water"], 6)
            self.assertEqual(after["epoch_high_water"], 7)
            self.assertNotIn(nonce, before["burned_nonce_commitments"])
            self.assertIn(nonce, after["burned_nonce_commitments"])

    def test_one_node_prepare_cannot_form_pair(self) -> None:
        transcript = self.simulate("one-node")
        self.assert_refused(transcript)
        self.assertEqual(transcript["terminal"]["reason"], "ONE_NODE_PREPARE")
        self.assertEqual(
            [item["record"]["phase"] for item in transcript["nodes"]["coordinator"]["records"]],
            ["PREPARED", "ABORTED"],
        )
        self.assertEqual(
            [item["record"]["phase"] for item in transcript["nodes"]["worker"]["records"]],
            ["ABORTED"],
        )

    def test_split_brain_binding_is_refused(self) -> None:
        transcript = self.simulate("split-brain")
        result = self.assert_refused(transcript)
        self.assertIn("BINDING_DIVERGENCE", result["issues"])
        self.assertNotEqual(
            transcript["nodes"]["coordinator"]["binding_sha256"],
            transcript["nodes"]["worker"]["binding_sha256"],
        )

    def test_expired_window_is_refused(self) -> None:
        result = self.assert_refused(self.simulate("expired"))
        self.assertIn("WINDOW_INACTIVE", result["issues"])

    def test_stale_epoch_is_refused(self) -> None:
        result = self.assert_refused(self.simulate("stale-epoch"))
        self.assertIn("STALE_EPOCH", result["issues"])

    def test_nonce_replay_is_refused(self) -> None:
        result = self.assert_refused(self.simulate("replay"))
        self.assertIn("NONCE_REPLAY", result["issues"])

    def test_real_reuse_of_persistent_fake_state_is_refused(self) -> None:
        nodes = {
            role: protocol.FakeDurableNode(role, epoch_high_water=6)
            for role in protocol.ROLES
        }
        first = self.simulate(nodes=nodes)
        self.assertEqual(self.verify(first)["classification"], "SIMULATED_COMPLETE")
        second_binding = self.binding()
        second_binding["epoch"] = 8
        second_binding["attempt_id"] = "aa" * 32
        second = self.simulate(binding=second_binding, nodes=nodes)
        result = self.assert_refused(second)
        self.assertIn("NONCE_REPLAY", result["issues"])

    def test_same_nonce_under_different_authorization_is_still_replay(self) -> None:
        nodes = {
            role: protocol.FakeDurableNode(role, epoch_high_water=6)
            for role in protocol.ROLES
        }
        first = self.simulate(nodes=nodes)
        self.assertEqual(self.verify(first)["classification"], "SIMULATED_COMPLETE")
        second_binding = self.binding()
        second_binding["authorization_sha256"] = "ab" * 32
        second_binding["epoch"] = 8
        second_binding["attempt_id"] = "bc" * 32
        second = self.simulate(binding=second_binding, nodes=nodes)
        result = self.assert_refused(second)
        self.assertIn("NONCE_REPLAY", result["issues"])

    def test_asymmetric_epoch_prestates_are_not_admitted(self) -> None:
        nodes = {
            "coordinator": protocol.FakeDurableNode(
                "coordinator", epoch_high_water=6
            ),
            "worker": protocol.FakeDurableNode("worker", epoch_high_water=7),
        }
        with self.assertRaisesRegex(
            protocol.ProtocolError, "persistent prestates differ"
        ):
            self.simulate(nodes=nodes)
        self.assertEqual(nodes["coordinator"].export()["records"], [])
        self.assertEqual(nodes["worker"].export()["records"], [])

    def test_asymmetric_nonce_burn_prestates_are_not_admitted(self) -> None:
        prior_commitment = "ab" * 32
        nodes = {
            "coordinator": protocol.FakeDurableNode(
                "coordinator",
                epoch_high_water=6,
                burned_nonce_commitments=(prior_commitment,),
            ),
            "worker": protocol.FakeDurableNode("worker", epoch_high_water=6),
        }
        with self.assertRaisesRegex(
            protocol.ProtocolError, "persistent prestates differ"
        ):
            self.simulate(nodes=nodes)
        self.assertEqual(nodes["coordinator"].export()["records"], [])
        self.assertEqual(nodes["worker"].export()["records"], [])

    def test_transcript_with_coherent_but_asymmetric_prestates_is_in_doubt(
        self,
    ) -> None:
        transcript = self.simulate("policy-abort")
        prior_commitment = "ab" * 32
        for state_name in ("before", "after"):
            burned = transcript["nodes"]["worker"][state_name][
                "burned_nonce_commitments"
            ]
            burned.append(prior_commitment)
            burned.sort()
        result = self.assert_in_doubt(transcript)
        self.assertIn("PERSISTENT_PRESTATE_DIVERGENCE", result["issues"])
        self.assertIn("SCENARIO_ISSUE_DIVERGENCE", result["issues"])
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_lost_prepare_response_is_in_doubt(self) -> None:
        transcript = self.simulate("lost-prepare-response")
        self.assert_in_doubt(transcript)
        transaction_id = protocol.parse_binding(transcript["binding"]).transaction_id
        for role in protocol.ROLES:
            self.assertEqual(
                transcript["nodes"][role]["after"]["quarantine_transaction_id"],
                transaction_id,
            )

    def test_lost_commit_response_is_in_doubt(self) -> None:
        transcript = self.simulate("lost-commit-response")
        self.assert_in_doubt(transcript)
        self.assertEqual(
            transcript["nodes"]["coordinator"]["records"][-1]["record"]["phase"],
            "COMMITTED",
        )
        self.assertEqual(
            transcript["nodes"]["worker"]["records"][-1]["record"]["phase"],
            "PREPARED",
        )

    def test_lost_abort_response_is_still_in_doubt(self) -> None:
        self.assert_in_doubt(self.simulate("lost-abort-response"))

    def test_in_doubt_fake_pair_rejects_every_later_attempt(self) -> None:
        nodes = {
            role: protocol.FakeDurableNode(role, epoch_high_water=6)
            for role in protocol.ROLES
        }
        first = self.simulate("lost-prepare-response", nodes=nodes)
        self.assert_in_doubt(first)
        later = self.binding()
        later["nonce"] = "ab" * 32
        later["attempt_id"] = "bc" * 32
        later["epoch"] = 8
        with self.assertRaisesRegex(protocol.ProtocolError, "quarantined"):
            self.simulate(binding=later, nodes=nodes)

    def test_in_doubt_transcript_requires_pair_quarantine(self) -> None:
        transcript = self.simulate("lost-commit-response")
        transcript["nodes"]["worker"]["after"][
            "quarantine_transaction_id"
        ] = protocol.ZERO_HASH
        result = self.assert_in_doubt(transcript)
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_unexpected_refusal_quarantine_is_sticky_ambiguity(self) -> None:
        transcript = self.simulate("policy-abort")
        transaction_id = protocol.parse_binding(transcript["binding"]).transaction_id
        transcript["nodes"]["worker"]["after"][
            "quarantine_transaction_id"
        ] = transaction_id
        result = self.assert_in_doubt(transcript)
        self.assertIn("UNEXPECTED_PAIR_QUARANTINE", result["issues"])
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_policy_abort_with_lost_nonce_burn_is_in_doubt(self) -> None:
        transcript = self.simulate("policy-abort")
        nonce = protocol.parse_binding(transcript["binding"]).nonce_commitment
        transcript["nodes"]["worker"]["after"][
            "burned_nonce_commitments"
        ].remove(nonce)
        result = self.assert_in_doubt(transcript)
        self.assertIn("STATE_TRANSITION_DIVERGENCE", result["issues"])
        self.assertIn("SCENARIO_ISSUE_DIVERGENCE", result["issues"])
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_policy_abort_with_lost_epoch_high_water_is_in_doubt(self) -> None:
        transcript = self.simulate("policy-abort")
        transcript["nodes"]["worker"]["after"]["epoch_high_water"] = 6
        result = self.assert_in_doubt(transcript)
        self.assertIn("STATE_TRANSITION_DIVERGENCE", result["issues"])
        self.assertIn("SCENARIO_ISSUE_DIVERGENCE", result["issues"])
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_commit_before_both_prepares_is_definitely_refused(self) -> None:
        result = self.assert_refused(self.simulate("reordered-commit"))
        self.assertIn("COMMIT_BEFORE_BOTH_PREPARED", result["issues"])

    def test_corrupt_commit_record_is_in_doubt(self) -> None:
        result = self.assert_in_doubt(self.simulate("corrupt-commit-record"))
        self.assertIn("CORRUPT_DURABLE_RECORD", result["issues"])

    def test_every_generated_scenario_has_its_exact_allowed_issue_set(self) -> None:
        for scenario in protocol.SCENARIOS:
            with self.subTest(scenario=scenario):
                result = self.verify(self.simulate(scenario))
                self.assertEqual(
                    set(result["issues"]),
                    set(protocol.SCENARIO_ALLOWED_ISSUES[scenario]),
                )

    def test_missing_worker_after_commit_evidence_is_in_doubt(self) -> None:
        transcript = self.simulate()
        del transcript["nodes"]["worker"]
        self.assert_in_doubt(transcript)

    def test_removing_one_commit_is_in_doubt(self) -> None:
        transcript = self.simulate()
        transcript["nodes"]["worker"]["records"].pop()
        self.assert_in_doubt(transcript)

    def test_orphan_prepared_record_cannot_be_relabelled_refused(self) -> None:
        transcript = self.simulate("lost-prepare-response")
        transcript["fault_scenario"] = "expired"
        response = transcript["events"][1]["event"]
        response["outcome"] = "REJECTED"
        response["record_sha256"] = protocol.ZERO_HASH
        transcript["events"][-1]["event"]["outcome"] = "REFUSED"
        transcript["terminal"]["status"] = "REFUSED"
        transcript["terminal"]["reason"] = "WINDOW_INACTIVE"
        self.clear_pair_quarantine(transcript)
        self.rehash_events(transcript)
        result = self.assert_in_doubt(transcript)
        self.assertIn("RECORD_RESPONSE_BIJECTION", result["issues"])
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_orphan_aborted_record_is_in_doubt(self) -> None:
        transcript = self.simulate("policy-abort")
        response = transcript["events"][7]["event"]
        response["outcome"] = "REJECTED"
        response["record_sha256"] = protocol.ZERO_HASH
        self.rehash_events(transcript)
        result = self.assert_in_doubt(transcript)
        self.assertIn("RECORD_RESPONSE_BIJECTION", result["issues"])
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_orphan_committed_record_is_in_doubt(self) -> None:
        transcript = self.simulate()
        response = transcript["events"][7]["event"]
        response["outcome"] = "REJECTED"
        response["record_sha256"] = protocol.ZERO_HASH
        self.rehash_events(transcript)
        result = self.assert_in_doubt(transcript)
        self.assertIn("RECORD_RESPONSE_BIJECTION", result["issues"])

    def test_event_reorder_after_commit_evidence_is_in_doubt(self) -> None:
        transcript = self.simulate()
        transcript["events"][4], transcript["events"][6] = (
            transcript["events"][6],
            transcript["events"][4],
        )
        self.assert_in_doubt(transcript)

    def test_coherently_rehashed_commit_event_binding_divergence_is_in_doubt(self) -> None:
        transcript = self.simulate()
        divergent = "ab" * 32
        for entry in transcript["events"]:
            event = entry["event"]
            if event["role"] == "worker" and event["kind"].startswith("COMMIT_"):
                event["binding_sha256"] = divergent
        self.rehash_events(transcript)
        result = self.assert_in_doubt(transcript)
        self.assertIn("BINDING_DIVERGENCE", result["issues"])

    def test_coherently_rehashed_event_timestamp_reorder_is_in_doubt(self) -> None:
        transcript = self.simulate()
        transcript["events"][5]["event"]["observed_at_utc"] = (
            transcript["events"][4]["event"]["observed_at_utc"]
        )
        self.rehash_events(transcript)
        result = self.assert_in_doubt(transcript)
        self.assertIn("PHASE_REORDER", result["issues"])

    def test_verification_time_must_follow_final_event(self) -> None:
        transcript = self.simulate()
        transcript["verification_time_utc"] = transcript["events"][-1]["event"][
            "observed_at_utc"
        ]
        result = self.assert_in_doubt(transcript)
        self.assertIn("VERIFICATION_TIME_REORDER", result["issues"])

    def test_success_cannot_be_relabelled_lost_commit_scenario(self) -> None:
        transcript = self.simulate()
        transcript["fault_scenario"] = "lost-commit-response"
        result = self.assert_in_doubt(transcript)
        self.assertIn("SCENARIO_EVENT_GRAMMAR", result["issues"])
        self.assertIn("SCENARIO_TERMINAL_GRAMMAR", result["issues"])

    def test_policy_abort_cannot_be_relabelled_split_brain(self) -> None:
        transcript = self.simulate("policy-abort")
        transcript["fault_scenario"] = "split-brain"
        transcript["terminal"]["reason"] = "BINDING_DIVERGENCE"
        result = self.assert_in_doubt(transcript)
        self.assertIn("SCENARIO_FAULT_PREDICATE", result["issues"])

    def test_stale_epoch_cannot_be_relabelled_nonce_replay(self) -> None:
        transcript = self.simulate("stale-epoch")
        transcript["fault_scenario"] = "replay"
        transcript["terminal"]["reason"] = "NONCE_REPLAY"
        result = self.assert_in_doubt(transcript)
        self.assertIn("SCENARIO_FAULT_PREDICATE", result["issues"])

    def test_impossible_prepended_abort_rejection_is_in_doubt(self) -> None:
        transcript = self.simulate()
        binding_sha256 = transcript["binding_sha256"]
        prefix_log = protocol.FakeEventLog()
        prefix_log.append(
            kind="ABORT_REQUEST",
            role="worker",
            outcome="SENT",
            binding_sha256=binding_sha256,
            record_sha256=protocol.ZERO_HASH,
            observed=NOW - dt.timedelta(seconds=2),
        )
        prefix_log.append(
            kind="ABORT_RESPONSE",
            role="worker",
            outcome="REJECTED",
            binding_sha256=binding_sha256,
            record_sha256=protocol.ZERO_HASH,
            observed=NOW - dt.timedelta(seconds=1),
        )
        transcript["events"] = prefix_log.entries + transcript["events"]
        self.rehash_events(transcript)
        result = self.assert_in_doubt(transcript)
        self.assertIn("SCENARIO_EVENT_GRAMMAR", result["issues"])

    def test_coherently_rehashed_record_timestamp_reorder_is_in_doubt(self) -> None:
        transcript = self.simulate()
        self.rewrite_commit_timestamp(
            transcript,
            "worker",
            transcript["nodes"]["worker"]["records"][0]["record"][
                "observed_at_utc"
            ],
        )
        result = self.assert_in_doubt(transcript)
        self.assertIn("PHASE_REORDER", result["issues"])

    def test_coherently_rehashed_record_before_request_is_in_doubt(self) -> None:
        transcript = self.simulate()
        self.rewrite_commit_timestamp(
            transcript,
            "worker",
            transcript["events"][5]["event"]["observed_at_utc"],
        )
        result = self.assert_in_doubt(transcript)
        self.assertIn("PHASE_REORDER", result["issues"])

    def test_extra_event_after_terminal_is_rejected(self) -> None:
        transcript = self.simulate()
        transcript["events"].append(copy.deepcopy(transcript["events"][-1]))
        self.assert_in_doubt(transcript)

    def test_replayed_prepare_response_is_rejected(self) -> None:
        transcript = self.simulate()
        transcript["events"].insert(2, copy.deepcopy(transcript["events"][1]))
        self.rehash_events(transcript)
        result = self.assert_in_doubt(transcript)
        self.assertIn("EVENT_REPLAY", result["issues"])

    def test_terminal_cannot_downgrade_lost_response_to_refused(self) -> None:
        transcript = self.simulate("lost-commit-response")
        transcript["terminal"]["status"] = "REFUSED"
        result = self.assert_in_doubt(transcript)
        self.assertIn("TERMINAL_CLASSIFICATION_DIVERGENCE", result["issues"])

    def test_terminal_authorized_true_is_never_propagated(self) -> None:
        transcript = self.simulate()
        transcript["terminal"]["authorized"] = True
        result = self.assert_in_doubt(transcript)
        self.assertIn("UNSAFE_TERMINAL_CLAIM", result["issues"])

    def test_unknown_terminal_reason_is_rejected(self) -> None:
        transcript = self.simulate()
        transcript["terminal"]["reason"] = "operator-says-ok"
        result = self.assert_in_doubt(transcript)
        self.assertIn("MALFORMED_TERMINAL", result["issues"])

    def test_terminal_reason_must_match_terminal_status(self) -> None:
        transcript = self.simulate()
        transcript["terminal"]["reason"] = "POLICY_ABORT"
        result = self.assert_in_doubt(transcript)
        self.assertIn("MALFORMED_TERMINAL", result["issues"])

    def test_refusal_with_nonzero_terminal_certificate_is_in_doubt(self) -> None:
        transcript = self.simulate("policy-abort")
        transcript["terminal"]["pair_certificate_sha256"] = "ab" * 32
        result = self.assert_in_doubt(transcript)
        self.assertIn("UNSAFE_TERMINAL_CERTIFICATE", result["issues"])
        self.assertIn("SCENARIO_ISSUE_DIVERGENCE", result["issues"])
        self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_non_string_terminal_status_never_crashes_or_authorizes(self) -> None:
        transcript = self.simulate()
        transcript["terminal"]["status"] = {"not": "a status"}
        result = self.assert_in_doubt(transcript)
        self.assertTrue(
            any(issue.startswith("MALFORMED_TRANSCRIPT:") for issue in result["issues"])
        )

    def test_certificate_authorized_true_is_never_propagated(self) -> None:
        transcript = self.simulate()
        transcript["pair_certificate"]["authorized"] = True
        result = self.assert_in_doubt(transcript)
        self.assertIn("UNSAFE_CERTIFICATE_CLAIM", result["issues"])

    def test_certificate_simulation_qualified_true_is_rejected(self) -> None:
        transcript = self.simulate()
        transcript["pair_certificate"]["simulation_qualified"] = True
        result = self.assert_in_doubt(transcript)
        self.assertIn("UNSAFE_CERTIFICATE_CLAIM", result["issues"])

    def test_certificate_digest_corruption_is_in_doubt(self) -> None:
        transcript = self.simulate()
        transcript["pair_certificate"]["certificate_sha256"] = "00" * 32
        result = self.assert_in_doubt(transcript)
        self.assertIn("CORRUPT_PAIR_CERTIFICATE", result["issues"])

    def test_committed_transcript_verified_after_expiry_is_in_doubt(self) -> None:
        transcript = self.simulate()
        transcript["verification_time_utc"] = "2026-08-13T08:00:01Z"
        result = self.assert_in_doubt(transcript)
        self.assertIn("WINDOW_INACTIVE", result["issues"])

    def test_explicit_late_window_never_emits_a_pair_certificate(self) -> None:
        transcript = self.simulate("late-window")
        binding = protocol.parse_binding(transcript["binding"])
        result = self.assert_in_doubt(transcript)
        self.assertEqual(result["issues"], ["WINDOW_INACTIVE"])
        self.assertEqual(transcript["fault_scenario"], "late-window")
        self.assertEqual(transcript["terminal"]["reason"], "WINDOW_INACTIVE")
        self.assertIsNone(transcript["pair_certificate"])
        verification_time = protocol.parse_utc(
            transcript["verification_time_utc"], "verification time"
        )
        self.assertFalse(protocol.active_at(binding, verification_time))
        for role in protocol.ROLES:
            self.assertEqual(
                transcript["nodes"][role]["after"]["quarantine_transaction_id"],
                binding.transaction_id,
            )

    def test_success_window_boundary_is_rejected_before_attempt(self) -> None:
        binding = self.binding()
        binding["expires_utc"] = protocol.format_utc(
            NOW
            + dt.timedelta(
                seconds=protocol.SUCCESS_VERIFICATION_OFFSET_SECONDS
            )
        )
        nodes = {
            role: protocol.FakeDurableNode(role, epoch_high_water=6)
            for role in protocol.ROLES
        }
        with self.assertRaisesRegex(
            protocol.ProtocolError, "schedule does not fit"
        ):
            self.simulate(binding=binding, nodes=nodes)
        for role in protocol.ROLES:
            self.assertEqual(nodes[role].export()["records"], [])
            self.assertEqual(nodes[role].epoch_high_water, 6)
            self.assertEqual(nodes[role].burned_nonce_commitments, set())

    def test_success_window_one_second_past_verification_can_complete(self) -> None:
        binding = self.binding()
        binding["expires_utc"] = protocol.format_utc(
            NOW
            + dt.timedelta(
                seconds=protocol.SUCCESS_VERIFICATION_OFFSET_SECONDS + 1
            )
        )
        transcript = self.simulate(binding=binding)
        result = self.verify(transcript)
        self.assertEqual(result["classification"], "SIMULATED_COMPLETE")
        self.assertEqual(result["issues"], [])
        self.assertIsNotNone(transcript["pair_certificate"])

    def test_every_ordinary_scenario_has_an_exact_expiry_boundary(self) -> None:
        self.assertEqual(
            set(protocol.SCENARIO_ACTIVE_THROUGH_OFFSET_SECONDS),
            set(protocol.SCENARIOS) - {"expired", "late-window"},
        )
        for scenario, offset in (
            protocol.SCENARIO_ACTIVE_THROUGH_OFFSET_SECONDS.items()
        ):
            with self.subTest(scenario=scenario, boundary="before-not-before"):
                binding = self.binding()
                binding["not_before_utc"] = protocol.format_utc(
                    NOW + dt.timedelta(seconds=1)
                )
                with self.assertRaisesRegex(
                    protocol.ProtocolError, "schedule does not fit"
                ):
                    self.simulate(scenario, binding=binding)

            for remaining in range(offset + 1):
                with self.subTest(
                    scenario=scenario,
                    boundary="insufficient-remaining-window",
                    remaining=remaining,
                ):
                    binding = self.binding()
                    binding["expires_utc"] = protocol.format_utc(
                        NOW + dt.timedelta(seconds=remaining)
                    )
                    with self.assertRaisesRegex(
                        protocol.ProtocolError, "schedule does not fit"
                    ):
                        self.simulate(scenario, binding=binding)

            with self.subTest(scenario=scenario, boundary="expiry-plus-one"):
                binding = self.binding()
                binding["expires_utc"] = protocol.format_utc(
                    NOW + dt.timedelta(seconds=offset + 1)
                )
                result = self.verify(self.simulate(scenario, binding=binding))
                self.assertEqual(
                    set(result["issues"]),
                    set(protocol.SCENARIO_ALLOWED_ISSUES[scenario]),
                )
                self.assertNotIn("SCENARIO_FAULT_PREDICATE", result["issues"])
                self.assertNotIn("SCENARIO_ISSUE_DIVERGENCE", result["issues"])

    def test_late_window_requires_twelve_second_authorization_window(self) -> None:
        too_short = self.binding()
        too_short["not_before_utc"] = protocol.format_utc(NOW)
        too_short["expires_utc"] = protocol.format_utc(
            NOW
            + dt.timedelta(
                seconds=protocol.LATE_WINDOW_START_OFFSET_SECONDS - 1
            )
        )
        with self.assertRaisesRegex(
            protocol.ProtocolError, "schedule does not fit"
        ):
            self.simulate("late-window", binding=too_short)

        exact = self.binding()
        exact["not_before_utc"] = protocol.format_utc(NOW)
        exact["expires_utc"] = protocol.format_utc(
            NOW
            + dt.timedelta(seconds=protocol.LATE_WINDOW_START_OFFSET_SECONDS)
        )
        transcript = self.simulate("late-window", binding=exact)
        result = self.assert_in_doubt(transcript)
        self.assertEqual(result["issues"], ["WINDOW_INACTIVE"])

    def test_expired_scenario_is_self_consistent_for_one_second_window(self) -> None:
        binding = self.binding()
        binding["not_before_utc"] = protocol.format_utc(NOW)
        binding["expires_utc"] = protocol.format_utc(
            NOW + dt.timedelta(seconds=1)
        )
        result = self.assert_refused(
            self.simulate("expired", binding=binding)
        )
        self.assertEqual(result["issues"], ["WINDOW_INACTIVE"])

    def test_each_material_binding_change_is_rejected(self) -> None:
        mutations = {
            "authorization": lambda value: value.__setitem__("authorization_sha256", "aa" * 32),
            "nonce": lambda value: value.__setitem__("nonce", "bb" * 32),
            "window": lambda value: value.__setitem__("expires_utc", "2026-08-13T07:59:59Z"),
            "commit": lambda value: value["source"].__setitem__("commit", "c" * 40),
            "tree": lambda value: value["source"].__setitem__("tree_sha256", "cc" * 32),
            "executable": lambda value: value["source"].__setitem__("executable_sha256", "dd" * 32),
            "plan": lambda value: value.__setitem__("plan_sha256", "ee" * 32),
            "policy": lambda value: value.__setitem__("policy_sha256", "dd" * 32),
            "incident": lambda value: value.__setitem__("incident_sha256", "cc" * 32),
        }
        for name, mutate in mutations.items():
            with self.subTest(name=name):
                transcript = self.simulate()
                mutate(transcript["binding"])
                self.assert_in_doubt(transcript)

    def test_unexpected_policy_abort_binding_divergence_is_in_doubt(self) -> None:
        mutations = {
            "authorization": lambda value: value.__setitem__("authorization_sha256", "aa" * 32),
            "nonce": lambda value: value.__setitem__("nonce", "bb" * 32),
            "window": lambda value: value.__setitem__("expires_utc", "2026-08-13T07:59:59Z"),
            "commit": lambda value: value["source"].__setitem__("commit", "c" * 40),
            "tree": lambda value: value["source"].__setitem__("tree_sha256", "cc" * 32),
            "executable": lambda value: value["source"].__setitem__("executable_sha256", "dd" * 32),
            "plan": lambda value: value.__setitem__("plan_sha256", "ee" * 32),
            "policy": lambda value: value.__setitem__("policy_sha256", "dd" * 32),
            "incident": lambda value: value.__setitem__("incident_sha256", "cc" * 32),
        }
        for name, mutate in mutations.items():
            with self.subTest(name=name):
                transcript = self.simulate("policy-abort")
                self.rewrite_role_binding(transcript, "worker", mutate)
                result = self.assert_in_doubt(transcript)
                self.assertIn("BINDING_DIVERGENCE", result["issues"])
                self.assertIn("SCENARIO_ISSUE_DIVERGENCE", result["issues"])
                self.assertIn("MISSING_PAIR_QUARANTINE", result["issues"])

    def test_cross_transaction_node_splice_is_in_doubt(self) -> None:
        first = self.simulate()
        second_binding = self.binding()
        second_binding["nonce"] = "ab" * 32
        second_binding["attempt_id"] = "bc" * 32
        second = self.simulate(binding=second_binding)
        first["nodes"]["worker"] = second["nodes"]["worker"]
        self.assert_in_doubt(first)

    def test_record_hash_chain_corruption_is_in_doubt(self) -> None:
        transcript = self.simulate()
        transcript["nodes"]["worker"]["records"][1]["record"][
            "previous_record_sha256"
        ] = protocol.ZERO_HASH
        self.assert_in_doubt(transcript)

    def test_replayed_record_is_in_doubt(self) -> None:
        transcript = self.simulate()
        transcript["nodes"]["worker"]["records"].append(
            copy.deepcopy(transcript["nodes"]["worker"]["records"][-1])
        )
        self.assert_in_doubt(transcript)

    def test_boolean_epoch_is_not_an_integer(self) -> None:
        binding = self.binding()
        binding["epoch"] = True
        with self.assertRaisesRegex(protocol.ProtocolError, "integer"):
            protocol.parse_binding(binding)

    def test_window_longer_than_eight_hours_is_rejected(self) -> None:
        binding = self.binding()
        binding["expires_utc"] = "2026-08-13T14:00:01Z"
        with self.assertRaisesRegex(protocol.ProtocolError, "eight hours"):
            protocol.parse_binding(binding)

    def test_noncanonical_timestamp_is_rejected(self) -> None:
        binding = self.binding()
        binding["not_before_utc"] = "2026-08-13T06:00:00+00:00"
        with self.assertRaisesRegex(protocol.ProtocolError, "canonical UTC"):
            protocol.parse_binding(binding)

    def test_unknown_binding_field_is_rejected(self) -> None:
        binding = self.binding()
        binding["owner"] = "self-asserted"
        with self.assertRaisesRegex(protocol.ProtocolError, "closed field set"):
            protocol.parse_binding(binding)

    def test_duplicate_json_key_is_rejected(self) -> None:
        with self.assertRaisesRegex(protocol.ProtocolError, "duplicate JSON key"):
            protocol.parse_closed_json(b'{"schema":"a","schema":"b"}', "fixture")

    def test_malformed_transcript_never_authorizes(self) -> None:
        result = self.assert_refused({"schema": protocol.TRANSCRIPT_SCHEMA})
        self.assertTrue(result["issues"])

    def test_all_scalar_fields_fail_closed_under_malformed_json_types(self) -> None:
        transcript = self.simulate()
        paths: list[tuple[str | int, ...]] = []

        def collect(value: Any, path: tuple[str | int, ...] = ()) -> None:
            if isinstance(value, dict):
                for key, item in value.items():
                    collect(item, path + (key,))
            elif isinstance(value, list):
                for index, item in enumerate(value):
                    collect(item, path + (index,))
            else:
                paths.append(path)

        collect(transcript)
        for path in paths:
            for malformed in ([], {}, None, True, 0, ""):
                candidate = copy.deepcopy(transcript)
                parent: Any = candidate
                for component in path[:-1]:
                    parent = parent[component]
                parent[path[-1]] = copy.deepcopy(malformed)
                try:
                    result = protocol.verify_transcript(candidate)
                except Exception as exc:  # pragma: no cover - failure diagnostic
                    self.fail(f"malformed path {path!r} raised {exc!r}")
                self.assertIs(result["authorized"], False)
                self.assertIs(result["simulation_qualified"], False)
                self.assertIs(result["target_execution_qualified"], False)
                self.assertIs(result["distributed_atomicity_proven"], False)

    def test_deep_json_cli_returns_closed_refusal_without_recursion_crash(self) -> None:
        depth = 2_000
        content = b'{"deep":' + (b"[" * depth) + b"0" + (b"]" * depth) + b"}"
        returncode, result = self.run_verify_cli(content)
        self.assertEqual(returncode, 2)
        self.assertIn(result["classification"], {"REFUSED", "IN_DOUBT"})
        self.assertIs(result["authorized"], False)

    def test_oversized_json_integer_cli_returns_closed_refusal(self) -> None:
        content = b'{"integer":' + (b"1" * 5_000) + b"}"
        returncode, result = self.run_verify_cli(content)
        self.assertEqual(returncode, 2)
        self.assertEqual(result["classification"], "REFUSED")
        self.assertIs(result["authorized"], False)

    def test_bounded_raw_scan_preserves_uncertainty_for_deep_commit_marker(self) -> None:
        depth = protocol.RAW_SCAN_MAX_DEPTH + 5
        content = (
            b'{"deep":'
            + (b"[" * depth)
            + b'{"kind":"COMMIT_REQUEST"}'
            + (b"]" * depth)
            + b"}"
        )
        returncode, result = self.run_verify_cli(content)
        self.assertEqual(returncode, 2)
        self.assertEqual(result["classification"], "IN_DOUBT")
        self.assertIn("RAW_EVIDENCE_SCAN_LIMIT", result["issues"])
        self.assertIs(result["authorized"], False)

    def test_bounded_raw_scan_preserves_uncertainty_for_wide_json(self) -> None:
        content = json.dumps(
            {"wide": [0] * (protocol.RAW_SCAN_MAX_NODES + 1)},
            separators=(",", ":"),
        ).encode("utf-8")
        returncode, result = self.run_verify_cli(content)
        self.assertEqual(returncode, 2)
        self.assertEqual(result["classification"], "IN_DOUBT")
        self.assertIn("RAW_EVIDENCE_SCAN_LIMIT", result["issues"])

    def test_naive_simulation_clock_is_rejected(self) -> None:
        with self.assertRaisesRegex(protocol.ProtocolError, "timezone-aware"):
            protocol.simulate_pair(
                self.binding(), observed_at=dt.datetime(2026, 8, 13, 7, 0)
            )

    def test_side_effectful_fake_subclass_is_not_admitted(self) -> None:
        class NotExact(protocol.FakeDurableNode):
            pass

        nodes = {
            "coordinator": NotExact("coordinator", epoch_high_water=6),
            "worker": protocol.FakeDurableNode("worker", epoch_high_water=6),
        }
        with self.assertRaisesRegex(protocol.ProtocolError, "exact in-memory fake"):
            self.simulate(nodes=nodes)

    def test_module_has_no_target_or_filesystem_implementation(self) -> None:
        source = SOURCE.read_text(encoding="utf-8")
        tree = ast.parse(source)
        imports: set[str] = set()
        called_names: set[str] = set()
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                imports.update(alias.name.split(".")[0] for alias in node.names)
            elif isinstance(node, ast.ImportFrom) and node.module:
                imports.add(node.module.split(".")[0])
            elif isinstance(node, ast.Call) and isinstance(node.func, ast.Name):
                called_names.add(node.func.id)
        self.assertTrue(protocol.TARGET_EXECUTION_ENABLED is False)
        self.assertFalse(
            imports
            & {
                "os",
                "pathlib",
                "socket",
                "subprocess",
                "urllib",
                "http",
                "requests",
                "paramiko",
            }
        )
        self.assertNotIn("open", called_names)
        self.assertNotIn("Runner", source)
        self.assertNotIn("systemctl", source)
        self.assertNotIn("ssh ", source.lower())

    def test_cli_exposes_only_simulate_and_stdin_verify(self) -> None:
        parser = protocol.build_parser()
        subcommands: set[str] = set()
        for action in parser._actions:
            choices = getattr(action, "choices", None)
            if isinstance(choices, dict):
                subcommands.update(choices)
        self.assertEqual(subcommands, {"simulate", "verify"})

    def test_canonical_success_round_trip_is_byte_stable(self) -> None:
        transcript = self.simulate()
        encoded = protocol.canonical_bytes(transcript)
        decoded = protocol.parse_closed_json(encoded, "round trip")
        self.assertEqual(protocol.canonical_bytes(decoded), encoded)
        self.assertEqual(
            self.verify(decoded)["classification"], "SIMULATED_COMPLETE"
        )


if __name__ == "__main__":
    unittest.main()
