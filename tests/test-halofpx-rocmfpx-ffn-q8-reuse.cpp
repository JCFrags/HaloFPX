#include "rocmfpx-ffn-q8-reuse.h"

#include <array>
#include <cstdio>

#ifndef HALOFPX_EXPECT_ROCMFPX_FFN_Q8_REUSE
#error "HALOFPX_EXPECT_ROCMFPX_FFN_Q8_REUSE must be defined"
#endif

static constexpr halofpx_rocmfpx_ffn_q8_reuse_contract eligible_contract(const ggml_type type) {
    return {
        type,
        type,
        9,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
    };
}

int main() {
    static_assert(halofpx_rocmfpx_ffn_q8_reuse_build_enabled() ==
                  (HALOFPX_EXPECT_ROCMFPX_FFN_Q8_REUSE != 0));

    constexpr std::array<ggml_type, 4> allowed = {
        GGML_TYPE_Q2_0_ROCMFPX,
        GGML_TYPE_Q3_0_ROCMFPX,
        GGML_TYPE_Q6_0_ROCMFPX,
        GGML_TYPE_Q8_0_ROCMFPX,
    };

    for (const ggml_type type : allowed) {
        const auto contract = eligible_contract(type);
        if (!halofpx_rocmfpx_ffn_q8_reuse_type(type) ||
            !halofpx_rocmfpx_ffn_q8_reuse_eligible(contract) ||
            halofpx_rocmfpx_ffn_q8_reuse_dispatch(contract) !=
                (HALOFPX_EXPECT_ROCMFPX_FFN_Q8_REUSE != 0)) {
            std::fprintf(stderr, "eligible ROCmFPX type %d failed the reuse contract\n", (int) type);
            return 1;
        }
    }

    constexpr std::array<ggml_type, 5> rejected = {
        GGML_TYPE_Q4_0_ROCMFP4,
        GGML_TYPE_Q4_0_ROCMFP4_FAST,
        GGML_TYPE_Q4_0,
        GGML_TYPE_Q8_0,
        GGML_TYPE_F32,
    };

    for (const ggml_type type : rejected) {
        if (halofpx_rocmfpx_ffn_q8_reuse_type(type) ||
            halofpx_rocmfpx_ffn_q8_reuse_eligible(eligible_contract(type))) {
            std::fprintf(stderr, "non-whitelisted type %d passed the reuse contract\n", (int) type);
            return 1;
        }
    }

    auto contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
    contract.weight_type_b = GGML_TYPE_Q8_0_ROCMFPX;
    if (halofpx_rocmfpx_ffn_q8_reuse_eligible(contract)) {
        return 1;
    }

    contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
    contract.activation_columns = 8;
    if (halofpx_rocmfpx_ffn_q8_reuse_eligible(contract)) {
        return 1;
    }

    const auto expect_rejected = [](halofpx_rocmfpx_ffn_q8_reuse_contract candidate) {
        return !halofpx_rocmfpx_ffn_q8_reuse_eligible(candidate) &&
               !halofpx_rocmfpx_ffn_q8_reuse_dispatch(candidate);
    };

#define HALOFPX_REJECT_BOOL_FIELD(field)                                      \
    do {                                                                       \
        contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);                 \
        contract.field = false;                                                \
        if (!expect_rejected(contract)) {                                      \
            std::fprintf(stderr, "selector failed to reject false %s\n", #field); \
            return 1;                                                          \
        }                                                                      \
    } while (false)

    HALOFPX_REJECT_BOOL_FIELD(ordinary_dense);
    HALOFPX_REJECT_BOOL_FIELD(no_bias_glu_pair);
    HALOFPX_REJECT_BOOL_FIELD(same_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_outputs);
    HALOFPX_REJECT_BOOL_FIELD(same_weight_layout);
    HALOFPX_REJECT_BOOL_FIELD(local_non_split);
    HALOFPX_REJECT_BOOL_FIELD(safe_allocation_views);
    HALOFPX_REJECT_BOOL_FIELD(both_mmq_eligible);

#undef HALOFPX_REJECT_BOOL_FIELD

    std::printf("ROCmFPX FFN Q8_1 reuse build=%s whitelist=4 boundary=9\n",
                halofpx_rocmfpx_ffn_q8_reuse_build_enabled() ? "on" : "off");
    return 0;
}
