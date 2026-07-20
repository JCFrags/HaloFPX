#pragma once

#include "halofpx-context-store-auth.h"

#include <cstddef>

namespace halofpx {

// The order and labels are the closed ADR-0003/context-store-v1 registry.
// Component digests are trusted authority inputs; this seam does not encode or
// validate the corresponding component submanifests.
constexpr size_t context_store_compatibility_v1_component_count =
    context_store_compatibility_component_count;

struct context_store_compatibility_component_digest_v1 {
    const char * label = nullptr;
    size_t label_size = 0;
    context_store_format_digest digest {};
};

enum class context_store_compatibility_build_status_v1 : uint8_t {
    built,
    invalid_input,
    wrong_component_count,
    unknown_component,
    duplicate_component,
    misordered_component,
    zero_component_digest,
};

struct context_store_compatibility_build_result_v1 {
    context_store_compatibility_build_status_v1 status =
        context_store_compatibility_build_status_v1::invalid_input;
    context_store_compatibility_expectation expectation {};
};

const char * context_store_compatibility_component_label_v1(size_t index) noexcept;

context_store_compatibility_build_result_v1 context_store_build_compatibility_expectation_v1(
    const context_store_compatibility_component_digest_v1 * components,
    size_t component_count) noexcept;

const char * context_store_compatibility_build_status_name_v1(
    context_store_compatibility_build_status_v1 status) noexcept;

} // namespace halofpx
