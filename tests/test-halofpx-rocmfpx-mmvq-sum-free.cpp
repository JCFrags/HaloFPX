#include "ggml.h"

#define GGML_COMMON_DECL_CPP
#include "ggml-common.h"

#include "rocmfpx-mmvq-sum-free.h"

#include <cmath>
#include <cstdio>

#ifndef HALOFPX_EXPECT_ROCMFPX_MMVQ_SUM_FREE
#error "host contract must state its expected build mode"
#endif

static_assert(ggml_rocmfpx_mmvq_sum_free_build_enabled() ==
              (HALOFPX_EXPECT_ROCMFPX_MMVQ_SUM_FREE != 0));

static bool expected_eligible(const ggml_type type) {
    return type == GGML_TYPE_Q2_0_ROCMFPX ||
           type == GGML_TYPE_Q3_0_ROCMFPX ||
           type == GGML_TYPE_Q6_0_ROCMFPX ||
           type == GGML_TYPE_Q8_0_ROCMFPX;
}

int main() {
    static_assert(sizeof(block_q8_1) == 36, "sum-free MMVQ must preserve the Q8_1 ABI");
    static_assert(ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q2_0_ROCMFPX));
    static_assert(ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q3_0_ROCMFPX));
    static_assert(ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q6_0_ROCMFPX));
    static_assert(ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q8_0_ROCMFPX));
    static_assert(!ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q4_0_ROCMFP4));
    static_assert(!ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q4_0_ROCMFP4_FAST));
    static_assert(!ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q4_0));
    static_assert(!ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q4_1));
    static_assert(!ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q5_0));
    static_assert(!ggml_rocmfpx_mmvq_sum_free_eligible(GGML_TYPE_Q5_1));

    for (int value = 0; value < GGML_TYPE_COUNT; ++value) {
        const ggml_type type = static_cast<ggml_type>(value);
        if (ggml_rocmfpx_mmvq_sum_free_eligible(type) != expected_eligible(type)) {
            std::fprintf(stderr, "unexpected ROCmFPX MMVQ sum-free eligibility for type %d\n", value);
            return 1;
        }

        const bool expected_dispatch = ggml_rocmfpx_mmvq_sum_free_build_enabled() && expected_eligible(type);
        if (ggml_rocmfpx_mmvq_use_sum_free_q8_1(type) != expected_dispatch) {
            std::fprintf(stderr, "unexpected ROCmFPX MMVQ sum-free dispatch for type %d\n", value);
            return 1;
        }
    }

    const float legacy_sum = ggml_rocmfpx_mmvq_q8_1_sum_lane<true>(-17.5f);
    const float unused_sum = ggml_rocmfpx_mmvq_q8_1_sum_lane<false>(-17.5f);
    if (legacy_sum != -17.5f || unused_sum != 0.0f || std::signbit(unused_sum)) {
        std::fprintf(stderr, "Q8_1 sum-lane policy is not deterministic\n");
        return 1;
    }

    std::printf("ROCmFPX MMVQ sum-free build=%s ABI=%zu whitelist=4\n",
            ggml_rocmfpx_mmvq_sum_free_build_enabled() ? "on" : "off",
            sizeof(block_q8_1));
    return 0;
}
