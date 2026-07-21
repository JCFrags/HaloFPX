#pragma once

#include "halofpx-context-store-format.h"

#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_exact_session_key_bytes = 32;
constexpr size_t context_store_exact_session_max_tokens = 1024 * 1024;

struct context_store_exact_session_key_view {
    const uint8_t * data = nullptr;
    size_t size = 0;
};

enum class context_store_exact_session_profile_v1 : uint8_t {
    unset = 0,
    target_only_greedy_memoryless = 1,
};

struct context_store_exact_session_inputs_v1 {
    context_store_exact_session_key_view derivation_key;
    context_store_format_digest scope_namespace {};
    context_store_format_digest compatibility_root {};
    const int32_t * tokens = nullptr;
    size_t token_count = 0;
    uint64_t logical_boundary = 0;
    uint64_t output_boundary = 0;
    context_store_exact_session_profile_v1 profile = context_store_exact_session_profile_v1::unset;
    context_store_format_digest global_plan_digest {};
    context_store_format_digest rank_ownership_digest {};
    context_store_format_digest rank_placement_digest {};
    uint64_t topology_epoch = 0;
    uint64_t world_size = 0;
    uint64_t rank = 0;
};

enum class context_store_exact_session_status_v1 : uint8_t {
    resolved,
    invalid_key,
    invalid_scope_namespace,
    invalid_compatibility_root,
    invalid_tokens,
    invalid_boundaries,
    invalid_profile,
    invalid_topology_digest,
    invalid_topology,
    derivation_failed,
};

struct context_store_exact_session_result_v1 {
    context_store_exact_session_status_v1 status =
        context_store_exact_session_status_v1::invalid_key;
    context_store_format_digest session_id {};

    bool resolved() const noexcept {
        return status == context_store_exact_session_status_v1::resolved;
    }
};

// Derives an opaque exact-session identifier. Input memory is borrowed only
// for the duration of this call. No principal or prompt/token material is
// retained or returned, and every rejection returns an all-zero identifier.
context_store_exact_session_result_v1 context_store_resolve_exact_session_v1(
    const context_store_exact_session_inputs_v1 & inputs) noexcept;

const char * context_store_exact_session_status_v1_name(
    context_store_exact_session_status_v1 status) noexcept;

} // namespace halofpx
