#include "rocmfpx-mmvq-qkv-q8-reuse.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#ifndef HALOFPX_EXPECT_ROCMFPX_MMVQ_QKV_Q8_REUSE
#error "HALOFPX_EXPECT_ROCMFPX_MMVQ_QKV_Q8_REUSE must be defined"
#endif

static constexpr halofpx_rocmfpx_mmvq_qkv_q8_reuse_contract eligible_contract(const ggml_type type) {
    return {
        type, type, type,
        1,
        true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true,
    };
}

struct graph_options {
    int64_t rows = 1;
    int64_t q_width = 256;
    int64_t kv_width = 64;
    ggml_type type_q = GGML_TYPE_Q6_0_ROCMFPX;
    ggml_type type_k = GGML_TYPE_Q6_0_ROCMFPX;
    ggml_type type_v = GGML_TYPE_Q6_0_ROCMFPX;
    bool distinct_v_activation = false;
    bool q_bias = false;
    bool fourth_matmul = false;
    bool wrong_projection_name = false;
    bool duplicate_projection_role = false;
    bool mixed_layer = false;
    bool wrong_weight_name = false;
    bool activation_view = false;
    bool weight_view = false;
    bool output_view = false;
    bool mul_mat_id = false;
    bool nondefault_precision = false;
    bool nondefault_hint = false;
    bool crossed_activation_write = false;
    bool crossed_weight_write = false;
    bool crossed_output_write = false;
    bool nested_crossed_output_write = false;
    bool wrong_role_order = false;
};

struct graph_fixture {
    static constexpr int64_t hidden = 256;

    ggml_context * ctx = nullptr;
    ggml_cgraph * graph = nullptr;
    ggml_tensor * activation = nullptr;
    ggml_tensor * q = nullptr;
    ggml_tensor * k = nullptr;
    ggml_tensor * v = nullptr;
    ggml_tensor * q_reshape = nullptr;

    explicit graph_fixture(const graph_options & options) {
        const ggml_init_params params = {
            ggml_tensor_overhead() * 160 + ggml_graph_overhead_custom(160, false),
            nullptr,
            true,
        };
        ctx = ggml_init(params);
        GGML_ASSERT(ctx);
        graph = ggml_new_graph_custom(ctx, 160, false);

        if (options.activation_view) {
            ggml_tensor * base = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden * 2, options.rows);
            activation = ggml_view_2d(ctx, base, hidden, options.rows, base->nb[1], 0);
        } else {
            ggml_tensor * a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.rows);
            ggml_tensor * b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.rows);
            activation = ggml_add(ctx, a, b);
        }
        ggml_set_name(activation, "attn_norm-0");

        ggml_tensor * activation_v = activation;
        if (options.distinct_v_activation) {
            ggml_tensor * a = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.rows);
            ggml_tensor * b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, hidden, options.rows);
            activation_v = ggml_add(ctx, a, b);
            ggml_set_name(activation_v, "attn_norm-v-0");
        }

        ggml_tensor * weight_q = ggml_new_tensor_2d(ctx, options.type_q, hidden, options.q_width);
        ggml_tensor * weight_k_base = ggml_new_tensor_2d(ctx, options.type_k, hidden, options.kv_width);
        ggml_tensor * weight_k = options.weight_view ? ggml_view_tensor(ctx, weight_k_base) : weight_k_base;
        ggml_tensor * weight_v = ggml_new_tensor_2d(ctx, options.type_v, hidden, options.kv_width);
        ggml_set_name(weight_q, "blk.0.attn_q.weight");
        ggml_set_name(weight_k, options.wrong_weight_name ? "blk.0.attn_output.weight" :
            (options.mixed_layer ? "blk.1.attn_k.weight" : "blk.0.attn_k.weight"));
        ggml_set_name(weight_v, "blk.0.attn_v.weight");

        q = ggml_mul_mat(ctx, weight_q, activation);
        k = ggml_mul_mat(ctx, weight_k, activation);
        v = ggml_mul_mat(ctx, weight_v, activation_v);
        ggml_set_name(q, options.wrong_projection_name ? "Qcur-bad" : "Qcur-0");
        ggml_set_name(k, options.duplicate_projection_role ? "Qcur-0" :
            (options.mixed_layer ? "Kcur-1" : "Kcur-0"));
        ggml_set_name(v, "Vcur-0");
        if (options.output_view) {
            q->view_src = activation;
        }
        if (options.mul_mat_id) {
            q->op = GGML_OP_MUL_MAT_ID;
            q->src[2] = ggml_new_tensor_1d(ctx, GGML_TYPE_I32, 1);
        }
        if (options.nondefault_precision) {
            ggml_mul_mat_set_prec(q, GGML_PREC_F32);
        }
        if (options.nondefault_hint) {
            ggml_mul_mat_set_hint(q, GGML_HINT_SRC0_IS_HADAMARD);
        }

        ggml_tensor * q_consumer = q;
        if (options.q_bias) {
            ggml_tensor * bias = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, options.q_width, options.rows);
            q_consumer = ggml_add(ctx, q, bias);
        }

        q_reshape = ggml_reshape_3d(ctx, q_consumer, 64, options.q_width / 64, options.rows);
        ggml_tensor * k_reshape = ggml_reshape_3d(ctx, k, 64, options.kv_width / 64, options.rows);
        ggml_tensor * v_reshape = ggml_reshape_3d(ctx, v, 64, options.kv_width / 64, options.rows);
        ggml_set_name(q_reshape, "Qcur-reshape-0");
        ggml_set_name(k_reshape, "Kcur-reshape-0");
        ggml_set_name(v_reshape, "Vcur-reshape-0");

        if (options.wrong_role_order) {
            ggml_build_forward_expand(graph, k_reshape);
            ggml_build_forward_expand(graph, q_reshape);
        } else {
            ggml_build_forward_expand(graph, q_reshape);
        }

        if (options.crossed_activation_write) {
            ggml_tensor * write = ggml_scale_inplace(ctx, activation, 2.0f);
            ggml_set_name(write, "crossed-activation-write");
            ggml_build_forward_expand(graph, write);
        }
        if (options.crossed_weight_write) {
            ggml_tensor * donor = ggml_new_tensor_2d(ctx, options.type_k, hidden, options.kv_width);
            ggml_tensor * write = ggml_cpy(ctx, donor, weight_k);
            ggml_set_name(write, "crossed-weight-write");
            ggml_build_forward_expand(graph, write);
        }
        if (options.crossed_output_write || options.nested_crossed_output_write) {
            // Root the in-place write through the otherwise valid direct
            // reshape so this exercises the crossed-allocation gate rather
            // than the separate direct-consumer-count refusal.
            ggml_tensor * write = ggml_scale_inplace(ctx, q_reshape, 2.0f);
            if (options.nested_crossed_output_write) {
                // ggml normally canonicalizes view chains. Model malformed or
                // reconstructed nested metadata explicitly so the ancestry
                // walk remains a tested fail-closed boundary.
                write->view_src = q_reshape;
            }
            ggml_set_name(write, "crossed-output-write");
            ggml_build_forward_expand(graph, write);
        }

        if (!options.wrong_role_order) {
            ggml_build_forward_expand(graph, k_reshape);
        }
        ggml_build_forward_expand(graph, v_reshape);

        if (options.fourth_matmul) {
            ggml_tensor * weight_x = ggml_new_tensor_2d(ctx, options.type_q, hidden, options.kv_width);
            ggml_tensor * x = ggml_mul_mat(ctx, weight_x, activation);
            ggml_tensor * x_reshape = ggml_reshape_3d(ctx, x, 64, options.kv_width / 64, options.rows);
            ggml_build_forward_expand(graph, x_reshape);
        }
    }

    ~graph_fixture() {
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

    std::vector<ggml_tensor *> order() const {
        return { graph->nodes, graph->nodes + graph->n_nodes };
    }

    std::vector<ggml_tensor *> nonprojections() const {
        std::vector<ggml_tensor *> result;
        for (int i = 0; i < graph->n_nodes; ++i) {
            if (graph->nodes[i] != q && graph->nodes[i] != k && graph->nodes[i] != v) {
                result.push_back(graph->nodes[i]);
            }
        }
        return result;
    }
};

static bool markers_clear(const graph_fixture & fixture) {
    for (const ggml_tensor * node : { fixture.q, fixture.k, fixture.v }) {
        if (node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] != 0 ||
            node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM] != 0) {
            return false;
        }
    }
    return true;
}

static bool expect_no_group(const graph_options & options, const char * label) {
    graph_fixture fixture(options);
    const auto before = fixture.order();
    if (!halofpx_rocmfpx_mmvq_qkv_find_graph_groups(fixture.graph).empty()) {
        std::fprintf(stderr, "negative graph passed MMVQ QKV recognizer: %s\n", label);
        return false;
    }
    const auto result = halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(fixture.graph);
    if (result.eligible_groups != 0 || result.moved_groups != 0 ||
        fixture.order() != before || !markers_clear(fixture)) {
        std::fprintf(stderr, "negative graph was changed or stamped: %s\n", label);
        return false;
    }
    return true;
}

static bool test_graph_reorder(const graph_options & options, const char * label) {
    graph_fixture fixture(options);
    const auto groups = halofpx_rocmfpx_mmvq_qkv_find_graph_groups(fixture.graph);
    if (groups.size() != 1 || groups[0].nodes[0] != fixture.q || groups[0].nodes[1] != fixture.k ||
        groups[0].nodes[2] != fixture.v || groups[0].layer != 0) {
        std::fprintf(stderr, "eligible %s graph was not recognized\n", label);
        return false;
    }
    const auto nonprojections_before = fixture.nonprojections();
    const auto result = halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(fixture.graph);
    const int q_index = fixture.index_of(fixture.q);
    if (result.eligible_groups != 1 || result.moved_groups != 1 || q_index < 0 ||
        fixture.graph->nodes[q_index + 1] != fixture.k || fixture.graph->nodes[q_index + 2] != fixture.v ||
        fixture.nonprojections() != nonprojections_before) {
        std::fprintf(stderr, "eligible %s graph was not stably compacted to Q/K/V\n", label);
        return false;
    }
    for (size_t role = 0; role < 3; ++role) {
        const ggml_tensor * node = fixture.graph->nodes[q_index + static_cast<int>(role)];
        if (node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] !=
                HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_GRAPH_MAGIC ||
            node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM] != static_cast<int32_t>(role)) {
            std::fprintf(stderr, "eligible %s graph lacks exact optimizer capability\n", label);
            return false;
        }
    }
    for (int i = 0; i < fixture.graph->n_nodes; ++i) {
        for (const ggml_tensor * src : fixture.graph->nodes[i]->src) {
            if (src && fixture.index_of(src) >= i) {
                std::fprintf(stderr, "eligible %s reorder violated topological order\n", label);
                return false;
            }
        }
    }

    const auto order_before_second = fixture.order();
    const auto second = halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(fixture.graph);
    if (second.eligible_groups != 1 || second.moved_groups != 0 || fixture.order() != order_before_second) {
        std::fprintf(stderr, "eligible %s reorder is not idempotent\n", label);
        return false;
    }
    return true;
}

static bool test_build_and_arch_gate() {
    graph_fixture wrong_arch({});
    const auto wrong_arch_order = wrong_arch.order();
    if (halofpx_rocmfpx_mmvq_qkv_dispatch_graph_reorder(wrong_arch.graph, false).eligible_groups != 0 ||
        wrong_arch.order() != wrong_arch_order || !markers_clear(wrong_arch)) {
        std::fprintf(stderr, "non-gfx1151 graph crossed the architecture gate\n");
        return false;
    }

    graph_fixture fixture({});
    const auto before = fixture.order();
    const auto result = halofpx_rocmfpx_mmvq_qkv_dispatch_graph_reorder(fixture.graph, true);
    const bool expected = HALOFPX_EXPECT_ROCMFPX_MMVQ_QKV_Q8_REUSE != 0;
    if ((result.eligible_groups == 1) != expected) {
        std::fprintf(stderr, "build gate did not match compile-time MMVQ QKV mode\n");
        return false;
    }
    if (!expected && (fixture.order() != before || !markers_clear(fixture))) {
        std::fprintf(stderr, "feature-off dispatch changed graph order or op_params\n");
        return false;
    }
    return true;
}

static bool test_stale_marker_revalidation() {
    graph_fixture fixture({});
    if (halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(fixture.graph).eligible_groups != 1) {
        return false;
    }
    ggml_tensor * alternate = ggml_new_tensor_2d(fixture.ctx, GGML_TYPE_F32, graph_fixture::hidden, 1);
    fixture.v->src[1] = alternate;
    const auto result = halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(fixture.graph);
    if (result.eligible_groups != 0 || !markers_clear(fixture)) {
        std::fprintf(stderr, "stale optimizer capability survived failed revalidation\n");
        return false;
    }
    return true;
}

static bool force_adjacent_forged_markers(graph_fixture & fixture) {
    const int q_index = fixture.index_of(fixture.q);
    const int k_index = fixture.index_of(fixture.k);
    const int v_index = fixture.index_of(fixture.v);
    if (q_index < 0 || k_index < 0 || v_index < 0) {
        return false;
    }
    const int insertion = std::min({ q_index, k_index, v_index });
    std::vector<ggml_tensor *> forged;
    forged.reserve(static_cast<size_t>(fixture.graph->n_nodes));
    for (int i = 0; i < fixture.graph->n_nodes; ++i) {
        if (i == insertion) {
            forged.push_back(fixture.q);
            forged.push_back(fixture.k);
            forged.push_back(fixture.v);
        }
        ggml_tensor * node = fixture.graph->nodes[i];
        if (node != fixture.q && node != fixture.k && node != fixture.v) {
            forged.push_back(node);
        }
    }
    if (forged.size() != static_cast<size_t>(fixture.graph->n_nodes)) {
        return false;
    }
    std::copy(forged.begin(), forged.end(), fixture.graph->nodes);
    for (size_t role = 0; role < 3; ++role) {
        ggml_tensor * node = role == 0 ? fixture.q : (role == 1 ? fixture.k : fixture.v);
        node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] =
            HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_GRAPH_MAGIC;
        node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM] = static_cast<int32_t>(role);
    }
    return true;
}

static bool test_runtime_graph_revalidation() {
    graph_fixture planned({});
    if (halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(planned.graph).eligible_groups != 1) {
        return false;
    }
    int q_index = planned.index_of(planned.q);
    if (!halofpx_rocmfpx_mmvq_qkv_validate_marked_runtime_group(planned.graph, q_index)) {
        std::fprintf(stderr, "valid optimizer capability failed runtime graph revalidation\n");
        return false;
    }
    planned.q_reshape->op = GGML_OP_SCALE;
    if (halofpx_rocmfpx_mmvq_qkv_validate_marked_runtime_group(planned.graph, q_index)) {
        std::fprintf(stderr, "post-plan consumer mutation retained runtime authorization\n");
        return false;
    }

    graph_options invalid;
    invalid.q_bias = true;
    graph_fixture forged(invalid);
    if (!force_adjacent_forged_markers(forged)) {
        return false;
    }
    q_index = forged.index_of(forged.q);
    if (halofpx_rocmfpx_mmvq_qkv_validate_marked_runtime_group(forged.graph, q_index)) {
        std::fprintf(stderr, "forged markers authorized an invalid current graph\n");
        return false;
    }
    return true;
}

static bool test_projection_name_parser() {
    halofpx_rocmfpx_mmvq_qkv_role role = HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_INVALID;
    int layer = -1;
    if (!halofpx_rocmfpx_mmvq_qkv_parse_projection_name("Qcur-0", role, layer) ||
        role != HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_Q || layer != 0 ||
        !halofpx_rocmfpx_mmvq_qkv_parse_projection_name("Kcur-2147483647", role, layer) ||
        role != HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_K || layer != 2147483647) {
        std::fprintf(stderr, "canonical projection name was rejected\n");
        return false;
    }
    constexpr std::array<const char *, 10> rejected = {
        nullptr, "", "Qcur-", "Qcur--1", "Qcur-+1", "Qcur- 1", "Qcur-1x", "Xcur-1",
        "Qcur-2147483648", "Qcur-999999999999999999999",
    };
    for (const char * name : rejected) {
        if (halofpx_rocmfpx_mmvq_qkv_parse_projection_name(name, role, layer)) {
            std::fprintf(stderr, "noncanonical projection name passed: %s\n", name ? name : "<null>");
            return false;
        }
    }
    return true;
}

int main() {
    static_assert(halofpx_rocmfpx_mmvq_qkv_q8_reuse_build_enabled() ==
                  (HALOFPX_EXPECT_ROCMFPX_MMVQ_QKV_Q8_REUSE != 0));

    constexpr std::array<ggml_type, 4> allowed = {
        GGML_TYPE_Q2_0_ROCMFPX, GGML_TYPE_Q3_0_ROCMFPX,
        GGML_TYPE_Q6_0_ROCMFPX, GGML_TYPE_Q8_0_ROCMFPX,
    };
    for (const ggml_type type : allowed) {
        const auto contract = eligible_contract(type);
        if (!halofpx_rocmfpx_mmvq_qkv_q8_reuse_type(type) ||
            !halofpx_rocmfpx_mmvq_qkv_q8_reuse_eligible(contract) ||
            halofpx_rocmfpx_mmvq_qkv_q8_reuse_dispatch(contract) !=
                (HALOFPX_EXPECT_ROCMFPX_MMVQ_QKV_Q8_REUSE != 0)) {
            std::fprintf(stderr, "eligible ROCmFPX type %d failed strict n=1 contract\n", (int) type);
            return 1;
        }
        graph_options graph_type;
        graph_type.type_q = graph_type.type_k = graph_type.type_v = type;
        graph_fixture fixture(graph_type);
        if (halofpx_rocmfpx_mmvq_qkv_find_graph_groups(fixture.graph).size() != 1) {
            std::fprintf(stderr, "eligible ROCmFPX type %d failed graph recognition\n", (int) type);
            return 1;
        }
    }

    constexpr std::array<ggml_type, 5> rejected_types = {
        GGML_TYPE_Q4_0_ROCMFP4, GGML_TYPE_Q4_0_ROCMFP4_FAST,
        GGML_TYPE_Q4_0, GGML_TYPE_Q8_0, GGML_TYPE_F32,
    };
    for (const ggml_type type : rejected_types) {
        if (halofpx_rocmfpx_mmvq_qkv_q8_reuse_type(type) ||
            halofpx_rocmfpx_mmvq_qkv_q8_reuse_eligible(eligible_contract(type))) {
            std::fprintf(stderr, "non-whitelisted type %d passed strict contract\n", (int) type);
            return 1;
        }
    }

    auto contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
    contract.weight_type_k = GGML_TYPE_Q8_0_ROCMFPX;
    if (halofpx_rocmfpx_mmvq_qkv_q8_reuse_eligible(contract)) {
        std::fprintf(stderr, "mixed concrete ROCmFPX types passed\n");
        return 1;
    }
    for (const int64_t rows : { INT64_C(0), INT64_C(2), INT64_C(8), INT64_C(9) }) {
        contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);
        contract.activation_rows = rows;
        if (halofpx_rocmfpx_mmvq_qkv_q8_reuse_eligible(contract)) {
            std::fprintf(stderr, "non-n=1 activation row count passed: %lld\n", (long long) rows);
            return 1;
        }
    }

    const auto expect_rejected = [](halofpx_rocmfpx_mmvq_qkv_q8_reuse_contract candidate) {
        return !halofpx_rocmfpx_mmvq_qkv_q8_reuse_eligible(candidate) &&
               !halofpx_rocmfpx_mmvq_qkv_q8_reuse_dispatch(candidate);
    };
#define HALOFPX_REJECT_BOOL_FIELD(field)                                           \
    do {                                                                            \
        contract = eligible_contract(GGML_TYPE_Q6_0_ROCMFPX);                      \
        contract.field = false;                                                     \
        if (!expect_rejected(contract)) {                                           \
            std::fprintf(stderr, "selector failed false %s\n", #field);          \
            return 1;                                                               \
        }                                                                           \
    } while (false)
    HALOFPX_REJECT_BOOL_FIELD(gfx1151_hip);
    HALOFPX_REJECT_BOOL_FIELD(ordinary_separate_qkv);
    HALOFPX_REJECT_BOOL_FIELD(no_fused_wqkv);
    HALOFPX_REJECT_BOOL_FIELD(no_lora_or_extra_matmul);
    HALOFPX_REJECT_BOOL_FIELD(no_bias_clamp_or_scale);
    HALOFPX_REJECT_BOOL_FIELD(exact_shared_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_activation);
    HALOFPX_REJECT_BOOL_FIELD(f32_outputs);
    HALOFPX_REJECT_BOOL_FIELD(default_matmul_params);
    HALOFPX_REJECT_BOOL_FIELD(local_non_split);
    HALOFPX_REJECT_BOOL_FIELD(owning_contiguous_tensors);
    HALOFPX_REJECT_BOOL_FIELD(nonoverlapping_ranges);
    HALOFPX_REJECT_BOOL_FIELD(all_mmvq_eligible);
    HALOFPX_REJECT_BOOL_FIELD(single_stream);
    HALOFPX_REJECT_BOOL_FIELD(valid_independent_geometry);
    HALOFPX_REJECT_BOOL_FIELD(optimizer_marked_qkv_order);
#undef HALOFPX_REJECT_BOOL_FIELD

    graph_options negative;
    negative.rows = 2;
    if (!expect_no_group(negative, "n=2")) return 1;
    negative = {};
    negative.type_k = GGML_TYPE_Q8_0_ROCMFPX;
    if (!expect_no_group(negative, "mixed-type")) return 1;
    negative = {};
    negative.distinct_v_activation = true;
    if (!expect_no_group(negative, "distinct-activation")) return 1;
    negative = {};
    negative.q_bias = true;
    if (!expect_no_group(negative, "bias-consumer")) return 1;
    negative = {};
    negative.fourth_matmul = true;
    if (!expect_no_group(negative, "fourth-matmul-or-lora")) return 1;
    negative = {};
    negative.wrong_projection_name = true;
    if (!expect_no_group(negative, "projection-name")) return 1;
    negative = {};
    negative.duplicate_projection_role = true;
    if (!expect_no_group(negative, "duplicate-projection-role")) return 1;
    negative = {};
    negative.mixed_layer = true;
    if (!expect_no_group(negative, "mixed-layer")) return 1;
    negative = {};
    negative.wrong_weight_name = true;
    if (!expect_no_group(negative, "weight-name")) return 1;
    negative = {};
    negative.activation_view = true;
    if (!expect_no_group(negative, "activation-view")) return 1;
    negative = {};
    negative.weight_view = true;
    if (!expect_no_group(negative, "weight-view")) return 1;
    negative = {};
    negative.output_view = true;
    if (!expect_no_group(negative, "output-view")) return 1;
    negative = {};
    negative.mul_mat_id = true;
    if (!expect_no_group(negative, "mul-mat-id")) return 1;
    negative = {};
    negative.nondefault_precision = true;
    if (!expect_no_group(negative, "precision")) return 1;
    negative = {};
    negative.nondefault_hint = true;
    if (!expect_no_group(negative, "hint")) return 1;
    negative = {};
    negative.crossed_activation_write = true;
    if (!expect_no_group(negative, "crossed-activation-write")) return 1;
    negative = {};
    negative.crossed_weight_write = true;
    if (!expect_no_group(negative, "crossed-weight-write")) return 1;
    negative = {};
    negative.crossed_output_write = true;
    if (!expect_no_group(negative, "crossed-output-write")) return 1;
    negative = {};
    negative.nested_crossed_output_write = true;
    if (!expect_no_group(negative, "nested-crossed-output-write")) return 1;
    negative = {};
    negative.wrong_role_order = true;
    if (!expect_no_group(negative, "wrong-QKV-order")) return 1;

    graph_options gqa;
    graph_options mha;
    mha.kv_width = mha.q_width;
    if (!test_graph_reorder(gqa, "GQA") || !test_graph_reorder(mha, "MHA") ||
        !test_build_and_arch_gate() || !test_stale_marker_revalidation() ||
        !test_runtime_graph_revalidation() || !test_projection_name_parser()) {
        return 1;
    }

    std::printf("ROCmFPX MMVQ QKV Q8 reuse build=%s n=1 whitelist=4 GQA=accepted\n",
                halofpx_rocmfpx_mmvq_qkv_q8_reuse_build_enabled() ? "on" : "off");
    return 0;
}
