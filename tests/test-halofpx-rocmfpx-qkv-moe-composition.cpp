#include "rocmfpx-mmvq-qkv-q8-reuse.h"
#include "rocmfpx-moe-q8-reuse.h"
#include "rocmfpx-qkv-q8-reuse.h"

#include <array>
#include <cstdio>

#if !defined(HALOFPX_EXPECT_PROMPT_QKV) || !defined(HALOFPX_EXPECT_DECODE_QKV) || \
    !defined(HALOFPX_EXPECT_ROUTED_MOE)
#error "composition target must declare all three expected feature states"
#endif

static_assert(HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM == 12);
static_assert(HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM == 13);
static_assert(HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM == 14);
static_assert(HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM == 15);
static_assert(halofpx_rocmfpx_qkv_q8_reuse_build_enabled() ==
              static_cast<bool>(HALOFPX_EXPECT_PROMPT_QKV));
static_assert(halofpx_rocmfpx_mmvq_qkv_q8_reuse_build_enabled() ==
              static_cast<bool>(HALOFPX_EXPECT_DECODE_QKV));
static_assert(halofpx_rocmfpx_moe_q8_reuse_build_enabled() ==
              static_cast<bool>(HALOFPX_EXPECT_ROUTED_MOE));

struct composition_fixture {
    static constexpr int64_t hidden = 256;
    static constexpr int64_t q_width = 256;
    static constexpr int64_t kv_width = 64;
    static constexpr int64_t moe_width = 128;
    static constexpr int64_t n_experts = 32;
    static constexpr int64_t n_expert_used = 4;
    static constexpr int64_t moe_tokens = 16;

    ggml_context * ctx = nullptr;
    ggml_cgraph * graph = nullptr;
    ggml_tensor * q = nullptr;
    ggml_tensor * k = nullptr;
    ggml_tensor * v = nullptr;
    ggml_tensor * moe_gate = nullptr;
    ggml_tensor * moe_up = nullptr;
    ggml_tensor * moe_glu = nullptr;
    std::array<std::array<int32_t, 4>, 3> moe_markers = {};

    explicit composition_fixture(const int64_t qkv_rows) {
        const ggml_init_params params = {
            ggml_tensor_overhead() * 256 + ggml_graph_overhead_custom(256, false),
            nullptr,
            true,
        };
        ctx = ggml_init(params);
        GGML_ASSERT(ctx);
        graph = ggml_new_graph_custom(ctx, 256, false);

        ggml_tensor * qkv_a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, qkv_rows);
        ggml_tensor * qkv_b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, qkv_rows);
        ggml_tensor * qkv_activation = ggml_add(ctx, qkv_a, qkv_b);
        ggml_set_name(qkv_activation, "attn_norm-0");

        ggml_tensor * weight_q = ggml_new_tensor_2d(ctx, GGML_TYPE_Q6_0_ROCMFPX, hidden, q_width);
        ggml_tensor * weight_k = ggml_new_tensor_2d(ctx, GGML_TYPE_Q6_0_ROCMFPX, hidden, kv_width);
        ggml_tensor * weight_v = ggml_new_tensor_2d(ctx, GGML_TYPE_Q6_0_ROCMFPX, hidden, kv_width);
        ggml_set_name(weight_q, "blk.0.attn_q.weight");
        ggml_set_name(weight_k, "blk.0.attn_k.weight");
        ggml_set_name(weight_v, "blk.0.attn_v.weight");

        q = ggml_mul_mat(ctx, weight_q, qkv_activation);
        k = ggml_mul_mat(ctx, weight_k, qkv_activation);
        v = ggml_mul_mat(ctx, weight_v, qkv_activation);
        ggml_set_name(q, "Qcur-0");
        ggml_set_name(k, "Kcur-0");
        ggml_set_name(v, "Vcur-0");

        ggml_tensor * q_reshape = ggml_reshape_3d(ctx, q, 64, q_width / 64, qkv_rows);
        ggml_tensor * k_reshape = ggml_reshape_3d(ctx, k, 64, kv_width / 64, qkv_rows);
        ggml_tensor * v_reshape = ggml_reshape_3d(ctx, v, 64, kv_width / 64, qkv_rows);

        ggml_tensor * moe_a = ggml_new_tensor_3d(
            ctx, GGML_TYPE_F32, hidden, n_expert_used, moe_tokens);
        ggml_tensor * moe_b = ggml_new_tensor_3d(
            ctx, GGML_TYPE_F32, hidden, n_expert_used, moe_tokens);
        ggml_tensor * moe_activation = ggml_add(ctx, moe_a, moe_b);
        ggml_tensor * ids = ggml_new_tensor_2d(ctx, GGML_TYPE_I32, n_expert_used, moe_tokens);
        ggml_tensor * weight_gate = ggml_new_tensor_3d(
            ctx, GGML_TYPE_Q6_0_ROCMFPX, hidden, moe_width, n_experts);
        ggml_tensor * weight_up = ggml_new_tensor_3d(
            ctx, GGML_TYPE_Q6_0_ROCMFPX, hidden, moe_width, n_experts);

        moe_gate = ggml_mul_mat_id(ctx, weight_gate, moe_activation, ids);
        moe_up = ggml_mul_mat_id(ctx, weight_up, moe_activation, ids);
        moe_glu = ggml_glu_split(ctx, moe_gate, moe_up, GGML_GLU_OP_SWIGLU);

        // Interleave a complete routed-MoE group between Q and K/V. The QKV
        // planner may compact its own group but must preserve MoE adjacency,
        // order, and marker bytes.
        ggml_build_forward_expand(graph, q_reshape);
        ggml_build_forward_expand(graph, moe_glu);
        ggml_build_forward_expand(graph, k_reshape);
        ggml_build_forward_expand(graph, v_reshape);

        const std::array<ggml_tensor *, 3> moe_nodes = { moe_gate, moe_up, moe_glu };
        for (size_t node = 0; node < moe_nodes.size(); ++node) {
            moe_markers[node] = {
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM],
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM],
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM],
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM],
            };
        }
    }

    ~composition_fixture() {
        ggml_free(ctx);
    }

    int index_of(const ggml_tensor * tensor) const {
        for (int i = 0; i < graph->n_nodes; ++i) {
            if (graph->nodes[i] == tensor) {
                return i;
            }
        }
        return -1;
    }

    bool moe_group_preserved() const {
        const int gate_index = index_of(moe_gate);
        if (gate_index < 0 || gate_index + 2 >= graph->n_nodes ||
            graph->nodes[gate_index + 1] != moe_up || graph->nodes[gate_index + 2] != moe_glu) {
            return false;
        }
        const std::array<ggml_tensor *, 3> moe_nodes = { moe_gate, moe_up, moe_glu };
        for (size_t node = 0; node < moe_nodes.size(); ++node) {
            const std::array<int32_t, 4> current = {
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM],
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM],
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM],
                moe_nodes[node]->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM],
            };
            if (current != moe_markers[node]) {
                return false;
            }
        }
        return true;
    }

    bool topological() const {
        for (int i = 0; i < graph->n_nodes; ++i) {
            for (const ggml_tensor * src : graph->nodes[i]->src) {
                const int src_index = src ? index_of(src) : -1;
                if (src_index >= i) {
                    return false;
                }
            }
        }
        return true;
    }
};

static bool qkv_adjacent(const composition_fixture & fixture) {
    const int q_index = fixture.index_of(fixture.q);
    return q_index >= 0 && q_index + 2 < fixture.graph->n_nodes &&
           fixture.graph->nodes[q_index + 1] == fixture.k &&
           fixture.graph->nodes[q_index + 2] == fixture.v;
}

static bool test_decode_then_moe() {
    composition_fixture fixture(1);
    if (!fixture.moe_group_preserved()) {
        std::fprintf(stderr, "decode composition fixture lacks initial MoE adjacency\n");
        return false;
    }
    const auto decode = halofpx_rocmfpx_mmvq_qkv_dispatch_graph_reorder(fixture.graph, true);
    const size_t expected_decode = HALOFPX_EXPECT_DECODE_QKV ? 1 : 0;
    if (decode.eligible_groups != expected_decode || decode.moved_groups != expected_decode ||
        qkv_adjacent(fixture) != static_cast<bool>(HALOFPX_EXPECT_DECODE_QKV) ||
        !fixture.moe_group_preserved() || !fixture.topological()) {
        std::fprintf(stderr, "decode optimizer did not preserve routed-MoE composition\n");
        return false;
    }
    const auto prompt = halofpx_rocmfpx_qkv_dispatch_graph_reorder(fixture.graph, true);
    return prompt.eligible_groups == 0 && prompt.moved_groups == 0 &&
           fixture.moe_group_preserved() && fixture.topological();
}

static bool test_decode_fallback_then_prompt_then_moe() {
    composition_fixture fixture(32);
    if (!fixture.moe_group_preserved()) {
        std::fprintf(stderr, "prompt composition fixture lacks initial MoE adjacency\n");
        return false;
    }
    const auto decode = halofpx_rocmfpx_mmvq_qkv_dispatch_graph_reorder(fixture.graph, true);
    if (decode.eligible_groups != 0 || decode.moved_groups != 0 || !fixture.moe_group_preserved()) {
        std::fprintf(stderr, "decode optimizer did not fall through without changing prompt/MoE graph\n");
        return false;
    }
    const auto prompt = halofpx_rocmfpx_qkv_dispatch_graph_reorder(fixture.graph, true);
    const size_t expected_prompt = HALOFPX_EXPECT_PROMPT_QKV ? 1 : 0;
    if (prompt.eligible_groups != expected_prompt || prompt.moved_groups != expected_prompt ||
        qkv_adjacent(fixture) != static_cast<bool>(HALOFPX_EXPECT_PROMPT_QKV) ||
        !fixture.moe_group_preserved() || !fixture.topological()) {
        std::fprintf(stderr, "prompt optimizer did not preserve routed-MoE composition\n");
        return false;
    }
    return true;
}

int main() {
    if (!test_decode_then_moe() || !test_decode_fallback_then_prompt_then_moe()) {
        return 1;
    }
    std::printf(
        "ROCmFPX composition prompt=%d decode=%d routed-MoE=%d order=decode->prompt markers=disjoint\n",
        HALOFPX_EXPECT_PROMPT_QKV,
        HALOFPX_EXPECT_DECODE_QKV,
        HALOFPX_EXPECT_ROUTED_MOE);
    return 0;
}
