#!/usr/bin/env python3
"""Evaluate a release summary against machine-readable gates.

The evaluator is deliberately evidence-first. Synthetic, stale, missing, unmatched,
or unverifiable evidence returns INSUFFICIENT_EVIDENCE even when invented metrics
appear favorable.
"""
from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

import yaml
from jsonschema import Draft202012Validator

LEVEL_RANK = {"D0": 0, "S0": 1, "M1": 2, "M2": 3, "R1": 4, "STABLE": 5}


def add(items: list[dict], gate_id: str, reason: str, evidence: str | None = None) -> None:
    rec = {"gate_id": gate_id, "reason": reason}
    if evidence:
        rec["evidence"] = evidence
    items.append(rec)


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def evaluate(summary: dict, gates: dict, stage_override: str | None = None) -> dict:
    stage = stage_override or summary["stage"]
    if stage not in gates["stages"]:
        raise ValueError(f"Unknown stage: {stage}")
    spec = gates["stages"][stage]
    global_gate = gates["global_stable"]
    profile = summary["release_profile"]
    profile_gate = gates["profiles"][profile]

    insufficient: list[dict] = []
    failures: list[dict] = []
    warnings: list[dict] = []
    passes: list[dict] = []

    # Evidence origin/level is checked first and cannot be waived.
    if stage in {"G1", "G2", "G3", "G4"} and summary.get("evidence_origin") != "machine":
        add(insufficient, "EVIDENCE_ORIGIN", "Machine evidence is required; synthetic/document evidence cannot satisfy this stage.")
    min_level = spec.get("minimum_evidence_level", "D0")
    actual_level = summary.get("evidence_level")
    if LEVEL_RANK.get(actual_level, -1) < LEVEL_RANK.get(min_level, 999):
        add(insufficient, "EVIDENCE_LEVEL", f"{stage} requires {min_level}; summary provides {actual_level}.")

    completed = set(summary["experiments"].get("completed", []))
    invalid = set(summary["experiments"].get("invalid", []))
    declared_missing = set(summary["experiments"].get("missing", []))
    required = set(spec.get("required_experiments", []))
    missing = sorted((required - completed) | (required & invalid) | (required & declared_missing))
    if missing:
        add(insufficient, "REQUIRED_EXPERIMENTS", "Missing or invalid required experiments: " + ", ".join(missing))
    else:
        add(passes, "REQUIRED_EXPERIMENTS", f"All {len(required)} required experiment IDs are complete.")

    prov = summary["provenance"]
    if prov.get("completeness", 0) < global_gate["provenance_completeness_min"]:
        add(insufficient, "PROVENANCE_COMPLETENESS", f"Provenance completeness {prov.get('completeness')} is below 1.0.")
    if not prov.get("raw_hashes_verified"):
        add(insufficient, "RAW_HASHES", "Raw evidence hashes are not verified.")

    if stage in {"G2", "G3", "G4"}:
        baselines = summary["baselines"]
        present = set(baselines.get("topologies_present", []))
        needed = set(global_gate["matched_topologies_required"])
        if not baselines.get("matched") or not needed.issubset(present):
            add(insufficient, "MATCHED_BASELINES", "Matched Node A, Node B, and dual evidence is required.")
        if stage == "G4" and baselines.get("independent_reproduction_blocks", 0) < global_gate["independent_reproduction_blocks_min"]:
            add(insufficient, "REPRODUCTION", "At least two independent reproduction blocks are required for stable.")

    slos = summary["absolute_slos"]
    if stage in {"G3", "G4"} and (not slos.get("configured") or not slos.get("approved")):
        add(insufficient, "ABSOLUTE_SLO_CONFIGURATION", "Absolute SLOs are not populated and approved.")
    elif stage in {"G2", "G3", "G4"} and not slos.get("met"):
        add(failures, "ABSOLUTE_SLOS", "One or more approved absolute SLOs failed: " + ", ".join(slos.get("failures", [])))

    # Hard events.
    hard_map = {
        "unexpected_crashes": "unexpected_crashes_max",
        "unexpected_hangs": "unexpected_hangs_max",
        "kernel_oops": "kernel_oops_max",
        "gpu_resets": "gpu_resets_max",
        "silent_corruption": "silent_corruption_max",
        "thermal_throttle_events": "thermal_throttle_events_max",
        "usb4_renegotiations": "normal_run_usb4_renegotiations_max",
    }
    for metric, limit_key in hard_map.items():
        value = summary["hard_events"].get(metric)
        if value is None:
            add(insufficient, f"HARD_EVENT_{metric.upper()}", f"Hard-event counter {metric} is missing.")
        elif value > global_gate[limit_key]:
            add(failures, f"HARD_EVENT_{metric.upper()}", f"{metric}={value}, allowed={global_gate[limit_key]}.")

    quality = summary["quality"]
    if quality.get("request_success_rate", -1) < global_gate["request_success_rate_min"]:
        add(failures, "REQUEST_SUCCESS", f"Request success {quality.get('request_success_rate')} is below {global_gate['request_success_rate_min']}.")
    if quality.get("critical_correctness_rate", -1) < global_gate["critical_correctness_rate_min"]:
        add(failures, "CRITICAL_CORRECTNESS", "Critical correctness must be 100%.")

    # Soak/fault/freshness are stage-dependent.
    if stage == "G4":
        soak = summary["soak"]
        if soak.get("hours", 0) < global_gate["soak_hours_min"] or not soak.get("completed"):
            add(insufficient, "SOAK_DURATION", f"Stable requires {global_gate['soak_hours_min']} measured hours.")
        if soak.get("max_evidence_gap_s", float("inf")) > global_gate["soak_evidence_gap_s_max"]:
            add(insufficient, "SOAK_EVIDENCE_GAP", f"Evidence gap exceeds {global_gate['soak_evidence_gap_s_max']} seconds.")

    if stage in {"G3", "G4"}:
        faults = summary["faults"]
        required_fault_ids=set(global_gate.get("mandatory_fault_scenario_ids", []))
        completed_fault_ids=set(faults.get("completed_scenario_ids", []))
        failed_fault_ids=set(faults.get("failed_scenario_ids", []))
        missing_fault_ids=sorted(required_fault_ids-completed_fault_ids)
        if missing_fault_ids:
            add(insufficient, "FAULT_SCENARIOS", "Missing mandatory fault scenarios: " + ", ".join(missing_fault_ids))
        if failed_fault_ids:
            add(failures, "FAULT_SCENARIO_FAILURES", "Failed mandatory fault scenarios: " + ", ".join(sorted(failed_fault_ids)))
        if faults.get("repeats_min", 0) < global_gate["mandatory_fault_repeats_min"]:
            add(insufficient, "FAULT_REPEATS", "Mandatory fault scenarios need at least three repeats.")
        if faults.get("pass_fraction", 0) < global_gate["mandatory_fault_success_fraction_min"]:
            add(failures, "FAULT_PASS_FRACTION", "Every mandatory fault repetition must pass.")
        if faults.get("lost_acknowledged_requests", 0) > 0:
            add(failures, "LOST_ACKNOWLEDGED_REQUESTS", "Acknowledged requests were lost during fault testing.")

    if stage == "G4":
        upstream = summary["upstream"]
        if not upstream.get("sources_fresh"):
            add(insufficient, "UPSTREAM_FRESHNESS", "One or more release-blocking upstream sources are stale.")
        if upstream.get("untriaged_p0_p1_blockers", 0) > global_gate["untriaged_p0_p1_blockers_max"]:
            add(insufficient, "UPSTREAM_BLOCKERS", "Untriaged P0/P1 upstream blockers remain.")

    security = summary.get("security") or {}
    if stage in {"G2", "G3", "G4"} and not security.get("rpc_network_isolated", False):
        add(insufficient, "RPC_ISOLATION", "RPC isolation evidence is missing or false.")

    # Profile-specific objective gates.
    metrics = summary.get("metrics", {})
    if stage in {"G3", "G4"} and profile == "scale_out":
        required_metrics = {
            "dual_goodput_over_best_single": (">=", profile_gate["dual_goodput_over_best_single_min"]),
            "dual_p95_ttft_over_best_single": ("<=", profile_gate["dual_p95_ttft_over_best_single_max"]),
            "dual_p95_itl_over_best_single": ("<=", profile_gate["dual_p95_itl_over_best_single_max"]),
            "dual_p99_ttft_over_best_single": ("<=", profile_gate["dual_p99_ttft_over_best_single_max"]),
            "dual_p99_itl_over_best_single": ("<=", profile_gate["dual_p99_itl_over_best_single_max"]),
        }
        for name, (op, threshold) in required_metrics.items():
            value = metrics.get(name)
            if value is None:
                add(insufficient, f"PROFILE_{name.upper()}", f"Scale-out metric {name} is missing.")
            elif (op == ">=" and value < threshold) or (op == "<=" and value > threshold):
                add(failures, f"PROFILE_{name.upper()}", f"{name}={value} fails {op} {threshold}.")
    elif stage in {"G3", "G4"} and profile == "capacity_extension":
        for key in ["large_workload_single_node_infeasible", "large_workload_absolute_slos_met", "matched_smaller_control_present"]:
            if key not in metrics:
                add(insufficient, f"PROFILE_{key.upper()}", f"Capacity-extension evidence {key} is missing.")
            elif not bool(metrics[key]):
                add(failures, f"PROFILE_{key.upper()}", f"Capacity-extension requirement {key} is false.")
        headroom = metrics.get("minimum_memory_headroom_fraction")
        if headroom is None:
            add(insufficient, "PROFILE_MEMORY_HEADROOM", "Capacity-extension memory headroom is missing.")
        elif headroom < profile_gate["minimum_memory_headroom_fraction"]:
            add(failures, "PROFILE_MEMORY_HEADROOM", f"Memory headroom {headroom} is below {profile_gate['minimum_memory_headroom_fraction']}.")
    elif stage in {"G3", "G4"} and profile == "availability_recovery":
        for key in ["fault_detection_slo_met", "service_recovery_slo_met", "post_recovery_correctness_met"]:
            if key not in metrics:
                add(insufficient, f"PROFILE_{key.upper()}", f"Recovery evidence {key} is missing.")
            elif not bool(metrics[key]):
                add(failures, f"PROFILE_{key.upper()}", f"Recovery requirement {key} is false.")

    for reg in summary.get("regressions", []):
        status = reg.get("status")
        if status == "FAIL":
            add(failures, "REGRESSION_" + reg.get("metric", "UNKNOWN").upper(), f"Confirmed regression: {reg}")
        elif status in {"WARN_RETEST", "INSUFFICIENT_EVIDENCE"}:
            add(insufficient, "REGRESSION_" + reg.get("metric", "UNKNOWN").upper(), f"Unresolved regression evidence: {reg}")
        elif status == "WARN":
            add(warnings, "REGRESSION_" + reg.get("metric", "UNKNOWN").upper(), f"Warning: {reg}")

    if insufficient:
        decision = "INSUFFICIENT_EVIDENCE"
        authorized = None
    elif failures:
        decision = "FAIL"
        authorized = None
    else:
        decision = "PASS"
        authorized = spec.get("authorized_outcome")

    return {
        "schema_version": 1,
        "evaluation_type": "unsigned_gate_evaluation",
        "evaluated_at": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "candidate_id": summary.get("candidate_id"),
        "profile": profile,
        "stage": stage,
        "decision": decision,
        "authorized_outcome": authorized,
        "machine_validation_claimed": decision == "PASS" and stage in {"G2", "G3", "G4"},
        "insufficient_evidence": insufficient,
        "failures": failures,
        "warnings": warnings,
        "passes": passes,
        "note": "This unsigned evaluation does not replace required human sign-off or an immutable release decision.",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("summary", type=Path)
    parser.add_argument("--gates", type=Path, default=Path(__file__).resolve().parents[1] / "config/release-gates.yaml")
    parser.add_argument("--schema", type=Path, default=Path(__file__).resolve().parents[1] / "schemas/summary.schema.json")
    parser.add_argument("--stage", choices=["G0", "G1", "G2", "G3", "G4"])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    summary = load_json(args.summary)
    schema = load_json(args.schema)
    errors = sorted(Draft202012Validator(schema).iter_errors(summary), key=lambda e: list(e.path))
    if errors:
        for err in errors:
            print(f"SCHEMA ERROR {err.json_path}: {err.message}", file=sys.stderr)
        return 2
    gates = yaml.safe_load(args.gates.read_text(encoding="utf-8"))
    result = evaluate(summary, gates, args.stage)
    rendered = json.dumps(result, indent=2)
    print(rendered)
    if args.output:
        args.output.write_text(rendered + "\n", encoding="utf-8")
    return 0 if result["decision"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
