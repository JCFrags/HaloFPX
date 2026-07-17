#!/usr/bin/env python3
"""Auditable communication and memory calculator for the dual-Strix-Halo wiki.

The module contains no benchmark constants. A 40 Gb/s USB4 rate is exposed only
through ``nominal_payload_floor`` and is labeled as a protocol-free lower bound.
All effective bandwidth, latency, compute time, overlap, and acceptance values are
caller-supplied measurements or explicit scenario assumptions.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Iterable


@dataclass(frozen=True)
class ModelConfig:
    key: str
    name: str
    parameter_count: float | None
    active_parameter_count: float | None
    layers: int
    hidden_size: int
    attention_heads: int
    kv_heads: int
    head_dim: int
    vocab_size: int
    context_limit: int | None
    moe_experts: int = 0
    experts_per_token: int = 0
    moe_layers: int = 0
    source: str = ""

    def validate(self) -> None:
        positive = {
            "layers": self.layers,
            "hidden_size": self.hidden_size,
            "attention_heads": self.attention_heads,
            "kv_heads": self.kv_heads,
            "head_dim": self.head_dim,
            "vocab_size": self.vocab_size,
        }
        for field, value in positive.items():
            if value <= 0:
                raise ValueError(f"{field} must be positive")
        if self.hidden_size != self.attention_heads * self.head_dim:
            raise ValueError(
                f"{self.key}: hidden_size must equal attention_heads * head_dim"
            )
        if self.moe_experts < 0 or self.experts_per_token < 0 or self.moe_layers < 0:
            raise ValueError("MoE fields cannot be negative")
        if self.moe_experts and not (0 < self.experts_per_token <= self.moe_experts):
            raise ValueError("experts_per_token must be in [1, moe_experts]")
        if self.moe_layers > self.layers:
            raise ValueError("moe_layers cannot exceed layers")


@dataclass(frozen=True)
class Workload:
    prefill_tokens: int = 4096
    decode_sequences: int = 1
    activation_bytes: float = 2.0
    kv_bytes: float = 2.0
    token_id_bytes: int = 4
    probability_bytes: float = 2.0

    def validate(self) -> None:
        if self.prefill_tokens <= 0 or self.decode_sequences <= 0:
            raise ValueError("token and sequence counts must be positive")
        if min(self.activation_bytes, self.kv_bytes, self.probability_bytes) <= 0:
            raise ValueError("element byte widths must be positive")
        if self.token_id_bytes <= 0:
            raise ValueError("token_id_bytes must be positive")


@dataclass(frozen=True)
class LinkMeasurement:
    """Measured transport inputs.

    effective_bandwidth_Bps is aggregate payload bandwidth in one direction for the
    message path actually used. It may represent one link or validated striping.
    one_way_latency_s is the fitted fixed cost for the relevant message-size range.
    """

    effective_bandwidth_Bps: float
    one_way_latency_s: float
    allreduce_phases_p2: int = 1

    def validate(self) -> None:
        if self.effective_bandwidth_Bps <= 0:
            raise ValueError("effective bandwidth must be positive")
        if self.one_way_latency_s < 0:
            raise ValueError("latency cannot be negative")
        if self.allreduce_phases_p2 <= 0:
            raise ValueError("allreduce phase count must be positive")


MODELS: dict[str, ModelConfig] = {
    "llama31-8b": ModelConfig(
        key="llama31-8b",
        name="Llama 3.1 8B",
        parameter_count=8e9,
        active_parameter_count=8e9,
        layers=32,
        hidden_size=4096,
        attention_heads=32,
        kv_heads=8,
        head_dim=128,
        vocab_size=128256,
        context_limit=131072,
        source="https://github.com/meta-llama/llama-models/blob/main/models/sku_list.py",
    ),
    "llama31-405b": ModelConfig(
        key="llama31-405b",
        name="Llama 3.1 405B",
        parameter_count=405e9,
        active_parameter_count=405e9,
        layers=126,
        hidden_size=16384,
        attention_heads=128,
        kv_heads=8,
        head_dim=128,
        vocab_size=128256,
        context_limit=131072,
        source="https://github.com/meta-llama/llama-models/blob/main/models/sku_list.py",
    ),
    "mixtral-8x7b": ModelConfig(
        key="mixtral-8x7b",
        name="Mixtral 8x7B",
        parameter_count=46.7e9,
        active_parameter_count=12.9e9,
        layers=32,
        hidden_size=4096,
        attention_heads=32,
        kv_heads=8,
        head_dim=128,
        vocab_size=32000,
        context_limit=32768,
        moe_experts=8,
        experts_per_token=2,
        moe_layers=32,
        source="https://huggingface.co/mistralai/Mixtral-8x7B-Instruct-v0.1/blob/main/config.json",
    ),
    "qwen3-30b-a3b": ModelConfig(
        key="qwen3-30b-a3b",
        name="Qwen3-30B-A3B-Instruct-2507",
        parameter_count=30e9,
        active_parameter_count=3e9,
        layers=48,
        hidden_size=2048,
        attention_heads=32,
        kv_heads=4,
        head_dim=64,  # overridden below: Qwen config reports explicit 128 head_dim with QK width != h
        vocab_size=151936,
        context_limit=262144,
        moe_experts=128,
        experts_per_token=8,
        moe_layers=48,
        source="https://huggingface.co/Qwen/Qwen3-30B-A3B-Instruct-2507/blob/main/config.json",
    ),
}
# Qwen3's config exposes head_dim=128 while hidden_size/num_attention_heads=64.
# Attention projection width is therefore not identical to residual hidden width. Replace
# the frozen record without applying the generic equality check for this architecture.
MODELS["qwen3-30b-a3b"] = ModelConfig(
    **{**asdict(MODELS["qwen3-30b-a3b"]), "head_dim": 128}
)


def validate_model(model: ModelConfig) -> None:
    """Validate generic fields while allowing explicit attention head dimensions."""
    positive = (
        model.layers,
        model.hidden_size,
        model.attention_heads,
        model.kv_heads,
        model.head_dim,
        model.vocab_size,
    )
    if any(v <= 0 for v in positive):
        raise ValueError(f"{model.key}: all architecture dimensions must be positive")
    if model.moe_experts and not (0 < model.experts_per_token <= model.moe_experts):
        raise ValueError(f"{model.key}: invalid top-k")
    if not 0 <= model.moe_layers <= model.layers:
        raise ValueError(f"{model.key}: invalid moe_layers")


def human_bytes(value: float) -> str:
    units = ["B", "KiB", "MiB", "GiB", "TiB"]
    v = float(value)
    for unit in units:
        if abs(v) < 1024.0 or unit == units[-1]:
            return f"{v:.3f} {unit}" if abs(v) < 10 else f"{v:.2f} {unit}"
        v /= 1024.0
    raise AssertionError("unreachable")


def ideal_weight_bytes(parameter_count: float, quant_bits: float) -> float:
    """Ideal packed-weight lower bound; excludes all quantization/runtime overhead."""
    if parameter_count < 0 or quant_bits <= 0:
        raise ValueError("parameter_count >= 0 and quant_bits > 0 required")
    return parameter_count * quant_bits / 8.0


def boundary_activation_bytes(model: ModelConfig, tokens: int, activation_bytes: float) -> float:
    return tokens * model.hidden_size * activation_bytes


def kv_bytes_per_token(model: ModelConfig, kv_bytes: float) -> float:
    """Whole-model K+V bytes for one sequence token."""
    return 2.0 * model.layers * model.kv_heads * model.head_dim * kv_bytes


def kv_bytes_for_layers(
    model: ModelConfig, layer_count: int, tokens: int, kv_bytes: float
) -> float:
    if not 0 <= layer_count <= model.layers:
        raise ValueError("layer_count outside model")
    return 2.0 * layer_count * model.kv_heads * model.head_dim * kv_bytes * tokens


def optimal_two_link_split(
    volume_bytes: float,
    bandwidth_1_Bps: float,
    latency_1_s: float,
    bandwidth_2_Bps: float,
    latency_2_s: float,
    reassembly_s: float = 0.0,
) -> dict[str, float | bool]:
    """Minimum first-order completion time for concurrent static striping.

    This is a model over *measured* per-path inputs, not a bonding claim. The
    interior solution equalizes path completion times. If it assigns a negative
    byte count to one path, the model selects the faster single-path solution.
    """
    if volume_bytes < 0 or min(bandwidth_1_Bps, bandwidth_2_Bps) <= 0:
        raise ValueError("volume must be non-negative and bandwidths positive")
    if min(latency_1_s, latency_2_s, reassembly_s) < 0:
        raise ValueError("latencies cannot be negative")
    single_1 = latency_1_s + volume_bytes / bandwidth_1_Bps
    single_2 = latency_2_s + volume_bytes / bandwidth_2_Bps
    t_equal = (
        volume_bytes
        + bandwidth_1_Bps * latency_1_s
        + bandwidth_2_Bps * latency_2_s
    ) / (bandwidth_1_Bps + bandwidth_2_Bps)
    x1 = bandwidth_1_Bps * (t_equal - latency_1_s)
    x2 = bandwidth_2_Bps * (t_equal - latency_2_s)
    both_valid = x1 >= 0 and x2 >= 0
    striped = t_equal + reassembly_s if both_valid else math.inf
    choices = [(single_1, volume_bytes, 0.0, False), (single_2, 0.0, volume_bytes, False)]
    if both_valid:
        choices.append((striped, x1, x2, True))
    best_time, best_x1, best_x2, uses_both = min(choices, key=lambda row: row[0])
    return {
        "path_1_bytes": best_x1,
        "path_2_bytes": best_x2,
        "completion_time_s": best_time,
        "uses_both_paths": uses_both,
        "interior_solution_valid": both_valid,
        "single_path_1_time_s": single_1,
        "single_path_2_time_s": single_2,
        "interior_striped_time_s": striped,
    }


def symmetric_striping_payload_threshold(
    bandwidth_per_path_Bps: float, extra_striping_overhead_s: float
) -> float:
    """Payload threshold V > 2*B*delta for two identical measured paths."""
    if bandwidth_per_path_Bps <= 0 or extra_striping_overhead_s < 0:
        raise ValueError("bandwidth positive and overhead non-negative required")
    return 2.0 * bandwidth_per_path_Bps * extra_striping_overhead_s


def allreduce_time_p2(message_bytes: float, link: LinkMeasurement) -> float:
    """P=2 all-reduce critical path.

    Per-rank sent bytes are one tensor for both direct-exchange and two-phase ring.
    The transport phase count is runtime-dependent and therefore a measured input.
    """
    link.validate()
    return (
        link.allreduce_phases_p2 * link.one_way_latency_s
        + message_bytes / link.effective_bandwidth_Bps
    )


def tensor_parallel(
    model: ModelConfig, tokens: int, activation_bytes: float, link: LinkMeasurement | None = None
) -> dict[str, float | int | None]:
    """Megatron-style TP=2 forward: two activation all-reduces per layer."""
    message = boundary_activation_bytes(model, tokens, activation_bytes)
    collectives = 2 * model.layers
    per_rank_sent = collectives * message
    aggregate_bidirectional_cut = 2 * per_rank_sent
    result: dict[str, float | int | None] = {
        "message_bytes_per_collective": message,
        "collectives": collectives,
        "per_rank_sent_bytes": per_rank_sent,
        "per_rank_received_bytes": per_rank_sent,
        "aggregate_bidirectional_cut_bytes": aggregate_bidirectional_cut,
        "transport_phases": None,
        "communication_time_s": None,
    }
    if link is not None:
        result["transport_phases"] = collectives * link.allreduce_phases_p2
        result["communication_time_s"] = collectives * allreduce_time_p2(message, link)
    return result


def contiguous_layer_split(
    model: ModelConfig,
    tokens: int,
    activation_bytes: float,
    token_id_count: int = 0,
    token_id_bytes: int = 4,
    messages: int = 1,
    link: LinkMeasurement | None = None,
) -> dict[str, float | int | None]:
    activation = boundary_activation_bytes(model, tokens, activation_bytes)
    feedback = token_id_count * token_id_bytes
    result: dict[str, float | int | None] = {
        "activation_bytes_A_to_B": activation,
        "token_feedback_bytes_B_to_A": feedback,
        "activation_messages": messages,
        "feedback_messages": 1 if feedback else 0,
        "communication_time_s": None,
    }
    if link is not None:
        t = messages * link.one_way_latency_s + activation / link.effective_bandwidth_Bps
        if feedback:
            t += link.one_way_latency_s + feedback / link.effective_bandwidth_Bps
        result["communication_time_s"] = t
    return result


def moe_expert_service(
    model: ModelConfig,
    tokens: int,
    activation_bytes: float,
    remote_fraction: float,
    metadata_bytes_per_assignment: float = 16.0,
    link: LinkMeasurement | None = None,
) -> dict[str, float | int | None]:
    if not model.moe_experts or not model.experts_per_token or not model.moe_layers:
        raise ValueError(f"{model.name} is not configured as MoE")
    if not 0.0 <= remote_fraction <= 1.0:
        raise ValueError("remote_fraction must be in [0, 1]")
    remote_assignments_per_layer = tokens * model.experts_per_token * remote_fraction
    per_layer = remote_assignments_per_layer * (
        2.0 * model.hidden_size * activation_bytes + metadata_bytes_per_assignment
    )
    total = model.moe_layers * per_layer
    active_layers = model.moe_layers if remote_assignments_per_layer > 0 else 0
    phases = 2 * active_layers
    result: dict[str, float | int | None] = {
        "remote_assignments_per_layer": remote_assignments_per_layer,
        "bytes_per_moe_layer": per_layer,
        "total_cross_cut_bytes": total,
        "communication_phases": phases,
        "communication_time_s": None,
    }
    if link is not None and phases:
        result["communication_time_s"] = (
            phases * link.one_way_latency_s + total / link.effective_bandwidth_Bps
        )
    return result


def speculative_expected_committed(draft_length: int, acceptance: float) -> float:
    if draft_length < 1:
        raise ValueError("draft_length must be >= 1")
    if not 0.0 <= acceptance <= 1.0:
        raise ValueError("acceptance must be in [0, 1]")
    return sum(acceptance**i for i in range(draft_length + 1))


def remote_speculation_volume(
    draft_length: int,
    token_id_bytes: int,
    vocab_size: int,
    probability_bytes: float,
    q_vectors: float = 0.0,
    response_token_ids: int = 1,
    fixed_metadata_bytes: int = 32,
) -> dict[str, float]:
    """Network volume template.

    q_vectors=0 represents greedy/exact deterministic verification or a stochastic
    protocol that reconstructs q elsewhere. q_vectors=draft_length represents an
    eager full-draft probability-vector protocol. Fractional values support expected
    on-demand protocols and must be labeled as scenario assumptions.
    """
    if draft_length < 1 or q_vectors < 0:
        raise ValueError("invalid speculative parameters")
    request = (
        draft_length * token_id_bytes
        + q_vectors * vocab_size * probability_bytes
        + fixed_metadata_bytes
    )
    response = response_token_ids * token_id_bytes + fixed_metadata_bytes
    return {
        "request_bytes": request,
        "response_bytes": response,
        "roundtrip_bytes": request + response,
    }


def remote_speculation_break_even(
    expected_committed_tokens: float,
    target_single_token_s: float,
    draft_block_s: float,
    verify_block_s: float,
    network_bytes: float,
    one_way_latency_s: float,
) -> dict[str, float | bool | None]:
    """Required payload bandwidth for synchronous remote speculation.

    T_round = T_draft + T_verify + 2*ell + V/B.
    Break-even requires T_round < E[K] * T_target_one_token.
    """
    baseline_budget = expected_committed_tokens * target_single_token_s
    budget_before_network = baseline_budget - draft_block_s - verify_block_s
    payload_budget = budget_before_network - 2.0 * one_way_latency_s
    feasible = payload_budget > 0
    required_bandwidth = network_bytes / payload_budget if feasible else None
    return {
        "baseline_time_budget_s": baseline_budget,
        "network_roundtrip_budget_s": budget_before_network,
        "payload_budget_after_latency_s": payload_budget,
        "required_bandwidth_Bps": required_bandwidth,
        "latency_compute_gate_pass": feasible,
    }


def tensor_parallel_break_even(
    model: ModelConfig,
    tokens: int,
    activation_bytes: float,
    single_node_compute_s: float,
    tp_compute_efficiency: float,
    one_way_latency_s: float,
    allreduce_phases_p2: int,
) -> dict[str, float | bool | None]:
    """Required effective payload bandwidth to beat measured single-node compute.

    TP compute is modeled as C1/(2*eta). This is a decomposition variable, not a
    predicted efficiency.
    """
    if single_node_compute_s <= 0:
        raise ValueError("single_node_compute_s must be positive")
    if not 0 < tp_compute_efficiency <= 1:
        raise ValueError("tp_compute_efficiency must be in (0,1]")
    message = boundary_activation_bytes(model, tokens, activation_bytes)
    collectives = 2 * model.layers
    compute_saving = single_node_compute_s * (
        1.0 - 1.0 / (2.0 * tp_compute_efficiency)
    )
    latency_cost = collectives * allreduce_phases_p2 * one_way_latency_s
    payload_budget = compute_saving - latency_cost
    required_bandwidth = collectives * message / payload_budget if payload_budget > 0 else None
    return {
        "compute_saving_before_communication_s": compute_saving,
        "collective_latency_cost_s": latency_cost,
        "payload_budget_s": payload_budget,
        "required_bandwidth_Bps": required_bandwidth,
        "latency_compute_gate_pass": payload_budget > 0,
    }


def layer_split_break_even(
    boundary_bytes: float,
    single_node_time_s: float,
    stage_a_time_s: float,
    stage_b_time_s: float,
    one_way_latency_s: float,
    feedback_bytes: float = 0.0,
    feedback_messages: int = 0,
) -> dict[str, float | bool | None]:
    """Required payload bandwidth for a serial contiguous split to beat baseline."""
    if boundary_bytes < 0 or feedback_bytes < 0:
        raise ValueError("byte volumes cannot be negative")
    if min(single_node_time_s, stage_a_time_s, stage_b_time_s) < 0:
        raise ValueError("times cannot be negative")
    if one_way_latency_s < 0 or feedback_messages < 0:
        raise ValueError("latency/message count cannot be negative")
    latency_messages = 1 + feedback_messages
    compute_budget = single_node_time_s - stage_a_time_s - stage_b_time_s
    payload_budget = compute_budget - latency_messages * one_way_latency_s
    total_bytes = boundary_bytes + feedback_bytes
    return {
        "compute_and_network_budget_s": compute_budget,
        "payload_budget_after_latency_s": payload_budget,
        "required_bandwidth_Bps": total_bytes / payload_budget if payload_budget > 0 else None,
        "latency_compute_gate_pass": payload_budget > 0,
    }


def moe_expert_break_even(
    cross_cut_bytes: float,
    measured_saved_local_critical_path_s: float,
    measured_remote_compute_and_imbalance_s: float,
    one_way_latency_s: float,
    communication_phases: int = 2,
) -> dict[str, float | bool | None]:
    """Required bandwidth for remote experts to improve a measured critical path."""
    if cross_cut_bytes < 0 or communication_phases < 0:
        raise ValueError("bytes/phases cannot be negative")
    if min(measured_saved_local_critical_path_s, measured_remote_compute_and_imbalance_s, one_way_latency_s) < 0:
        raise ValueError("times cannot be negative")
    payload_budget = (
        measured_saved_local_critical_path_s
        - measured_remote_compute_and_imbalance_s
        - communication_phases * one_way_latency_s
    )
    return {
        "payload_budget_s": payload_budget,
        "required_bandwidth_Bps": cross_cut_bytes / payload_budget if payload_budget > 0 else None,
        "latency_compute_gate_pass": payload_budget > 0,
    }


def replicated_decode_model_path(
    independent_sessions_rank_0: int, independent_sessions_rank_1: int
) -> dict[str, int | bool]:
    """Ownership-level result: replicas have zero cross-node model-path bytes."""
    if min(independent_sessions_rank_0, independent_sessions_rank_1) < 0:
        raise ValueError("session counts cannot be negative")
    return {
        "cross_node_model_path_bytes": 0,
        "cross_node_model_path_synchronizations": 0,
        "both_replicas_have_runnable_work": independent_sessions_rank_0 > 0 and independent_sessions_rank_1 > 0,
    }


def pipeline_makespan(
    microbatches: int,
    stage_a_s: float,
    transfer_s: float,
    stage_b_s: float,
    communication_overlaps_compute: bool = True,
) -> dict[str, float | bool | int | None]:
    if microbatches < 1:
        raise ValueError("microbatches must be >= 1")
    if min(stage_a_s, transfer_s, stage_b_s) < 0:
        raise ValueError("stage times cannot be negative")
    if communication_overlaps_compute:
        service = max(stage_a_s, transfer_s, stage_b_s)
    else:
        service = max(stage_a_s, stage_b_s) + transfer_s
    fill = stage_a_s + transfer_s + stage_b_s
    makespan = fill + (microbatches - 1) * service
    serial = microbatches * (stage_a_s + stage_b_s)
    denom = stage_a_s + stage_b_s - service
    if denom <= 0:
        minimum = None
    else:
        threshold = (fill - service) / denom
        minimum = math.floor(threshold) + 1
        minimum = max(1, minimum)
    return {
        "service_interval_s": service,
        "pipeline_makespan_s": makespan,
        "serial_compute_baseline_s": serial,
        "beats_serial_compute": makespan < serial,
        "minimum_microbatches_for_strict_break_even": minimum,
    }


def kv_migration_break_even(
    model: ModelConfig,
    context_tokens: int,
    kv_bytes: float,
    one_way_latency_s: float,
    effective_bandwidth_Bps: float,
    future_decode_tokens: int,
    measured_saving_per_future_token_s: float,
) -> dict[str, float | bool]:
    volume = kv_bytes_per_token(model, kv_bytes) * context_tokens
    transfer = one_way_latency_s + volume / effective_bandwidth_Bps
    available_saving = future_decode_tokens * measured_saving_per_future_token_s
    return {
        "kv_transfer_bytes": volume,
        "kv_transfer_time_s": transfer,
        "future_measured_saving_s": available_saving,
        "break_even": available_saving > transfer,
    }


def nominal_payload_floor(volume_bytes: float, links: int = 1, link_rate_gbps: float = 40.0) -> float:
    """Protocol-free payload time at nominal decimal line rate; not a forecast."""
    if links <= 0 or link_rate_gbps <= 0:
        raise ValueError("links and rate must be positive")
    return volume_bytes / (links * link_rate_gbps * 1e9 / 8.0)


def model_summary(model: ModelConfig, workload: Workload) -> dict[str, Any]:
    validate_model(model)
    workload.validate()
    pf_tp = tensor_parallel(model, workload.prefill_tokens, workload.activation_bytes)
    dec_tp = tensor_parallel(model, workload.decode_sequences, workload.activation_bytes)
    pf_cut = contiguous_layer_split(
        model, workload.prefill_tokens, workload.activation_bytes
    )
    dec_cut = contiguous_layer_split(
        model,
        workload.decode_sequences,
        workload.activation_bytes,
        token_id_count=workload.decode_sequences,
        token_id_bytes=workload.token_id_bytes,
    )
    result: dict[str, Any] = {
        "model": asdict(model),
        "workload": asdict(workload),
        "calculated": {
            "boundary_bytes_per_token": boundary_activation_bytes(
                model, 1, workload.activation_bytes
            ),
            "kv_bytes_per_token": kv_bytes_per_token(model, workload.kv_bytes),
            "prefill_tensor_parallel": pf_tp,
            "decode_tensor_parallel": dec_tp,
            "prefill_contiguous_split": pf_cut,
            "decode_contiguous_split": dec_cut,
        },
    }
    if model.moe_experts:
        result["calculated"]["moe_expert_service_rho_0_5_scenario"] = moe_expert_service(
            model,
            workload.prefill_tokens,
            workload.activation_bytes,
            remote_fraction=0.5,
        )
    if model.parameter_count is not None:
        result["calculated"]["ideal_q4_weight_lower_bound_bytes"] = ideal_weight_bytes(
            model.parameter_count, 4
        )
    return result


def demo_payload() -> dict[str, Any]:
    workload = Workload()
    return {
        "metadata": {
            "evidence": "CALCULATED",
            "warning": (
                "Nominal line-rate floors exclude all overhead and latency. "
                "rho=0.5 is an illustrative scenario assumption."
            ),
        },
        "models": {key: model_summary(model, workload) for key, model in MODELS.items()},
    }


def flatten_demo_rows(payload: dict[str, Any]) -> Iterable[dict[str, Any]]:
    for key, entry in payload["models"].items():
        calc = entry["calculated"]
        model = entry["model"]
        metrics: list[tuple[str, float, str, str]] = [
            ("boundary_bytes_per_token", calc["boundary_bytes_per_token"], "B", "CALCULATED"),
            ("kv_bytes_per_token", calc["kv_bytes_per_token"], "B", "CALCULATED"),
            (
                "tp_prefill_per_rank_sent_4096",
                calc["prefill_tensor_parallel"]["per_rank_sent_bytes"],
                "B",
                "CALCULATED",
            ),
            (
                "tp_decode_per_rank_sent_Q1",
                calc["decode_tensor_parallel"]["per_rank_sent_bytes"],
                "B/token-step",
                "CALCULATED",
            ),
            (
                "layer_cut_prefill_A_to_B_4096",
                calc["prefill_contiguous_split"]["activation_bytes_A_to_B"],
                "B",
                "CALCULATED",
            ),
            (
                "layer_cut_decode_A_to_B_Q1",
                calc["decode_contiguous_split"]["activation_bytes_A_to_B"],
                "B/token-step",
                "CALCULATED",
            ),
        ]
        if "ideal_q4_weight_lower_bound_bytes" in calc:
            metrics.append(
                (
                    "ideal_q4_weight_lower_bound",
                    calc["ideal_q4_weight_lower_bound_bytes"],
                    "B",
                    "CALCULATED LOWER BOUND",
                )
            )
        if "moe_expert_service_rho_0_5_scenario" in calc:
            metrics.append(
                (
                    "expert_service_prefill_4096_rho_0_5",
                    calc["moe_expert_service_rho_0_5_scenario"]["total_cross_cut_bytes"],
                    "B",
                    "SCENARIO ASSUMPTION + CALCULATED",
                )
            )
        for metric, value, unit, evidence in metrics:
            yield {
                "model_key": key,
                "model": model["name"],
                "metric": metric,
                "value": value,
                "unit": unit,
                "human_binary": human_bytes(value) if unit.startswith("B") else "",
                "one_link_nominal_floor_ms": nominal_payload_floor(value, 1) * 1e3,
                "two_link_nominal_floor_ms": nominal_payload_floor(value, 2) * 1e3,
                "evidence": evidence,
                "notes": (
                    "Payload-only floor at nominal 40 Gb/s/link; excludes all overhead and latency."
                ),
            }


def write_demo(output: Path, csv_output: Path | None = None) -> None:
    payload = demo_payload()
    output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    if csv_output:
        rows = list(flatten_demo_rows(payload))
        with csv_output.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list-models", action="store_true")
    parser.add_argument("--model", choices=sorted(MODELS))
    parser.add_argument("--prefill-tokens", type=int, default=4096)
    parser.add_argument("--decode-sequences", type=int, default=1)
    parser.add_argument("--activation-bytes", type=float, default=2.0)
    parser.add_argument("--kv-bytes", type=float, default=2.0)
    parser.add_argument("--demo", action="store_true", help="emit all worked calculations")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--csv-output", type=Path)
    return parser


def main() -> int:
    args = _parser().parse_args()
    if args.list_models:
        for key, model in MODELS.items():
            print(f"{key}\t{model.name}")
        return 0
    if args.demo:
        payload = demo_payload()
    else:
        if not args.model:
            raise SystemExit("--model is required unless --demo or --list-models is used")
        payload = model_summary(
            MODELS[args.model],
            Workload(
                prefill_tokens=args.prefill_tokens,
                decode_sequences=args.decode_sequences,
                activation_bytes=args.activation_bytes,
                kv_bytes=args.kv_bytes,
            ),
        )
    text = json.dumps(payload, indent=2) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    else:
        print(text, end="")
    if args.csv_output:
        rows = list(flatten_demo_rows(demo_payload()))
        with args.csv_output.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
            writer.writeheader()
            writer.writerows(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
