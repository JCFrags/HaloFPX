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

// Host-visible selector and graph-order contract for the default-off Q/K/V
// activation reuse experiment.  Keeping this independent of HIP headers lets
// the exact admission and pre-allocation reorder run in ordinary CI.
struct halofpx_rocmfpx_qkv_q8_reuse_contract {
    ggml_type weight_type_q;
    ggml_type weight_type_k;
    ggml_type weight_type_v;
    int64_t   activation_columns;
    bool      gfx1151_hip;
    bool      ordinary_separate_qkv;
    bool      no_fused_wqkv;
    bool      no_lora;
    bool      no_bias_or_clamp;
    bool      exact_shared_activation;
    bool      f32_activation;
    bool      f32_outputs;
    bool      default_matmul_params;
    bool      local_non_split;
    bool      safe_allocation_views;
    bool      nonoverlapping_outputs;
    bool      all_mmq_eligible;
    bool      single_stream;
    bool      valid_independent_geometry;
    bool      reordered_adjacent;
};

struct halofpx_rocmfpx_qkv_q8_reuse_metrics_v1 {
    uint32_t struct_size;
    uint32_t version;
    uint64_t graph_groups_planned;
    uint64_t triple_dispatches;
    uint64_t q8_conversions_submitted;
    uint64_t mmq_submissions;
};

static constexpr uint32_t HALOFPX_ROCMFPX_QKV_Q8_REUSE_METRICS_VERSION = 1;
static constexpr int32_t HALOFPX_ROCMFPX_QKV_Q8_REUSE_GRAPH_MAGIC = 0x48515631; // "HQV1"
static constexpr size_t HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM = 14;
static constexpr size_t HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM = 15;

static constexpr bool halofpx_rocmfpx_qkv_q8_reuse_type(const ggml_type type) {
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

static constexpr bool halofpx_rocmfpx_qkv_q8_reuse_compatible_layouts(
        const ggml_type q,
        const ggml_type k,
        const ggml_type v) {
    // Equal concrete types make the type-selected Q8_1 layout an explicit
    // invariant.  Mixed ROCmFPX recipes remain a conservative fallback even
    // though the current four kernels all select D4.
    return halofpx_rocmfpx_qkv_q8_reuse_type(q) && q == k && q == v;
}

static constexpr bool halofpx_rocmfpx_qkv_q8_reuse_build_enabled() {
#if defined(GGML_HIP_ROCMFPX_QKV_Q8_REUSE)
    return true;
#else
    return false;
#endif
}

static constexpr bool halofpx_rocmfpx_qkv_q8_reuse_eligible(
        const halofpx_rocmfpx_qkv_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_qkv_q8_reuse_compatible_layouts(
               contract.weight_type_q, contract.weight_type_k, contract.weight_type_v) &&
           contract.activation_columns > 8 &&
           contract.gfx1151_hip &&
           contract.ordinary_separate_qkv &&
           contract.no_fused_wqkv &&
           contract.no_lora &&
           contract.no_bias_or_clamp &&
           contract.exact_shared_activation &&
           contract.f32_activation &&
           contract.f32_outputs &&
           contract.default_matmul_params &&
           contract.local_non_split &&
           contract.safe_allocation_views &&
           contract.nonoverlapping_outputs &&
           contract.all_mmq_eligible &&
           contract.single_stream &&
           contract.valid_independent_geometry &&
           contract.reordered_adjacent;
}

static constexpr bool halofpx_rocmfpx_qkv_q8_reuse_dispatch(
        const halofpx_rocmfpx_qkv_q8_reuse_contract & contract) {
    return halofpx_rocmfpx_qkv_q8_reuse_build_enabled() &&
           halofpx_rocmfpx_qkv_q8_reuse_eligible(contract);
}

enum halofpx_rocmfpx_qkv_role : uint8_t {
    HALOFPX_ROCMFPX_QKV_ROLE_Q = 0,
    HALOFPX_ROCMFPX_QKV_ROLE_K = 1,
    HALOFPX_ROCMFPX_QKV_ROLE_V = 2,
    HALOFPX_ROCMFPX_QKV_ROLE_INVALID = 3,
};

struct halofpx_rocmfpx_qkv_graph_group {
    std::array<ggml_tensor *, 3> nodes = { nullptr, nullptr, nullptr };
    std::array<int, 3> indices = { -1, -1, -1 };
    ggml_tensor * activation = nullptr;
    int layer = -1;
};

struct halofpx_rocmfpx_qkv_graph_reorder_result {
    size_t eligible_groups = 0;
    size_t moved_groups = 0;
};

static inline bool halofpx_rocmfpx_qkv_parse_projection_name(
        const char * name,
        halofpx_rocmfpx_qkv_role & role,
        int & layer) {
    if (!name) {
        return false;
    }
    struct prefix_role {
        const char * prefix;
        halofpx_rocmfpx_qkv_role role;
    };
    static constexpr std::array<prefix_role, 3> prefixes = {{
        { "Qcur-", HALOFPX_ROCMFPX_QKV_ROLE_Q },
        { "Kcur-", HALOFPX_ROCMFPX_QKV_ROLE_K },
        { "Vcur-", HALOFPX_ROCMFPX_QKV_ROLE_V },
    }};

    for (const auto & candidate : prefixes) {
        const size_t prefix_len = std::strlen(candidate.prefix);
        if (std::strncmp(name, candidate.prefix, prefix_len) != 0) {
            continue;
        }
        const char * digit = name + prefix_len;
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

static inline bool halofpx_rocmfpx_qkv_weight_matches(
        const ggml_tensor * weight,
        const halofpx_rocmfpx_qkv_role role,
        const int layer) {
    static constexpr std::array<const char *, 3> names = {
        "attn_q", "attn_k", "attn_v",
    };
    if (!weight || role >= HALOFPX_ROCMFPX_QKV_ROLE_INVALID) {
        return false;
    }
    char expected[GGML_MAX_NAME];
    const int written = std::snprintf(expected, sizeof(expected), "blk.%d.%s.weight", layer, names[role]);
    return written > 0 && static_cast<size_t>(written) < sizeof(expected) &&
           std::strcmp(weight->name, expected) == 0;
}

static inline bool halofpx_rocmfpx_qkv_direct_geometry_ok(
        const ggml_tensor * node,
        const ggml_tensor * activation) {
    if (!node || !activation || !node->src[0]) {
        return false;
    }
    const ggml_tensor * weight = node->src[0];
    return weight->ne[2] > 0 && weight->ne[3] > 0 &&
           weight->ne[0] == activation->ne[0] &&
           node->ne[0] == weight->ne[1] &&
           node->ne[1] == activation->ne[1] &&
           node->ne[2] == activation->ne[2] / weight->ne[2] &&
           node->ne[3] == activation->ne[3] / weight->ne[3] &&
           activation->ne[2] % weight->ne[2] == 0 &&
           activation->ne[3] % weight->ne[3] == 0;
}

static inline bool halofpx_rocmfpx_qkv_is_ordinary_mul_mat(const ggml_tensor * node) {
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

// Moving a projection before an intervening in-place write can change the
// bytes that it reads even when the tensor dependency graph remains
// topologically valid. ggml represents an in-place result as a view of the
// allocation that it writes, and ggml_new_tensor_impl canonicalizes nested
// views to their owning root. Refuse the whole semantic group if compaction
// would cross a non-metadata write to the activation or any projection weight.
static inline bool halofpx_rocmfpx_qkv_crossed_writes_are_safe(
        const ggml_cgraph * cgraph,
        const halofpx_rocmfpx_qkv_graph_group & group) {
    const int earliest = *std::min_element(group.indices.begin(), group.indices.end());
    const int latest = *std::max_element(group.indices.begin(), group.indices.end());
    const std::array<const ggml_tensor *, 4> protected_roots = {
        group.activation,
        group.nodes[HALOFPX_ROCMFPX_QKV_ROLE_Q]->src[0],
        group.nodes[HALOFPX_ROCMFPX_QKV_ROLE_K]->src[0],
        group.nodes[HALOFPX_ROCMFPX_QKV_ROLE_V]->src[0],
    };

    for (int i = earliest + 1; i < latest; ++i) {
        const ggml_tensor * crossed = cgraph->nodes[i];
        if (!crossed || ggml_op_is_empty(crossed->op) || crossed->view_src == nullptr) {
            continue;
        }
        if (std::find(protected_roots.begin(), protected_roots.end(), crossed->view_src) !=
                protected_roots.end()) {
            return false;
        }
    }
    return true;
}

// Finds exact, ordinary, unscaled Q/K/V projection groups.  This is semantic
// recognition only: allocation locality, pointer ranges, MMQ selection and the
// exact gfx1151 runtime are checked again at dispatch.
static inline std::vector<halofpx_rocmfpx_qkv_graph_group>
halofpx_rocmfpx_qkv_find_graph_groups(const ggml_cgraph * cgraph) {
    std::vector<halofpx_rocmfpx_qkv_graph_group> groups;
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
            activation->ne[1] <= 8 || activation->view_src || !ggml_is_contiguous(activation)) {
            continue;
        }

        halofpx_rocmfpx_qkv_graph_group group;
        group.activation = const_cast<ggml_tensor *>(activation);
        bool valid = true;
        for (const int index : indices) {
            ggml_tensor * node = cgraph->nodes[index];
            halofpx_rocmfpx_qkv_role role = HALOFPX_ROCMFPX_QKV_ROLE_INVALID;
            int layer = -1;
            if (!halofpx_rocmfpx_qkv_is_ordinary_mul_mat(node) || node->src[1] != activation ||
                node->type != GGML_TYPE_F32 || node->view_src || !ggml_is_contiguous(node) ||
                !node->src[0] || node->src[0]->view_src || !ggml_is_contiguous(node->src[0]) ||
                node->op_params[0] != GGML_PREC_DEFAULT || node->op_params[1] != GGML_HINT_NONE ||
                !halofpx_rocmfpx_qkv_parse_projection_name(node->name, role, layer) ||
                !halofpx_rocmfpx_qkv_weight_matches(node->src[0], role, layer) ||
                !halofpx_rocmfpx_qkv_direct_geometry_ok(node, activation) ||
                group.nodes[role] != nullptr ||
                (group.layer != -1 && group.layer != layer)) {
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

        const ggml_type type = group.nodes[0]->src[0]->type;
        if (!halofpx_rocmfpx_qkv_q8_reuse_compatible_layouts(
                type, group.nodes[1]->src[0]->type, group.nodes[2]->src[0]->type)) {
            continue;
        }

        const int earliest = *std::min_element(group.indices.begin(), group.indices.end());
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

        // Bias and clamp nodes are explicit direct consumers in build_qkv.
        // LoRA and tensor-scale paths fail earlier because they add extra
        // activation matmuls or leave the named node as ADD/MUL, respectively.
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
        valid = valid && halofpx_rocmfpx_qkv_crossed_writes_are_safe(cgraph, group);
        if (valid) {
            groups.push_back(group);
        }
    }

    std::sort(groups.begin(), groups.end(), [](const auto & a, const auto & b) {
        return *std::min_element(a.indices.begin(), a.indices.end()) <
               *std::min_element(b.indices.begin(), b.indices.end());
    });
    return groups;
}

static inline halofpx_rocmfpx_qkv_graph_reorder_result
halofpx_rocmfpx_qkv_plan_graph_reorder(ggml_cgraph * cgraph) {
    halofpx_rocmfpx_qkv_graph_reorder_result result;
    const auto groups = halofpx_rocmfpx_qkv_find_graph_groups(cgraph);
    result.eligible_groups = groups.size();
    if (groups.empty()) {
        return result;
    }

    std::unordered_map<const ggml_tensor *, size_t> member_group;
    std::unordered_map<int, size_t> start_group;
    member_group.reserve(groups.size() * 3);
    start_group.reserve(groups.size());

    for (size_t group_index = 0; group_index < groups.size(); ++group_index) {
        const auto & group = groups[group_index];
        const int earliest = *std::min_element(group.indices.begin(), group.indices.end());
        start_group.emplace(earliest, group_index);
        for (const ggml_tensor * node : group.nodes) {
            member_group.emplace(node, group_index);
        }
        for (size_t role = 0; role < group.nodes.size(); ++role) {
            group.nodes[role]->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_MAGIC_PARAM] =
                HALOFPX_ROCMFPX_QKV_Q8_REUSE_GRAPH_MAGIC;
            group.nodes[role]->op_params[HALOFPX_ROCMFPX_QKV_Q8_REUSE_ROLE_PARAM] =
                static_cast<int32_t>(role);
        }
        if (group.indices[0] != earliest || group.indices[1] != earliest + 1 ||
            group.indices[2] != earliest + 2) {
            ++result.moved_groups;
        }
    }

    if (result.moved_groups == 0) {
        return result;
    }

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
    if (reordered.size() != static_cast<size_t>(cgraph->n_nodes)) {
        // Refuse to publish a partial order if graph recognition ever produced
        // overlapping groups.  This cannot occur for distinct activation roots.
        return {};
    }
    std::copy(reordered.begin(), reordered.end(), cgraph->nodes);
    return result;
}

static inline halofpx_rocmfpx_qkv_graph_reorder_result
halofpx_rocmfpx_qkv_dispatch_graph_reorder(ggml_cgraph * cgraph, const bool gfx1151_hip) {
    if (!halofpx_rocmfpx_qkv_q8_reuse_build_enabled() || !gfx1151_hip) {
        return {};
    }
    return halofpx_rocmfpx_qkv_plan_graph_reorder(cgraph);
}
