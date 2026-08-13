#pragma once

#include "halofpx-context-store-exact-session.h"
#include "halofpx-context-store-v1-catalog.h"

#include <cstddef>
#include <cstdint>

namespace halofpx {

// The catalog itself has at most eight immutable entries. Bounding the caller's
// semantic checkpoints to the same cardinality keeps lookup work deterministic.
constexpr size_t context_store_v1_prefix_selector_max_boundaries =
    context_store_v1_catalog_max_slots;

enum class context_store_v1_prefix_selector_status : uint8_t {
    hit,
    miss_not_found,
    miss_corrupt,
    miss_incompatible,
    source_rejected,
    busy,
    storage,
};

// Fixed-cardinality controller telemetry. It carries no token, prompt, path,
// digest, identity, key, or state material.
enum class context_store_v1_prefix_fallback_reason : uint8_t {
    none,
    no_candidate_boundaries,
    no_eligible_prefix,
    invalid_request,
    invalid_boundaries,
    unsupported_profile,
    incompatible_topology,
    authenticated_state_corrupt,
    authenticated_state_incompatible,
    catalog_busy,
    storage_error,
};

struct context_store_v1_prefix_selector_request {
    // `tokens` is the complete canonical request-token sequence. The selector
    // borrows it only for this synchronous call. Full-request logical/output
    // boundaries must both equal token_count; candidate derivations replace
    // both with each exact prefix boundary.
    context_store_exact_session_inputs_v1 exact_session;

    // Complete semantic checkpoint boundaries in strictly increasing order.
    // They are explicit token counts, never inferred from text or fuzzy input.
    const size_t * candidate_boundaries = nullptr;
    size_t candidate_boundary_count = 0;

    uint64_t policy_epoch = 0;
    context_store_transformer_profile_v1 profile;
};

struct context_store_v1_prefix_selector_result {
    context_store_v1_prefix_selector_status status =
        context_store_v1_prefix_selector_status::miss_not_found;
    context_store_v1_prefix_fallback_reason fallback_reason =
        context_store_v1_prefix_fallback_reason::no_eligible_prefix;
    context_store_v1_catalog_status last_catalog_status =
        context_store_v1_catalog_status::miss_not_found;
    context_store_exact_session_status_v1 authority_status =
        context_store_exact_session_status_v1::invalid_key;
    context_store_transformer_snapshot_v1 snapshot;
    size_t matched_token_count = 0;
    size_t restored_token_count = 0;
    size_t residual_token_offset = 0;
    size_t residual_token_count = 0;
    size_t candidates_examined = 0;
    uint64_t validation_time_ns = 0;

    bool hit() const noexcept {
        return status == context_store_v1_prefix_selector_status::hit;
    }
};

// Selects the longest authenticated exact-token checkpoint and returns the
// untouched suffix as offset/count for caller-owned replay. This function does
// not mutate a live llama_context, publish state, infer boundaries, or perform
// product-server routing. Corruption or ambiguity is a cold miss, never a
// reason to accept a shorter candidate. The caller must hold catalog-mutation
// custody for the complete call: no publish or other catalog mutation may run
// concurrently between the longest and shortest exact probes. Concurrent
// read-only restores are allowed.
context_store_v1_prefix_selector_result
context_store_v1_restore_longest_prefix(
    context_store_v1_catalog & catalog,
    const context_store_v1_prefix_selector_request & request) noexcept;

const char * context_store_v1_prefix_selector_status_name(
    context_store_v1_prefix_selector_status status) noexcept;
const char * context_store_v1_prefix_fallback_reason_name(
    context_store_v1_prefix_fallback_reason reason) noexcept;

} // namespace halofpx
