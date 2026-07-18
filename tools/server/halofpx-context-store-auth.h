#pragma once

#include "halofpx-context-store-format.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_master_key_max_bytes = 1024;

enum class context_store_key_disposition : uint8_t {
    active,
    unknown,
    revoked,
    read_disabled,
};

enum class context_store_manifest_verify_status : uint8_t {
    authenticated_unadmitted,
    structural_rejection,
    invalid_policy,
    unknown_key,
    revoked_key,
    read_disabled_key,
    key_generation_mismatch,
    authentication_failed,
    authority_mismatch,
    replay_mismatch,
    compatibility_corrupt,
    compatibility_mismatch,
};

struct context_store_key_view {
    // Borrowed synchronously. The verifier never retains or mutates key bytes.
    const uint8_t * data = nullptr;
    size_t size = 0;
};

struct context_store_manifest_key_record {
    context_store_key_disposition disposition = context_store_key_disposition::unknown;
    context_store_registered_id key_id;
    uint64_t generation = 0;
    context_store_key_view master_key;
};

struct context_store_replay_anchor {
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest checkpoint_lineage_id {};
    context_store_format_digest namespace_id {};
    uint64_t policy_epoch = 0;
    uint64_t key_generation = 0;
    uint64_t generation = 0;
    bool has_predecessor = false;
    context_store_format_digest predecessor_manifest_digest {};
    context_store_format_digest selected_manifest_digest {};
};

struct context_store_compatibility_expectation {
    std::array<context_store_format_digest, context_store_compatibility_component_count> components {};
    context_store_format_digest root {};
};

struct context_store_manifest_verification_policy {
    context_store_manifest_key_record key;
    context_store_replay_anchor anchor;
    context_store_compatibility_expectation compatibility;
};

struct context_store_manifest_verify_result {
    context_store_manifest_verify_status status = context_store_manifest_verify_status::structural_rejection;
    context_store_manifest_parse_status parse_status = context_store_manifest_parse_status::input_empty;
    context_store_format_digest manifest_digest {};
    bool has_authenticated_carrier() const noexcept {
        return authenticated_carrier_ &&
            status == context_store_manifest_verify_status::authenticated_unadmitted;
    }

    size_t authenticated_object_count() const noexcept {
        return has_authenticated_carrier() ? object_count_ : 0;
    }

    const context_store_object_reference * authenticated_object_reference(size_t index) const noexcept {
        return has_authenticated_carrier() && index < object_count_ ? &object_references_[index] : nullptr;
    }

private:
    bool authenticated_carrier_ = false;
    size_t object_count_ = 0;
    std::array<context_store_object_reference, context_store_manifest_max_objects> object_references_ {};

    friend context_store_manifest_verify_result context_store_verify_manifest_v1(
        const uint8_t *, size_t, const context_store_manifest_verification_policy &) noexcept;
};

// Offline verification only. Even the sole authenticated result remains a
// miss: no profile or codec is admitted, and this API creates no candidate.
context_store_manifest_verify_result context_store_verify_manifest_v1(
    const uint8_t * data,
    size_t size,
    const context_store_manifest_verification_policy & policy) noexcept;

// Internal deterministic primitives exposed for target-owned golden-vector
// tests. They perform no I/O and return false on invalid pointers or bounds.
bool context_store_sha256(
    const uint8_t * data,
    size_t size,
    context_store_format_digest & digest) noexcept;

bool context_store_sha256_bounded(
    const uint8_t * data,
    size_t size,
    uint64_t max_size,
    context_store_format_digest & digest) noexcept;

bool context_store_hmac_sha256(
    const uint8_t * key,
    size_t key_size,
    const uint8_t * data,
    size_t size,
    context_store_format_digest & tag) noexcept;

const char * context_store_manifest_verify_status_name(
    context_store_manifest_verify_status status) noexcept;

} // namespace halofpx
