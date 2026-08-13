#include "rocmfpx-qkv-q8-reuse.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#ifndef HALOFPX_EXPECT_ROCMFPX_QKV_Q8_REUSE
#error "HALOFPX_EXPECT_ROCMFPX_QKV_Q8_REUSE must be defined"
#endif

static constexpr halofpx_rocmfpx_qkv_q8_reuse_contract eligible_contract(const ggml_type type) {
    return {
        type,
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
        true,
        true,
        true,
        true,
        true,
        true,
        true,
    };
}

struct graph_options {
    int64_t columns = 32;
    ggml_type type_q = GGML_TYPE_Q6_0_ROCMFPX;
    ggml_type type_k = GGML_TYPE_Q6_0_ROCMFPX;
    ggml_type type_v = GGML_TYPE_Q6_0_ROCMFPX;
    bool distinct_v_activation = false;
    bool q_bias = false;
    bool fourth_matmul = false;
    bool wrong_projection_name = false;
    bool wrong_weight_name = false;
    bool activation_view = false;
    bool nondefault_precision = false;
    bool nondefault_hint = false;
    bool crossed_activation_write = false;
    bool crossed_weight_write = false;
    bool crossed_activation_metadata = false;
    bool crossed_q_output_write = false;
    bool v_before_k = false;
};

struct graph_fixture {
    static constexpr int64_t hidden = 256;
    static constexpr int64_t q_width = 256;
    static constexpr int64_t kv_width = 64;

    ggml_context * ctx = nullptr;
    ggml_cgraph * graph = nullptr;
    ggml_tensor * q = nullptr;
    ggml_tensor * k = nullptr;
    ggml_tensor * v = nullptr;

    explicit graph_fixture(const graph_options & options) {
        const ggml_init_params params = {
            ggml_tensor_overhead() * 128 + ggml_graph_overhead_custom(128, false),
            nullptr,
            true,
        };
        ctx = ggml_init(params);
        GGML_ASSERT(ctx);
        graph = ggml_new_graph_custom(ctx, 128, false);

        ggml_tensor * activation = nullptr;
        if (options.activation_view) {
            ggml_tensor * base = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden * 2, options.columns);
            activation = ggml_view_2d(ctx, base, hidden, options.columns, base->nb[1], 0);
        } else {
            ggml_tensor * activation_a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.columns);
            ggml_tensor * activation_b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.columns);
            activation = ggml_add(ctx, activation_a, activation_b);
        }
        ggml_set_name(activation, "attn_norm-0");

        ggml_tensor * activation_v = activation;
        if (options.distinct_v_activation) {
            ggml_tensor * activation_v_a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.columns);
            ggml_tensor * activation_v_b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.columns);
            activation_v = ggml_add(ctx, activation_v_a, activation_v_b);
            ggml_set_name(activation_v, "attn_norm-v-0");
        }

        ggml_tensor * weight_q = ggml_new_tensor_2d(ctx, options.type_q, hidden, q_width);
        ggml_tensor * weight_k = ggml_new_tensor_2d(ctx, options.type_k, hidden, kv_width);
        ggml_tensor * weight_v = ggml_new_tensor_2d(ctx, options.type_v, hidden, kv_width);
        ggml_set_name(weight_q, "blk.0.attn_q.weight");
        ggml_set_name(weight_k, options.wrong_weight_name ? "blk.0.attn_output.weight" : "blk.0.attn_k.weight");
        ggml_set_name(weight_v, "blk.0.attn_v.weight");

        q = ggml_mul_mat(ctx, weight_q, activation);
        k = ggml_mul_mat(ctx, weight_k, activation);
        v = ggml_mul_mat(ctx, weight_v, activation_v);
        ggml_set_name(q, options.wrong_projection_name ? "Qcur-bad" : "Qcur-0");
        ggml_set_name(k, "Kcur-0");
        ggml_set_name(v, "Vcur-0");

        if (options.nondefault_precision) {
            ggml_mul_mat_set_prec(q, GGML_PREC_F32);
        }
        if (options.nondefault_hint) {
            ggml_mul_mat_set_hint(q, GGML_HINT_SRC0_IS_HADAMARD);
        }

        ggml_tensor * q_consumer = q;
        if (options.q_bias) {
            ggml_tensor * bias = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, q_width, options.columns);
            q_consumer = ggml_add(ctx, q, bias);
            ggml_set_name(q_consumer, "Qcur-bias-0");
        }

        ggml_tensor * q_reshape = ggml_reshape_3d(ctx, q_consumer, 64, 4, options.columns);
        ggml_tensor * k_reshape = ggml_reshape_3d(ctx, k, 64, 1, options.columns);
        ggml_tensor * v_reshape = ggml_reshape_3d(ctx, v, 64, 1, options.columns);
        ggml_set_name(q_reshape, "Qcur-reshape-0");
        ggml_set_name(k_reshape, "Kcur-reshape-0");
        ggml_set_name(v_reshape, "Vcur-reshape-0");

        ggml_build_forward_expand(graph, q_reshape);

        if (options.crossed_activation_write) {
            ggml_tensor * write = ggml_scale_inplace(ctx, activation, 2.0f);
            ggml_set_name(write, "crossed-activation-write");
            ggml_build_forward_expand(graph, write);
        }
        if (options.crossed_weight_write) {
            ggml_tensor * donor = ggml_new_tensor_2d(ctx, options.type_k, hidden, kv_width);
            ggml_tensor * write = ggml_cpy(ctx, donor, weight_k);
            ggml_set_name(write, "crossed-weight-write");
            ggml_build_forward_expand(graph, write);
        }
        if (options.crossed_activation_metadata) {
            ggml_tensor * metadata = ggml_reshape_2d(ctx, activation, hidden, options.columns);
            ggml_set_name(metadata, "crossed-activation-metadata");
            ggml_build_forward_expand(graph, metadata);
        }
        if (options.crossed_q_output_write) {
            ggml_tensor * write = ggml_scale_inplace(ctx, q_reshape, 2.0f);
            ggml_set_name(write, "crossed-q-output-write");
            ggml_build_forward_expand(graph, write);
        }

        if (options.v_before_k) {
            ggml_build_forward_expand(graph, v_reshape);
            ggml_build_forward_expand(graph, k_reshape);
        } else {
            ggml_build_forward_expand(graph, k_reshape);
            ggml_build_forward_expand(graph, v_reshape);
        }

        if (options.fourth_matmul) {
            ggml_tensor * weight_x = ggml_new_tensor_2d(ctx, options.type_q, hidden, kv_width);
            ggml_set_name(weight_x, "blk.0.attn_extra.weight");
            ggml_tensor * x = ggml_mul_mat(ctx, weight_x, activation);
            ggml_set_name(x, "Xcur-0");
            ggml_tensor * x_reshape = ggml_reshape_3d(ctx, x, 64, 1, options.columns);
            ggml_build_forward_expand(graph, x_reshape);
        }
    }

    ~graph_fixture() {
        ggml_free(ctx);
    }

    std::vector<ggml_tensor *> nonprojections() const {
        std::vector<ggml_tensor *> result;
        for (int i = 0; i < graph->n_nodes; ++i) {
            ggml_tensor * node = graph->nodes[i];
            if (node != q && node != k && node != v) {
                result.push_back(node);
            }
        }
        return result;
    }

    int index_of(const ggml_tensor * tensor) const {
        for (int i = 0; i < graph->n_nodes; ++i) {
            if (graph->nodes[i] == tensor) {
                return i;
            }
        }
        return -1;
    }
};

static bool expect_no_group(const graph_options & options, const char * label) {
    graph_fixture fixture(options);
    if (!halofpx_rocmfpx_qkv_find_graph_groups(fixture.graph).empty()) {
        std::fprintf(stderr, "negative graph passed QKV recognizer: %s\n", label);
        return false;
    }
    const std::vector<ggml_tensor *> before(
        fixture.graph->nodes, fixture.graph->nodes + fixture.graph->n_nodes);
    const auto result = halofpx_rocmfpx_qkv_plan_graph_reorder(fixture.graph);
    const bool unchanged = std::equal(before.begin(), before.end(), fixture.graph->nodes);
    const bool unstamped =
        fixture.q->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM] == 0 &&
        fixture.k->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM] == 0 &&
        fixture.v->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM] == 0;
    if (result.eligible_groups != 0 || result.moved_groups != 0 || !unchanged || !unstamped) {
        std::fprintf(stderr, "negative graph was changed by QKV planner: %s\n", label);
        return false;
    }
    return true;
}

static bool test_graph_reorder() {
    graph_fixture fixture({});
    const auto groups = halofpx_rocmfpx_qkv_find_graph_groups(fixture.graph);
    if (groups.size() != 1 || groups[0].nodes[0] != fixture.q || groups[0].nodes[1] != fixture.k ||
        groups[0].nodes[2] != fixture.v || groups[0].layer != 0) {
        std::fprintf(stderr, "eligible graph was not recognized as one Q/K/V group\n");
        return false;
    }
    if (fixture.q->ne[0] == fixture.k->ne[0]) {
        std::fprintf(stderr, "graph fixture failed to exercise unequal Q versus K/V widths\n");
        return false;
    }

    const auto nonprojections_before = fixture.nonprojections();
    const auto result = halofpx_rocmfpx_qkv_plan_graph_reorder(fixture.graph);
    const int q_index = fixture.index_of(fixture.q);
    if (result.eligible_groups != 1 || result.moved_groups != 1 || q_index < 0 ||
        fixture.graph->nodes[q_index + 1] != fixture.k || fixture.graph->nodes[q_index + 2] != fixture.v ||
        fixture.nonprojections() != nonprojections_before) {
        std::fprintf(stderr, "eligible graph was not stably reordered to Q/K/V\n");
        return false;
    }
    for (size_t role = 0; role < 3; ++role) {
        const ggml_tensor * node = fixture.graph->nodes[q_index + static_cast<int>(role)];
        if (node->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM] !=
                HALOFPX_ROCMFPX_QKV_Q8_REUSE_GRAPH_MAGIC ||
            node->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM] != static_cast<int32_t>(role)) {
            std::fprintf(stderr, "eligible graph node is missing QKV reorder provenance\n");
            return false;
        }
    }
    for (int i = 0; i < fixture.graph->n_nodes; ++i) {
        for (const ggml_tensor * src : fixture.graph->nodes[i]->src) {
            if (!src) {
                continue;
            }
            const int src_index = fixture.index_of(src);
            if (src_index >= i) {
                std::fprintf(stderr, "QKV graph reorder violated topological source order\n");
                return false;
            }
        }
    }

    std::vector<ggml_tensor *> order_before_second(
        fixture.graph->nodes, fixture.graph->nodes + fixture.graph->n_nodes);
    const auto second = halofpx_rocmfpx_qkv_plan_graph_reorder(fixture.graph);
    if (second.eligible_groups != 1 || second.moved_groups != 0 ||
        !std::equal(order_before_second.begin(), order_before_second.end(), fixture.graph->nodes)) {
        std::fprintf(stderr, "QKV graph reorder is not idempotent\n");
        return false;
    }
    return true;
}

static bool test_safe_crossings_and_projection_order() {
    std::array<graph_options, 3> safe_options = {};
    safe_options[0].crossed_activation_metadata = true;
    safe_options[1].crossed_q_output_write = true;
    safe_options[2].v_before_k = true;
    for (const graph_options & options : safe_options) {
        graph_fixture fixture(options);
        const auto groups = halofpx_rocmfpx_qkv_find_graph_groups(fixture.graph);
        const auto result = halofpx_rocmfpx_qkv_plan_graph_reorder(fixture.graph);
        const int q_index = fixture.index_of(fixture.q);
        if (groups.size() != 1 || result.eligible_groups != 1 || result.moved_groups != 1 ||
            q_index < 0 || fixture.graph->nodes[q_index + 1] != fixture.k ||
            fixture.graph->nodes[q_index + 2] != fixture.v) {
            std::fprintf(stderr, "safe QKV crossing/order positive control was rejected\n");
            return false;
        }
    }
    return true;
}

static bool test_build_gate() {
    graph_fixture disabled_by_arch({});
    if (halofpx_rocmfpx_qkv_dispatch_graph_reorder(disabled_by_arch.graph, false).eligible_groups != 0) {
        std::fprintf(stderr, "non-gfx1151 graph passed build dispatch\n");
        return false;
    }

    graph_fixture fixture({});
    const auto result = halofpx_rocmfpx_qkv_dispatch_graph_reorder(fixture.graph, true);
    const bool expected = HALOFPX_EXPECT_ROCMFPX_QKV_Q8_REUSE != 0;
    if ((result.eligible_groups == 1) != expected || (result.moved_groups == 1) != expected) {
        std::fprintf(stderr, "QKV build gate did not match the compile-time mode\n");
        return false;
    }
    return true;
}

static bool test_projection_name_parser() {
    halofpx_rocmfpx_qkv_role role = HALOFPX_ROCMFPX_QKV_ROLE_INVALID;
    int layer = -1;
    if (!halofpx_rocmfpx_qkv_parse_projection_name("Qcur-0", role, layer) ||
        role != HALOFPX_ROCMFPX_QKV_ROLE_Q || layer != 0 ||
        !halofpx_rocmfpx_qkv_parse_projection_name("Kcur-2147483647", role, layer) ||
        role != HALOFPX_ROCMFPX_QKV_ROLE_K || layer != 2147483647) {
        std::fprintf(stderr, "canonical QKV projection name was rejected\n");
        return false;
    }
    constexpr std::array<const char *, 10> rejected = {
        nullptr,
        "",
        "Qcur-",
        "Qcur--1",
        "Qcur-+1",
        "Qcur- 1",
        "Qcur-1x",
        "Xcur-1",
        "Qcur-2147483648",
        "Qcur-999999999999999999999",
    };
    for (const char * name : rejected) {
        role = HALOFPX_ROCMFPX_QKV_ROLE_INVALID;
        layer = -1;
        if (halofpx_rocmfpx_qkv_parse_projection_name(name, role, layer)) {
            std::fprintf(stderr, "noncanonical QKV projection name passed parser: %s\n",
                         name ? name : "<null>");
            return false;
        }
    }
    return true;
}

int main() {
    static_assert(halofpx_rocmfpx_qkv_q8_reuse_build_enabled() ==
                  (HALOFPX_EXPECT_ROCMFPX_QKV_Q8_REUSE != 0));

    constexpr std::array<ggml_type, 4> allowed = {
        GGML_TYPE_Q2_0_ROCMFPX,
        GGML_TYPE_Q3_0_ROCMFPX,
        GGML_TYPE_Q6_0_ROCMFPX,
        GGML_TYPE_Q8_0_ROCMFPX,
    };
    for (const ggml_type type : allowed) {
        const auto contract = eligible_contract(type);
        if (!halofpx_rocmfpx_qkv_q8_reuse_type(type) ||
            !halofpx_rocmfpx_qkv_q8_reuse_eligible(contract) ||
            halofpx_rocmfpx_qkv_q8_reuse_dispatch(contract) !=
                (HALOFPX_EXPECT_ROCMFPX_QKV_Q8_REUSE != 0)) {
            std::fprintf(stderr, "eligible ROCmFPX type %d failed the QKV reuse contract\n", (int) type);
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
        if (halofpx_rocmfpx_qkv_q8_reuse_type(type) ||
            halofpx_rocmfpx_qkv_q8_reuse_eligible(eligible_contract(type))) {
            std::fprintf(stderr, "non-whitelisted type %d passed the QKV reuse contract\n", (int) type);
            return 1;
        }
    }

    auto contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
    contract.weight_type_k = GGML_TYPE_Q8_0_ROCMFPX;
    if (halofpx_rocmfpx_qkv_q8_reuse_eligible(contract)) {
        std::fprintf(stderr, "mixed ROCmFPX layout passed the conservative selector\n");
        return 1;
    }
    contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
    contract.activation_columns = 8;
    if (halofpx_rocmfpx_qkv_q8_reuse_eligible(contract)) {
        std::fprintf(stderr, "eight-column boundary passed the prompt selector\n");
        return 1;
    }

    const auto expect_rejected = [](halofpx_rocmfpx_qkv_q8_reuse_contract candidate) {
        return !halofpx_rocmfpx_qkv_q8_reuse_eligible(candidate) &&
               !halofpx_rocmfpx_qkv_q8_reuse_dispatch(candidate);
    };

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
    HALOFPX_REJECT_BOOL_FIELD(ordinary_separate_qkv);
    HALOFPX_REJECT_BOOL_FIELD(no_fused_wqkv);
    HALOFPX_REJECT_BOOL_FIELD(no_lora);
    HALOFPX_REJECT_BOOL_FIELD(no_bias_or_clamp);
    HALOFPX_REJECT_BOOL_FIELD(exact_shared_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_outputs);
    HALOFPX_REJECT_BOOL_FIELD(default_matmul_params);
    HALOFPX_REJECT_BOOL_FIELD(local_non_split);
    HALOFPX_REJECT_BOOL_FIELD(safe_allocation_views);
    HALOFPX_REJECT_BOOL_FIELD(nonoverlapping_outputs);
    HALOFPX_REJECT_BOOL_FIELD(all_mmq_eligible);
    HALOFPX_REJECT_BOOL_FIELD(single_stream);
    HALOFPX_REJECT_BOOL_FIELD(valid_independent_geometry);
    HALOFPX_REJECT_BOOL_FIELD(reordered_adjacent);

#undef HALOFPX_REJECT_BOOL_FIELD

    graph_options negative;
    negative.columns = 8;
    if (!expect_no_group(negative, "prompt-boundary")) return 1;
    negative = {};
    negative.type_k = GGML_TYPE_Q8_0_ROCMFPX;
    if (!expect_no_group(negative, "mixed-layout")) return 1;
    negative = {};
    negative.distinct_v_activation = true;
    if (!expect_no_group(negative, "distinct-activation")) return 1;
    negative = {};
    negative.q_bias = true;
    if (!expect_no_group(negative, "bias-consumer")) return 1;
    negative = {};
    negative.fourth_matmul = true;
    if (!expect_no_group(negative, "extra-matmul-or-lora")) return 1;
    negative = {};
    negative.wrong_projection_name = true;
    if (!expect_no_group(negative, "projection-name")) return 1;
    negative = {};
    negative.wrong_weight_name = true;
    if (!expect_no_group(negative, "weight-name")) return 1;
    negative = {};
    negative.activation_view = true;
    if (!expect_no_group(negative, "activation-view")) return 1;
    negative = {};
    negative.nondefault_precision = true;
    if (!expect_no_group(negative, "precision")) return 1;
    negative = {};
    negative.nondefault_hint = true;
    if (!expect_no_group(negative, "hint")) return 1;
    negative = {};
    negative.crossed_activation_write = true;
    if (!expect_no_group(negative, "crossed-activation-inplace-write")) return 1;
    negative = {};
    negative.crossed_weight_write = true;
    if (!expect_no_group(negative, "crossed-weight-copy-write")) return 1;

    if (!test_graph_reorder() || !test_safe_crossings_and_projection_order() ||
        !test_build_gate() || !test_projection_name_parser()) {
        return 1;
    }

    std::printf("ROCmFPX QKV Q8_1 reuse build=%s whitelist=4 boundary=9 graph-reorder=stable\n",
                halofpx_rocmfpx_qkv_q8_reuse_build_enabled() ? "on" : "off");
    return 0;
}
