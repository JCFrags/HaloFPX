from __future__ import annotations

import copy
import errno
import hashlib
import importlib.util
import json
import math
import os
import shutil
import stat
import sys
import tempfile
import types
import unittest
from contextlib import contextmanager
from pathlib import Path
from typing import Any, Callable, Iterator
from unittest import mock

from tests.halofpx_strix_adapter_evidence_fixture import (
    create_control_repository,
    materialize_complete_adapter_tree,
)


REPO = Path(__file__).resolve().parents[1]
HMM_EXAMPLE_RESULT = json.loads(
    (REPO / "scripts" / "halofpx-strix-hmm-admission-result.example.json").read_text(
        encoding="utf-8"))
PRODUCTION_IDENTITIES = {
    role: HMM_EXAMPLE_RESULT["roles"][role]["production_identity_sha256"]
    for role in ("coordinator", "worker")
}


def load_module(name: str, path: Path) -> Any:
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


core = load_module("halofpx_strix_ab", REPO / "scripts" / "halofpx_strix_ab.py")
adapter = load_module(
    "halofpx_strix_ab_cachyos", REPO / "scripts" / "halofpx_strix_ab_cachyos.py")
evidence = load_module(
    "halofpx_strix_adapter_evidence",
    REPO / "scripts" / "halofpx_strix_adapter_evidence.py",
)


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def read_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    assert isinstance(value, dict)
    return value


def write_json(path: Path, value: Any) -> None:
    path.write_bytes(
        (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8"))


def rebuild_samples_jsonl(root: Path) -> None:
    samples = [read_json(path) for path in (root / "raw").glob("*/sample.json")]
    samples.sort(key=lambda sample: (sample["pair_id"], sample["order_index"]))
    (root / "samples.jsonl").write_bytes(b"".join(
        json.dumps(sample, sort_keys=True, separators=(",", ":")).encode("utf-8") + b"\n"
        for sample in samples
    ))


def rebuild_ledger(root: Path) -> None:
    """Rebuild the outer ledger independently, with platform-neutral LF bytes."""
    rows: list[bytes] = []
    for path in sorted(root.rglob("*"), key=lambda item: item.relative_to(root).as_posix()):
        if path.is_symlink() or not path.is_file() or path.name == "SHA256SUMS":
            continue
        relative = path.relative_to(root).as_posix()
        rows.append(f"{sha256_bytes(path.read_bytes())}  {relative}\n".encode("ascii"))
    (root / "SHA256SUMS").write_bytes(b"".join(rows))


def sample_directory(entry: dict[str, Any]) -> str:
    return f"raw/pair-{entry['pair_id']:03d}-order-{entry['order_index']}-{entry['condition']}"


def replace_receipt_and_dependent_copy(
    root: Path, entry_index: int, receipt: dict[str, Any],
) -> None:
    """Keep copied receipt/sample bindings coherent for deep identity attacks."""
    receipt_path = root / f"execution/entry-{entry_index:03d}/execution.json"
    write_json(receipt_path, receipt)
    receipt_bytes = receipt_path.read_bytes()
    sample_root = root / sample_directory(receipt["entry"])
    copied_receipt = sample_root / "extra-00-execution.json"
    copied_receipt.write_bytes(receipt_bytes)
    sample_path = sample_root / "sample.json"
    sample = read_json(sample_path)
    sample["raw"]["extra_0"].update({
        "size_bytes": len(receipt_bytes),
        "sha256": sha256_bytes(receipt_bytes),
    })
    write_json(sample_path, sample)
    rebuild_samples_jsonl(root)
    rebuild_ledger(root)


def replace_measured_artifact_and_bindings(
    root: Path, entry_index: int, receipt: dict[str, Any], *,
    receipt_field: str, filename: str, sample_extra: str, value: Any,
) -> None:
    """Rebind one measured semantic artifact through receipt and raw sample."""
    artifact_path = root / f"execution/entry-{entry_index:03d}/measured/{filename}"
    write_json(artifact_path, value)
    receipt["cycles"][-1][receipt_field] = value
    replace_receipt_and_dependent_copy(root, entry_index, receipt)
    sample_root = root / sample_directory(receipt["entry"])
    copied_path = sample_root / {
        "telemetry": "extra-03-telemetry.json",
        "gpu_admission": "extra-04-gpu-admission.json",
    }[receipt_field]
    copied_path.write_bytes(artifact_path.read_bytes())
    sample_path = sample_root / "sample.json"
    sample = read_json(sample_path)
    sample["raw"][sample_extra].update({
        "size_bytes": copied_path.stat().st_size,
        "sha256": sha256_bytes(copied_path.read_bytes()),
    })
    write_json(sample_path, sample)
    rebuild_samples_jsonl(root)
    rebuild_ledger(root)


def iter_cycle_live_proofs(cycle: dict[str, Any]) -> Iterator[dict[str, Any]]:
    """Yield proof payloads while preserving the post-request controller envelope."""
    for name, value in cycle["live_proofs"].items():
        yield value["proof"] if name == "worker_after_request" else value


def replace_measured_raw_and_bindings(
    root: Path, entry_index: int, receipt: dict[str, Any], raw_bytes: bytes,
) -> None:
    raw_path = root / f"execution/entry-{entry_index:03d}/measured/measurement-0-response.raw"
    raw_path.write_bytes(raw_bytes)
    receipt["cycles"][-1]["request"]["raw_http_sha256"] = sha256_bytes(raw_bytes)
    replace_receipt_and_dependent_copy(root, entry_index, receipt)
    sample_root = root / sample_directory(receipt["entry"])
    copied_raw = sample_root / "extra-02-measurement-0-response.raw"
    copied_raw.write_bytes(raw_bytes)
    sample_path = sample_root / "sample.json"
    sample = read_json(sample_path)
    sample["raw"]["extra_2"].update({
        "size_bytes": len(raw_bytes),
        "sha256": sha256_bytes(raw_bytes),
    })
    write_json(sample_path, sample)
    rebuild_samples_jsonl(root)
    rebuild_ledger(root)


class AdapterEvidenceTreeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temporary = tempfile.TemporaryDirectory(prefix="halofpx-adapter-evidence-test-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        cls.control = create_control_repository(
            cls.root / "repository",
            core=core,
            adapter=adapter,
            incident_source=REPO / adapter.ISSUE41_MANIFEST_RELATIVE,
        )
        cls.plan_bytes = cls.control["plan"].read_bytes()
        cls.policy_bytes = cls.control["policy"].read_bytes()
        cls.incident_bytes = cls.control["incident"].read_bytes()
        cls.baseline = cls.root / "complete-evidence"
        materialize_complete_adapter_tree(
            cls.baseline,
            core=core,
            adapter=adapter,
            plan_path=cls.control["plan"],
            policy_path=cls.control["policy"],
            incident_bytes=cls.incident_bytes,
            hmm_snapshot_bytes=cls.control["hmm_snapshot"].read_bytes(),
            hmm_policy_bytes=cls.control["hmm_policy"].read_bytes(),
            hmm_result_bytes=cls.control["hmm_result"].read_bytes(),
            selected_schedule_index=0,
        )
        # The production harness writes text ledgers with the host newline.
        # The retained validator contract is canonical ASCII/LF.
        rebuild_ledger(cls.baseline)
        cls.positive = cls.verify(cls.baseline)
        cls.sidecar_baseline: Path | None = None
        cls.sidecar_positive: dict[str, Any] | None = None
        if evidence.sampling_sync is None:
            raise AssertionError("merged PR67 sampling-output-sync sibling is required")
        cls.sidecar_baseline = cls.root / "complete-evidence-with-sampling-sidecar"
        materialize_complete_adapter_tree(
            cls.sidecar_baseline,
            core=core,
            adapter=adapter,
            plan_path=cls.control["plan"],
            policy_path=cls.control["policy"],
            incident_bytes=cls.incident_bytes,
            hmm_snapshot_bytes=cls.control["hmm_snapshot"].read_bytes(),
            hmm_policy_bytes=cls.control["hmm_policy"].read_bytes(),
            hmm_result_bytes=cls.control["hmm_result"].read_bytes(),
            selected_schedule_index=0,
            sampling_sync=evidence.sampling_sync,
        )
        rebuild_ledger(cls.sidecar_baseline)
        cls.sidecar_positive = cls.verify(cls.sidecar_baseline)

    @classmethod
    def verify(cls, root: Path, **overrides: Any) -> dict[str, Any]:
        arguments = {
            "expected_plan_bytes": cls.plan_bytes,
            "expected_policy_bytes": cls.policy_bytes,
            "expected_incident_bytes": cls.incident_bytes,
            "expected_schedule_index": 0,
            "expected_production_identity_sha256": dict(PRODUCTION_IDENTITIES),
        }
        arguments.update(overrides)
        return evidence.verify_adapter_evidence_tree(root, **arguments)

    @contextmanager
    def case_tree(self, baseline: Path | None = None) -> Iterator[Path]:
        with tempfile.TemporaryDirectory(prefix="case-", dir=self.root) as directory:
            case = Path(directory) / "evidence"
            shutil.copytree(baseline or self.baseline, case)
            yield case

    def require_sidecar(self) -> tuple[Path, dict[str, Any]]:
        if self.sidecar_baseline is None or self.sidecar_positive is None:
            raise AssertionError("merged sampling-output-sync profile was not constructed")
        return self.sidecar_baseline, self.sidecar_positive

    def rebuild_sidecar_derivatives(self, root: Path, sample_root: Path) -> None:
        sync = evidence.sampling_sync
        assert sync is not None
        plan = core.load_plan(root / "plan.json")
        side_plan = core.read_json(root / sync.PLAN_FILENAME)
        sample = core.read_json(sample_root / "sample.json")
        sidecar = sample_root / sync.EVIDENCE_DIRECTORY
        summary = sync.build_sample_summary(
            plan, side_plan, sample["pair_id"], sample["condition"],
            sample["order_index"], sidecar / "before.prom", sidecar / "after.prom",
            sidecar / "capture.json", sample["raw"]["response"]["sha256"],
            sample["raw"]["client"]["sha256"])
        write_json(sidecar / "summary.json", summary)
        sync.validate_frozen_run(root, plan)
        rebuild_ledger(root)

    def assert_rejected(self, root: Path, pattern: str | None = None, **overrides: Any) -> str:
        with self.assertRaises(evidence.AdapterEvidenceError) as caught:
            self.verify(root, **overrides)
        detail = str(caught.exception)
        if pattern is not None:
            self.assertRegex(detail, pattern)
        return detail

    def test_complete_two_entry_tree_is_non_promotional(self) -> None:
        result = self.positive
        self.assertEqual(result["schema"], evidence.VERIFY_SCHEMA)
        self.assertEqual(result["schedule_entries"], 2)
        self.assertEqual(result["retained_samples"], 2)
        self.assertEqual(len(result["sample_identities"]), 2)
        self.assertEqual(set(result["file_sha256"]), set(result["files"]))
        self.assertEqual(
            result["file_sha256"]["plan.json"], sha256_bytes(self.plan_bytes))
        self.assertEqual(result["selected_schedule_index"], 0)
        self.assertEqual(result["selected_receipt_bytes"], (
            self.baseline / "execution/entry-000/execution.json").read_bytes())
        self.assertIs(result["target_execution_authority"], False)
        self.assertIs(result["performance_result"], False)
        self.assertIs(result["hmm_measured_cycle_boot_bound"], True)
        self.assertIs(result["hmm_warmup_cycle_boot_bound"], True)
        self.assertIs(result["hmm_planned_increment_bound_to_adapter"], False)
        self.assertIs(result["workload_allocation_authority"], False)
        status = read_json(self.baseline / "status.json")
        analysis = read_json(self.baseline / "analysis.json")
        self.assertIs(status["execution_qualified"], False)
        self.assertIs(status["measurement_ready"], False)
        self.assertIs(status["performance_claim"], False)
        self.assertIs(analysis["execution_qualified"], False)
        self.assertIs(analysis["measurement_ready"], False)
        self.assertIs(analysis["performance_claim"], False)

    def test_native_completion_stream_is_closed_and_success_terminal(self) -> None:
        response_path = (
            self.baseline
            / "execution/entry-000/measured/measurement-0-response.json"
        )
        raw_path = (
            self.baseline
            / "execution/entry-000/measured/measurement-0-response.raw"
        )
        response = read_json(response_path)
        request = json.loads(self.control["request"].read_bytes())
        raw = raw_path.read_bytes()
        evidence._validate_sse(raw, response, request, "native stream")

        attacks = {
            "error event": raw.replace(
                b"data: {", b'data: {"error":{"message":"failure"},', 1),
            "DONE marker": raw + b"data: [DONE]\n\n",
            "event after terminal": raw + (
                b'data: {"index":0,"content":"x","tokens":[99],'
                b'"stop":false,"id_slot":-1,"tokens_predicted":9,'
                b'"tokens_evaluated":512}\n\n'),
            "missing terminal": raw.rsplit(b"data: ", 1)[0],
        }
        for name, candidate in attacks.items():
            with self.subTest(name=name), self.assertRaises(evidence.AdapterEvidenceError):
                evidence._validate_sse(candidate, response, request, "native stream")

        events = [block for block in raw.split(b"\n\n") if block]
        first = json.loads(events[0][len(b"data: "):].decode("utf-8"))
        second = json.loads(events[1][len(b"data: "):].decode("utf-8"))
        first["content"] = ""
        second["content"] = "xx"
        events[0] = b"data: " + json.dumps(first, separators=(",", ":")).encode()
        events[1] = b"data: " + json.dumps(second, separators=(",", ":")).encode()
        evidence._validate_sse(
            b"\n\n".join(events) + b"\n\n", response, request, "empty token")

        # llama-server may defer an incomplete UTF-8 token and emit accumulated
        # text later. That is valid server behavior, but this performance profile
        # admits only runs with one observable partial event for each token.
        suppressed = [block for block in raw.split(b"\n\n") if block]
        first = json.loads(suppressed[0][len(b"data: "):].decode("utf-8"))
        second = json.loads(suppressed[1][len(b"data: "):].decode("utf-8"))
        second["content"] = first["content"] + second["content"]
        suppressed[1] = b"data: " + json.dumps(second, separators=(",", ":")).encode()
        with self.assertRaises(evidence.AdapterEvidenceError):
            evidence._validate_sse(
                b"\n\n".join(suppressed[1:]) + b"\n\n", response, request,
                "deferred utf8 is unsupported by the admitted performance profile")

        aliased = [block for block in raw.split(b"\n\n") if block]
        terminal = json.loads(aliased[-1][len(b"data: "):].decode("utf-8"))
        terminal["generation_settings"]["stream"] = 1
        aliased[-1] = b"data: " + json.dumps(terminal, separators=(",", ":")).encode()
        with self.assertRaises(evidence.AdapterEvidenceError):
            evidence._validate_sse(
                b"\n\n".join(aliased) + b"\n\n", response, request, "bool integer alias")

    def test_native_server_timings_have_exact_nonspeculative_shape(self) -> None:
        response = read_json(
            self.baseline
            / "execution/entry-000/measured/measurement-0-response.json"
        )
        plan = core.load_plan(self.control["plan"])
        result = evidence._validate_response(response, plan, "native response")
        self.assertEqual(
            result["prompt_tokens_per_second"],
            1_000.0 / response["timings"]["prompt_ms"] * response["timings"]["prompt_n"],
        )
        for name, mutate in (
            ("missing per-token", lambda value: value["timings"].pop("prompt_per_token_ms")),
            ("draft extension", lambda value: value["timings"].update({"draft_n": 1})),
            ("wrong per-token", lambda value: value["timings"].update(
                {"predicted_per_token_ms": value["timings"]["predicted_per_token_ms"] + 0.1})),
            ("inflated reported rate", lambda value: value["timings"].update(
                {"prompt_per_second": value["timings"]["prompt_per_second"] * 1.009})),
        ):
            with self.subTest(name=name):
                candidate = copy.deepcopy(response)
                mutate(candidate)
                with self.assertRaises(evidence.AdapterEvidenceError):
                    evidence._validate_response(candidate, plan, "native response")

        nonround = copy.deepcopy(response)
        nonround["timings"].update({
            "prompt_ms": 9654.62232333005,
            "prompt_n": 512,
            "prompt_per_token_ms": 9654.62232333005 / 512,
            "prompt_per_second": 1_000.0 / 9654.62232333005 * 512,
        })
        normalized = evidence._validate_response(nonround, plan, "nonround response")
        self.assertEqual(
            normalized["prompt_tokens_per_second"],
            1_000.0 / 9654.62232333005 * 512,
        )

    def test_terminal_tokens_cached_cannot_be_coherently_rebound(self) -> None:
        with self.case_tree() as root:
            raw_path = (
                root / "execution/entry-000/measured/measurement-0-response.raw"
            )
            events = [block for block in raw_path.read_bytes().split(b"\n\n") if block]
            terminal = json.loads(events[-1][len(b"data: "):].decode("utf-8"))
            self.assertEqual(
                terminal["tokens_cached"],
                terminal["tokens_evaluated"] + terminal["tokens_predicted"] - 1,
            )
            terminal["tokens_cached"] = 0
            events[-1] = b"data: " + json.dumps(
                terminal, separators=(",", ":")).encode("utf-8")
            raw_bytes = b"\n\n".join(events) + b"\n\n"
            raw_path.write_bytes(raw_bytes)

            receipt = read_json(root / "execution/entry-000/execution.json")
            receipt["cycles"][-1]["request"]["raw_http_sha256"] = sha256_bytes(raw_bytes)
            replace_receipt_and_dependent_copy(root, 0, receipt)

            sample_root = root / sample_directory(receipt["entry"])
            copied_raw = sample_root / "extra-02-measurement-0-response.raw"
            copied_raw.write_bytes(raw_bytes)
            sample_path = sample_root / "sample.json"
            sample = read_json(sample_path)
            sample["raw"]["extra_2"].update({
                "size_bytes": len(raw_bytes),
                "sha256": sha256_bytes(raw_bytes),
            })
            write_json(sample_path, sample)
            rebuild_samples_jsonl(root)
            rebuild_ledger(root)
            self.assert_rejected(root, "terminal event")

    def test_terminal_newline_state_cannot_be_coherently_rebound(self) -> None:
        with self.case_tree() as root:
            raw_path = root / "execution/entry-000/measured/measurement-0-response.raw"
            events = [block for block in raw_path.read_bytes().split(b"\n\n") if block]
            terminal = json.loads(events[-1][len(b"data: "):].decode("utf-8"))
            self.assertIs(terminal["has_new_line"], False)
            terminal["has_new_line"] = True
            events[-1] = b"data: " + json.dumps(
                terminal, separators=(",", ":")).encode("utf-8")
            raw_bytes = b"\n\n".join(events) + b"\n\n"
            receipt = read_json(root / "execution/entry-000/execution.json")
            replace_measured_raw_and_bindings(root, 0, receipt, raw_bytes)
            self.assert_rejected(root, "SSE token/content/timing evidence")

        with tempfile.TemporaryDirectory(prefix="newline-case-", dir=self.root) as directory:
            case_root = Path(directory)
            content = "\n" + "x" * 7
            control = create_control_repository(
                case_root / "repository", core=core, adapter=adapter,
                incident_source=REPO / adapter.ISSUE41_MANIFEST_RELATIVE,
                response_content=content,
            )
            evidence_root = case_root / "evidence"
            materialize_complete_adapter_tree(
                evidence_root, core=core, adapter=adapter, plan_path=control["plan"],
                policy_path=control["policy"], incident_bytes=control["incident"].read_bytes(),
                hmm_snapshot_bytes=control["hmm_snapshot"].read_bytes(),
                hmm_policy_bytes=control["hmm_policy"].read_bytes(),
                hmm_result_bytes=control["hmm_result"].read_bytes(),
                selected_schedule_index=0, response_content=content,
            )
            rebuild_ledger(evidence_root)
            raw_path = evidence_root / "execution/entry-000/measured/measurement-0-response.raw"
            events = [block for block in raw_path.read_bytes().split(b"\n\n") if block]
            terminal = json.loads(events[-1][len(b"data: "):].decode("utf-8"))
            self.assertIs(terminal["has_new_line"], True)
            terminal["has_new_line"] = False
            events[-1] = b"data: " + json.dumps(
                terminal, separators=(",", ":")).encode("utf-8")
            raw_bytes = b"\n\n".join(events) + b"\n\n"
            receipt = read_json(evidence_root / "execution/entry-000/execution.json")
            replace_measured_raw_and_bindings(evidence_root, 0, receipt, raw_bytes)
            with self.assertRaisesRegex(
                    evidence.AdapterEvidenceError, "SSE token/content/timing evidence"):
                evidence.verify_adapter_evidence_tree(
                    evidence_root,
                    expected_plan_bytes=control["plan"].read_bytes(),
                    expected_policy_bytes=control["policy"].read_bytes(),
                    expected_incident_bytes=control["incident"].read_bytes(),
                    expected_schedule_index=0,
                    expected_production_identity_sha256=dict(PRODUCTION_IDENTITIES),
                )

    def test_worker_post_request_proof_is_controller_and_host_ordered(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            measured = receipt["cycles"][-1]
            envelope = measured["live_proofs"]["worker_after_request"]
            envelope["proof"] = copy.deepcopy(
                measured["live_proofs"]["worker_after_coordinator_ready"])
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "replays earlier host-local liveness")

        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            measured = receipt["cycles"][-1]
            envelope = measured["live_proofs"]["worker_after_request"]
            request_end = measured["request"]["controller_ended_monotonic_ns"]
            envelope["controller_started_monotonic_ns"] = request_end
            envelope["controller_ended_monotonic_ns"] = request_end + 1
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "outside the strict controller order")

    def test_legacy_v1_receipt_cannot_alias_the_v2_proof_envelope(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            receipt["schema"] = "halofpx.strix-ab-cachyos-execution.v1"
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "differs from the frozen plan/schedule/policy")

    def test_missing_sparse_and_non_directory_trees_are_rejected(self) -> None:
        missing = self.root / "does-not-exist"
        self.assert_rejected(missing)
        with tempfile.TemporaryDirectory(prefix="sparse-", dir=self.root) as directory:
            self.assert_rejected(Path(directory))
        with tempfile.TemporaryDirectory(prefix="root-file-", dir=self.root) as directory:
            root_file = Path(directory) / "evidence"
            root_file.write_bytes(b"not a tree")
            self.assert_rejected(root_file, "not a directory")

    def test_missing_cycle_directory_is_rejected(self) -> None:
        with self.case_tree() as root:
            shutil.rmtree(root / "execution/entry-000/warmup-0")
            rebuild_ledger(root)
            self.assert_rejected(root, "inventory")

    def test_missing_artifact_from_each_inventory_class_is_rejected(self) -> None:
        representatives = (
            "plan.json",
            "inputs/request.raw",
            "preflight/coordinator.json",
            "execution/entry-000/intent.json",
            "execution/entry-000/warmup-0/warmup-0-response.json",
            "execution/entry-000/measured/telemetry.json",
            "raw/pair-001-order-0-off/sample.json",
            "analysis.json",
            "status.json",
            "SHA256SUMS",
        )
        for relative in representatives:
            with self.subTest(relative=relative), self.case_tree() as root:
                (root / relative).unlink()
                if relative != "SHA256SUMS":
                    rebuild_ledger(root)
                self.assert_rejected(root, "inventory|SHA256SUMS|plan.json differs")

    def test_each_bound_hmm_input_is_mandatory(self) -> None:
        for relative in (
            "hmm-admission-snapshot.raw.json",
            "hmm-admission-policy.raw.json",
            "hmm-admission-result.raw.json",
        ):
            with self.subTest(relative=relative), self.case_tree() as root:
                (root / relative).unlink()
                rebuild_ledger(root)
                self.assert_rejected(root, "inventory")

    def test_extra_file_and_directory_in_each_inventory_class_is_rejected(self) -> None:
        parents = (
            "",
            "inputs",
            "preflight",
            "execution/entry-000",
            "execution/entry-000/warmup-0",
            "execution/entry-000/measured",
            "raw/pair-001-order-0-off",
        )
        for parent in parents:
            with self.subTest(parent=parent or "root"), self.case_tree() as root:
                (root / parent / "unexpected-sidecar.bin").write_bytes(b"unsupported\n")
                rebuild_ledger(root)
                self.assert_rejected(root, "inventory")
        with self.case_tree() as root:
            (root / "execution/unexpected-directory").mkdir()
            rebuild_ledger(root)
            self.assert_rejected(root, "directory inventory")

    def test_reserved_and_temporary_names_are_rejected(self) -> None:
        for reserved in ("CON", "prn.json", "AUX.txt", "LPT1.log", "clock$.raw"):
            with self.subTest(reserved=reserved):
                with self.assertRaises(evidence.AdapterEvidenceError):
                    evidence._validate_name(reserved, "fixture")
        for temporary in (".hidden", "tmp-orphan", "temp-orphan", ".record-orphan", "orphan.tmp", "orphan.bak"):
            with self.subTest(temporary=temporary), self.case_tree() as root:
                (root / temporary).write_bytes(b"uncommitted\n")
                self.assert_rejected(root, "reserved or temporary")

    def test_duplicate_json_key_is_rejected(self) -> None:
        with self.case_tree() as root:
            status = root / "status.json"
            content = status.read_bytes()
            self.assertTrue(content.startswith(b"{"))
            status.write_bytes(b'{"schema":"duplicate",' + content[1:])
            rebuild_ledger(root)
            self.assert_rejected(root, "duplicate")

    def test_cross_swapped_execution_entries_are_rejected(self) -> None:
        with self.case_tree() as root:
            entry_zero = root / "execution/entry-000"
            entry_one = root / "execution/entry-001"
            temporary = root / "execution/swap-in-progress"
            entry_zero.rename(temporary)
            entry_one.rename(entry_zero)
            temporary.rename(entry_one)
            rebuild_ledger(root)
            self.assert_rejected(root, "frozen plan|schedule")

    def test_artifact_class_tampering_fails_with_rebuilt_ledger(self) -> None:
        artifacts = {
            "receipt": "execution/entry-000/execution.json",
            "policy": "execution/entry-000/policy.raw",
            "preflight": "preflight/coordinator.json",
            "incident": "incident.raw",
            "HMM": "hmm-admission-result.raw.json",
            "response": "execution/entry-000/measured/measurement-0-response.json",
            "client": "execution/entry-000/measured/measurement-0-client.json",
            "raw": "execution/entry-000/measured/measurement-0-response.raw",
            "telemetry": "execution/entry-000/measured/telemetry.json",
            "GPU": "execution/entry-000/measured/gpu-admission.json",
            "journal": "execution/entry-000/measured/measurement-0-worker.journal",
            "analysis": "analysis.json",
            "status": "status.json",
        }
        for artifact_class, relative in artifacts.items():
            with self.subTest(artifact_class=artifact_class), self.case_tree() as root:
                path = root / relative
                if path.suffix == ".json" or path.name.endswith(".raw.json"):
                    value = read_json(path)
                    value["unsupported_tamper"] = True
                    write_json(path, value)
                else:
                    path.write_bytes(path.read_bytes() + b"tamper\n")
                rebuild_ledger(root)
                self.assert_rejected(root)

    def test_result_and_sample_tampering_are_rejected(self) -> None:
        with self.case_tree() as root:
            sample_path = root / "raw/pair-001-order-0-off/sample.json"
            sample = read_json(sample_path)
            sample["result"]["content_sha256"] = "0" * 64
            write_json(sample_path, sample)
            rebuild_samples_jsonl(root)
            rebuild_ledger(root)
            self.assert_rejected(root, "summaries|response")
        with self.case_tree() as root:
            sample_path = root / "raw/pair-001-order-0-off/sample.json"
            sample = read_json(sample_path)
            sample["status"] = "failure"
            sample["failure_code"] = "synthetic-tamper"
            write_json(sample_path, sample)
            rebuild_samples_jsonl(root)
            rebuild_ledger(root)
            self.assert_rejected(root, "successful frozen schedule entry")

    def test_analysis_and_status_cannot_promote_the_evidence(self) -> None:
        for relative in ("analysis.json", "status.json"):
            with self.subTest(relative=relative), self.case_tree() as root:
                value = read_json(root / relative)
                value["execution_qualified"] = True
                value["measurement_ready"] = True
                value["performance_claim"] = True
                write_json(root / relative, value)
                rebuild_ledger(root)
                self.assert_rejected(root, "analysis|status")

    def test_corrupt_ledger_is_rejected(self) -> None:
        with self.case_tree() as root:
            ledger = root / "SHA256SUMS"
            ledger.write_bytes(ledger.read_bytes().replace(b"a", b"b", 1))
            self.assert_rejected(root, "SHA256SUMS")

    def test_unsupported_sidecar_is_rejected(self) -> None:
        with self.case_tree() as root:
            sidecar = root / "execution/entry-000/measured/perf-profile.data"
            sidecar.write_bytes(b"not admitted by the evidence schema\n")
            rebuild_ledger(root)
            self.assert_rejected(root, "inventory")

    def test_supported_sampling_sidecar_is_reparsed_and_non_promotional(self) -> None:
        _, result = self.require_sidecar()
        self.assertEqual(result["observability_profile"], evidence.SAMPLING_SYNC_CONTRACT)
        self.assertEqual(len(result["files"]), 72)
        self.assertEqual(len(result["directories"]), 14)
        self.assertIs(result["target_execution_authority"], False)
        self.assertIs(result["performance_result"], False)

    def test_sampling_sidecar_missing_partial_or_orphan_inventory_is_rejected(self) -> None:
        baseline, _ = self.require_sidecar()
        missing = (
            "sampling-output-sync-plan.json",
            "sampling-output-sync-analysis.json",
            "raw/pair-001-order-0-off/sampling-output-sync/before.prom",
        )
        for relative in missing:
            with self.subTest(relative=relative), self.case_tree(baseline) as root:
                (root / relative).unlink()
                rebuild_ledger(root)
                self.assert_rejected(root, "inventory")
        with self.case_tree(baseline) as root:
            orphan = root / "raw/orphan/sampling-output-sync"
            orphan.mkdir(parents=True)
            for filename in evidence.SAMPLING_SYNC_SAMPLE_FILES:
                (orphan / filename).write_bytes(b"counterfeit\n")
            rebuild_ledger(root)
            self.assert_rejected(root, "inventory")

    def test_sampling_sidecar_raw_summary_analysis_and_cross_swap_tamper_reject(self) -> None:
        baseline, _ = self.require_sidecar()
        paths = (
            "raw/pair-001-order-0-off/sampling-output-sync/before.prom",
            "raw/pair-001-order-0-off/sampling-output-sync/summary.json",
            "sampling-output-sync-analysis.json",
        )
        for relative in paths:
            with self.subTest(relative=relative), self.case_tree(baseline) as root:
                path = root / relative
                path.write_bytes(path.read_bytes() + b"tamper\n")
                rebuild_ledger(root)
                self.assert_rejected(root, "sampling-output-sync|strict duplicate-free")
        with self.case_tree(baseline) as root:
            left = root / "raw/pair-001-order-0-off/sampling-output-sync"
            right = root / "raw/pair-001-order-1-on/sampling-output-sync"
            temporary = root / "raw/sidecar-swap-in-progress"
            left.rename(temporary)
            right.rename(left)
            temporary.rename(right)
            rebuild_ledger(root)
            self.assert_rejected(root, "sampling-output-sync")

    def test_sampling_sidecar_cannot_rebind_adapter_identity_or_request_interval(self) -> None:
        baseline, _ = self.require_sidecar()
        sample_relative = "raw/pair-001-order-0-off"
        for attack in ("identity", "interval"):
            with self.subTest(attack=attack), self.case_tree(baseline) as root:
                sample_root = root / sample_relative
                capture_path = sample_root / "sampling-output-sync/capture.json"
                capture = read_json(capture_path)
                if attack == "identity":
                    for section in (capture["before"], capture["request"], capture["after"]):
                        section["identity"]["pid"] += 10_000
                else:
                    capture["before"]["captured_monotonic_ns"] += 100
                    capture["request"]["started_monotonic_ns"] += 100
                    capture["request"]["ended_monotonic_ns"] += 100
                    capture["after"]["captured_monotonic_ns"] += 100
                write_json(capture_path, capture)
                self.rebuild_sidecar_derivatives(root, sample_root)
                self.assert_rejected(
                    root, "differs from the measured adapter coordinator|measured adapter request")

    def test_sampling_sidecar_is_inside_runtime_and_freshness_lifecycle(self) -> None:
        baseline, _ = self.require_sidecar()
        sample_relative = "raw/pair-001-order-0-off"
        with self.case_tree(baseline) as root:
            sample_root = root / sample_relative
            capture_path = sample_root / "sampling-output-sync/capture.json"
            capture = read_json(capture_path)
            client = read_json(
                root / "execution/entry-000/measured/measurement-0-client.json")
            client_epoch = int(evidence._timestamp(
                client["started_at"], "client.started_at").timestamp())
            for section in (capture["before"], capture["request"], capture["after"]):
                section["identity"]["metrics_process_start_time_unix"] = client_epoch - 3600
            write_json(capture_path, capture)
            self.rebuild_sidecar_derivatives(root, sample_root)
            self.assert_rejected(root, "metrics process wall time escapes RuntimeMaxSec")

        with self.case_tree(baseline) as root:
            sample_root = root / sample_relative
            capture_path = sample_root / "sampling-output-sync/capture.json"
            capture = read_json(capture_path)
            receipt = read_json(root / "execution/entry-000/execution.json")
            start_ns = receipt["cycles"][-1]["identities"]["coordinator"][
                "start_monotonic_us"] * 1000
            capture["after"]["captured_monotonic_ns"] = start_ns + 3600 * 1_000_000_000
            write_json(capture_path, capture)
            self.rebuild_sidecar_derivatives(root, sample_root)
            self.assert_rejected(root, "freshness/lifetime window")

    def test_counterfeit_sidecar_complete_flag_without_profile_refuses(self) -> None:
        with self.case_tree() as root:
            write_json(root / "sampling-output-sync-analysis.json", {
                "schema": "halofpx.sampling-output-sync-analysis.v1",
                "contract": evidence.SAMPLING_SYNC_CONTRACT,
                "enabled": True,
                "evidence_complete": True,
            })
            rebuild_ledger(root)
            self.assert_rejected(root, "inventory")

    def test_file_symlink_is_rejected(self) -> None:
        with self.case_tree() as root:
            link = root / "linked-plan.json"
            try:
                os.symlink(root / "plan.json", link)
            except OSError as exc:
                if exc.errno in {errno.EACCES, errno.EPERM} or getattr(exc, "winerror", None) == 1314:
                    self.skipTest(f"symbolic-link privilege unavailable: {exc}")
                raise
            self.assert_rejected(root, "symbolic link or reparse point")

    def test_directory_symlink_or_reparse_point_is_rejected(self) -> None:
        with self.case_tree() as root:
            link = root / "linked-inputs"
            try:
                os.symlink(root / "inputs", link, target_is_directory=True)
            except OSError as exc:
                if exc.errno in {errno.EACCES, errno.EPERM} or getattr(exc, "winerror", None) == 1314:
                    self.skipTest(f"directory-link privilege unavailable: {exc}")
                raise
            if os.name == "nt":
                self.assertTrue(evidence._is_reparse(os.lstat(link)))
            self.assert_rejected(root, "symbolic link or reparse point")

    def test_evidence_root_under_linked_parent_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory(prefix="linked-parent-", dir=self.root) as directory:
            container = Path(directory)
            real_parent = container / "real-parent"
            real_parent.mkdir()
            shutil.copytree(self.baseline, real_parent / "evidence")
            linked_parent = container / "linked-parent"
            try:
                os.symlink(real_parent, linked_parent, target_is_directory=True)
            except OSError as exc:
                if exc.errno in {errno.EACCES, errno.EPERM} or getattr(exc, "winerror", None) == 1314:
                    self.skipTest(f"directory-link privilege unavailable: {exc}")
                raise
            self.assert_rejected(
                linked_parent / "evidence", "evidence-root path component.*symbolic link|reparse point")

    def test_unrelated_ancestor_directory_churn_is_not_a_tree_race(self) -> None:
        with self.case_tree() as root:
            ancestor = self.root.resolve()
            original_lstat = evidence.os.lstat
            calls = 0

            def changing_ancestor_lstat(path: Any) -> Any:
                nonlocal calls
                result = original_lstat(path)
                if Path(os.path.abspath(os.fspath(path))) != ancestor:
                    return result
                calls += 1
                if calls == 1:
                    return result
                # Directory contents outside the evidence subtree may change
                # size/link-count/timestamps without replacing the traversed
                # ancestor itself.
                return types.SimpleNamespace(
                    st_dev=result.st_dev,
                    st_ino=result.st_ino,
                    st_mode=result.st_mode,
                    st_nlink=result.st_nlink + 1,
                    st_size=result.st_size + 1,
                    st_mtime=result.st_mtime + 1,
                    st_mtime_ns=getattr(result, "st_mtime_ns", 0) + 1,
                    st_ctime=result.st_ctime + 1,
                    st_ctime_ns=getattr(result, "st_ctime_ns", 0) + 1,
                    st_file_attributes=getattr(result, "st_file_attributes", 0),
                )

            with mock.patch.object(evidence.os, "lstat", side_effect=changing_ancestor_lstat):
                result = self.verify(root)
            self.assertEqual(result["schedule_entries"], 2)
            self.assertGreaterEqual(calls, 2)

    def test_nonregular_entry_is_rejected(self) -> None:
        with self.case_tree() as root:
            nonregular = root / "nonregular-entry"
            if hasattr(os, "mkfifo"):
                try:
                    os.mkfifo(nonregular)
                except OSError as exc:
                    if exc.errno in {errno.EACCES, errno.EPERM}:
                        self.skipTest(f"FIFO privilege unavailable: {exc}")
                    raise
                self.assert_rejected(root, "neither a directory nor a regular file")
            else:
                # Windows has no filesystem FIFO API. Simulate the stat
                # classification at the same boundary while using a real tree.
                nonregular.write_bytes(b"placeholder")
                original_lstat = evidence.os.lstat

                def nonregular_lstat(path: Any) -> Any:
                    result = original_lstat(path)
                    if os.path.abspath(os.fspath(path)) != os.path.abspath(os.fspath(nonregular)):
                        return result
                    class NonRegularStat:
                        pass

                    altered = NonRegularStat()
                    for name in (
                        "st_dev", "st_ino", "st_nlink", "st_size", "st_mtime",
                        "st_mtime_ns", "st_ctime", "st_ctime_ns", "st_file_attributes",
                    ):
                        setattr(altered, name, getattr(result, name, 0))
                    altered.st_mode = stat.S_IFIFO
                    return altered

                with mock.patch.object(evidence.os, "lstat", side_effect=nonregular_lstat), \
                        mock.patch.object(evidence, "_direntry_matches_lstat", return_value=True):
                    self.assert_rejected(root, "neither a directory nor a regular file")

    def test_hard_link_is_rejected(self) -> None:
        with self.case_tree() as root:
            try:
                os.link(root / "plan.json", root / "hardlink-copy.json")
            except OSError as exc:
                if exc.errno in {errno.EACCES, errno.EPERM, errno.ENOTSUP} or \
                        getattr(exc, "winerror", None) in {1, 5, 1314}:
                    self.skipTest(f"hard-link privilege unavailable: {exc}")
                raise
            self.assert_rejected(root, "hard-linked")

    def test_two_pass_capture_race_is_rejected(self) -> None:
        with self.case_tree() as root:
            original_scan = evidence._scan_once
            calls = 0

            def racing_scan(
                path: str, expected_identities: dict[str, tuple[int, ...]] | None = None,
                *, max_tree_bytes: int = evidence._MAX_TREE_BYTES,
            ) -> dict[str, Any]:
                nonlocal calls
                snapshot = original_scan(
                    path, expected_identities, max_tree_bytes=max_tree_bytes)
                calls += 1
                if calls == 1:
                    status = root / "status.json"
                    status.write_bytes(status.read_bytes() + b"race")
                return snapshot

            with mock.patch.object(evidence, "_scan_once", side_effect=racing_scan):
                self.assert_rejected(root, "between capture passes")
            self.assertEqual(calls, 2)

    def test_root_aba_swap_between_initial_identity_and_each_scan_is_rejected(self) -> None:
        with self.case_tree() as root:
            attacker = root.parent / "attacker-tree"
            shutil.copytree(root, attacker)
            original_scan = evidence._scan_once
            calls = 0

            def aba_scan(
                path: str, expected_identities: dict[str, tuple[int, ...]] | None = None,
                *, max_tree_bytes: int = evidence._MAX_TREE_BYTES,
            ) -> dict[str, Any]:
                nonlocal calls
                calls += 1
                parked = root.parent / f"parked-{calls}"
                root.rename(parked)
                attacker.rename(root)
                try:
                    snapshot = original_scan(
                        path, expected_identities, max_tree_bytes=max_tree_bytes)
                finally:
                    root.rename(attacker)
                    parked.rename(root)
                return snapshot

            with mock.patch.object(evidence, "_scan_once", side_effect=aba_scan):
                self.assert_rejected(root, "initially captured directory identity")
            self.assertEqual(calls, 1)

    def test_nested_directory_identity_is_anchored_across_capture_passes(self) -> None:
        with self.case_tree() as root:
            original_scan = evidence._scan_once
            calls = 0

            def nested_swap(
                path: str, expected_identities: dict[str, tuple[int, ...]] | None = None,
                *, max_tree_bytes: int = evidence._MAX_TREE_BYTES,
            ) -> dict[str, Any]:
                nonlocal calls
                calls += 1
                if calls == 2:
                    original = root / "inputs"
                    parked = root / "inputs-parked"
                    substitute = root / "inputs-substitute"
                    shutil.copytree(original, substitute)
                    original.rename(parked)
                    substitute.rename(original)
                    try:
                        return original_scan(
                            path, expected_identities, max_tree_bytes=max_tree_bytes)
                    finally:
                        shutil.rmtree(original)
                        parked.rename(original)
                return original_scan(
                    path, expected_identities, max_tree_bytes=max_tree_bytes)

            with mock.patch.object(evidence, "_scan_once", side_effect=nested_swap):
                self.assert_rejected(root, "differs from the initially captured directory identity")
            self.assertEqual(calls, 2)

    def test_root_disappearance_after_component_census_is_typed_refusal(self) -> None:
        with self.case_tree() as root:
            original_lstat = evidence.os.lstat
            target = os.path.abspath(os.fspath(root))
            calls = 0

            def disappearing_root(path: Any) -> Any:
                nonlocal calls
                if os.path.abspath(os.fspath(path)) == target:
                    calls += 1
                    if calls == 2:
                        raise FileNotFoundError("synthetic root disappearance")
                return original_lstat(path)

            with mock.patch.object(evidence.os, "lstat", side_effect=disappearing_root):
                self.assert_rejected(root, "cannot bind initial evidence-root identity")

    def test_caller_supplied_counterfeit_incident_cannot_launder_pr44_identity(self) -> None:
        with self.case_tree() as root:
            incident_path = root / "incident.raw"
            counterfeit = read_json(incident_path)
            counterfeit["candidate_commit"] = "f" * 40
            write_json(incident_path, counterfeit)
            rebuild_ledger(root)
            self.assert_rejected(
                root, "pinned PR44",
                expected_incident_bytes=incident_path.read_bytes(),
            )

    def test_identity_reuse_is_rejected_after_dependent_bindings_are_rebuilt(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            warmup = receipt["cycles"][0]
            measured = receipt["cycles"][1]
            role = "worker"
            reused = warmup["identities"][role]["invocation_id"]
            measured_identity = measured["identities"][role]
            measured_identity["invocation_id"] = reused
            measured["readiness"][role]["identity"]["invocation_id"] = reused
            for proof in iter_cycle_live_proofs(measured):
                if proof["process"]["pid"] == measured_identity["pid"]:
                    proof["systemd"]["InvocationID"] = reused
            measured["terminal"][role]["properties"]["InvocationID"] = reused
            measured["cleanup"][role]["pre_state"]["InvocationID"] = reused
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "not fresh and increasing")

    def test_distinct_pids_may_share_one_kernel_start_tick(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            warmup = receipt["cycles"][0]
            measured = receipt["cycles"][1]
            role = "worker"
            shared_tick = warmup["identities"][role]["process_start_ticks"]
            measured_identity = measured["identities"][role]
            self.assertNotEqual(
                warmup["identities"][role]["pid"], measured_identity["pid"])
            measured_identity["process_start_ticks"] = shared_tick
            measured["readiness"][role]["identity"]["process_start_ticks"] = shared_tick
            for proof in iter_cycle_live_proofs(measured):
                if proof["process"]["pid"] == measured_identity["pid"]:
                    proof["process"]["process_start_ticks"] = shared_tick
            replace_receipt_and_dependent_copy(root, 0, receipt)
            result = self.verify(root)
            self.assertEqual(result["schedule_entries"], 2)

    def test_cross_host_invocation_replay_is_rejected(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            measured = receipt["cycles"][1]
            reused = measured["identities"]["coordinator"]["invocation_id"]
            worker = measured["identities"]["worker"]
            worker["invocation_id"] = reused
            measured["readiness"]["worker"]["identity"]["invocation_id"] = reused
            for proof in iter_cycle_live_proofs(measured):
                if proof["process"]["pid"] == worker["pid"]:
                    proof["systemd"]["InvocationID"] = reused
            measured["terminal"]["worker"]["properties"]["InvocationID"] = reused
            measured["cleanup"]["worker"]["pre_state"]["InvocationID"] = reused
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "reuses or reverses a disposable identity")

    def test_bound_terminal_cleanup_production_and_gpu_tampering_is_rejected(self) -> None:
        attacks: tuple[tuple[str, Callable[[dict[str, Any]], None], str], ...] = (
            (
                "terminal invocation",
                lambda receipt: receipt["cycles"][1]["terminal"]["worker"]["properties"].update(
                    {"InvocationID": "f" * 32}
                ),
                "terminal.*captured identity",
            ),
            (
                "live executable path",
                lambda receipt: [
                    proof["process"].update({"exe": "/var/tmp/substituted-binary"})
                    for proof in iter_cycle_live_proofs(receipt["cycles"][1])
                    if proof["process"]["pid"]
                    == receipt["cycles"][1]["identities"]["coordinator"]["pid"]
                ],
                "exact launched binary path",
            ),
            (
                "cleanup cgroup absence",
                lambda receipt: receipt["cycles"][1]["cleanup"]["worker"].update(
                    {"captured_cgroup_absent": False}
                ),
                "captured_cgroup_absent must be literal true",
            ),
            (
                "production after cgroup",
                lambda receipt: receipt["production_after"]["coordinator"].update(
                    {"control_group": "/system.slice/production.service"}
                ),
                "absent inactive process/cgroup",
            ),
            (
                "pre-intent foreign GPU owner",
                lambda receipt: receipt["gpu_admission_before_intent"]["worker"].update(
                    {"pids": [99999], "foreign_pids": [99999]}
                ),
                "exact allowed-process GPU census",
            ),
        )
        for name, mutate, pattern in attacks:
            with self.subTest(name=name), self.case_tree() as root:
                receipt = read_json(root / "execution/entry-000/execution.json")
                mutate(receipt)
                replace_receipt_and_dependent_copy(root, 0, receipt)
                self.assert_rejected(root, pattern)

    def test_cleanup_accepts_exact_already_unloaded_real_runner_variant(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            receipt["cycles"][1]["cleanup"]["worker"]["pre_state"] = {
                "LoadState": "not-found",
                "ActiveState": "inactive",
                "MainPID": "0",
                "InvocationID": "",
                "ControlGroup": "",
                "FragmentPath": "",
            }
            replace_receipt_and_dependent_copy(root, 0, receipt)
            result = self.verify(root)
            self.assertEqual(result["schedule_entries"], 2)

    def test_cross_clock_duration_tampering_is_rejected(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            request = receipt["cycles"][1]["request"]
            controller_duration = (
                request["controller_ended_monotonic_ns"]
                - request["controller_started_monotonic_ns"]
            )
            request["remote_ended_monotonic_ns"] = (
                request["remote_started_monotonic_ns"] + controller_duration * 10
            )
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "remote duration escapes")

        client_name = (
            self.baseline
            / "execution/entry-000/measured/measurement-0-client.json"
        )
        client = read_json(client_name)
        ended = evidence._timestamp(client["ended_at"], "client.ended_at")
        client["ended_at"] = (ended + evidence.dt.timedelta(minutes=1)).isoformat()
        with self.assertRaisesRegex(
            evidence.AdapterEvidenceError, "wall_ms differs from.*timestamp interval"
        ):
            evidence._validate_client(
                client,
                8,
                (
                    client["remote_started_monotonic_ns"],
                    client["remote_ended_monotonic_ns"],
                ),
                "client",
            )

    def test_readiness_replay_and_process_before_prior_cleanup_are_rejected(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            receipt["cycles"][-1]["readiness"]["worker"] = copy.deepcopy(
                receipt["cycles"][0]["readiness"]["worker"])
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "exact process/boot identity")

        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            warmup = receipt["cycles"][0]
            measured = receipt["cycles"][-1]
            measured["identities"]["coordinator"]["start_monotonic_us"] = (
                warmup["cleanup"]["coordinator"]["completed_monotonic_ns"] // 1000
            )
            replace_receipt_and_dependent_copy(root, 0, receipt)
            self.assert_rejected(root, "starts before predecessor cleanup")

    def test_hmm_freshness_deadline_is_half_open(self) -> None:
        with self.case_tree() as root:
            intent_path = root / "execution/entry-000/intent.json"
            intent = read_json(intent_path)
            intent["created_at"] = "2026-08-13T07:05:02+00:00"
            write_json(intent_path, intent)
            rebuild_ledger(root)
            self.assert_rejected(root, "HMM capture/validity window")

    def test_telemetry_requires_contained_in_request_and_fresh_samples(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            measured = receipt["cycles"][-1]
            telemetry = copy.deepcopy(measured["telemetry"])
            request = measured["request"]
            start = request["controller_started_monotonic_ns"]
            worker_host = "nimo-2"
            telemetry[worker_host][1]["controller_started_ns"] = start - 20
            telemetry[worker_host][1]["controller_ended_ns"] = start + 1
            telemetry[worker_host][2]["controller_started_ns"] = start - 10
            telemetry[worker_host][2]["controller_ended_ns"] = start + 2
            replace_measured_artifact_and_bindings(
                root, 0, receipt, receipt_field="telemetry", filename="telemetry.json",
                sample_extra="extra_3", value=telemetry)
            self.assert_rejected(root, "wholly contained")

        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            measured = receipt["cycles"][-1]
            telemetry = copy.deepcopy(measured["telemetry"])
            # Worker HMM capture completes at 2,000,003,000 ns and the frozen
            # max age is 300 s; equality is outside the half-open window.
            telemetry["nimo-2"][-1]["sample"]["monotonic_ns"] = 302_000_003_000
            replace_measured_artifact_and_bindings(
                root, 0, receipt, receipt_field="telemetry", filename="telemetry.json",
                sample_extra="extra_3", value=telemetry)
            self.assert_rejected(root, "freshness/lifetime window")

    def test_telemetry_must_precede_same_host_cleanup(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            measured = receipt["cycles"][-1]
            telemetry = copy.deepcopy(measured["telemetry"])
            coordinator_host = "nimo-1"
            telemetry[coordinator_host][-1]["sample"]["monotonic_ns"] = (
                measured["cleanup"]["coordinator"]["completed_monotonic_ns"] + 1
            )
            replace_measured_artifact_and_bindings(
                root, 0, receipt, receipt_field="telemetry", filename="telemetry.json",
                sample_extra="extra_3", value=telemetry)
            self.assert_rejected(root, "does not precede same-host cleanup")

    def test_gpu_requires_one_common_two_role_contained_witness(self) -> None:
        with self.case_tree() as root:
            receipt = read_json(root / "execution/entry-000/execution.json")
            measured = receipt["cycles"][-1]
            admission = copy.deepcopy(measured["gpu_admission"])
            start = measured["request"]["controller_started_monotonic_ns"]
            first, second = admission["samples"][1:3]
            first["controller_started_ns"], first["controller_ended_ns"] = start - 20, start + 30
            second["controller_started_ns"], second["controller_ended_ns"] = start - 10, start + 40
            first["admission"]["coordinator"]["controller_started_ns"] = start + 10
            first["admission"]["coordinator"]["controller_ended_ns"] = start + 20
            first["admission"]["worker"]["controller_started_ns"] = start - 10
            first["admission"]["worker"]["controller_ended_ns"] = start + 5
            second["admission"]["coordinator"]["controller_started_ns"] = start - 5
            second["admission"]["coordinator"]["controller_ended_ns"] = start + 10
            second["admission"]["worker"]["controller_started_ns"] = start + 15
            second["admission"]["worker"]["controller_ended_ns"] = start + 25
            replace_measured_artifact_and_bindings(
                root, 0, receipt, receipt_field="gpu_admission",
                filename="gpu-admission.json", sample_extra="extra_4", value=admission)
            self.assert_rejected(root, "common two-role census")

    def test_warmup_content_must_match_every_deterministic_cycle(self) -> None:
        with tempfile.TemporaryDirectory(prefix="no-golden-", dir=self.root) as directory:
            temporary = Path(directory)
            control = create_control_repository(
                temporary / "repository", core=core, adapter=adapter,
                incident_source=REPO / adapter.ISSUE41_MANIFEST_RELATIVE)
            plan = core.read_json(control["plan"])
            plan["request"].pop("expected_content_sha256")
            core.write_json(control["plan"], plan)
            root = temporary / "evidence"
            materialize_complete_adapter_tree(
                root, core=core, adapter=adapter, plan_path=control["plan"],
                policy_path=control["policy"],
                incident_bytes=control["incident"].read_bytes(),
                hmm_snapshot_bytes=control["hmm_snapshot"].read_bytes(),
                hmm_policy_bytes=control["hmm_policy"].read_bytes(),
                hmm_result_bytes=control["hmm_result"].read_bytes(),
                selected_schedule_index=0)
            response_path = root / "execution/entry-000/warmup-0/warmup-0-response.json"
            raw_path = root / "execution/entry-000/warmup-0/warmup-0-response.raw"
            response = read_json(response_path)
            response["content"] = "y" * 8
            write_json(response_path, response)
            blocks = [block for block in raw_path.read_bytes().split(b"\n\n") if block]
            for index in range(8):
                event = json.loads(blocks[index][len(b"data: "):].decode("utf-8"))
                event["content"] = "y"
                blocks[index] = b"data: " + json.dumps(
                    event, separators=(",", ":")).encode("utf-8")
            raw_path.write_bytes(b"\n\n".join(blocks) + b"\n\n")
            receipt = read_json(root / "execution/entry-000/execution.json")
            request = receipt["cycles"][0]["request"]
            request["response_sha256"] = sha256_bytes(response_path.read_bytes())
            request["raw_http_sha256"] = sha256_bytes(raw_path.read_bytes())
            replace_receipt_and_dependent_copy(root, 0, receipt)
            with self.assertRaisesRegex(
                    evidence.AdapterEvidenceError, "deterministic content differs"):
                evidence.verify_adapter_evidence_tree(
                    root,
                    expected_plan_bytes=control["plan"].read_bytes(),
                    expected_policy_bytes=control["policy"].read_bytes(),
                    expected_incident_bytes=control["incident"].read_bytes(),
                    expected_schedule_index=0,
                    expected_production_identity_sha256=dict(PRODUCTION_IDENTITIES),
                )

    def test_telemetry_sensor_path_is_canonical_and_contained(self) -> None:
        with self.case_tree() as root:
            telemetry_path = root / "execution/entry-000/measured/telemetry.json"
            telemetry = read_json(telemetry_path)
            for rows in telemetry.values():
                gpu = rows[0]["sample"]["gpu"]
                valid = next(iter(gpu))
                gpu[f"{valid}/../counterfeit-gpu_busy_percent"] = gpu.pop(valid)
            write_json(telemetry_path, telemetry)

            receipt = read_json(root / "execution/entry-000/execution.json")
            receipt["cycles"][1]["telemetry"] = telemetry
            replace_receipt_and_dependent_copy(root, 0, receipt)

            sample_root = root / sample_directory(receipt["entry"])
            copied_telemetry = sample_root / "extra-03-telemetry.json"
            copied_telemetry.write_bytes(telemetry_path.read_bytes())
            sample_path = sample_root / "sample.json"
            sample = read_json(sample_path)
            sample["raw"]["extra_3"].update({
                "size_bytes": copied_telemetry.stat().st_size,
                "sha256": sha256_bytes(copied_telemetry.read_bytes()),
            })
            write_json(sample_path, sample)
            rebuild_samples_jsonl(root)
            rebuild_ledger(root)
            self.assert_rejected(root, "unrecognized sensor path")

    def test_telemetry_values_are_semantically_bounded(self) -> None:
        telemetry_path = self.baseline / "execution/entry-000/measured/telemetry.json"
        original = read_json(telemetry_path)
        receipt = read_json(self.baseline / "execution/entry-000/execution.json")
        request = receipt["cycles"][1]["request"]
        plan = core.load_plan(self.control["plan"])
        attacks = (
            (
                "loadavg",
                lambda sample: sample.update({"loadavg": "not /proc/loadavg"}),
                "canonical /proc/loadavg",
            ),
            (
                "meminfo",
                lambda sample: sample.update({
                    "meminfo": "MemTotal: 1 kB\nMemAvailable: 2 kB\n"
                }),
                "available memory exceeds total",
            ),
            (
                "gpu busy",
                lambda sample: sample["gpu"].update({
                    "/sys/class/drm/card1/device/gpu_busy_percent": "101"
                }),
                r"outside \[0, 100\]",
            ),
        )
        for name, mutate, pattern in attacks:
            with self.subTest(name=name):
                telemetry = copy.deepcopy(original)
                mutate(next(iter(telemetry.values()))[0]["sample"])
                with self.assertRaisesRegex(evidence.AdapterEvidenceError, pattern):
                    evidence._validate_telemetry(
                        telemetry, plan,
                        request["controller_started_monotonic_ns"],
                        request["controller_ended_monotonic_ns"],
                        {
                            plan["topology"][role]["host"]: receipt["cycles"][1][
                                "identities"][role]["start_monotonic_us"] * 1000
                            for role in ("coordinator", "worker")
                        },
                        {
                            "nimo-1": "11111111-1111-4111-8111-111111111111",
                            "nimo-2": "22222222-2222-4222-8222-222222222222",
                        },
                        {"nimo-1": 10**30, "nimo-2": 10**30},
                        {"nimo-1": 10**30, "nimo-2": 10**30},
                        request["remote_started_monotonic_ns"],
                        request["remote_ended_monotonic_ns"],
                        "telemetry",
                    )

    def test_cross_role_identity_is_rejected(self) -> None:
        with self.case_tree() as root:
            receipt_path = root / "execution/entry-000/execution.json"
            receipt = read_json(receipt_path)
            receipt["cycles"][0]["identities"]["worker"]["role"] = "coordinator"
            write_json(receipt_path, receipt)
            rebuild_ledger(root)
            self.assert_rejected(root, "frozen entry")

    def test_refuse_hmm_decision_is_rejected(self) -> None:
        with self.case_tree() as root:
            hmm_path = root / "hmm-admission-result.raw.json"
            hmm = read_json(hmm_path)
            reason = "HMM_HEADROOM_INSUFFICIENT"
            hmm["decision"] = "REFUSE"
            hmm["reason_codes"] = [reason]
            hmm["roles"]["coordinator"]["classification"] = "REFUSE"
            hmm["roles"]["coordinator"]["reason_codes"] = [reason]
            write_json(hmm_path, hmm)
            rebuild_ledger(root)
            self.assert_rejected(root, "canonical recomputation")

    def test_hmm_snapshot_and_policy_tampering_are_bound(self) -> None:
        for relative in (
            "hmm-admission-snapshot.raw.json",
            "hmm-admission-policy.raw.json",
        ):
            with self.subTest(relative=relative), self.case_tree() as root:
                path = root / relative
                path.write_bytes(path.read_bytes() + b"tamper\n")
                rebuild_ledger(root)
                self.assert_rejected(root, "canonical recomputation")

    def test_coherently_recomputed_hmm_triple_remains_bound_to_authority_receipts(self) -> None:
        with self.case_tree() as root:
            snapshot_path = root / "hmm-admission-snapshot.raw.json"
            policy_path = root / "hmm-admission-policy.raw.json"
            result_path = root / "hmm-admission-result.raw.json"
            snapshot = read_json(snapshot_path)
            snapshot["capture"]["capture_id"] += "-rebound"
            write_json(snapshot_path, snapshot)
            old_result = read_json(result_path)
            rebound = evidence.hmm_admission.evaluate_bytes(
                snapshot_path.read_bytes(), policy_path.read_bytes(),
                trusted_now_utc=old_result["trusted_now_utc"],
            )
            result_path.write_bytes(evidence.hmm_admission.pretty_bytes(rebound))
            self.assertEqual(
                evidence.hmm_admission.validate_bound_admission_result_bytes(
                    result_path.read_bytes(), snapshot_path.read_bytes(),
                    policy_path.read_bytes()),
                rebound,
            )
            rebuild_ledger(root)
            self.assert_rejected(root, "does not bind the exact retained HMM-admission result")

    def test_result_only_hmm_admit_is_non_authorizing(self) -> None:
        result_bytes = self.control["hmm_result"].read_bytes()
        with self.assertRaisesRegex(
                evidence.hmm_admission.AdmissionError,
                "positive admission requires snapshot/policy-bound"):
            evidence.hmm_admission.validate_admission_result_bytes(result_bytes)

    def test_hmm_production_identity_is_bound_to_caller_authority(self) -> None:
        with self.case_tree() as root:
            self.assert_rejected(
                root,
                "differs from the caller-bound maintenance authority",
                expected_production_identity_sha256={
                    "coordinator": "3" * 64,
                    "worker": PRODUCTION_IDENTITIES["worker"],
                },
            )

    def test_required_hmm_validator_refuses_ambient_module(self) -> None:
        ambient = types.SimpleNamespace(
            validate_bound_admission_result_bytes=lambda *_: {})
        module_name = "halofpx_strix_adapter_evidence_ambient_test"
        with mock.patch.dict(
            sys.modules, {"halofpx_strix_hmm_admission": ambient}, clear=False
        ):
            with self.assertRaisesRegex(ImportError, "refusing ambient"):
                load_module(
                    module_name, REPO / "scripts" / "halofpx_strix_adapter_evidence.py"
                )
        sys.modules.pop(module_name, None)

    def test_required_sibling_loader_refuses_ambient_module(self) -> None:
        module_name = "counterfeit_halofpx_strix_ab"
        ambient = types.ModuleType(module_name)
        ambient.__file__ = str(self.root / "counterfeit-halofpx-strix-ab.py")
        with mock.patch.dict(sys.modules, {module_name: ambient}, clear=False):
            with self.assertRaisesRegex(ImportError, "refusing ambient"):
                evidence._load_exact_sibling(module_name, "halofpx_strix_ab.py")

    def test_exact_sibling_loader_never_executes_a_search_path_shadow(self) -> None:
        module_name = "halofpx_strix_shadow_sentinel"
        sys.modules.pop(module_name, None)
        with tempfile.TemporaryDirectory(prefix="shadow-", dir=self.root) as directory:
            shadow_root = Path(directory)
            sentinel = shadow_root / "ambient-executed.txt"
            (shadow_root / f"{module_name}.py").write_text(
                "from pathlib import Path\n"
                f"Path({str(sentinel)!r}).write_text('executed', encoding='utf-8')\n",
                encoding="utf-8",
            )
            with mock.patch.object(sys, "path", [str(shadow_root), *sys.path]):
                loaded = evidence._load_exact_sibling(
                    module_name, "halofpx_strix_ab.py")
            self.assertFalse(sentinel.exists())
            self.assertEqual(
                Path(loaded.__file__).resolve(),
                (REPO / "scripts" / "halofpx_strix_ab.py").resolve(),
            )
        sys.modules.pop(module_name, None)

    def test_unsupported_hmm_sibling_contract_refuses(self) -> None:
        class CounterfeitAdmissionError(RuntimeError):
            pass

        counterfeit = types.SimpleNamespace(
            RESULT_SCHEMA="halofpx.strix-hmm-admission-result.v999",
            AdmissionError=CounterfeitAdmissionError,
            validate_bound_admission_result_bytes=lambda result, snapshot, policy: json.loads(result),
        )
        with self.case_tree() as root, mock.patch.object(
                evidence, "hmm_admission", counterfeit):
            self.assert_rejected(root, "unsupported consumer contract")

    def test_bool_is_not_accepted_as_an_integer(self) -> None:
        with self.case_tree() as root:
            hmm_path = root / "hmm-admission-result.raw.json"
            hmm = read_json(hmm_path)
            hmm["roles"]["coordinator"]["hmm_headroom_bytes"] = True
            write_json(hmm_path, hmm)
            rebuild_ledger(root)
            self.assert_rejected(root, "canonical recomputation|must be an integer")
        with self.case_tree() as root:
            self.assert_rejected(
                root,
                "expected_schedule_index must be an integer",
                expected_schedule_index=True,
            )

    def test_plan_cardinality_is_bounded_before_schedule_expansion(self) -> None:
        for field, value in (("pairs", 65), ("warmups_per_condition", 17)):
            with self.subTest(field=field), self.case_tree() as root:
                plan = read_json(root / "plan.json")
                plan["execution"][field] = value
                write_json(root / "plan.json", plan)
                with self.assertRaisesRegex(
                        evidence.AdapterEvidenceError,
                        "bounded pairs/warmups/retained cardinality"):
                    self.verify(
                        root, expected_plan_bytes=(root / "plan.json").read_bytes())

        with self.case_tree() as root:
            plan = read_json(root / "plan.json")
            plan["execution"].update({"pairs": 64, "warmups_per_condition": 16})
            plan["request"]["output_tokens"] = 256
            write_json(root / "plan.json", plan)
            with self.assertRaisesRegex(
                    evidence.AdapterEvidenceError,
                    "coupled retained cycle/output-token record bound"):
                self.verify(
                    root, expected_plan_bytes=(root / "plan.json").read_bytes())

    def test_exact_inventory_formula_matches_materialized_sets_and_maximum(self) -> None:
        for entries, warmups, sidecar in (
                (2, 1, False), (2, 1, True), (10, 3, False),
                (128, 16, False), (128, 16, True)):
            with self.subTest(entries=entries, warmups=warmups, sidecar=sidecar):
                schedule = {"entries": [
                    {"pair_id": index // 2 + 1, "order_index": index % 2,
                     "condition": "off" if index % 2 == 0 else "on"}
                    for index in range(entries)
                ]}
                plan = {"execution": {"warmups_per_condition": warmups}}
                files, directories = evidence._expected_inventory(
                    schedule, plan, sampling_profile=sidecar)
                expected = 20 + entries * (21 + 6 * warmups)
                if sidecar:
                    expected += 2 + entries * 5
                self.assertEqual(
                    len(files) + len(directories), expected)
                self.assertEqual(
                    evidence._expected_inventory_entry_count(entries, warmups, sidecar),
                    expected)
                self.assertLessEqual(expected, evidence._MAX_FILES)
        self.assertEqual(
            evidence._expected_inventory_entry_count(128, 16, True), 15_638)

    def test_deep_wide_and_huge_integer_json_refuse_with_typed_error(self) -> None:
        attacks = (
            (b"[" * 100_000 + b"0" + b"]" * 100_000, "nesting depth"),
            (b"[" + b",".join(b"0" for _ in range(100_002)) + b"]", "node/token"),
            (b"{" + b'"n":' + b"9" * 129 + b"}", "numeric token"),
            (b'{"n":1e999}', "finite representable decimal"),
            (b'{"n":1e' + b"9" * 129 + b'}', "numeric token"),
        )
        for content, pattern in attacks:
            with self.subTest(pattern=pattern):
                with self.assertRaisesRegex(evidence.AdapterEvidenceError, pattern):
                    evidence._parse_json(content, "adversarial-json")
        with self.case_tree() as root:
            (root / "status.json").write_bytes(attacks[0][0])
            rebuild_ledger(root)
            self.assert_rejected(root, "nesting depth")
        self.assertTrue(math.isfinite(evidence._parse_json(b'{"n":1e308}', "finite")["n"]))


if __name__ == "__main__":
    unittest.main()
