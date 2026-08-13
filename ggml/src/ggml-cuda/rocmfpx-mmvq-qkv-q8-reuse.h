#pragma once

#include "ggml.h"
#include "ggml-impl.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <unordered_map>
#include <vector>

// Host-visible admission and graph-order contract for the default-off,
// single-token ROCmFPX Q/K/V MMVQ activation-reuse experiment.  This header
// deliberately has no HIP dependency so the fail-closed selector and the
// pre-allocation graph planner can be tested in ordinary host CI.
struct halofpx_rocmfpx_mmvq_qkv_q8_reuse_contract {
    ggml_type weight_type_q;
    ggml_type weight_type_k;
    ggml_type weight_type_v;
    int64_t   activation_rows;
    bool      gfx1151_hip;
    bool      ordinary_separate_qkv;
    bool      no_fused_wqkv;
    bool      no_lora_or_extra_matmul;
    bool      no_bias_clamp_or_scale;
    bool      exact_shared_activation;
    bool      f32_activation;
    bool      f32_outputs;
    bool      default_matmul_params;
    bool      local_non_split;
    bool      owning_contiguous_tensors;
    bool      nonoverlapping_ranges;
    bool      all_mmvq_eligible;
    bool      single_stream;
    bool      valid_independent_geometry;
    bool      optimizer_marked_qkv_order;
};

struct halofpx_rocmfpx_mmvq_qkv_q8_reuse_metrics_v1 {
    uint32_t struct_size;
    uint32_t version;
    uint64_t graph_groups_planned;
    uint64_t triple_dispatches;
    uint64_t q8_conversions_submitted;
    uint64_t mmvq_submissions;
};

static constexpr uint32_t HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_METRICS_VERSION = 1;
static constexpr int32_t  HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_GRAPH_MAGIC = 0x484d5131; // "HMQ1"

// MUL_MAT currently owns parameter slots 0 and 1.  Slots 12 and 13 are a
// capability published only by this optimizer; they intentionally do not
// collide with the unmerged prompt/MMQ prototype's slots 14 and 15.
static constexpr size_t HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM = 12;
static constexpr size_t HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM  = 13;

enum halofpx_rocmfpx_mmvq_qkv_role : int32_t {
    HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_Q = 0,
    HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_K = 1,
    HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_V = 2,
    HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_INVALID = 3,
};

static constexpr bool halofpx_rocmfpx_mmvq_qkv_q8_reuse_type(const ggml_type type) {
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

static constexpr bool halofpx_rocmfpx_mmvq_qkv_q8_reuse_compatible_types(
        const ggml_type q,
        const ggml_type k,
        const ggml_type v) {
    // Equal concrete types are required even though all four current kernels
    // consume Q8_1 activations.  This keeps the conversion recipe invariant
    // explicit and makes mixed recipes a conservative miss.
    return halofpx_rocmfpx_mmvq_qkv_q8_reuse_type(q) && q == k && q == v;
}

static constexpr bool halofpx_rocmfpx_mmvq_qkv_q8_reuse_build_enabled() {
#if defined(GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE)
    return true;
#else
    return false;
#endif
}

static constexpr bool halofpx_rocmfpx_mmvq_qkv_q8_reuse_eligible(
        const halofpx_rocmfpx_mmvq_qkv_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_mmvq_qkv_q8_reuse_compatible_types(
               contract.weight_type_q, contract.weight_type_k, contract.weight_type_v) &&
           contract.activation_rows == 1 &&
           contract.gfx1151_hip &&
           contract.ordinary_separate_qkv &&
           contract.no_fused_wqkv &&
           contract.no_lora_or_extra_matmul &&
           contract.no_bias_clamp_or_scale &&
           contract.exact_shared_activation &&
           contract.f32_activation &&
           contract.f32_outputs &&
           contract.default_matmul_params &&
           contract.local_non_split &&
           contract.owning_contiguous_tensors &&
           contract.nonoverlapping_ranges &&
           contract.all_mmvq_eligible &&
           contract.single_stream &&
           contract.valid_independent_geometry &&
           contract.optimizer_marked_qkv_order;
}

static constexpr bool halofpx_rocmfpx_mmvq_qkv_q8_reuse_dispatch(
        const halofpx_rocmfpx_mmvq_qkv_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_mmvq_qkv_q8_reuse_build_enabled() &&
           halofpx_rocmfpx_mmvq_qkv_q8_reuse_eligible(contract);
}

struct halofpx_rocmfpx_mmvq_qkv_graph_group {
    std::array<ggml_tensor *, 3> nodes = { nullptr, nullptr, nullptr };
    std::array<int, 3> indices = { -1, -1, -1 };
    ggml_tensor * activation = nullptr;
    int layer = -1;
};

struct halofpx_rocmfpx_mmvq_qkv_graph_plan_result {
    size_t eligible_groups = 0;
    size_t moved_groups = 0;
};

static inline bool halofpx_rocmfpx_mmvq_qkv_parse_projection_name(
        const char * name,
        halofpx_rocmfpx_mmvq_qkv_role & role,
        int & layer) {
    if (!name) {
        return false;
    }
    struct prefix_role {
        const char * prefix;
        halofpx_rocmfpx_mmvq_qkv_role role;
    };
    static constexpr std::array<prefix_role, 3> prefixes = {{
        { "Qcur-", HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_Q },
        { "Kcur-", HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_K },
        { "Vcur-", HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_V },
    }};

    for (const auto & candidate : prefixes) {
        const size_t prefix_length = std::strlen(candidate.prefix);
        if (std::strncmp(name, candidate.prefix, prefix_length) != 0) {
            continue;
        }
        const char * digit = name + prefix_length;
        if (*digit < '0' || *digit > '9') {
            return false;
        }
        int parsed = 0;
        for (; *digit != '\0'; ++digit) {
            if (*digit < '0' || *digit > '9') {
                return false;
            }
            const int value = *digit - '0';
            if (parsed > (std::numeric_limits<int>::max() - value) / 10) {
                return false;
            }
            parsed = parsed * 10 + value;
        }
        role = candidate.role;
        layer = parsed;
        return true;
    }
    return false;
}

static inline bool halofpx_rocmfpx_mmvq_qkv_weight_matches(
        const ggml_tensor * weight,
        const halofpx_rocmfpx_mmvq_qkv_role role,
        const int layer) {
    static constexpr std::array<const char *, 3> names = {
        "attn_q", "attn_k", "attn_v",
    };
    if (!weight || role >= HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_INVALID) {
        return false;
    }
    char expected[GGML_MAX_NAME];
    const int written = std::snprintf(expected, sizeof(expected), "blk.%d.%s.weight", layer, names[role]);
    return written > 0 && static_cast<size_t>(written) < sizeof(expected) &&
           std::strcmp(weight->name, expected) == 0;
}

static inline bool halofpx_rocmfpx_mmvq_qkv_is_ordinary_mul_mat(const ggml_tensor * node) {
    if (!node || node->op != GGML_OP_MUL_MAT || !node->src[0] || !node->src[1]) {
        return false;
    }
    for (int i = 2; i < GGML_MAX_SRC; ++i) {
        if (node->src[i] != nullptr) {
            return false;
        }
    }
    return true;
}

static inline bool halofpx_rocmfpx_mmvq_qkv_geometry_ok(
        const ggml_tensor * node,
        const ggml_tensor * activation) {
    if (!node || !activation || !node->src[0]) {
        return false;
    }
    const ggml_tensor * weight = node->src[0];
    return activation->ne[0] > 0 && ggml_nrows(activation) == 1 &&
           weight->ne[0] == activation->ne[0] && weight->ne[1] > 0 &&
           weight->ne[2] == 1 && weight->ne[3] == 1 &&
           node->ne[0] == weight->ne[1] && ggml_nrows(node) == 1;
}

static inline bool halofpx_rocmfpx_mmvq_qkv_owned_marker_is_clear(const ggml_tensor * node) {
    return node &&
           node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] == 0 &&
           node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM] == 0;
}

static inline void halofpx_rocmfpx_mmvq_qkv_clear_owned_markers(ggml_cgraph * cgraph) {
    if (!cgraph) {
        return;
    }
    for (int i = 0; i < cgraph->n_nodes; ++i) {
        ggml_tensor * node = cgraph->nodes[i];
        if (node && node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] ==
                        HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_GRAPH_MAGIC) {
            node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] = 0;
            node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM] = 0;
        }
    }
}

// Return the ultimate storage owner for a view chain. Cycles are malformed
// tensor metadata and conservatively have no usable root.
static inline const ggml_tensor * halofpx_rocmfpx_mmvq_qkv_view_root(const ggml_tensor * tensor) {
    const ggml_tensor * slow = tensor;
    const ggml_tensor * fast = tensor;
    while (fast && fast->view_src) {
        slow = slow ? slow->view_src : nullptr;
        fast = fast->view_src;
        fast = fast ? fast->view_src : nullptr;
        if (slow && fast && slow == fast) {
            return nullptr;
        }
    }
    while (tensor && tensor->view_src) {
        tensor = tensor->view_src;
    }
    return tensor;
}

// Moving K/V before intervening metadata nodes must not cross an in-place
// write to an input or output allocation.  Such a write can change semantics
// despite a superficially valid source DAG, so the entire group becomes a
// miss.  Empty metadata views are harmless and remain in stable order.
static inline bool halofpx_rocmfpx_mmvq_qkv_crossed_writes_are_safe(
        const ggml_cgraph * cgraph,
        const halofpx_rocmfpx_mmvq_qkv_graph_group & group) {
    const int earliest = group.indices[HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_Q];
    const int latest   = group.indices[HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_V];
    const std::array<const ggml_tensor *, 7> protected_roots = {
        group.activation,
        group.nodes[0]->src[0], group.nodes[1]->src[0], group.nodes[2]->src[0],
        group.nodes[0], group.nodes[1], group.nodes[2],
    };

    for (int i = earliest + 1; i < latest; ++i) {
        const ggml_tensor * crossed = cgraph->nodes[i];
        if (!crossed || ggml_op_is_empty(crossed->op) || crossed->view_src == nullptr) {
            continue;
        }
        const ggml_tensor * crossed_root = halofpx_rocmfpx_mmvq_qkv_view_root(crossed);
        if (!crossed_root) {
            return false;
        }
        for (const ggml_tensor * protected_tensor : protected_roots) {
            const ggml_tensor * protected_root = halofpx_rocmfpx_mmvq_qkv_view_root(protected_tensor);
            if (!protected_root || protected_root == crossed_root) {
                return false;
            }
        }
    }
    return true;
}

// Revalidate the complete read-only graph contract at execution time. The
// op_params capability is necessary but never sufficient: a reused or
// mutated graph must still be exactly the safe adjacent Q/K/V group that the
// optimizer was allowed to publish.
static inline bool halofpx_rocmfpx_mmvq_qkv_validate_marked_runtime_group(
        const ggml_cgraph * cgraph,
        const int q_index) {
    if (!cgraph || q_index < 0 || q_index + 2 >= cgraph->n_nodes) {
        return false;
    }

    halofpx_rocmfpx_mmvq_qkv_graph_group group;
    group.indices = { q_index, q_index + 1, q_index + 2 };
    group.nodes = {
        cgraph->nodes[q_index], cgraph->nodes[q_index + 1], cgraph->nodes[q_index + 2],
    };
    if (!group.nodes[0] || !group.nodes[1] || !group.nodes[2] ||
        group.nodes[0] == group.nodes[1] || group.nodes[0] == group.nodes[2] ||
        group.nodes[1] == group.nodes[2]) {
        return false;
    }

    const ggml_tensor * activation = group.nodes[0]->src[1];
    if (!activation || activation->type != GGML_TYPE_F32 || ggml_nrows(activation) != 1 ||
        activation->view_src || !ggml_is_contiguous(activation)) {
        return false;
    }
    group.activation = const_cast<ggml_tensor *>(activation);

    for (size_t role_index = 0; role_index < group.nodes.size(); ++role_index) {
        ggml_tensor * node = group.nodes[role_index];
        halofpx_rocmfpx_mmvq_qkv_role parsed_role = HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_INVALID;
        int parsed_layer = -1;
        if (!halofpx_rocmfpx_mmvq_qkv_is_ordinary_mul_mat(node) || node->src[1] != activation ||
            node->type != GGML_TYPE_F32 || node->view_src || !ggml_is_contiguous(node) ||
            node->src[0]->view_src || !ggml_is_contiguous(node->src[0]) ||
            ggml_get_op_params_i32(node, 0) != GGML_PREC_DEFAULT ||
            ggml_get_op_params_i32(node, 1) != GGML_HINT_NONE ||
            node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] !=
                HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_GRAPH_MAGIC ||
            node->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM] !=
                static_cast<int32_t>(role_index) ||
            !halofpx_rocmfpx_mmvq_qkv_parse_projection_name(node->name, parsed_role, parsed_layer) ||
            parsed_role != static_cast<halofpx_rocmfpx_mmvq_qkv_role>(role_index) ||
            !halofpx_rocmfpx_mmvq_qkv_weight_matches(node->src[0], parsed_role, parsed_layer) ||
            !halofpx_rocmfpx_mmvq_qkv_geometry_ok(node, activation) ||
            (group.layer != -1 && group.layer != parsed_layer)) {
            return false;
        }
        group.layer = parsed_layer;
    }

    if (group.nodes[0]->src[0] == group.nodes[1]->src[0] ||
        group.nodes[0]->src[0] == group.nodes[2]->src[0] ||
        group.nodes[1]->src[0] == group.nodes[2]->src[0] ||
        !halofpx_rocmfpx_mmvq_qkv_q8_reuse_compatible_types(
            group.nodes[0]->src[0]->type,
            group.nodes[1]->src[0]->type,
            group.nodes[2]->src[0]->type)) {
        return false;
    }

    size_t shared_activation_matmuls = 0;
    std::array<int, 3> direct_consumers = { 0, 0, 0 };
    for (int i = 0; i < cgraph->n_nodes; ++i) {
        const ggml_tensor * candidate = cgraph->nodes[i];
        if (!candidate) {
            return false;
        }
        if ((candidate->op == GGML_OP_MUL_MAT || candidate->op == GGML_OP_MUL_MAT_ID) &&
            candidate->src[1] == activation) {
            ++shared_activation_matmuls;
        }
        for (size_t role_index = 0; role_index < group.nodes.size(); ++role_index) {
            bool uses_projection = false;
            for (const ggml_tensor * src : candidate->src) {
                uses_projection = uses_projection || src == group.nodes[role_index];
            }
            if (!uses_projection) {
                continue;
            }
            ++direct_consumers[role_index];
            if (candidate->op != GGML_OP_RESHAPE || candidate->src[0] != group.nodes[role_index]) {
                return false;
            }
        }
    }
    if (shared_activation_matmuls != 3 || direct_consumers[0] != 1 ||
        direct_consumers[1] != 1 || direct_consumers[2] != 1) {
        return false;
    }

    // Every graph-resident source must precede Q so compaction cannot move a
    // projection before one of its producers.
    for (const ggml_tensor * node : group.nodes) {
        for (const ggml_tensor * src : node->src) {
            if (!src) {
                continue;
            }
            for (int i = q_index; i < cgraph->n_nodes; ++i) {
                if (cgraph->nodes[i] == src) {
                    return false;
                }
            }
        }
    }
    return halofpx_rocmfpx_mmvq_qkv_crossed_writes_are_safe(cgraph, group);
}

static inline std::vector<halofpx_rocmfpx_mmvq_qkv_graph_group>
halofpx_rocmfpx_mmvq_qkv_find_graph_groups(const ggml_cgraph * cgraph) {
    std::vector<halofpx_rocmfpx_mmvq_qkv_graph_group> groups;
    if (!cgraph || cgraph->n_nodes < 3) {
        return groups;
    }

    std::unordered_map<const ggml_tensor *, int> node_indices;
    std::unordered_map<const ggml_tensor *, std::vector<int>> matmuls_by_activation;
    node_indices.reserve(static_cast<size_t>(cgraph->n_nodes));
    matmuls_by_activation.reserve(static_cast<size_t>(cgraph->n_nodes));

    for (int i = 0; i < cgraph->n_nodes; ++i) {
        const ggml_tensor * node = cgraph->nodes[i];
        if (!node) {
            continue;
        }
        node_indices[node] = i;
        if ((node->op == GGML_OP_MUL_MAT || node->op == GGML_OP_MUL_MAT_ID) && node->src[1]) {
            matmuls_by_activation[node->src[1]].push_back(i);
        }
    }

    for (const auto & entry : matmuls_by_activation) {
        const ggml_tensor * activation = entry.first;
        const std::vector<int> & indices = entry.second;
        if (indices.size() != 3 || !activation || activation->type != GGML_TYPE_F32 ||
            ggml_nrows(activation) != 1 || activation->view_src || !ggml_is_contiguous(activation)) {
            continue;
        }

        halofpx_rocmfpx_mmvq_qkv_graph_group group;
        group.activation = const_cast<ggml_tensor *>(activation);
        bool valid = true;
        for (const int index : indices) {
            ggml_tensor * node = cgraph->nodes[index];
            halofpx_rocmfpx_mmvq_qkv_role role = HALOFPX_ROCMFPX_MMVQ_QKV_ROLE_INVALID;
            int layer = -1;
            if (!halofpx_rocmfpx_mmvq_qkv_is_ordinary_mul_mat(node) || node->src[1] != activation ||
                node->type != GGML_TYPE_F32 || node->view_src || !ggml_is_contiguous(node) ||
                node->src[0]->view_src || !ggml_is_contiguous(node->src[0]) ||
                ggml_get_op_params_i32(node, 0) != GGML_PREC_DEFAULT ||
                ggml_get_op_params_i32(node, 1) != GGML_HINT_NONE ||
                !halofpx_rocmfpx_mmvq_qkv_owned_marker_is_clear(node) ||
                !halofpx_rocmfpx_mmvq_qkv_parse_projection_name(node->name, role, layer) ||
                !halofpx_rocmfpx_mmvq_qkv_weight_matches(node->src[0], role, layer) ||
                !halofpx_rocmfpx_mmvq_qkv_geometry_ok(node, activation) ||
                group.nodes[role] != nullptr || (group.layer != -1 && group.layer != layer)) {
                valid = false;
                break;
            }
            group.layer = layer;
            group.nodes[role] = node;
            group.indices[role] = index;
        }
        if (!valid || std::any_of(group.nodes.begin(), group.nodes.end(), [](const ggml_tensor * node) {
                return node == nullptr;
            })) {
            continue;
        }

        // Preserve the model graph's semantic projection order.  The planner
        // compacts metadata gaps only; it never turns K/V/Q into Q/K/V.
        if (!(group.indices[0] < group.indices[1] && group.indices[1] < group.indices[2])) {
            continue;
        }
        if (group.nodes[0]->src[0] == group.nodes[1]->src[0] ||
            group.nodes[0]->src[0] == group.nodes[2]->src[0] ||
            group.nodes[1]->src[0] == group.nodes[2]->src[0]) {
            continue;
        }
        if (!halofpx_rocmfpx_mmvq_qkv_q8_reuse_compatible_types(
                group.nodes[0]->src[0]->type,
                group.nodes[1]->src[0]->type,
                group.nodes[2]->src[0]->type)) {
            continue;
        }

        const int earliest = group.indices[0];
        for (const ggml_tensor * candidate : group.nodes) {
            for (const ggml_tensor * src : candidate->src) {
                if (!src) {
                    continue;
                }
                const auto found = node_indices.find(src);
                if (found != node_indices.end() && found->second >= earliest) {
                    valid = false;
                    break;
                }
            }
            if (!valid) {
                break;
            }
        }
        if (!valid) {
            continue;
        }

        // Generic separate build_qkv has one direct reshape per raw Q/K/V.
        // Bias, clamp, scale and custom normalized-Q/K paths therefore miss.
        // A fourth activation matmul (including LoRA's low-rank projection)
        // already failed the exact-three grouping above.
        std::array<int, 3> direct_consumers = { 0, 0, 0 };
        for (int i = 0; i < cgraph->n_nodes && valid; ++i) {
            const ggml_tensor * consumer = cgraph->nodes[i];
            if (!consumer) {
                continue;
            }
            for (size_t role = 0; role < group.nodes.size(); ++role) {
                bool uses_projection = false;
                for (const ggml_tensor * src : consumer->src) {
                    uses_projection = uses_projection || src == group.nodes[role];
                }
                if (!uses_projection) {
                    continue;
                }
                ++direct_consumers[role];
                if (consumer->op != GGML_OP_RESHAPE || consumer->src[0] != group.nodes[role]) {
                    valid = false;
                    break;
                }
            }
        }
        valid = valid && direct_consumers[0] == 1 && direct_consumers[1] == 1 && direct_consumers[2] == 1;
        valid = valid && halofpx_rocmfpx_mmvq_qkv_crossed_writes_are_safe(cgraph, group);
        if (valid) {
            groups.push_back(group);
        }
    }

    std::sort(groups.begin(), groups.end(), [](const auto & a, const auto & b) {
        return a.indices[0] < b.indices[0];
    });
    return groups;
}

static inline bool halofpx_rocmfpx_mmvq_qkv_graph_order_is_topological(
        const std::vector<ggml_tensor *> & nodes) {
    std::unordered_map<const ggml_tensor *, size_t> indices;
    indices.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (!nodes[i] || !indices.emplace(nodes[i], i).second) {
            return false;
        }
    }
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (const ggml_tensor * src : nodes[i]->src) {
            const auto found = indices.find(src);
            if (found != indices.end() && found->second >= i) {
                return false;
            }
        }
    }
    return true;
}

static inline halofpx_rocmfpx_mmvq_qkv_graph_plan_result
halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(ggml_cgraph * cgraph) {
    halofpx_rocmfpx_mmvq_qkv_graph_plan_result result;
    if (!cgraph) {
        return result;
    }

    // Remove only capabilities previously owned by this feature before
    // revalidating a reused graph.  A failed new plan can never retain a stale
    // dispatch authorization.
    halofpx_rocmfpx_mmvq_qkv_clear_owned_markers(cgraph);
    const auto groups = halofpx_rocmfpx_mmvq_qkv_find_graph_groups(cgraph);
    if (groups.empty()) {
        return result;
    }

    std::unordered_map<const ggml_tensor *, size_t> member_group;
    std::unordered_map<int, size_t> start_group;
    member_group.reserve(groups.size() * 3);
    start_group.reserve(groups.size());
    for (size_t group_index = 0; group_index < groups.size(); ++group_index) {
        const auto & group = groups[group_index];
        if (!start_group.emplace(group.indices[0], group_index).second) {
            return {};
        }
        for (const ggml_tensor * node : group.nodes) {
            if (!member_group.emplace(node, group_index).second) {
                return {};
            }
        }
    }

    // Construct and validate the complete candidate order before publishing
    // either graph slots or marker capabilities.
    std::vector<ggml_tensor *> reordered;
    reordered.reserve(static_cast<size_t>(cgraph->n_nodes));
    for (int i = 0; i < cgraph->n_nodes; ++i) {
        const auto start = start_group.find(i);
        if (start != start_group.end()) {
            const auto & group = groups[start->second];
            reordered.insert(reordered.end(), group.nodes.begin(), group.nodes.end());
        }
        if (member_group.find(cgraph->nodes[i]) == member_group.end()) {
            reordered.push_back(cgraph->nodes[i]);
        }
    }
    if (reordered.size() != static_cast<size_t>(cgraph->n_nodes) ||
        !halofpx_rocmfpx_mmvq_qkv_graph_order_is_topological(reordered)) {
        return {};
    }

    result.eligible_groups = groups.size();
    for (const auto & group : groups) {
        if (group.indices[1] != group.indices[0] + 1 || group.indices[2] != group.indices[0] + 2) {
            ++result.moved_groups;
        }
    }
    std::copy(reordered.begin(), reordered.end(), cgraph->nodes);
    for (const auto & group : groups) {
        for (size_t role = 0; role < group.nodes.size(); ++role) {
            group.nodes[role]->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_MAGIC_PARAM] =
                HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_GRAPH_MAGIC;
            group.nodes[role]->op_params[HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE_ROLE_PARAM] =
                static_cast<int32_t>(role);
        }
    }
    return result;
}

static inline halofpx_rocmfpx_mmvq_qkv_graph_plan_result
halofpx_rocmfpx_mmvq_qkv_dispatch_graph_reorder(ggml_cgraph * cgraph, const bool gfx1151_hip) {
    if (!halofpx_rocmfpx_mmvq_qkv_q8_reuse_build_enabled() || !gfx1151_hip) {
        return {};
    }
    return halofpx_rocmfpx_mmvq_qkv_plan_graph_reorder(cgraph);
}
