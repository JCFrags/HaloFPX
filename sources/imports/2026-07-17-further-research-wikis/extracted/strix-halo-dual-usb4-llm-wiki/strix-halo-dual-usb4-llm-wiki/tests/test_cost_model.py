from __future__ import annotations

import importlib.util
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("cost_model", ROOT / "tools" / "cost_model.py")
assert SPEC and SPEC.loader
cm = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = cm
SPEC.loader.exec_module(cm)


def test_llama31_8b_boundary_and_kv() -> None:
    model = cm.MODELS["llama31-8b"]
    assert cm.boundary_activation_bytes(model, 1, 2) == 8192
    assert cm.kv_bytes_per_token(model, 2) == 131072
    assert cm.kv_bytes_for_layers(model, 16, 131072, 2) == 8 * 1024**3


def test_tensor_parallel_counts_and_volume() -> None:
    model = cm.MODELS["llama31-8b"]
    result = cm.tensor_parallel(model, 4096, 2)
    assert result["collectives"] == 64
    assert result["message_bytes_per_collective"] == 32 * 1024**2
    assert result["per_rank_sent_bytes"] == 2 * 1024**3
    assert result["aggregate_bidirectional_cut_bytes"] == 4 * 1024**3


def test_contiguous_split_single_boundary() -> None:
    model = cm.MODELS["qwen3-30b-a3b"]
    result = cm.contiguous_layer_split(model, 4096, 2)
    assert result["activation_bytes_A_to_B"] == 16 * 1024**2


def test_qwen_explicit_head_dim_kv() -> None:
    model = cm.MODELS["qwen3-30b-a3b"]
    assert model.head_dim == 128
    assert cm.kv_bytes_per_token(model, 2) == 96 * 1024


def test_moe_volume_rho_scenario() -> None:
    model = cm.MODELS["mixtral-8x7b"]
    result = cm.moe_expert_service(model, 1, 2, 0.5, metadata_bytes_per_assignment=0)
    assert result["remote_assignments_per_layer"] == 1
    assert result["bytes_per_moe_layer"] == 2 * 4096 * 2
    assert result["total_cross_cut_bytes"] == 512 * 1024
    assert result["communication_phases"] == 64


def test_speculative_expected_committed_edges() -> None:
    assert cm.speculative_expected_committed(4, 0) == 1
    assert cm.speculative_expected_committed(4, 1) == 5
    assert math.isclose(cm.speculative_expected_committed(2, 0.5), 1.75)


def test_tp_break_even_fails_when_efficiency_at_most_half() -> None:
    model = cm.MODELS["llama31-8b"]
    result = cm.tensor_parallel_break_even(model, 1, 2, 0.1, 0.5, 0.0, 1)
    assert result["latency_compute_gate_pass"] is False
    assert result["required_bandwidth_Bps"] is None


def test_pipeline_integer_threshold() -> None:
    result = cm.pipeline_makespan(4, 1.0, 0.1, 1.0, True)
    assert result["service_interval_s"] == 1.0
    assert result["minimum_microbatches_for_strict_break_even"] == 2
    assert result["beats_serial_compute"] is True


def test_two_link_optimal_split_symmetric() -> None:
    result = cm.optimal_two_link_split(100.0, 10.0, 1.0, 10.0, 1.0)
    assert result["uses_both_paths"] is True
    assert math.isclose(result["path_1_bytes"], 50.0)
    assert math.isclose(result["path_2_bytes"], 50.0)
    assert math.isclose(result["completion_time_s"], 6.0)


def test_two_link_split_can_choose_one_path() -> None:
    result = cm.optimal_two_link_split(1.0, 10.0, 0.01, 10.0, 10.0)
    assert result["uses_both_paths"] is False
    assert result["path_1_bytes"] == 1.0


def test_symmetric_striping_threshold() -> None:
    assert cm.symmetric_striping_payload_threshold(5e9, 100e-6) == 1e6


def test_replica_model_path_zero() -> None:
    result = cm.replicated_decode_model_path(2, 3)
    assert result["cross_node_model_path_bytes"] == 0
    assert result["both_replicas_have_runnable_work"] is True
