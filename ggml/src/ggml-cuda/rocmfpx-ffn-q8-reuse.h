#pragma once

#include "ggml.h"

#include <cstdint>

// This contract is deliberately independent of HIP headers so the selector can
// be exercised in the host-only CI lane.  The runtime supplies the facts after
// checking the concrete tensors and backend buffers.
struct halofpx_rocmfpx_ffn_q8_reuse_contract {
    ggml_type weight_type_a;
    ggml_type weight_type_b;
    int64_t   activation_columns;
    bool      ordinary_dense;
    bool      no_bias_glu_pair;
    bool      same_activation;
    bool      f32_activation;
    bool      f32_outputs;
    bool      same_weight_layout;
    bool      local_non_split;
    bool      safe_allocation_views;
    bool      both_mmq_eligible;
};

static constexpr bool halofpx_rocmfpx_ffn_q8_reuse_type(const ggml_type type) {
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

static constexpr bool halofpx_rocmfpx_ffn_q8_reuse_build_enabled() {
#if defined(GGML_HIP_ROCMFPX_FFN_Q8_REUSE)
    return true;
#else
    return false;
#endif
}

static constexpr bool halofpx_rocmfpx_ffn_q8_reuse_eligible(
        const halofpx_rocmfpx_ffn_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_ffn_q8_reuse_type(contract.weight_type_a) &&
           contract.weight_type_a == contract.weight_type_b &&
           contract.activation_columns > 8 &&
           contract.ordinary_dense &&
           contract.no_bias_glu_pair &&
           contract.same_activation &&
           contract.f32_activation &&
           contract.f32_outputs &&
           contract.same_weight_layout &&
           contract.local_non_split &&
           contract.safe_allocation_views &&
           contract.both_mmq_eligible;
}

static constexpr bool halofpx_rocmfpx_ffn_q8_reuse_dispatch(
        const halofpx_rocmfpx_ffn_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_ffn_q8_reuse_build_enabled() &&
           halofpx_rocmfpx_ffn_q8_reuse_eligible(contract);
}
