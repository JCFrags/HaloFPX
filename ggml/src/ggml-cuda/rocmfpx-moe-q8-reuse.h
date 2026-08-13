#pragma once

#include "ggml.h"

#include <cstdint>

// Host-visible admission contract for the default-off routed-MoE gate/up
// experiment. The runtime supplies these facts after inspecting the concrete
// HIP tensors and selected kernel paths; no HIP headers are needed here.
struct halofpx_rocmfpx_moe_q8_reuse_contract {
    ggml_type weight_type_a;
    ggml_type weight_type_b;
    int64_t   activation_tokens;
    int64_t   expert_count;
    int64_t   experts_used;
    bool      gfx1151_hip;
    bool      routed_moe_pair;
    bool      no_bias_glu_pair;
    bool      exact_shared_activation;
    bool      f32_activation;
    bool      f32_outputs;
    bool      same_weight_layout;
    bool      same_output_layout;
    bool      exact_shared_ids;
    bool      valid_routing_layout;
    bool      valid_index_geometry;
    bool      local_non_split;
    bool      safe_allocation_views;
    bool      nonoverlapping_outputs;
    bool      both_mmq_paths;
    bool      single_stream;
};

struct halofpx_rocmfpx_moe_q8_reuse_metrics_v1 {
    uint32_t struct_size;
    uint32_t version;
    uint64_t pair_dispatches;
    uint64_t ids_helper_submissions;
    uint64_t q8_conversions_submitted;
    uint64_t mmq_submissions;
};

static constexpr uint32_t HALOFPX_ROCMFPX_MOE_Q8_REUSE_METRICS_VERSION = 1;

static constexpr bool halofpx_rocmfpx_moe_q8_reuse_type(const ggml_type type) {
    switch (type) {
        case GGML_TYPE_Q2_0_ROCMFPX:
        case GGML_TYPE_Q3_0_ROCMFPX:
        case GGML_TYPE_Q6_0_ROCMFPX:
        case GGML_TYPE_Q8_0_ROCMFPX:
            return true;
        default:
            return false;
    }
}

static constexpr bool halofpx_rocmfpx_moe_q8_reuse_build_enabled() {
#if defined(GGML_HIP_ROCMFPX_MOE_Q8_REUSE)
    return true;
#else
    return false;
#endif
}

static constexpr bool halofpx_rocmfpx_moe_q8_reuse_eligible(
        const halofpx_rocmfpx_moe_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_moe_q8_reuse_type(contract.weight_type_a) &&
           contract.weight_type_a == contract.weight_type_b &&
           contract.activation_tokens > 0 &&
           contract.expert_count > 0 &&
           contract.experts_used > 0 &&
           contract.experts_used <= contract.expert_count &&
           contract.expert_count <= INT32_MAX &&
           contract.experts_used < (1 << 10) &&
           contract.activation_tokens < (1 << 22) &&
           contract.activation_tokens <= INT32_MAX / contract.experts_used &&
           contract.gfx1151_hip &&
           contract.routed_moe_pair &&
           contract.no_bias_glu_pair &&
           contract.exact_shared_activation &&
           contract.f32_activation &&
           contract.f32_outputs &&
           contract.same_weight_layout &&
           contract.same_output_layout &&
           contract.exact_shared_ids &&
           contract.valid_routing_layout &&
           contract.valid_index_geometry &&
           contract.local_non_split &&
           contract.safe_allocation_views &&
           contract.nonoverlapping_outputs &&
           contract.both_mmq_paths &&
           contract.single_stream;
}

static constexpr bool halofpx_rocmfpx_moe_q8_reuse_dispatch(
        const halofpx_rocmfpx_moe_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_moe_q8_reuse_build_enabled() &&
           halofpx_rocmfpx_moe_q8_reuse_eligible(contract);
}
