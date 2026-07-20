#include "halofpx-context-store-state-transformer-v1.h"

#include <algorithm>
#include <new>

namespace halofpx {
namespace {

bool digest_is_nonzero(const context_store_digest & digest) noexcept {
    uint8_t combined = 0;
    for (const uint8_t byte : digest) {
        combined |= byte;
    }
    return combined != 0;
}

bool identity_is_complete(const context_store_identity & identity) noexcept {
    return digest_is_nonzero(identity.compatibility_root) &&
        digest_is_nonzero(identity.scope_namespace) &&
        digest_is_nonzero(identity.checkpoint_lineage_id);
}

bool identities_equal(
        const context_store_identity & left,
        const context_store_identity & right) noexcept {
    return left.compatibility_root == right.compatibility_root &&
        left.scope_namespace == right.scope_namespace &&
        left.checkpoint_lineage_id == right.checkpoint_lineage_id &&
        left.policy_epoch == right.policy_epoch;
}

bool profiles_equal(
        const context_store_transformer_profile_v1 & left,
        const context_store_transformer_profile_v1 & right) noexcept {
    return left.target_only == right.target_only &&
        left.world_size == right.world_size &&
        left.rank == right.rank &&
        left.architecture == right.architecture &&
        left.has_draft_context == right.has_draft_context &&
        left.has_speculative_state == right.has_speculative_state &&
        left.has_mtp_state == right.has_mtp_state &&
        left.has_multimodal_state == right.has_multimodal_state &&
        left.has_adapters == right.has_adapters &&
        left.has_grammar_state == right.has_grammar_state &&
        left.has_tool_state == right.has_tool_state &&
        left.has_sampler_state == right.has_sampler_state &&
        left.greedy_memoryless_sampling == right.greedy_memoryless_sampling;
}

void wipe(std::vector<uint8_t> & state) noexcept {
    volatile uint8_t * output = state.data();
    for (size_t index = 0; index < state.size(); ++index) {
        output[index] = 0;
    }
    state.clear();
}

void reject_partial_capture(context_store_transformer_capture_result_v1 & result) noexcept {
    wipe(result.snapshot.state);
    result.snapshot.tokens.clear();
    result.snapshot.compatibility_identity = {};
    result.snapshot.profile = {};
}

class llama_transformer_state_api_v1 final : public context_store_transformer_state_api_v1 {
public:
    size_t get_size(
            llama_context * ctx,
            llama_seq_id seq_id,
            llama_state_seq_flags flags) const noexcept override {
        return llama_state_seq_get_size_ext(ctx, seq_id, flags);
    }

    size_t get_data(
            llama_context * ctx,
            uint8_t * destination,
            size_t size,
            llama_seq_id seq_id,
            llama_state_seq_flags flags) const noexcept override {
        return llama_state_seq_get_data_ext(ctx, destination, size, seq_id, flags);
    }

    size_t set_data(
            llama_context * ctx,
            const uint8_t * source,
            size_t size,
            llama_seq_id seq_id,
            llama_state_seq_flags flags) const noexcept override {
        return llama_state_seq_set_data_ext(ctx, source, size, seq_id, flags);
    }
};

const context_store_transformer_state_api_v1 & production_api() noexcept {
    static const llama_transformer_state_api_v1 api;
    return api;
}

} // namespace

bool context_store_transformer_profile_v1_is_admitted(
        const context_store_transformer_profile_v1 & profile) noexcept {
    return profile.target_only &&
        profile.world_size == 1 &&
        profile.rank == 0 &&
        profile.architecture == context_store_transformer_architecture_v1::transformer &&
        !profile.has_draft_context &&
        !profile.has_speculative_state &&
        !profile.has_mtp_state &&
        !profile.has_multimodal_state &&
        !profile.has_adapters &&
        !profile.has_grammar_state &&
        !profile.has_tool_state &&
        !profile.has_sampler_state &&
        profile.greedy_memoryless_sampling;
}

context_store_transformer_capture_result_v1 context_store_capture_transformer_state_v1_with_api(
        const context_store_transformer_state_api_v1 & api,
        llama_context * ctx,
        llama_seq_id seq_id,
        const llama_token * tokens,
        size_t token_count,
        const context_store_identity & compatibility_identity,
        const context_store_transformer_profile_v1 & profile,
        const context_store_transformer_limits_v1 & limits) noexcept {
    context_store_transformer_capture_result_v1 result;
    try {
        if (ctx == nullptr || tokens == nullptr || limits.max_state_bytes == 0 || limits.max_tokens == 0) {
            result.status = context_store_transformer_status_v1::invalid_argument;
            return result;
        }
        if (!identity_is_complete(compatibility_identity)) {
            result.status = context_store_transformer_status_v1::incomplete_identity;
            return result;
        }
        if (!context_store_transformer_profile_v1_is_admitted(profile)) {
            result.status = context_store_transformer_status_v1::unsupported_profile;
            return result;
        }
        if (token_count == 0) {
            result.status = context_store_transformer_status_v1::empty_tokens;
            return result;
        }
        if (token_count > limits.max_tokens) {
            result.status = context_store_transformer_status_v1::token_limit_exceeded;
            return result;
        }

        const size_t state_size = api.get_size(ctx, seq_id, LLAMA_STATE_SEQ_FLAGS_NONE);
        if (state_size == 0) {
            result.status = context_store_transformer_status_v1::state_unavailable;
            return result;
        }
        if (state_size > limits.max_state_bytes) {
            result.status = context_store_transformer_status_v1::state_limit_exceeded;
            return result;
        }

        result.snapshot.compatibility_identity = compatibility_identity;
        result.snapshot.profile = profile;
        result.snapshot.tokens.assign(tokens, tokens + token_count);
        result.snapshot.state.resize(state_size);
        const size_t captured = api.get_data(
            ctx, result.snapshot.state.data(), result.snapshot.state.size(), seq_id,
            LLAMA_STATE_SEQ_FLAGS_NONE);
        if (captured != state_size) {
            result.status = context_store_transformer_status_v1::state_capture_failed;
            reject_partial_capture(result);
            return result;
        }

        result.status = context_store_transformer_status_v1::captured;
        return result;
    } catch (const std::bad_alloc &) {
        result.status = context_store_transformer_status_v1::allocation_failed;
    } catch (...) {
        result.status = context_store_transformer_status_v1::internal_error;
    }
    reject_partial_capture(result);
    return result;
}

context_store_transformer_capture_result_v1 context_store_capture_transformer_state_v1(
        llama_context * ctx,
        llama_seq_id seq_id,
        const llama_token * tokens,
        size_t token_count,
        const context_store_identity & compatibility_identity,
        const context_store_transformer_profile_v1 & profile,
        const context_store_transformer_limits_v1 & limits) noexcept {
    return context_store_capture_transformer_state_v1_with_api(
        production_api(), ctx, seq_id, tokens, token_count,
        compatibility_identity, profile, limits);
}

context_store_transformer_status_v1 context_store_restore_transformer_state_v1_with_api(
        const context_store_transformer_state_api_v1 & api,
        llama_context * ctx,
        llama_seq_id seq_id,
        const context_store_transformer_snapshot_v1 & snapshot,
        const llama_token * expected_tokens,
        size_t expected_token_count,
        const context_store_identity & compatibility_identity,
        const context_store_transformer_profile_v1 & profile,
        const context_store_transformer_limits_v1 & limits) noexcept {
    try {
        if (ctx == nullptr || expected_tokens == nullptr ||
                limits.max_state_bytes == 0 || limits.max_tokens == 0) {
            return context_store_transformer_status_v1::invalid_argument;
        }
        if (!identity_is_complete(compatibility_identity)) {
            return context_store_transformer_status_v1::incomplete_identity;
        }
        if (!context_store_transformer_profile_v1_is_admitted(profile)) {
            return context_store_transformer_status_v1::unsupported_profile;
        }
        if (snapshot.tokens.empty() || expected_token_count == 0) {
            return context_store_transformer_status_v1::empty_tokens;
        }
        if (snapshot.tokens.size() > limits.max_tokens || expected_token_count > limits.max_tokens) {
            return context_store_transformer_status_v1::token_limit_exceeded;
        }
        if (snapshot.state.empty()) {
            return context_store_transformer_status_v1::state_unavailable;
        }
        if (snapshot.state.size() > limits.max_state_bytes) {
            return context_store_transformer_status_v1::state_limit_exceeded;
        }
        if (!identity_is_complete(snapshot.compatibility_identity) ||
                !identities_equal(snapshot.compatibility_identity, compatibility_identity)) {
            return context_store_transformer_status_v1::incompatible_identity;
        }
        if (!context_store_transformer_profile_v1_is_admitted(snapshot.profile) ||
                !profiles_equal(snapshot.profile, profile)) {
            return context_store_transformer_status_v1::incompatible_profile;
        }
        if (expected_token_count != snapshot.tokens.size() ||
                !std::equal(snapshot.tokens.begin(), snapshot.tokens.end(), expected_tokens)) {
            return context_store_transformer_status_v1::token_mismatch;
        }

        const size_t restored = api.set_data(
            ctx, snapshot.state.data(), snapshot.state.size(), seq_id,
            LLAMA_STATE_SEQ_FLAGS_NONE);
        return restored == snapshot.state.size()
            ? context_store_transformer_status_v1::restored
            : context_store_transformer_status_v1::state_restore_failed;
    } catch (const std::bad_alloc &) {
        return context_store_transformer_status_v1::allocation_failed;
    } catch (...) {
        return context_store_transformer_status_v1::internal_error;
    }
}

context_store_transformer_status_v1 context_store_restore_transformer_state_v1(
        llama_context * ctx,
        llama_seq_id seq_id,
        const context_store_transformer_snapshot_v1 & snapshot,
        const llama_token * expected_tokens,
        size_t expected_token_count,
        const context_store_identity & compatibility_identity,
        const context_store_transformer_profile_v1 & profile,
        const context_store_transformer_limits_v1 & limits) noexcept {
    return context_store_restore_transformer_state_v1_with_api(
        production_api(), ctx, seq_id, snapshot, expected_tokens, expected_token_count,
        compatibility_identity, profile, limits);
}

const char * context_store_transformer_status_v1_name(
        context_store_transformer_status_v1 status) noexcept {
    switch (status) {
        case context_store_transformer_status_v1::captured: return "captured";
        case context_store_transformer_status_v1::restored: return "restored";
        case context_store_transformer_status_v1::invalid_argument: return "invalid_argument";
        case context_store_transformer_status_v1::unsupported_profile: return "unsupported_profile";
        case context_store_transformer_status_v1::incomplete_identity: return "incomplete_identity";
        case context_store_transformer_status_v1::empty_tokens: return "empty_tokens";
        case context_store_transformer_status_v1::token_limit_exceeded: return "token_limit_exceeded";
        case context_store_transformer_status_v1::state_unavailable: return "state_unavailable";
        case context_store_transformer_status_v1::state_limit_exceeded: return "state_limit_exceeded";
        case context_store_transformer_status_v1::state_capture_failed: return "state_capture_failed";
        case context_store_transformer_status_v1::incompatible_identity: return "incompatible_identity";
        case context_store_transformer_status_v1::incompatible_profile: return "incompatible_profile";
        case context_store_transformer_status_v1::token_mismatch: return "token_mismatch";
        case context_store_transformer_status_v1::state_restore_failed: return "state_restore_failed";
        case context_store_transformer_status_v1::allocation_failed: return "allocation_failed";
        case context_store_transformer_status_v1::internal_error: return "internal_error";
    }
    return "unknown";
}

} // namespace halofpx
