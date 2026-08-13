#include "halofpx-context-store-world1-prefix-product-v1.h"
#include "halofpx-context-store-compatibility-v1.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <string>
#include <utility>

namespace halofpx {
namespace {

using clock_type = std::chrono::steady_clock;

template <size_t N>
bool nonzero(const std::array<uint8_t, N> & value) noexcept {
    return std::any_of(value.begin(), value.end(),
        [](uint8_t byte) { return byte != 0; });
}

bool exact_session_authority_matches(
        const context_store_world1_cache_authority_v1 & authority,
        const context_store_exact_session_inputs_v1 & exact) noexcept {
    return exact.compatibility_root == authority.compatibility.root &&
        exact.global_plan_digest == authority.global_plan_digest &&
        exact.rank_ownership_digest == authority.rank_ownership_digest &&
        exact.rank_placement_digest == authority.rank_placement_digest &&
        exact.topology_epoch == authority.topology_epoch &&
        exact.world_size == authority.world_size && exact.rank == authority.rank;
}

bool same_authority(
        const context_store_world1_cache_authority_v1 & left,
        const context_store_world1_cache_authority_v1 & right) noexcept {
    return left.compatibility.root == right.compatibility.root &&
        left.compatibility.components == right.compatibility.components &&
        left.producer_identity == right.producer_identity &&
        left.global_plan_digest == right.global_plan_digest &&
        left.rank_ownership_digest == right.rank_ownership_digest &&
        left.rank_placement_digest == right.rank_placement_digest &&
        left.topology_epoch == right.topology_epoch &&
        left.model_generation == right.model_generation &&
        left.world_size == right.world_size && left.rank == right.rank;
}

bool same_identity(const context_store_identity & left,
                   const context_store_identity & right) noexcept {
    return left.compatibility_root == right.compatibility_root &&
        left.scope_namespace == right.scope_namespace &&
        left.checkpoint_lineage_id == right.checkpoint_lineage_id &&
        left.policy_epoch == right.policy_epoch;
}

void secure_wipe(void * memory, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(memory);
    while (size-- != 0) *bytes++ = 0;
}

void wipe_snapshot(context_store_transformer_snapshot_v1 & snapshot) noexcept {
    if (!snapshot.state.empty()) {
        secure_wipe(snapshot.state.data(), snapshot.state.size());
    }
    if (!snapshot.tokens.empty()) {
        secure_wipe(snapshot.tokens.data(),
                    snapshot.tokens.size() * sizeof(llama_token));
    }
}

void consume_lookup(context_store_world1_prefix_lookup_result_v1 & lookup) noexcept {
    wipe_snapshot(lookup.snapshot);
    lookup.snapshot.state.clear();
    lookup.snapshot.tokens.clear();
    lookup.selected_identity = {};
    lookup.bound_authority = {};
    lookup.authority_bound = false;
    lookup.source = context_store_world1_prefix_source_v1::cold;
    lookup.selected_prefix_tokens = 0;
    lookup.restored_tokens = 0;
    lookup.residual_tokens = 0;
}

void move_lookup(context_store_world1_prefix_lookup_result_v1 & destination,
                 context_store_world1_prefix_lookup_result_v1 & source) noexcept {
    consume_lookup(destination);
    destination.source = source.source;
    destination.fallback = source.fallback;
    destination.snapshot = std::move(source.snapshot);
    destination.selected_identity = source.selected_identity;
    destination.bound_authority = source.bound_authority;
    destination.authority_bound = source.authority_bound;
    destination.selected_prefix_tokens = source.selected_prefix_tokens;
    destination.restored_tokens = source.restored_tokens;
    destination.residual_tokens = source.residual_tokens;
    destination.candidates_examined = source.candidates_examined;
    destination.validation_time_ns = source.validation_time_ns;
    consume_lookup(source);
    source.fallback = context_store_world1_prefix_fallback_v1::feature_off;
    source.candidates_examined = 0;
    source.validation_time_ns = 0;
}

context_store_world1_prefix_fallback_v1 map_catalog(
        context_store_v1_catalog_status status) noexcept {
    switch (status) {
        case context_store_v1_catalog_status::miss_not_found:
        case context_store_v1_catalog_status::capacity_exhausted:
            return context_store_world1_prefix_fallback_v1::no_authenticated_checkpoint;
        case context_store_v1_catalog_status::miss_corrupt:
            return context_store_world1_prefix_fallback_v1::authenticated_state_corrupt;
        case context_store_v1_catalog_status::miss_incompatible:
            return context_store_world1_prefix_fallback_v1::authenticated_state_incompatible;
        case context_store_v1_catalog_status::busy:
            return context_store_world1_prefix_fallback_v1::catalog_busy;
        case context_store_v1_catalog_status::storage:
            return context_store_world1_prefix_fallback_v1::storage_error;
        default:
            return context_store_world1_prefix_fallback_v1::invalid_request;
    }
}

context_store_world1_prefix_fallback_v1 map_selector(
        context_store_v1_prefix_fallback_reason reason) noexcept {
    switch (reason) {
        case context_store_v1_prefix_fallback_reason::none:
            return context_store_world1_prefix_fallback_v1::none;
        case context_store_v1_prefix_fallback_reason::no_candidate_boundaries:
        case context_store_v1_prefix_fallback_reason::no_eligible_prefix:
            return context_store_world1_prefix_fallback_v1::no_authenticated_checkpoint;
        case context_store_v1_prefix_fallback_reason::authenticated_state_corrupt:
            return context_store_world1_prefix_fallback_v1::authenticated_state_corrupt;
        case context_store_v1_prefix_fallback_reason::authenticated_state_incompatible:
            return context_store_world1_prefix_fallback_v1::authenticated_state_incompatible;
        case context_store_v1_prefix_fallback_reason::catalog_busy:
            return context_store_world1_prefix_fallback_v1::catalog_busy;
        case context_store_v1_prefix_fallback_reason::storage_error:
            return context_store_world1_prefix_fallback_v1::storage_error;
        default:
            return context_store_world1_prefix_fallback_v1::invalid_request;
    }
}

} // namespace

context_store_world1_prefix_lookup_result_v1::
context_store_world1_prefix_lookup_result_v1(
        context_store_world1_prefix_lookup_result_v1 && other) noexcept {
    move_lookup(*this, other);
}

context_store_world1_prefix_lookup_result_v1 &
context_store_world1_prefix_lookup_result_v1::operator=(
        context_store_world1_prefix_lookup_result_v1 && other) noexcept {
    if (this != &other) move_lookup(*this, other);
    return *this;
}

context_store_world1_prefix_lookup_result_v1::
~context_store_world1_prefix_lookup_result_v1() noexcept {
    consume_lookup(*this);
}

bool context_store_world1_cache_authority_v1_is_valid(
        const context_store_world1_cache_authority_v1 & authority) noexcept {
    if (!nonzero(authority.compatibility.root) ||
        !nonzero(authority.producer_identity) ||
        !nonzero(authority.global_plan_digest) ||
        !nonzero(authority.rank_ownership_digest) ||
        !nonzero(authority.rank_placement_digest) ||
        authority.topology_epoch == 0 || authority.model_generation == 0 ||
        authority.world_size != 1 || authority.rank != 0) {
        return false;
    }
    std::array<context_store_compatibility_component_digest_v1,
               context_store_compatibility_v1_component_count> components {};
    for (size_t index = 0; index != components.size(); ++index) {
        const char * label = context_store_compatibility_component_label_v1(index);
        if (label == nullptr || !nonzero(authority.compatibility.components[index])) {
            return false;
        }
        components[index].label = label;
        components[index].label_size = std::char_traits<char>::length(label);
        components[index].digest = authority.compatibility.components[index];
    }
    const auto rebuilt = context_store_build_compatibility_expectation_v1(
        components.data(), components.size());
    return rebuilt.status == context_store_compatibility_build_status_v1::built &&
        rebuilt.expectation.root == authority.compatibility.root;
}

bool context_store_world1_cache_authority_v1_matches(
        const context_store_world1_cache_authority_v1 & left,
        const context_store_world1_cache_authority_v1 & right) noexcept {
    return same_authority(left, right);
}

context_store_world1_prefix_lookup_result_v1
context_store_world1_prefix_lookup_v1(
        const context_store_world1_prefix_lookup_request_v1 & request) noexcept {
    context_store_world1_prefix_lookup_result_v1 result;
    result.residual_tokens = request.exact_session.token_count;
    const auto started = clock_type::now();
    const auto finish = [&result, started]() noexcept {
        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            clock_type::now() - started).count();
        result.validation_time_ns = elapsed <= 0 ? 0 : static_cast<uint64_t>(elapsed);
        return std::move(result);
    };

    if (!request.enabled) return finish();
    result.fallback =
        context_store_world1_prefix_fallback_v1::live_authority_unavailable;
    if (request.authority == nullptr) return finish();
    if (!context_store_world1_cache_authority_v1_is_valid(*request.authority)) {
        result.fallback =
            context_store_world1_prefix_fallback_v1::live_authority_invalid;
        return finish();
    }
    if (request.expected_model_generation == 0 ||
        request.expected_model_generation != request.authority->model_generation) {
        result.fallback =
            context_store_world1_prefix_fallback_v1::authority_generation_changed;
        return finish();
    }
    if (request.catalog == nullptr) {
        result.fallback =
            context_store_world1_prefix_fallback_v1::catalog_unavailable;
        return finish();
    }
    if (request.policy_epoch == 0 || request.exact_session.tokens == nullptr ||
        request.exact_session.token_count == 0 ||
        request.exact_session.logical_boundary != request.exact_session.token_count ||
        request.exact_session.output_boundary != request.exact_session.token_count ||
        !exact_session_authority_matches(*request.authority, request.exact_session) ||
        !context_store_transformer_profile_v1_is_admitted(request.profile) ||
        request.profile.world_size != 1 || request.profile.rank != 0 ||
        request.profile.architecture !=
            context_store_transformer_architecture_v1::transformer) {
        result.fallback = context_store_world1_prefix_fallback_v1::invalid_request;
        return finish();
    }

    // ADR-0051 probes exact candidates under separate bounded catalog locks.
    // Hold the catalog's mutation-only custody token across discovery and the
    // complete longest-to-shortest selector call so publication cannot
    // interleave and create a mixed catalog view.
    auto mutation_custody = request.catalog->acquire_mutation_custody();
    if (!mutation_custody.owns_custody()) {
        result.fallback = context_store_world1_prefix_fallback_v1::catalog_busy;
        return finish();
    }

    context_store_v1_catalog_prefix_query query;
    query.compatibility_root = request.authority->compatibility.root;
    query.producer_identity = request.authority->producer_identity;
    query.scope_namespace = request.exact_session.scope_namespace;
    query.policy_epoch = request.policy_epoch;
    query.max_token_count = request.exact_session.token_count;
    query.profile = request.profile;
    const auto discovered = request.catalog->discover_prefix_token_counts(query);
    if (discovered.status != context_store_v1_catalog_status::ready) {
        result.fallback = map_catalog(discovered.status);
        return finish();
    }

    context_store_v1_prefix_selector_request selection;
    selection.exact_session = request.exact_session;
    selection.candidate_boundaries = discovered.token_counts.data();
    selection.candidate_boundary_count = discovered.token_count;
    selection.policy_epoch = request.policy_epoch;
    selection.profile = request.profile;
    auto selected = context_store_v1_restore_longest_prefix(
        *request.catalog, selection);
    result.candidates_examined = selected.candidates_examined;
    if (!selected.hit()) {
        result.fallback = map_selector(selected.fallback_reason);
        return finish();
    }
    auto selected_session = request.exact_session;
    selected_session.token_count = selected.matched_token_count;
    selected_session.logical_boundary = selected.matched_token_count;
    selected_session.output_boundary = selected.matched_token_count;
    const auto resolved = context_store_resolve_exact_session_v1(selected_session);
    context_store_identity selected_identity;
    selected_identity.compatibility_root = request.exact_session.compatibility_root;
    selected_identity.scope_namespace = request.exact_session.scope_namespace;
    selected_identity.checkpoint_lineage_id = resolved.session_id;
    selected_identity.policy_epoch = request.policy_epoch;
    if (!resolved.resolved() ||
        !same_identity(selected.snapshot.compatibility_identity,
                       selected_identity)) {
        wipe_snapshot(selected.snapshot);
        result.fallback =
            context_store_world1_prefix_fallback_v1::authenticated_state_corrupt;
        return finish();
    }
    result.source = selected.matched_token_count == request.exact_session.token_count
        ? context_store_world1_prefix_source_v1::exact
        : context_store_world1_prefix_source_v1::prefix;
    result.fallback = context_store_world1_prefix_fallback_v1::none;
    result.selected_prefix_tokens = selected.matched_token_count;
    result.restored_tokens = selected.restored_token_count;
    result.residual_tokens = selected.residual_token_count;
    result.selected_identity = selected_identity;
    result.bound_authority = *request.authority;
    result.authority_bound = true;
    result.snapshot = std::move(selected.snapshot);
    return finish();
}

namespace {

template <typename Apply>
context_store_world1_prefix_install_result_v1 install(
        const context_store_world1_prefix_install_request_v1 & request,
        Apply && apply) noexcept {
    context_store_world1_prefix_install_result_v1 result;
    result.residual_tokens = request.full_token_count;
    if (request.lookup == nullptr) return result;
    auto & lookup = *request.lookup;
    const auto finish = [&lookup, &result]() noexcept {
        consume_lookup(lookup);
        return result;
    };
    if (request.authority == nullptr ||
        !context_store_world1_cache_authority_v1_is_valid(*request.authority) ||
        request.expected_model_generation == 0 ||
        request.expected_model_generation != request.authority->model_generation) {
        result.status =
            context_store_world1_prefix_install_status_v1::
                authority_generation_changed;
        return finish();
    }
    if (!lookup.hit()) return finish();
    if (!lookup.authority_bound ||
        !same_authority(lookup.bound_authority, *request.authority)) {
        result.status =
            context_store_world1_prefix_install_status_v1::authority_changed;
        return finish();
    }
    if (request.context == nullptr || request.sequence < 0 ||
        request.sequence_limit == 0 ||
        static_cast<size_t>(request.sequence) >= request.sequence_limit ||
        request.full_tokens == nullptr || request.full_token_count == 0 ||
        lookup.selected_prefix_tokens == 0 ||
        lookup.selected_prefix_tokens > request.full_token_count ||
        lookup.restored_tokens != lookup.selected_prefix_tokens ||
        lookup.residual_tokens !=
            request.full_token_count - lookup.selected_prefix_tokens ||
        lookup.snapshot.tokens.size() != lookup.selected_prefix_tokens ||
        !same_identity(lookup.snapshot.compatibility_identity,
                       lookup.selected_identity) ||
        lookup.selected_identity.compatibility_root !=
            request.authority->compatibility.root ||
        !context_store_transformer_profile_v1_is_admitted(request.profile) ||
        request.profile.world_size != 1 || request.profile.rank != 0 ||
        request.profile.architecture !=
            context_store_transformer_architecture_v1::transformer) {
        return finish();
    }
    result.state_status = apply(
        lookup.snapshot, request.full_tokens, lookup.selected_prefix_tokens,
        lookup.selected_identity, request.profile, request.limits);
    if (result.state_status != context_store_transformer_status_v1::restored) {
        result.status =
            context_store_world1_prefix_install_status_v1::state_apply_failed;
        return finish();
    }
    result.status = context_store_world1_prefix_install_status_v1::installed;
    result.installed_prefix_tokens = lookup.selected_prefix_tokens;
    result.residual_tokens = lookup.residual_tokens;
    result.state_apply_input_bytes =
        static_cast<uint64_t>(lookup.snapshot.state.size());
    result.state_apply_input_bytes_valid =
        static_cast<size_t>(result.state_apply_input_bytes) ==
            lookup.snapshot.state.size();
    if (!result.state_apply_input_bytes_valid) {
        result.state_apply_input_bytes = 0;
    }
    return finish();
}

} // namespace

context_store_world1_cache_maintenance_total_v1
context_store_world1_finalize_cache_maintenance_v1(
        const context_store_world1_cache_maintenance_measurements_v1 &
            measurements) noexcept {
    context_store_world1_cache_maintenance_total_v1 result;
    if ((!measurements.selected_slot_transition_measured &&
         measurements.selected_slot_transition_ns != 0) ||
        (!measurements.postlaunch_idle_slot_saves_measured &&
         measurements.postlaunch_idle_slot_saves_ns != 0)) {
        return result;
    }
    const uint64_t components[] = {
        measurements.selected_slot_transition_ns,
        measurements.lookup_total_ns,
        measurements.state_install_cleanup_ns,
        measurements.postlaunch_idle_slot_saves_ns,
    };
    uint64_t total = 0;
    for (const uint64_t component : components) {
        if (component > std::numeric_limits<uint64_t>::max() - total) {
            return result;
        }
        total += component;
    }
    result.valid = true;
    result.preprompt_cache_maintenance_ns = total;
    return result;
}

context_store_world1_prefix_install_result_v1
context_store_world1_prefix_install_v1(
        const context_store_world1_prefix_install_request_v1 & request) noexcept {
    return install(request, [&request](
            const context_store_transformer_snapshot_v1 & snapshot,
            const llama_token * tokens,
            size_t token_count,
            const context_store_identity & identity,
            const context_store_transformer_profile_v1 & profile,
            const context_store_transformer_limits_v1 & limits) noexcept {
        return context_store_restore_transformer_state_v1(
            request.context, request.sequence, snapshot, tokens, token_count,
            identity, profile, limits);
    });
}

context_store_world1_prefix_install_result_v1
context_store_world1_prefix_install_v1_with_api(
        const context_store_transformer_state_api_v1 & api,
        const context_store_world1_prefix_install_request_v1 & request) noexcept {
    return install(request, [&api, &request](
            const context_store_transformer_snapshot_v1 & snapshot,
            const llama_token * tokens,
            size_t token_count,
            const context_store_identity & identity,
            const context_store_transformer_profile_v1 & profile,
            const context_store_transformer_limits_v1 & limits) noexcept {
        return context_store_restore_transformer_state_v1_with_api(
            api, request.context, request.sequence, snapshot, tokens,
            token_count, identity, profile, limits);
    });
}

context_store_world1_work_accounting_v1
context_store_world1_finalize_work_accounting_v1(
        size_t request_prompt_tokens, int64_t actual_prompt_tokens) noexcept {
    context_store_world1_work_accounting_v1 result;
    if (actual_prompt_tokens < 0 ||
        static_cast<uint64_t>(actual_prompt_tokens) > request_prompt_tokens) {
        return result;
    }
    result.valid = true;
    result.actual_prompt_tokens = static_cast<size_t>(actual_prompt_tokens);
    result.avoided_prompt_tokens =
        request_prompt_tokens - result.actual_prompt_tokens;
    return result;
}

const char * context_store_world1_prefix_source_name_v1(
        context_store_world1_prefix_source_v1 source) noexcept {
    switch (source) {
        case context_store_world1_prefix_source_v1::cold: return "cold";
        case context_store_world1_prefix_source_v1::exact: return "exact";
        case context_store_world1_prefix_source_v1::prefix: return "prefix";
    }
    return "cold";
}

const char * context_store_world1_prefix_fallback_name_v1(
        context_store_world1_prefix_fallback_v1 fallback) noexcept {
    switch (fallback) {
        case context_store_world1_prefix_fallback_v1::none: return "none";
        case context_store_world1_prefix_fallback_v1::feature_off: return "feature-off";
        case context_store_world1_prefix_fallback_v1::live_authority_unavailable: return "live-authority-unavailable";
        case context_store_world1_prefix_fallback_v1::live_authority_invalid: return "live-authority-invalid";
        case context_store_world1_prefix_fallback_v1::authority_generation_changed: return "authority-generation-changed";
        case context_store_world1_prefix_fallback_v1::authority_changed: return "authority-changed";
        case context_store_world1_prefix_fallback_v1::invalid_request: return "invalid-request";
        case context_store_world1_prefix_fallback_v1::catalog_unavailable: return "catalog-unavailable";
        case context_store_world1_prefix_fallback_v1::live_slot_state_present: return "live-slot-state-present";
        case context_store_world1_prefix_fallback_v1::no_authenticated_checkpoint: return "no-authenticated-checkpoint";
        case context_store_world1_prefix_fallback_v1::authenticated_state_corrupt: return "authenticated-state-corrupt";
        case context_store_world1_prefix_fallback_v1::authenticated_state_incompatible: return "authenticated-state-incompatible";
        case context_store_world1_prefix_fallback_v1::catalog_busy: return "catalog-busy";
        case context_store_world1_prefix_fallback_v1::storage_error: return "storage-error";
    }
    return "invalid-request";
}

} // namespace halofpx
