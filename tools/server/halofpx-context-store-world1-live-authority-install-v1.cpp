#include "halofpx-context-store-world1-live-authority-install-v1.h"

#include <new>

namespace halofpx {

namespace {

bool authority_identities_are_distinct(
        const context_store_world1_cache_authority_v1 & authority) noexcept {
    return authority.producer_identity != authority.global_plan_digest &&
        authority.producer_identity != authority.rank_ownership_digest &&
        authority.producer_identity != authority.rank_placement_digest &&
        authority.global_plan_digest != authority.rank_ownership_digest &&
        authority.global_plan_digest != authority.rank_placement_digest &&
        authority.rank_ownership_digest != authority.rank_placement_digest;
}

} // namespace

context_store_world1_live_authority_install_result_v1
context_store_install_world1_live_authority_v1(
        const context_store_world1_live_authority_install_request_v1 & request) noexcept {
    context_store_world1_live_authority_install_result_v1 result;
    if (!request.enabled) return result;

    result.status =
        context_store_world1_live_authority_install_status_v1::source_unavailable;
    if (request.source == nullptr) return result;

    if (request.expected_model_generation == 0) {
        result.status = context_store_world1_live_authority_install_status_v1::
            model_generation_unavailable;
        return result;
    }

    const auto snapshot = request.source->capture();
    if (snapshot.source_kind != context_store_world1_live_authority_source_kind_v1::
            trusted_live_loader_context_lifecycle) {
        result.status =
            context_store_world1_live_authority_install_status_v1::untrusted_source;
        return result;
    }
    if (snapshot.captured_facts !=
            context_store_world1_live_authority_required_facts_v1) {
        result.status = context_store_world1_live_authority_install_status_v1::
            incomplete_fact_custody;
        return result;
    }
    if (!context_store_world1_cache_authority_v1_is_valid(snapshot.authority) ||
            !authority_identities_are_distinct(snapshot.authority)) {
        result.status =
            context_store_world1_live_authority_install_status_v1::invalid_authority;
        return result;
    }
    if (snapshot.authority.model_generation != request.expected_model_generation) {
        result.status = context_store_world1_live_authority_install_status_v1::
            model_generation_changed;
        return result;
    }

    const auto * owned = new (std::nothrow)
        context_store_world1_cache_authority_v1(snapshot.authority);
    if (owned == nullptr) {
        result.status =
            context_store_world1_live_authority_install_status_v1::allocation_failed;
        return result;
    }
    result.authority.reset(owned);
    result.status = context_store_world1_live_authority_install_status_v1::installed;
    return result;
}

const char * context_store_world1_live_authority_install_status_name_v1(
        context_store_world1_live_authority_install_status_v1 status) noexcept {
    switch (status) {
        case context_store_world1_live_authority_install_status_v1::installed:
            return "installed";
        case context_store_world1_live_authority_install_status_v1::feature_off:
            return "feature-off";
        case context_store_world1_live_authority_install_status_v1::source_unavailable:
            return "source-unavailable";
        case context_store_world1_live_authority_install_status_v1::untrusted_source:
            return "untrusted-source";
        case context_store_world1_live_authority_install_status_v1::incomplete_fact_custody:
            return "incomplete-fact-custody";
        case context_store_world1_live_authority_install_status_v1::invalid_authority:
            return "invalid-authority";
        case context_store_world1_live_authority_install_status_v1::model_generation_unavailable:
            return "model-generation-unavailable";
        case context_store_world1_live_authority_install_status_v1::model_generation_changed:
            return "model-generation-changed";
        case context_store_world1_live_authority_install_status_v1::allocation_failed:
            return "allocation-failed";
    }
    return "unknown";
}

} // namespace halofpx
