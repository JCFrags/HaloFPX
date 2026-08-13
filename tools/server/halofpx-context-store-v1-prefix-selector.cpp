#include "halofpx-context-store-v1-prefix-selector.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace halofpx {
namespace {

using clock_type = std::chrono::steady_clock;

bool same_identity(const context_store_identity & left,
                   const context_store_identity & right) noexcept {
    return left.compatibility_root == right.compatibility_root &&
        left.scope_namespace == right.scope_namespace &&
        left.checkpoint_lineage_id == right.checkpoint_lineage_id &&
        left.policy_epoch == right.policy_epoch;
}

bool same_profile(const context_store_transformer_profile_v1 & left,
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

void reject(context_store_v1_prefix_selector_result & result,
            context_store_v1_prefix_fallback_reason reason) noexcept {
    result.status = context_store_v1_prefix_selector_status::source_rejected;
    result.fallback_reason = reason;
}

bool validate_boundaries(const context_store_v1_prefix_selector_request & request,
                         context_store_v1_prefix_selector_result & result) noexcept {
    if (request.candidate_boundary_count == 0) return true;
    if (request.candidate_boundaries == nullptr ||
        request.candidate_boundary_count >
            context_store_v1_prefix_selector_max_boundaries) {
        reject(result, context_store_v1_prefix_fallback_reason::invalid_boundaries);
        return false;
    }
    size_t previous = 0;
    for (size_t index = 0; index < request.candidate_boundary_count; ++index) {
        const size_t boundary = request.candidate_boundaries[index];
        if (boundary == 0 || boundary > request.exact_session.token_count ||
            boundary <= previous) {
            reject(result, context_store_v1_prefix_fallback_reason::invalid_boundaries);
            return false;
        }
        previous = boundary;
    }
    return true;
}

context_store_identity identity_for(
        const context_store_exact_session_inputs_v1 & exact,
        const context_store_exact_session_result_v1 & resolved,
        uint64_t policy_epoch) noexcept {
    context_store_identity identity;
    identity.compatibility_root = exact.compatibility_root;
    identity.scope_namespace = exact.scope_namespace;
    identity.checkpoint_lineage_id = resolved.session_id;
    identity.policy_epoch = policy_epoch;
    return identity;
}

void fail_catalog(context_store_v1_prefix_selector_result & result,
                  context_store_v1_catalog_status status) noexcept {
    result.last_catalog_status = status;
    switch (status) {
        case context_store_v1_catalog_status::miss_corrupt:
            result.status = context_store_v1_prefix_selector_status::miss_corrupt;
            result.fallback_reason =
                context_store_v1_prefix_fallback_reason::authenticated_state_corrupt;
            return;
        case context_store_v1_catalog_status::miss_incompatible:
            result.status = context_store_v1_prefix_selector_status::miss_incompatible;
            result.fallback_reason =
                context_store_v1_prefix_fallback_reason::authenticated_state_incompatible;
            return;
        case context_store_v1_catalog_status::busy:
            result.status = context_store_v1_prefix_selector_status::busy;
            result.fallback_reason =
                context_store_v1_prefix_fallback_reason::catalog_busy;
            return;
        case context_store_v1_catalog_status::source_rejected:
            result.status = context_store_v1_prefix_selector_status::source_rejected;
            result.fallback_reason =
                context_store_v1_prefix_fallback_reason::invalid_request;
            return;
        case context_store_v1_catalog_status::storage:
        case context_store_v1_catalog_status::ready:
        case context_store_v1_catalog_status::published:
            result.status = context_store_v1_prefix_selector_status::storage;
            result.fallback_reason =
                context_store_v1_prefix_fallback_reason::storage_error;
            return;
        case context_store_v1_catalog_status::hit:
        case context_store_v1_catalog_status::miss_not_found:
        case context_store_v1_catalog_status::capacity_exhausted:
            break;
    }
    result.status = context_store_v1_prefix_selector_status::storage;
    result.fallback_reason = context_store_v1_prefix_fallback_reason::storage_error;
}

} // namespace

context_store_v1_prefix_selector_result
context_store_v1_restore_longest_prefix(
        context_store_v1_catalog & catalog,
        const context_store_v1_prefix_selector_request & request) noexcept {
    context_store_v1_prefix_selector_result result;
    result.residual_token_count = request.exact_session.token_count;
    const auto started = clock_type::now();
    const auto finish = [&result, started]() noexcept {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            clock_type::now() - started).count();
        result.validation_time_ns = elapsed <= 0 ? 0 : static_cast<uint64_t>(elapsed);
        return std::move(result);
    };

    if (request.policy_epoch == 0 ||
        request.exact_session.logical_boundary != request.exact_session.token_count ||
        request.exact_session.output_boundary != request.exact_session.token_count) {
        reject(result, context_store_v1_prefix_fallback_reason::invalid_request);
        return finish();
    }
    if (!context_store_transformer_profile_v1_is_admitted(request.profile) ||
        request.exact_session.profile != context_store_exact_session_profile_v1::
            target_only_greedy_memoryless) {
        reject(result, context_store_v1_prefix_fallback_reason::unsupported_profile);
        return finish();
    }
    if (request.exact_session.world_size != 1 || request.exact_session.rank != 0 ||
        request.profile.world_size != request.exact_session.world_size ||
        request.profile.rank != request.exact_session.rank) {
        reject(result, context_store_v1_prefix_fallback_reason::incompatible_topology);
        return finish();
    }

    const auto full_authority =
        context_store_resolve_exact_session_v1(request.exact_session);
    result.authority_status = full_authority.status;
    if (!full_authority.resolved()) {
        reject(result, context_store_v1_prefix_fallback_reason::invalid_request);
        return finish();
    }
    if (!validate_boundaries(request, result)) return finish();
    if (request.candidate_boundary_count == 0) {
        result.fallback_reason =
            context_store_v1_prefix_fallback_reason::no_candidate_boundaries;
        return finish();
    }

    for (size_t reverse = request.candidate_boundary_count; reverse != 0; --reverse) {
        const size_t boundary = request.candidate_boundaries[reverse - 1];
        ++result.candidates_examined;

        auto exact = request.exact_session;
        exact.token_count = boundary;
        exact.logical_boundary = boundary;
        exact.output_boundary = boundary;
        const auto resolved = context_store_resolve_exact_session_v1(exact);
        result.authority_status = resolved.status;
        if (!resolved.resolved()) {
            reject(result, context_store_v1_prefix_fallback_reason::invalid_request);
            return finish();
        }
        const auto identity = identity_for(exact, resolved, request.policy_epoch);
        auto restored = catalog.restore_exact(
            request.exact_session.tokens, boundary, identity, request.profile);
        result.last_catalog_status = restored.status;

        if (restored.status == context_store_v1_catalog_status::miss_not_found ||
            restored.status == context_store_v1_catalog_status::capacity_exhausted) {
            continue;
        }
        if (restored.status != context_store_v1_catalog_status::hit) {
            fail_catalog(result, restored.status);
            return finish();
        }

        // The child already authenticates and validates these fields. Repeat
        // exact equality at this selection boundary so later refactors cannot
        // turn a catalog hit into implicit prefix authority.
        if (restored.snapshot.tokens.size() != boundary ||
            !std::equal(restored.snapshot.tokens.begin(), restored.snapshot.tokens.end(),
                        request.exact_session.tokens) ||
            !same_identity(restored.snapshot.compatibility_identity, identity) ||
            !same_profile(restored.snapshot.profile, request.profile)) {
            std::fill(restored.snapshot.state.begin(), restored.snapshot.state.end(), 0);
            restored.snapshot.tokens.clear();
            restored.snapshot.state.clear();
            result.status = context_store_v1_prefix_selector_status::miss_corrupt;
            result.fallback_reason =
                context_store_v1_prefix_fallback_reason::authenticated_state_corrupt;
            result.last_catalog_status = context_store_v1_catalog_status::miss_corrupt;
            return finish();
        }

        result.status = context_store_v1_prefix_selector_status::hit;
        result.fallback_reason = context_store_v1_prefix_fallback_reason::none;
        result.matched_token_count = boundary;
        result.restored_token_count = restored.snapshot.tokens.size();
        result.residual_token_offset = boundary;
        result.residual_token_count = request.exact_session.token_count - boundary;
        result.snapshot = std::move(restored.snapshot);
        return finish();
    }

    result.status = context_store_v1_prefix_selector_status::miss_not_found;
    result.fallback_reason =
        context_store_v1_prefix_fallback_reason::no_eligible_prefix;
    return finish();
}

const char * context_store_v1_prefix_selector_status_name(
        context_store_v1_prefix_selector_status status) noexcept {
    switch (status) {
        case context_store_v1_prefix_selector_status::hit:               return "hit";
        case context_store_v1_prefix_selector_status::miss_not_found:    return "miss-not-found";
        case context_store_v1_prefix_selector_status::miss_corrupt:      return "miss-corrupt";
        case context_store_v1_prefix_selector_status::miss_incompatible: return "miss-incompatible";
        case context_store_v1_prefix_selector_status::source_rejected:   return "source-rejected";
        case context_store_v1_prefix_selector_status::busy:              return "busy";
        case context_store_v1_prefix_selector_status::storage:           return "storage";
    }
    return "unknown";
}

const char * context_store_v1_prefix_fallback_reason_name(
        context_store_v1_prefix_fallback_reason reason) noexcept {
    switch (reason) {
        case context_store_v1_prefix_fallback_reason::none:                           return "none";
        case context_store_v1_prefix_fallback_reason::no_candidate_boundaries:        return "no-candidate-boundaries";
        case context_store_v1_prefix_fallback_reason::no_eligible_prefix:             return "no-eligible-prefix";
        case context_store_v1_prefix_fallback_reason::invalid_request:                return "invalid-request";
        case context_store_v1_prefix_fallback_reason::invalid_boundaries:             return "invalid-boundaries";
        case context_store_v1_prefix_fallback_reason::unsupported_profile:            return "unsupported-profile";
        case context_store_v1_prefix_fallback_reason::incompatible_topology:          return "incompatible-topology";
        case context_store_v1_prefix_fallback_reason::authenticated_state_corrupt:    return "authenticated-state-corrupt";
        case context_store_v1_prefix_fallback_reason::authenticated_state_incompatible:return "authenticated-state-incompatible";
        case context_store_v1_prefix_fallback_reason::catalog_busy:                   return "catalog-busy";
        case context_store_v1_prefix_fallback_reason::storage_error:                  return "storage-error";
    }
    return "unknown";
}

} // namespace halofpx
