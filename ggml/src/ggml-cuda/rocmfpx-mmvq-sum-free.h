#pragma once

#include "ggml.h"

// The ROCmFPX MMVQ consumers for these four weight formats use the Q8_1
// activation scale and all 32 quantized bytes, but not the activation sum.
// Keep this as an exact whitelist. ROCmFP4 remains intentionally excluded
// from this candidate, and stock affine formats that consume the sum must
// continue to receive the legacy block contents.
static constexpr bool ggml_rocmfpx_mmvq_sum_free_eligible(const ggml_type type) {
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

static constexpr bool ggml_rocmfpx_mmvq_sum_free_build_enabled() {
#if defined(GGML_HIP_ROCMFPX_MMVQ_SUM_FREE)
    return true;
#else
    return false;
#endif
}

static constexpr bool ggml_rocmfpx_mmvq_use_sum_free_q8_1(const ggml_type type) {
    return ggml_rocmfpx_mmvq_sum_free_build_enabled() &&
           ggml_rocmfpx_mmvq_sum_free_eligible(type);
}

#if defined(__CUDACC__) || defined(__HIPCC__)
#define GGML_ROCMFPX_HOST_DEVICE __host__ __device__
#else
#define GGML_ROCMFPX_HOST_DEVICE
#endif

template<bool compute_sum>
static constexpr GGML_ROCMFPX_HOST_DEVICE float ggml_rocmfpx_mmvq_q8_1_sum_lane(const float sum) {
    return compute_sum ? sum : 0.0f;
}

#undef GGML_ROCMFPX_HOST_DEVICE
