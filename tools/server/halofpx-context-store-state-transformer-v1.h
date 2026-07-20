#pragma once

#include "halofpx-context-store.h"
#include "llama.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace halofpx {

enum class context_store_transformer_architecture_v1 : uint8_t {
    transformer,
    recurrent,
    hybrid,
};

// L07 admits only the deliberately narrow, target-only direct-session profile.
// Every field is explicit so a later capability cannot silently widen it.
struct context_store_transformer_profile_v1 {
    bool target_only = false;
    uint32_t world_size = 0;
    uint32_t rank = UINT32_MAX;
    context_store_transformer_architecture_v1 architecture =
        context_store_transformer_architecture_v1::transformer;
    bool has_draft_context = false;
    bool has_speculative_state = false;
    bool has_mtp_state = false;
    bool has_multimodal_state = false;
    bool has_adapters = false;
    bool has_grammar_state = false;
    bool has_tool_state = false;
    bool has_sampler_state = false;
    bool greedy_memoryless_sampling = false;
};

struct context_store_transformer_limits_v1 {
    size_t max_state_bytes = 0;
    size_t max_tokens = 0;
};

struct context_store_transformer_snapshot_v1 {
    context_store_identity compatibility_identity {};
    context_store_transformer_profile_v1 profile {};
    std::vector<llama_token> tokens;
    std::vector<uint8_t> state;
};

enum class context_store_transformer_status_v1 : uint8_t {
    captured,
    restored,
    invalid_argument,
    unsupported_profile,
    incomplete_identity,
    empty_tokens,
    token_limit_exceeded,
    state_unavailable,
    state_limit_exceeded,
    state_capture_failed,
    incompatible_identity,
    incompatible_profile,
    token_mismatch,
    state_restore_failed,
    allocation_failed,
    internal_error,
};

struct context_store_transformer_capture_result_v1 {
    context_store_transformer_status_v1 status =
        context_store_transformer_status_v1::invalid_argument;
    context_store_transformer_snapshot_v1 snapshot;
};

// This boundary is injectable for a model-free contract test. Production callers
// use context_store_capture/restore_transformer_state_v1(), whose implementation
// delegates to llama_state_seq_*_ext with LLAMA_STATE_SEQ_FLAGS_NONE.
class context_store_transformer_state_api_v1 {
public:
    virtual ~context_store_transformer_state_api_v1() = default;

    virtual size_t get_size(
        llama_context * ctx,
        llama_seq_id seq_id,
        llama_state_seq_flags flags) const noexcept = 0;
    virtual size_t get_data(
        llama_context * ctx,
        uint8_t * destination,
        size_t size,
        llama_seq_id seq_id,
        llama_state_seq_flags flags) const noexcept = 0;
    virtual size_t set_data(
        llama_context * ctx,
        const uint8_t * source,
        size_t size,
        llama_seq_id seq_id,
        llama_state_seq_flags flags) const noexcept = 0;
};

context_store_transformer_capture_result_v1 context_store_capture_transformer_state_v1(
    llama_context * ctx,
    llama_seq_id seq_id,
    const llama_token * tokens,
    size_t token_count,
    const context_store_identity & compatibility_identity,
    const context_store_transformer_profile_v1 & profile,
    const context_store_transformer_limits_v1 & limits) noexcept;

context_store_transformer_status_v1 context_store_restore_transformer_state_v1(
    llama_context * ctx,
    llama_seq_id seq_id,
    const context_store_transformer_snapshot_v1 & snapshot,
    const llama_token * expected_tokens,
    size_t expected_token_count,
    const context_store_identity & compatibility_identity,
    const context_store_transformer_profile_v1 & profile,
    const context_store_transformer_limits_v1 & limits) noexcept;

// Test seam; it has identical validation and ownership semantics.
context_store_transformer_capture_result_v1 context_store_capture_transformer_state_v1_with_api(
    const context_store_transformer_state_api_v1 & api,
    llama_context * ctx,
    llama_seq_id seq_id,
    const llama_token * tokens,
    size_t token_count,
    const context_store_identity & compatibility_identity,
    const context_store_transformer_profile_v1 & profile,
    const context_store_transformer_limits_v1 & limits) noexcept;

context_store_transformer_status_v1 context_store_restore_transformer_state_v1_with_api(
    const context_store_transformer_state_api_v1 & api,
    llama_context * ctx,
    llama_seq_id seq_id,
    const context_store_transformer_snapshot_v1 & snapshot,
    const llama_token * expected_tokens,
    size_t expected_token_count,
    const context_store_identity & compatibility_identity,
    const context_store_transformer_profile_v1 & profile,
    const context_store_transformer_limits_v1 & limits) noexcept;

bool context_store_transformer_profile_v1_is_admitted(
    const context_store_transformer_profile_v1 & profile) noexcept;

const char * context_store_transformer_status_v1_name(
    context_store_transformer_status_v1 status) noexcept;

} // namespace halofpx
