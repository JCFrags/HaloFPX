from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile
import types
import unittest
from unittest import mock
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[1]
SOURCE = REPO / "scripts" / "halofpx_strix_hmm_admission.py"
MAINTENANCE_SOURCE = REPO / "scripts" / "halofpx_strix_maintenance.py"
SNAPSHOT = REPO / "scripts" / "halofpx-strix-hmm-admission-snapshot.example.json"
POLICY = REPO / "scripts" / "halofpx-strix-hmm-admission-policy.example.json"
RESULT = REPO / "scripts" / "halofpx-strix-hmm-admission-result.example.json"
INCIDENT = REPO / "docs" / "halofpx" / "evidence" / "2026-08-12-target-hmm-oom-incident"
SPEC = importlib.util.spec_from_file_location("halofpx_strix_hmm_admission", SOURCE)
assert SPEC is not None and SPEC.loader is not None
admission = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(admission)
MAINTENANCE_SPEC = importlib.util.spec_from_file_location(
    "halofpx_strix_maintenance_for_hmm_test", MAINTENANCE_SOURCE)
assert MAINTENANCE_SPEC is not None and MAINTENANCE_SPEC.loader is not None
maintenance = importlib.util.module_from_spec(MAINTENANCE_SPEC)
sys.modules[MAINTENANCE_SPEC.name] = maintenance
MAINTENANCE_SPEC.loader.exec_module(maintenance)

TRUSTED_NOW = "2026-08-13T07:01:00Z"


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def bytes_of(value: Any) -> bytes:
    return json.dumps(value, indent=2, sort_keys=True).encode("utf-8") + b"\n"


def evaluate(snapshot: dict[str, Any], policy: dict[str, Any] | None = None,
             now: str = TRUSTED_NOW) -> dict[str, Any]:
    return admission.evaluate_bytes(
        bytes_of(snapshot), bytes_of(policy or read_json(POLICY)),
        trusted_now_utc=now,
    )


def role_observation(snapshot: dict[str, Any], role: str) -> dict[str, Any]:
    return snapshot["roles"][role]["observation"]


class HMMAdmissionHappyPathTests(unittest.TestCase):
    def test_example_pair_admits_but_never_claims_target_authority(self) -> None:
        result = admission.evaluate_bytes(
            SNAPSHOT.read_bytes(), POLICY.read_bytes(), trusted_now_utc=TRUSTED_NOW)
        self.assertEqual("ADMIT", result["decision"])
        self.assertEqual([], result["reason_codes"])
        self.assertFalse(result["target_execution_authority"])
        self.assertFalse(result["performance_result"])
        self.assertEqual(17_179_869_184,
                         result["roles"]["coordinator"]["hmm_headroom_bytes"])
        self.assertEqual(17_179_869_184,
                         result["roles"]["worker"]["hmm_headroom_bytes"])
        self.assertEqual(hashlib.sha256(SNAPSHOT.read_bytes()).hexdigest(),
                         result["snapshot_sha256"])
        self.assertEqual(hashlib.sha256(POLICY.read_bytes()).hexdigest(),
                         result["policy_sha256"])
        with self.assertRaisesRegex(admission.AdmissionError, "bound canonical"):
            admission.validate_admission_result_bytes(admission.pretty_bytes(result))
        with self.assertRaisesRegex(admission.AdmissionError, "bound canonical"):
            admission.validate_admission_result_bytes(RESULT.read_bytes())
        self.assertEqual(RESULT.read_bytes(), admission.pretty_bytes(result))
        self.assertEqual(result, admission.validate_bound_admission_result_bytes(
            RESULT.read_bytes(), SNAPSHOT.read_bytes(), POLICY.read_bytes()))

    def test_example_raw_bytes_have_frozen_golden_digests(self) -> None:
        expected = {
            SNAPSHOT: "e942b3d9dcd10e9bddc5484d1e8eb19bf804cf29dae28851a420c13f8846837a",
            POLICY: "e75f18b1c564643bed5882afd377a70d9e6e78ca10572de6bd87a97c81984fab",
            RESULT: "3dff6309c8ae51a3eda0d340e0228d41284897865329be993bee82e1b65c351a",
        }
        for path, digest in expected.items():
            with self.subTest(path=path.name):
                self.assertEqual(digest, admission.sha256_bytes(path.read_bytes()))

    def test_exact_per_owner_sum_reconciles_to_host_aggregate(self) -> None:
        snapshot = read_json(SNAPSHOT)
        for role in admission.ROLES:
            observation = role_observation(snapshot, role)
            owners = observation["device_census"]["owners"]
            aggregate = observation["device_census"]["hmm_aggregate_bytes"]
            self.assertEqual(aggregate, sum(owner["hmm_allocated_bytes"] for owner in owners))
            self.assertEqual(aggregate, observation["memory"]["gpu_active_bytes"])

    def test_module_has_no_live_execution_surface(self) -> None:
        self.assertFalse(admission.TARGET_EXECUTION_AUTHORITY)
        self.assertFalse(hasattr(admission, "Runner"))
        self.assertFalse(hasattr(admission, "ssh"))
        source = SOURCE.read_text(encoding="utf-8")
        self.assertNotIn("import socket", source)
        self.assertNotIn("import subprocess", source)
        self.assertNotIn("systemctl ", source)
        parser = admission.build_parser()
        self.assertEqual(
            {"help", "snapshot", "policy", "trusted_now_utc", "output"},
            {action.dest for action in parser._actions},
        )

    def test_shared_identity_contract_loads_exact_sibling_not_ambient_module(self) -> None:
        ambient = types.SimpleNamespace(
            PRODUCTION_IDENTITY_FIELDS=frozenset(),
            production_identity_digest=lambda _value: "f" * 64,
        )
        with mock.patch.dict(
            sys.modules, {"halofpx_strix_production_identity": ambient}, clear=False,
        ):
            spec = importlib.util.spec_from_file_location(
                "halofpx_strix_hmm_admission_exact_sibling_test", SOURCE)
            assert spec is not None and spec.loader is not None
            loaded = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(loaded)
        self.assertIsNot(loaded.production_identity_contract, ambient)
        self.assertEqual(
            (REPO / "scripts" / "halofpx_strix_production_identity.py").resolve(),
            Path(loaded.production_identity_contract.__file__).resolve(),
        )

    def test_owner_identity_digests_are_bound_by_policy(self) -> None:
        snapshot = read_json(SNAPSHOT)
        policy = read_json(POLICY)
        for role in admission.ROLES:
            node = snapshot["roles"][role]
            owner = node["observation"]["device_census"]["owners"][0]
            digest = admission.digest_value(admission.device_owner_identity(node["host"], owner))
            self.assertEqual([digest], policy["roles"][role][
                "required_device_owner_identity_sha256s"])

    def test_production_digest_exactly_reuses_maintenance_identity_domain(self) -> None:
        policy = read_json(POLICY)
        result = admission.evaluate_bytes(
            SNAPSHOT.read_bytes(), POLICY.read_bytes(), trusted_now_utc=TRUSTED_NOW)
        expected_literals = {
            "coordinator": "3131533450592b2c6fe152095c4e15becdb4532e346d1747495d59370468a13a",
            "worker": "c622e76a01b08c180d63c008e08b1272e3e73f073cb2e3fa02d590b48054883b",
        }
        for role in admission.ROLES:
            with self.subTest(role=role):
                value = policy["roles"][role]["expected_identity"]
                identity = maintenance.parse_identity(value, role, f"golden.{role}")
                self.assertEqual(expected_literals[role], identity.digest)
                self.assertEqual(
                    identity.digest,
                    hashlib.sha256(maintenance.core.canonical_bytes(value)).hexdigest(),
                )
                self.assertEqual(
                    identity.digest,
                    admission.production_identity_contract.production_identity_digest(value),
                )
                self.assertEqual(
                    identity.digest,
                    result["roles"][role]["production_identity_sha256"],
                )

    def test_every_production_identity_field_changes_shared_digest(self) -> None:
        value = read_json(POLICY)["roles"]["coordinator"]["expected_identity"]
        mutations = {
            "role": "worker",
            "host": "nimo-x",
            "unit": "changed.service",
            "pid": value["pid"] + 1,
            "invocation_id": "f" * 32,
            "nrestarts": value["nrestarts"] + 1,
            "process_start_ticks": value["process_start_ticks"] + 1,
            "start_monotonic_us": value["start_monotonic_us"] + 1,
            "executable_sha256": "d" * 64,
            "argv_sha256": "e" * 64,
            "control_group": "/system.slice/changed.service",
            "listener_port": value["listener_port"] + 1,
            "listener_pid": value["listener_pid"] + 1,
            "health_sha256": "f" * 64,
        }
        self.assertEqual(
            set(admission.production_identity_contract.PRODUCTION_IDENTITY_FIELDS),
            set(mutations),
        )
        golden = admission.production_identity_contract.production_identity_digest(value)
        for field, replacement in mutations.items():
            with self.subTest(field=field):
                changed = copy.deepcopy(value)
                changed[field] = replacement
                self.assertNotEqual(
                    golden,
                    admission.production_identity_contract.production_identity_digest(changed),
                )

    def test_shared_identity_domain_rejects_nonfinite_and_surrogate_values(self) -> None:
        value = read_json(POLICY)["roles"]["coordinator"]["expected_identity"]
        for field, replacement in (
            ("pid", float("nan")),
            ("unit", "synthetic-\ud800.service"),
        ):
            with self.subTest(field=field):
                changed = copy.deepcopy(value)
                changed[field] = replacement
                with self.assertRaises(
                    admission.production_identity_contract.ProductionIdentityDomainError
                ):
                    admission.production_identity_contract.production_identity_digest(changed)


class HMMAdmissionClosedSchemaTests(unittest.TestCase):
    def test_duplicate_json_key_refuses(self) -> None:
        with self.assertRaisesRegex(admission.AdmissionError, "duplicate JSON key"):
            admission.parse_snapshot_bytes(b'{"schema":1,"schema":2}')

    def test_non_finite_json_refuses(self) -> None:
        content = SNAPSHOT.read_bytes().replace(b'"issue": 41', b'"issue": NaN')
        with self.assertRaisesRegex(admission.AdmissionError, "non-finite"):
            admission.parse_snapshot_bytes(content)

    def test_invalid_utf8_refuses(self) -> None:
        with self.assertRaisesRegex(admission.AdmissionError, "unreadable"):
            admission.parse_snapshot_bytes(b"\xff")

    def test_escaped_surrogate_and_control_string_refuse_cleanly(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["capture_id"] = "synthetic-\ud800"
        content = json.dumps(snapshot, sort_keys=True).encode("ascii")
        with self.assertRaisesRegex(admission.AdmissionError, "Unicode scalar"):
            admission.parse_snapshot_bytes(content)
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["capture_id"] = "synthetic\ncontrol"
        with self.assertRaisesRegex(admission.AdmissionError, "control characters"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["capture_id"] = "synthetic-\u0085-control"
        with self.assertRaisesRegex(admission.AdmissionError, "control characters"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_oversized_input_refuses(self) -> None:
        with self.assertRaisesRegex(admission.AdmissionError, "exceeds"):
            admission.parse_snapshot_bytes(b" " * (admission.MAX_INPUT_BYTES + 1))

    def test_excessive_json_nesting_refuses_cleanly(self) -> None:
        content = ("[" * 2000 + "0" + "]" * 2000).encode("ascii")
        with self.assertRaises(admission.AdmissionError):
            admission.parse_snapshot_bytes(content)

    def test_extra_snapshot_field_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["unexpected"] = True
        with self.assertRaisesRegex(admission.AdmissionError, "wrong closed field set"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_missing_nested_field_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        del role_observation(snapshot, "worker")["kernel"]["journal_cursor_end"]
        with self.assertRaisesRegex(admission.AdmissionError, "wrong closed field set"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_unknown_collection_state_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["roles"]["worker"]["collection_state"] = "partial"
        with self.assertRaisesRegex(admission.AdmissionError, "unsupported"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_complete_node_cannot_carry_collection_errors(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["roles"]["worker"]["errors"] = [
            {"code": "READ_ERROR", "detail": "synthetic"}]
        with self.assertRaisesRegex(admission.AdmissionError, "may not contain errors"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_unreadable_node_requires_null_observation_and_error(self) -> None:
        snapshot = read_json(SNAPSHOT)
        node = snapshot["roles"]["worker"]
        node["collection_state"] = "unreadable"
        with self.assertRaisesRegex(admission.AdmissionError, "requires null observation"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_policy_requires_elevated_closed_world_zero_kernel_events(self) -> None:
        for field in ("elevated", "closed_world", "require_zero_kernel_counts"):
            with self.subTest(field=field):
                policy = read_json(POLICY)
                policy["capture_requirements"][field] = False
                with self.assertRaisesRegex(admission.AdmissionError, "must require"):
                    admission.parse_policy_bytes(bytes_of(policy))

    def test_policy_source_kind_wrong_type_refuses_cleanly(self) -> None:
        policy = read_json(POLICY)
        policy["capture_requirements"]["source_kind"] = []
        with self.assertRaises(admission.AdmissionError):
            admission.parse_policy_bytes(bytes_of(policy))

    def test_policy_expected_boot_id_is_closed_and_canonical(self) -> None:
        for value in (True, "not-a-boot-id"):
            with self.subTest(value=value):
                policy = read_json(POLICY)
                policy["roles"]["worker"]["expected_boot_id"] = value
                with self.assertRaises(admission.AdmissionError):
                    admission.parse_policy_bytes(bytes_of(policy))

    def test_device_paths_are_restricted_to_kfd_and_render_nodes(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["device_census"]["devices"].append(
            "/dev/random")
        role_observation(snapshot, "worker")["device_census"]["devices"].sort()
        with self.assertRaisesRegex(admission.AdmissionError, "must include"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_device_owner_unit_and_cgroup_are_canonical(self) -> None:
        snapshot = read_json(SNAPSHOT)
        owner = role_observation(snapshot, "worker")["device_census"]["owners"][0]
        owner["unit"] = "not-a-service"
        with self.assertRaisesRegex(admission.AdmissionError, "unit is malformed"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_kernel_baseline_window_must_be_nonempty(self) -> None:
        snapshot = read_json(SNAPSHOT)
        kernel = role_observation(snapshot, "worker")["kernel"]
        kernel["observed_monotonic_ns"] = kernel["window_start_monotonic_ns"]
        with self.assertRaisesRegex(admission.AdmissionError, "window is empty"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))
        snapshot = read_json(SNAPSHOT)
        kernel = role_observation(snapshot, "worker")["kernel"]
        kernel["journal_cursor_end"] = kernel["journal_cursor_start"]
        with self.assertRaisesRegex(admission.AdmissionError, "cursor window is empty"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_capture_monotonic_window_must_be_nonempty(self) -> None:
        snapshot = read_json(SNAPSHOT)
        clock = role_observation(snapshot, "worker")["capture_clock"]
        clock["completed_monotonic_ns"] = clock["started_monotonic_ns"]
        with self.assertRaisesRegex(admission.AdmissionError, "interval is empty"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_capture_monotonic_integer_fields_reject_booleans(self) -> None:
        for field in ("started_monotonic_ns", "completed_monotonic_ns"):
            with self.subTest(field=field):
                snapshot = read_json(SNAPSHOT)
                role_observation(snapshot, "worker")["capture_clock"][field] = True
                with self.assertRaises(admission.AdmissionError):
                    admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_policy_must_require_protected_production_device_owner(self) -> None:
        policy = read_json(POLICY)
        role = policy["roles"]["worker"]
        role["allowed_device_owner_identity_sha256s"] = ["f" * 64]
        role["required_device_owner_identity_sha256s"] = ["f" * 64]
        with self.assertRaisesRegex(admission.AdmissionError, "protected production"):
            admission.parse_policy_bytes(bytes_of(policy))

    def test_policy_reserve_must_be_positive(self) -> None:
        policy = read_json(POLICY)
        policy["roles"]["worker"]["capacity"]["required_reserve_bytes"] = 0
        with self.assertRaises(admission.AdmissionError):
            admission.parse_policy_bytes(bytes_of(policy))

    def test_policy_capacity_arithmetic_is_checked(self) -> None:
        policy = read_json(POLICY)
        capacity = policy["roles"]["worker"]["capacity"]
        capacity["planned_increment_bytes"] = admission.MAX_U64
        with self.assertRaises(admission.AdmissionError):
            admission.parse_policy_bytes(bytes_of(policy))

    def test_policy_identity_integer_fields_reject_booleans(self) -> None:
        for field in (
            "pid", "nrestarts", "process_start_ticks", "start_monotonic_us",
            "listener_port", "listener_pid",
        ):
            with self.subTest(field=field):
                policy = read_json(POLICY)
                policy["roles"]["coordinator"]["expected_identity"][field] = True
                with self.assertRaises(admission.AdmissionError):
                    admission.parse_policy_bytes(bytes_of(policy))

    def test_policy_identity_listener_port_is_range_checked(self) -> None:
        policy = read_json(POLICY)
        policy["roles"]["coordinator"]["expected_identity"]["listener_port"] = 65536
        with self.assertRaises(admission.AdmissionError):
            admission.parse_policy_bytes(bytes_of(policy))

    def test_snapshot_identity_integer_fields_reject_booleans(self) -> None:
        locations = (
            ("service", "main_pid"),
            ("service", "nrestarts"),
            ("service", "start_monotonic_us"),
            ("process", "pid"),
            ("process", "start_ticks"),
            ("listener", "port"),
        )
        for section, field in locations:
            with self.subTest(section=section, field=field):
                snapshot = read_json(SNAPSHOT)
                role_observation(snapshot, "coordinator")[section][field] = True
                with self.assertRaises(admission.AdmissionError):
                    admission.parse_snapshot_bytes(bytes_of(snapshot))
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "coordinator")["listener"]["owner_pids"] = [True]
        with self.assertRaises(admission.AdmissionError):
            admission.parse_snapshot_bytes(bytes_of(snapshot))

    def test_device_owner_bytes_are_u64_bounded(self) -> None:
        snapshot = read_json(SNAPSHOT)
        owner = role_observation(snapshot, "worker")["device_census"]["owners"][0]
        owner["hmm_allocated_bytes"] = admission.MAX_U64 + 1
        with self.assertRaisesRegex(admission.AdmissionError, "integer"):
            admission.parse_snapshot_bytes(bytes_of(snapshot))


class HMMAdmissionExplicitStateTests(unittest.TestCase):
    def test_unreadable_role_is_typed_refusal(self) -> None:
        snapshot = read_json(SNAPSHOT)
        node = snapshot["roles"]["worker"]
        node["collection_state"] = "unreadable"
        node["observation"] = None
        node["errors"] = [{"code": "PROC_UNREADABLE", "detail": "permission denied"}]
        result = evaluate(snapshot)
        self.assertEqual("REFUSE", result["decision"])
        self.assertEqual(
            ["COLLECTION_UNREADABLE", "NODE_ERROR"],
            result["roles"]["worker"]["reason_codes"],
        )
        self.assertIsNone(result["roles"]["worker"]["production_identity_sha256"])

    def test_refused_role_is_typed_refusal(self) -> None:
        snapshot = read_json(SNAPSHOT)
        node = snapshot["roles"]["coordinator"]
        node["collection_state"] = "refused"
        node["observation"] = None
        node["errors"] = [{"code": "ELEVATION_REFUSED", "detail": "not authorized"}]
        result = evaluate(snapshot)
        self.assertIn("COLLECTION_REFUSED", result["reason_codes"])
        self.assertEqual("REFUSE", result["roles"]["coordinator"]["classification"])

    def test_capture_and_snapshot_errors_are_refusals(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["errors"] = [{"code": "CLOCK_ERROR", "detail": "synthetic"}]
        snapshot["errors"] = [{"code": "HOST_ERROR", "detail": "synthetic"}]
        result = evaluate(snapshot)
        self.assertIn("CAPTURE_ERROR", result["reason_codes"])
        self.assertIn("SNAPSHOT_ERROR", result["reason_codes"])

    def test_non_elevated_or_open_world_refuses(self) -> None:
        for field, reason in (
            ("elevated", "CAPTURE_NOT_ELEVATED"),
            ("closed_world", "CAPTURE_NOT_CLOSED_WORLD"),
        ):
            with self.subTest(field=field):
                snapshot = read_json(SNAPSHOT)
                snapshot["capture"][field] = False
                self.assertIn(reason, evaluate(snapshot)["reason_codes"])


class HMMAdmissionIdentityAndOwnershipTests(unittest.TestCase):
    def test_every_observed_production_identity_field_is_rejected_on_mutation(self) -> None:
        def change(section: str, field: str, value: Any):
            return lambda snapshot: role_observation(snapshot, "coordinator")[section].__setitem__(
                field, value)

        mutations = {
            "role": lambda snapshot: snapshot["roles"]["coordinator"].__setitem__("role", "worker"),
            "host": lambda snapshot: snapshot["roles"]["coordinator"].__setitem__("host", "nimo-x"),
            "unit": change("service", "unit", "changed.service"),
            "pid": change("service", "main_pid", 3113344),
            "invocation_id": change("service", "invocation_id", "f" * 32),
            "nrestarts": change("service", "nrestarts", 2),
            "process_start_ticks": change("process", "start_ticks", 102),
            "start_monotonic_us": change("service", "start_monotonic_us", 1000002),
            "executable_sha256": change("process", "executable_sha256", "d" * 64),
            "argv_sha256": change("process", "argv_sha256", "e" * 64),
            "control_group": change("service", "control_group", "/system.slice/changed.service"),
            "listener_port": change("listener", "port", 8082),
            "listener_pid": change("listener", "owner_pids", [3113344]),
            "health_sha256": change("listener", "health_sha256", "f" * 64),
        }
        self.assertEqual(
            set(admission.production_identity_contract.PRODUCTION_IDENTITY_FIELDS),
            set(mutations),
        )
        for field, mutate in mutations.items():
            with self.subTest(field=field):
                snapshot = read_json(SNAPSHOT)
                mutate(snapshot)
                try:
                    result = evaluate(snapshot)
                except admission.AdmissionError:
                    # Role and host are fixed topology fields and fail structurally.
                    self.assertIn(field, {"role", "host"})
                else:
                    role = result["roles"]["coordinator"]
                    self.assertEqual("REFUSE", role["classification"])
                    self.assertIn("SERVICE_IDENTITY_MISMATCH", role["reason_codes"])

    def test_service_identity_drift_refuses(self) -> None:
        for field, replacement in (
            ("active_state", "inactive"),
            ("sub_state", "dead"),
            ("invocation_id", "f" * 32),
            ("nrestarts", 2),
            ("start_monotonic_us", 9999999),
        ):
            with self.subTest(field=field):
                snapshot = read_json(SNAPSHOT)
                role_observation(snapshot, "worker")["service"][field] = replacement
                result = evaluate(snapshot)
                self.assertIn("SERVICE_IDENTITY_MISMATCH",
                              result["roles"]["worker"]["reason_codes"])

    def test_pid_or_process_identity_drift_refuses(self) -> None:
        for field, replacement in (
            ("start_ticks", 999),
            ("executable_sha256", "c" * 64),
            ("argv_sha256", "d" * 64),
        ):
            with self.subTest(field=field):
                snapshot = read_json(SNAPSHOT)
                role_observation(snapshot, "coordinator")["process"][field] = replacement
                self.assertIn(
                    "SERVICE_IDENTITY_MISMATCH",
                    evaluate(snapshot)["roles"]["coordinator"]["reason_codes"],
                )

    def test_cgroup_membership_must_be_exact(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["process"]["cgroup_member_pids"].append(999999)
        role_observation(snapshot, "worker")["process"]["cgroup_member_pids"].sort()
        result = evaluate(snapshot)
        self.assertIn("CGROUP_MEMBERSHIP_MISMATCH",
                      result["roles"]["worker"]["reason_codes"])

    def test_listener_owner_must_be_exact(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "coordinator")["listener"]["owner_pids"] = [999999]
        result = evaluate(snapshot)
        self.assertIn("LISTENER_OWNERSHIP_MISMATCH",
                      result["roles"]["coordinator"]["reason_codes"])

    def test_health_identity_drift_refuses(self) -> None:
        for role, replacement in (("coordinator", "d" * 64), ("worker", "d" * 64)):
            with self.subTest(role=role):
                snapshot = read_json(SNAPSHOT)
                role_observation(snapshot, role)["listener"]["health_sha256"] = replacement
                self.assertIn(
                    "SERVICE_IDENTITY_MISMATCH",
                    evaluate(snapshot)["roles"][role]["reason_codes"],
                )

    def test_device_set_mismatch_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["device_census"]["devices"].insert(
            1, "/dev/dri/renderD129")
        result = evaluate(snapshot)
        self.assertIn("DEVICE_SET_MISMATCH",
                      result["roles"]["worker"]["reason_codes"])

    def test_foreign_device_owner_refuses_even_when_aggregate_reconciles(self) -> None:
        snapshot = read_json(SNAPSHOT)
        observation = role_observation(snapshot, "worker")
        census = observation["device_census"]
        original = census["owners"][0]
        original["hmm_allocated_bytes"] -= 4096
        foreign = copy.deepcopy(original)
        foreign.update({
            "pid": 999999,
            "process_start_ticks": 777,
            "unit": "foreign.service",
            "control_group": "/system.slice/foreign.service",
            "hmm_allocated_bytes": 4096,
        })
        census["owners"].append(foreign)
        census["owners"].sort(key=lambda owner: owner["pid"])
        result = evaluate(snapshot)
        self.assertIn("FOREIGN_DEVICE_OWNER",
                      result["roles"]["worker"]["reason_codes"])
        self.assertNotIn("HMM_ACCOUNTING_RECONCILIATION_FAILED",
                         result["roles"]["worker"]["reason_codes"])

    def test_required_device_owner_missing_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        observation = role_observation(snapshot, "coordinator")
        observation["device_census"]["owners"] = []
        observation["device_census"]["hmm_aggregate_bytes"] = 0
        observation["memory"]["gpu_active_bytes"] = 0
        result = evaluate(snapshot)
        self.assertIn("REQUIRED_DEVICE_OWNER_MISSING",
                      result["roles"]["coordinator"]["reason_codes"])

    def test_device_census_error_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["device_census"]["errors"] = [
            {"code": "FD_SCAN_FAILED", "detail": "synthetic"}]
        self.assertIn(
            "DEVICE_CENSUS_ERROR",
            evaluate(snapshot)["roles"]["worker"]["reason_codes"],
        )


class HMMAdmissionAccountingAndCapacityTests(unittest.TestCase):
    def test_per_owner_sum_overflow_is_typed_refusal(self) -> None:
        snapshot = read_json(SNAPSHOT)
        observation = role_observation(snapshot, "worker")
        original = observation["device_census"]["owners"][0]
        original["hmm_allocated_bytes"] = admission.MAX_U64
        second = copy.deepcopy(original)
        second.update({
            "pid": original["pid"] + 1,
            "process_start_ticks": original["process_start_ticks"] + 1,
            "unit": "synthetic-helper.service",
            "control_group": "/system.slice/synthetic-helper.service",
            "hmm_allocated_bytes": 1,
        })
        observation["device_census"]["owners"].append(second)
        observation["device_census"]["hmm_aggregate_bytes"] = admission.MAX_U64
        observation["memory"]["gpu_active_bytes"] = admission.MAX_U64
        result = evaluate(snapshot)
        self.assertIn(
            "HMM_ACCOUNTING_RECONCILIATION_FAILED",
            result["roles"]["worker"]["reason_codes"],
        )

    def test_per_owner_sum_must_equal_exact_aggregate(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["device_census"]["owners"][0][
            "hmm_allocated_bytes"] -= 1
        result = evaluate(snapshot)
        self.assertIn("HMM_ACCOUNTING_RECONCILIATION_FAILED",
                      result["roles"]["worker"]["reason_codes"])

    def test_gpu_active_must_equal_same_exact_aggregate(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "coordinator")["memory"]["gpu_active_bytes"] += 1024
        result = evaluate(snapshot)
        self.assertIn("HMM_ACCOUNTING_RECONCILIATION_FAILED",
                      result["roles"]["coordinator"]["reason_codes"])

    def test_coherently_rehashed_unknown_or_approximate_source_is_invalid(self) -> None:
        snapshot = read_json(SNAPSHOT)
        policy = read_json(POLICY)
        for role in admission.ROLES:
            role_observation(snapshot, role)["device_census"][
                "hmm_accounting_source"] = "approximate-rss-v1"
            policy["roles"][role]["hmm_accounting_source"] = "approximate-rss-v1"
        with self.assertRaisesRegex(admission.AdmissionError, "reviewed v1 registry"):
            evaluate(snapshot, policy)

    def test_registered_nonexact_source_is_typed_refusal(self) -> None:
        for source_kind, source in (
            ("retained-incident-evidence", "retained-incident-aggregate-gpuactive-v1"),
            ("future-elevated-collector", "future-elevated-collector-unqualified-v1"),
        ):
            with self.subTest(source_kind=source_kind):
                snapshot = read_json(SNAPSHOT)
                policy = read_json(POLICY)
                snapshot["capture"]["source_kind"] = source_kind
                policy["capture_requirements"]["source_kind"] = source_kind
                for role in admission.ROLES:
                    role_observation(snapshot, role)["device_census"][
                        "hmm_accounting_source"] = source
                    policy["roles"][role]["hmm_accounting_source"] = source
                result = evaluate(snapshot, policy)
                self.assertEqual("REFUSE", result["decision"])
                for role in admission.ROLES:
                    self.assertIn(
                        "HMM_ACCOUNTING_SOURCE_NOT_ADMISSIBLE",
                        result["roles"][role]["reason_codes"],
                    )

    def test_physical_capacity_mismatch_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["memory"]["physical_capacity_bytes"] -= 4096
        result = evaluate(snapshot)
        self.assertIn("PHYSICAL_CAPACITY_MISMATCH",
                      result["roles"]["worker"]["reason_codes"])

    def test_aggregate_over_admitted_capacity_refuses_without_negative_headroom(self) -> None:
        snapshot = read_json(SNAPSHOT)
        observation = role_observation(snapshot, "worker")
        over = 128_849_018_881
        observation["device_census"]["owners"][0]["hmm_allocated_bytes"] = over
        observation["device_census"]["hmm_aggregate_bytes"] = over
        observation["memory"]["gpu_active_bytes"] = over
        result = evaluate(snapshot)
        role = result["roles"]["worker"]
        self.assertIn("HMM_CAPACITY_EXCEEDED", role["reason_codes"])
        self.assertIsNone(role["hmm_headroom_bytes"])

    def test_planned_increment_over_raw_headroom_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        observation = role_observation(snapshot, "coordinator")
        used = 125_000_000_000
        observation["device_census"]["owners"][0]["hmm_allocated_bytes"] = used
        observation["device_census"]["hmm_aggregate_bytes"] = used
        observation["memory"]["gpu_active_bytes"] = used
        result = evaluate(snapshot)
        self.assertIn("HMM_CAPACITY_EXCEEDED",
                      result["roles"]["coordinator"]["reason_codes"])

    def test_residual_headroom_below_policy_reserve_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        observation = role_observation(snapshot, "worker")
        used = 112_000_000_000
        observation["device_census"]["owners"][0]["hmm_allocated_bytes"] = used
        observation["device_census"]["hmm_aggregate_bytes"] = used
        observation["memory"]["gpu_active_bytes"] = used
        result = evaluate(snapshot)
        role = result["roles"]["worker"]
        self.assertIn("HMM_HEADROOM_INSUFFICIENT", role["reason_codes"])
        self.assertGreaterEqual(role["hmm_headroom_bytes"], 0)


class HMMAdmissionKernelAndTimeTests(unittest.TestCase):
    def test_each_kernel_event_counter_refuses(self) -> None:
        for field in admission.KERNEL_COUNTERS:
            with self.subTest(field=field):
                snapshot = read_json(SNAPSHOT)
                role_observation(snapshot, "worker")["kernel"][field] = 1
                self.assertIn(
                    "KERNEL_BASELINE_NOT_CLEAN",
                    evaluate(snapshot)["roles"]["worker"]["reason_codes"],
                )

    def test_kernel_collection_error_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "coordinator")["kernel"]["errors"] = [
            {"code": "JOURNAL_REFUSED", "detail": "synthetic"}]
        self.assertIn(
            "KERNEL_BASELINE_NOT_CLEAN",
            evaluate(snapshot)["roles"]["coordinator"]["reason_codes"],
        )

    def test_coherently_rehashed_kernel_window_predating_service_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        observation = role_observation(snapshot, "worker")
        observation["kernel"]["window_start_monotonic_ns"] = 1
        observation["capture_clock"]["started_monotonic_ns"] = 2
        observation["capture_clock"]["completed_monotonic_ns"] = 3
        observation["kernel"]["observed_monotonic_ns"] = 4
        result = evaluate(snapshot)
        self.assertIn(
            "KERNEL_BASELINE_COVERAGE_INVALID",
            result["roles"]["worker"]["reason_codes"],
        )

    def test_each_service_capture_kernel_boundary_is_enforced(self) -> None:
        base = role_observation(read_json(SNAPSHOT), "worker")
        service_start = base["service"]["start_monotonic_us"] * 1000
        mutations = {
            "kernel-start-after-service": lambda observation: observation["kernel"].__setitem__(
                "window_start_monotonic_ns", service_start + 1),
            "capture-start-before-service": lambda observation: observation[
                "capture_clock"].__setitem__("started_monotonic_ns", service_start - 1),
            "kernel-end-before-capture": lambda observation: observation["kernel"].__setitem__(
                "observed_monotonic_ns",
                observation["capture_clock"]["completed_monotonic_ns"] - 1),
        }
        for name, mutate in mutations.items():
            with self.subTest(name=name):
                snapshot = read_json(SNAPSHOT)
                mutate(role_observation(snapshot, "worker"))
                self.assertIn(
                    "KERNEL_BASELINE_COVERAGE_INVALID",
                    evaluate(snapshot)["roles"]["worker"]["reason_codes"],
                )

    def test_capture_and_kernel_boot_must_match_policy_boot(self) -> None:
        for fields in (("capture_clock",), ("kernel",), ("capture_clock", "kernel")):
            with self.subTest(fields=fields):
                snapshot = read_json(SNAPSHOT)
                observation = role_observation(snapshot, "worker")
                changed_boot = "33333333-3333-4333-8333-333333333333"
                for field in fields:
                    observation[field]["boot_id"] = changed_boot
                result = evaluate(snapshot)
                self.assertIn(
                    "KERNEL_BOOT_ID_MISMATCH",
                    result["roles"]["worker"]["reason_codes"],
                )

    def test_stale_snapshot_refuses(self) -> None:
        result = admission.evaluate_bytes(
            SNAPSHOT.read_bytes(), POLICY.read_bytes(),
            trusted_now_utc="2026-08-13T07:10:00Z")
        self.assertIn("SNAPSHOT_STALE", result["reason_codes"])

    def test_trusted_time_before_capture_refuses(self) -> None:
        result = admission.evaluate_bytes(
            SNAPSHOT.read_bytes(), POLICY.read_bytes(),
            trusted_now_utc="2026-08-13T07:00:01Z")
        self.assertIn("TRUSTED_TIME_BEFORE_CAPTURE", result["reason_codes"])

    def test_invalid_trusted_time_has_no_clock_fallback(self) -> None:
        for value in ("", "2026-08-13T07:01:00+00:00", "now", "2026-08-13T07:01:00.1Z"):
            with self.subTest(value=value), self.assertRaises(admission.AdmissionError):
                admission.evaluate_bytes(SNAPSHOT.read_bytes(), POLICY.read_bytes(),
                                         trusted_now_utc=value)

    def test_policy_window_and_capture_source_are_enforced(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["source_kind"] = "retained-incident-evidence"
        for role in admission.ROLES:
            role_observation(snapshot, role)["device_census"][
                "hmm_accounting_source"] = "retained-incident-aggregate-gpuactive-v1"
        result = evaluate(snapshot)
        self.assertIn("CAPTURE_SOURCE_MISMATCH", result["reason_codes"])
        self.assertIn("RETAINED_INCIDENT_NON_ADMISSIBLE", result["reason_codes"])
        result = evaluate(read_json(SNAPSHOT), now="2026-08-13T08:00:01Z")
        self.assertIn("TRUSTED_TIME_OUTSIDE_POLICY_WINDOW", result["reason_codes"])

    def test_policy_expiry_is_exclusive(self) -> None:
        result = evaluate(read_json(SNAPSHOT), now="2026-08-13T08:00:00Z")
        self.assertIn("TRUSTED_TIME_OUTSIDE_POLICY_WINDOW", result["reason_codes"])
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["completed_utc"] = "2026-08-13T08:00:00Z"
        self.assertIn("SNAPSHOT_AFTER_POLICY_WINDOW", evaluate(snapshot)["reason_codes"])

    def test_reversed_capture_clock_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["started_utc"] = "2026-08-13T07:00:03Z"
        self.assertIn("CAPTURE_CLOCK_ORDER_INVALID", evaluate(snapshot)["reason_codes"])

    def test_empty_capture_clock_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"]["started_utc"] = snapshot["capture"]["completed_utc"]
        self.assertIn("CAPTURE_CLOCK_ORDER_INVALID", evaluate(snapshot)["reason_codes"])


class HMMAdmissionRetainedIncidentTests(unittest.TestCase):
    def test_retained_gpu_active_kib_is_checked_and_converted_to_bytes(self) -> None:
        nimo1 = (INCIDENT / "raw" / "nimo-1-current-authority.stdout.log").read_bytes()
        nimo2 = (INCIDENT / "raw" / "nimo-2-current-authority.stdout.log").read_bytes()
        self.assertEqual(108_245_408 * 1024,
                         admission.extract_meminfo_kib_as_bytes(nimo1, "GPUActive"))
        self.assertEqual(114_067_524 * 1024,
                         admission.extract_meminfo_kib_as_bytes(nimo2, "GPUActive"))
        oom = (INCIDENT / "raw" / "nimo-2-kernel-oom-window.stdout.log").read_bytes()
        self.assertEqual(114_041_696 * 1024,
                         admission.extract_kernel_gpu_active_kib_as_bytes(oom))

    def test_checked_kib_conversion_rejects_overflow(self) -> None:
        with self.assertRaisesRegex(admission.AdmissionError, "multiplied"):
            admission.kib_to_bytes(admission.MAX_U64 // 1024 + 1, "retained KiB")

    def test_retained_kib_extractors_reject_ambiguous_records(self) -> None:
        for content in (
            b"GPUActive:\n 1 kB\n",
            b"GPUActive: 1 kB\nGPUActive: 1 kB\n",
        ):
            with self.subTest(content=content), self.assertRaises(admission.AdmissionError):
                admission.extract_meminfo_kib_as_bytes(content, "GPUActive")
        with self.assertRaises(admission.AdmissionError):
            admission.extract_kernel_gpu_active_kib_as_bytes(
                b"gpu_active:1kB gpu_active:2kB")

    def test_retained_incident_cannot_be_promoted_to_complete_snapshot(self) -> None:
        snapshot = read_json(SNAPSHOT)
        snapshot["capture"].update({
            "source_kind": "retained-incident-evidence",
            "elevated": False,
            "closed_world": False,
            "errors": [{
                "code": "RETAINED_FACT_GAP",
                "detail": "no elevated closed-world FD owner or exact per-owner HMM census",
            }],
        })
        for role in admission.ROLES:
            node = snapshot["roles"][role]
            node["collection_state"] = "refused"
            node["observation"] = None
            node["errors"] = [{
                "code": "RETAINED_FACT_GAP",
                "detail": "systemd and aggregate GPUActive do not close v1 admission",
            }]
        result = evaluate(snapshot)
        self.assertEqual("REFUSE", result["decision"])
        self.assertIn("CAPTURE_NOT_ELEVATED", result["reason_codes"])
        self.assertIn("CAPTURE_NOT_CLOSED_WORLD", result["reason_codes"])
        self.assertIn("COLLECTION_REFUSED", result["reason_codes"])

    def test_coherently_rehashed_complete_retained_source_still_refuses(self) -> None:
        snapshot = read_json(SNAPSHOT)
        policy = read_json(POLICY)
        snapshot["capture"]["source_kind"] = "retained-incident-evidence"
        policy["capture_requirements"]["source_kind"] = "retained-incident-evidence"
        source = "retained-incident-aggregate-gpuactive-v1"
        for role in admission.ROLES:
            role_observation(snapshot, role)["device_census"][
                "hmm_accounting_source"] = source
            policy["roles"][role]["hmm_accounting_source"] = source
        result = evaluate(snapshot, policy)
        self.assertEqual("REFUSE", result["decision"])
        self.assertIn("RETAINED_INCIDENT_NON_ADMISSIBLE", result["reason_codes"])


class HMMAdmissionResultAndCliTests(unittest.TestCase):
    def setUp(self) -> None:
        self.result = admission.evaluate_bytes(
            SNAPSHOT.read_bytes(), POLICY.read_bytes(), trusted_now_utc=TRUSTED_NOW)

    def test_result_rejects_authority_or_performance_promotion(self) -> None:
        for field in ("target_execution_authority", "performance_result"):
            with self.subTest(field=field):
                value = copy.deepcopy(self.result)
                value[field] = True
                with self.assertRaisesRegex(admission.AdmissionError, "claim"):
                    admission.validate_admission_result_bytes(bytes_of(value))

    def test_result_rejects_unknown_classification(self) -> None:
        value = copy.deepcopy(self.result)
        value["roles"]["worker"]["classification"] = "PASS"
        with self.assertRaisesRegex(admission.AdmissionError, "unsupported"):
            admission.validate_admission_result_bytes(bytes_of(value))

    def test_result_rejects_admit_with_reason(self) -> None:
        value = copy.deepcopy(self.result)
        value["reason_codes"] = ["SNAPSHOT_STALE"]
        with self.assertRaisesRegex(admission.AdmissionError, "inconsistent"):
            admission.validate_admission_result_bytes(bytes_of(value))

    def test_result_rejects_omitted_role_reason(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["listener"]["owner_pids"] = [999999]
        value = evaluate(snapshot)
        value["reason_codes"] = []
        with self.assertRaises(admission.AdmissionError):
            admission.validate_admission_result_bytes(bytes_of(value))

    def test_raw_result_digest_is_external_not_self_referential(self) -> None:
        content = admission.pretty_bytes(self.result)
        parsed = admission.validate_bound_admission_result_bytes(
            content, SNAPSHOT.read_bytes(), POLICY.read_bytes())
        self.assertNotIn("hmm_admission_result_sha256", parsed)
        self.assertRegex(admission.sha256_bytes(content), r"^[0-9a-f]{64}$")

    def test_result_only_api_rejects_coherently_rehashed_forged_admit(self) -> None:
        snapshot = read_json(SNAPSHOT)
        role_observation(snapshot, "worker")["kernel"]["global_oom_count"] = 1
        snapshot_content = bytes_of(snapshot)
        forged = copy.deepcopy(self.result)
        forged["snapshot_sha256"] = admission.sha256_bytes(snapshot_content)
        forged["roles"]["worker"]["node_snapshot_sha256"] = admission.digest_value(
            snapshot["roles"]["worker"])
        forged_content = bytes_of(forged)
        with self.assertRaisesRegex(admission.AdmissionError, "bound canonical"):
            admission.validate_admission_result_bytes(forged_content)
        with self.assertRaisesRegex(admission.AdmissionError, "canonical recomputation"):
            admission.validate_bound_admission_result_bytes(
                forged_content, snapshot_content, POLICY.read_bytes())

    def test_result_only_api_rejects_any_positive_role(self) -> None:
        value = copy.deepcopy(self.result)
        value["decision"] = "REFUSE"
        value["reason_codes"] = ["COLLECTION_REFUSED"]
        worker = value["roles"]["worker"]
        worker.update({
            "classification": "REFUSE",
            "hmm_headroom_bytes": None,
            "production_identity_sha256": None,
            "reason_codes": ["COLLECTION_REFUSED"],
        })
        with self.assertRaisesRegex(admission.AdmissionError, "bound canonical"):
            admission.validate_admission_result_bytes(bytes_of(value))

    def test_result_only_api_accepts_only_a_wholly_negative_envelope(self) -> None:
        snapshot = read_json(SNAPSHOT)
        for role in admission.ROLES:
            node = snapshot["roles"][role]
            node["collection_state"] = "refused"
            node["observation"] = None
            node["errors"] = [{"code": "SYNTHETIC_REFUSAL", "detail": role}]
        result = evaluate(snapshot)
        self.assertEqual("REFUSE", result["decision"])
        self.assertTrue(all(
            result["roles"][role]["classification"] == "REFUSE"
            for role in admission.ROLES
        ))
        self.assertEqual(
            result, admission.validate_admission_result_bytes(bytes_of(result)))

    def test_bound_result_rejects_changed_snapshot_policy_or_reserialization(self) -> None:
        snapshot = SNAPSHOT.read_bytes()
        policy = POLICY.read_bytes()
        result = RESULT.read_bytes()
        for changed_snapshot, changed_policy, changed_result in (
            (snapshot + b" ", policy, result),
            (snapshot, policy + b" ", result),
            (snapshot, policy, result.rstrip() + b"\n\n"),
        ):
            with self.subTest(
                snapshot_changed=changed_snapshot != snapshot,
                policy_changed=changed_policy != policy,
                result_changed=changed_result != result,
            ), self.assertRaises(admission.AdmissionError):
                admission.validate_bound_admission_result_bytes(
                    changed_result, changed_snapshot, changed_policy)

    def test_cli_writes_only_a_new_local_result_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "result.json"
            command = [
                sys.executable, "-B", str(SOURCE),
                "--snapshot", str(SNAPSHOT), "--policy", str(POLICY),
                "--trusted-now-utc", TRUSTED_NOW, "--output", str(output),
            ]
            first = subprocess.run(command, cwd=REPO, capture_output=True, check=False)
            self.assertEqual(0, first.returncode, first.stderr.decode(errors="replace"))
            admission.validate_bound_admission_result_bytes(
                output.read_bytes(), SNAPSHOT.read_bytes(), POLICY.read_bytes())
            second = subprocess.run(command, cwd=REPO, capture_output=True, check=False)
            self.assertEqual(2, second.returncode)
            self.assertIn(b"output must be a new file", second.stderr)

    def test_cli_refusal_uses_distinct_exit_code(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            snapshot = read_json(SNAPSHOT)
            snapshot["capture"]["elevated"] = False
            snapshot_path = Path(temporary) / "snapshot.json"
            snapshot_path.write_bytes(bytes_of(snapshot))
            run = subprocess.run([
                sys.executable, "-B", str(SOURCE),
                "--snapshot", str(snapshot_path), "--policy", str(POLICY),
                "--trusted-now-utc", TRUSTED_NOW,
            ], cwd=REPO, capture_output=True, check=False)
            self.assertEqual(3, run.returncode)
            parsed = json.loads(run.stdout.decode("utf-8"))
            self.assertEqual("REFUSE", parsed["decision"])


if __name__ == "__main__":
    unittest.main()
