#include "rocmfpx-moe-q8-reuse.h"

#include <array>
#include <cstdio>

#ifndef HALOFPX_EXPECT_ROCMFPX_MOE_Q8_REUSE
#error "HALOFPX_EXPECT_ROCMFPX_MOE_Q8_REUSE must be defined"
#endif

static constexpr halofpx_rocmfpx_moe_q8_reuse_contract eligible_contract(const ggml_type type) {
    return {
        type,
        type,
        9,
        32,
        4,
        true,
        true,
        true,
        true,
        true,
        true,
        true,
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

static bool expect_rejected(halofpx_rocmfpx_moe_q8_reuse_contract candidate) {
    return !halofpx_rocmfpx_moe_q8_reuse_eligible(candidate) &&
           !halofpx_rocmfpx_moe_q8_reuse_dispatch(candidate);
}

int main() {
    static_assert(halofpx_rocmfpx_moe_q8_reuse_build_enabled() ==
                  (HALOFPX_EXPECT_ROCMFPX_MOE_Q8_REUSE != 0));

    constexpr std::array<ggml_type, 4> allowed = {
        GGML_TYPE_Q2_0_ROCMFPX,
        GGML_TYPE_Q3_0_ROCMFPX,
        GGML_TYPE_Q6_0_ROCMFPX,
        GGML_TYPE_Q8_0_ROCMFPX,
    };
    for (const ggml_type type : allowed) {
        const auto contract = eligible_contract(type);
        if (!halofpx_rocmfpx_moe_q8_reuse_type(type) ||
            !halofpx_rocmfpx_moe_q8_reuse_eligible(contract) ||
            halofpx_rocmfpx_moe_q8_reuse_dispatch(contract) !=
                (HALOFPX_EXPECT_ROCMFPX_MOE_Q8_REUSE != 0)) {
            std::fprintf(stderr, "eligible ROCmFPX type %d failed MoE reuse contract\n", (int) type);
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
        if (halofpx_rocmfpx_moe_q8_reuse_type(type) ||
            halofpx_rocmfpx_moe_q8_reuse_eligible(eligible_contract(type))) {
            std::fprintf(stderr, "non-whitelisted type %d passed MoE reuse contract\n", (int) type);
            return 1;
        }
    }

    auto contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
    contract.weight_type_b = GGML_TYPE_Q8_0_ROCMFPX;
    if (!expect_rejected(contract)) {
        std::fprintf(stderr, "mixed ROCmFPX layouts passed MoE reuse contract\n");
        return 1;
    }

    // Independent routing/index geometry boundaries.
    for (const auto values : std::array<std::array<int64_t, 3>, 8> {{
             {{ 0, 32, 4 }},
             {{ 9, 0, 4 }},
             {{ 9, 32, 0 }},
             {{ 9, 32, 33 }},
             {{ -1, 32, 4 }},
             {{ 9, 2048, 1024 }},
             {{ INT32_MAX, 32, 4 }},
             {{ 1 << 22, 32, 4 }},
         }}) {
        contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
        contract.activation_tokens = values[0];
        contract.expert_count = values[1];
        contract.experts_used = values[2];
        if (!expect_rejected(contract)) {
            std::fprintf(stderr, "invalid token/expert index geometry passed selector\n");
            return 1;
        }
    }

#define HALOFPX_REJECT_BOOL_FIELD(field)                                           \
    do {                                                                            \
        contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);                      \
        contract.field = false;                                                     \
        if (!expect_rejected(contract)) {                                           \
            std::fprintf(stderr, "selector failed to reject false %s\n", #field); \
            return 1;                                                               \
        }                                                                           \
    } while (false)

    HALOFPX_REJECT_BOOL_FIELD(gfx1151_hip);
    HALOFPX_REJECT_BOOL_FIELD(routed_moe_pair);          // malformed graph group
    HALOFPX_REJECT_BOOL_FIELD(no_bias_glu_pair);         // malformed/bias group
    HALOFPX_REJECT_BOOL_FIELD(exact_shared_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_outputs);
    HALOFPX_REJECT_BOOL_FIELD(same_weight_layout);
    HALOFPX_REJECT_BOOL_FIELD(same_output_layout);
    HALOFPX_REJECT_BOOL_FIELD(exact_shared_ids);         // independent routing
    HALOFPX_REJECT_BOOL_FIELD(valid_routing_layout);     // stride/type metadata
    HALOFPX_REJECT_BOOL_FIELD(valid_index_geometry);     // ID/output geometry
    HALOFPX_REJECT_BOOL_FIELD(local_non_split);
    HALOFPX_REJECT_BOOL_FIELD(safe_allocation_views);
    HALOFPX_REJECT_BOOL_FIELD(nonoverlapping_outputs);
    HALOFPX_REJECT_BOOL_FIELD(both_mmq_paths);
    HALOFPX_REJECT_BOOL_FIELD(single_stream);

#undef HALOFPX_REJECT_BOOL_FIELD

    std::printf("ROCmFPX routed-MoE Q8_1 reuse build=%s whitelist=4 experts=4/32 tokens=9\n",
                halofpx_rocmfpx_moe_q8_reuse_build_enabled() ? "on" : "off");
    return 0;
}
