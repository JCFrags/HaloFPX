#pragma once

#include "halofpx-context-store-v1-prefix-selector.h"

#include <cstddef>
#include <cstdint>

namespace halofpx {

// Immutable capability supplied by a trusted live model/runtime loader.  The
// current server has no such provider; absence is a normal fail-closed cold
// state, never permission to substitute operator-provided component digests.
struct context_store_world1_cache_authority_v1 {
    context_store_compatibility_expectation compatibility {};
    context_store_format_digest producer_identity {};
    context_store_format_digest global_plan_digest {};
    context_store_format_digest rank_ownership_digest {};
    context_store_format_digest rank_placement_digest {};
    uint64_t topology_epoch = 0;
    uint64_t model_generation = 0;
    uint32_t world_size = 0;
    uint32_t rank = UINT32_MAX;
};

bool context_store_world1_cache_authority_v1_is_valid(
    const context_store_world1_cache_authority_v1 & authority) noexcept;
bool context_store_world1_cache_authority_v1_matches(
    const context_store_world1_cache_authority_v1 & left,
    const context_store_world1_cache_authority_v1 & right) noexcept;

enum class context_store_world1_prefix_source_v1 : uint8_t {
    cold,
    exact,
    prefix,
};

enum class context_store_world1_prefix_fallback_v1 : uint8_t {
    none,
    feature_off,
    live_authority_unavailable,
    live_authority_invalid,
    authority_generation_changed,
    authority_changed,
    invalid_request,
    catalog_unavailable,
    live_slot_state_present,
    no_authenticated_checkpoint,
    authenticated_state_corrupt,
    authenticated_state_incompatible,
    catalog_busy,
    storage_error,
};

struct context_store_world1_prefix_lookup_request_v1 {
    bool enabled = false;
    const context_store_world1_cache_authority_v1 * authority = nullptr;
    uint64_t expected_model_generation = 0;
    context_store_v1_catalog * catalog = nullptr;
    context_store_exact_session_inputs_v1 exact_session;
    uint64_t policy_epoch = 0;
    context_store_transformer_profile_v1 profile;
};

struct context_store_world1_prefix_lookup_result_v1 {
    context_store_world1_prefix_lookup_result_v1() = default;
    ~context_store_world1_prefix_lookup_result_v1() noexcept;
    context_store_world1_prefix_lookup_result_v1(
        const context_store_world1_prefix_lookup_result_v1 &) = delete;
    context_store_world1_prefix_lookup_result_v1 & operator=(
        const context_store_world1_prefix_lookup_result_v1 &) = delete;
    context_store_world1_prefix_lookup_result_v1(
        context_store_world1_prefix_lookup_result_v1 && other) noexcept;
    context_store_world1_prefix_lookup_result_v1 & operator=(
        context_store_world1_prefix_lookup_result_v1 && other) noexcept;

    context_store_world1_prefix_source_v1 source =
        context_store_world1_prefix_source_v1::cold;
    context_store_world1_prefix_fallback_v1 fallback =
        context_store_world1_prefix_fallback_v1::feature_off;
    context_store_transformer_snapshot_v1 snapshot;
    context_store_identity selected_identity {};
    context_store_world1_cache_authority_v1 bound_authority {};
    bool authority_bound = false;
    size_t selected_prefix_tokens = 0;
    size_t restored_tokens = 0;
    size_t residual_tokens = 0;
    size_t candidates_examined = 0;
    uint64_t validation_time_ns = 0;

    bool hit() const noexcept {
        return source == context_store_world1_prefix_source_v1::exact ||
            source == context_store_world1_prefix_source_v1::prefix;
    }
};

context_store_world1_prefix_lookup_result_v1
context_store_world1_prefix_lookup_v1(
    const context_store_world1_prefix_lookup_request_v1 & request) noexcept;

enum class context_store_world1_prefix_install_status_v1 : uint8_t {
    installed,
    rejected,
    authority_generation_changed,
    authority_changed,
    state_apply_failed,
};

struct context_store_world1_prefix_install_request_v1 {
    const context_store_world1_cache_authority_v1 * authority = nullptr;
    uint64_t expected_model_generation = 0;
    context_store_world1_prefix_lookup_result_v1 * lookup = nullptr;
    llama_context * context = nullptr;
    llama_seq_id sequence = -1;
    size_t sequence_limit = 0;
    const llama_token * full_tokens = nullptr;
    size_t full_token_count = 0;
    context_store_transformer_profile_v1 profile;
    context_store_transformer_limits_v1 limits;
};

struct context_store_world1_prefix_install_result_v1 {
    context_store_world1_prefix_install_status_v1 status =
        context_store_world1_prefix_install_status_v1::rejected;
    context_store_transformer_status_v1 state_status =
        context_store_transformer_status_v1::invalid_argument;
    size_t installed_prefix_tokens = 0;
    size_t residual_tokens = 0;

    bool installed() const noexcept {
        return status == context_store_world1_prefix_install_status_v1::installed;
    }
};

struct context_store_world1_work_accounting_v1 {
    bool valid = false;
    size_t actual_prompt_tokens = 0;
    size_t avoided_prompt_tokens = 0;
};

context_store_world1_work_accounting_v1
context_store_world1_finalize_work_accounting_v1(
    size_t request_prompt_tokens, int64_t actual_prompt_tokens) noexcept;

// Production and deterministic model-free seams share identical validation,
// authority, state-application, and state-wiping semantics.
context_store_world1_prefix_install_result_v1
context_store_world1_prefix_install_v1(
    const context_store_world1_prefix_install_request_v1 & request) noexcept;
context_store_world1_prefix_install_result_v1
context_store_world1_prefix_install_v1_with_api(
    const context_store_transformer_state_api_v1 & api,
    const context_store_world1_prefix_install_request_v1 & request) noexcept;

const char * context_store_world1_prefix_source_name_v1(
    context_store_world1_prefix_source_v1 source) noexcept;
const char * context_store_world1_prefix_fallback_name_v1(
    context_store_world1_prefix_fallback_v1 fallback) noexcept;

} // namespace halofpx
