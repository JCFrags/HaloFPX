#pragma once

#include "halofpx-context-store-auth.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_anchor_max_bytes = 1024;

struct context_store_anchor_body {
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest namespace_id {};
    uint64_t policy_epoch = 0;
    context_store_format_digest checkpoint_lineage_id {};
    uint64_t manifest_key_generation = 0;
    uint64_t authority_epoch = 0;
    uint64_t generation = 0;
    context_store_format_digest selected_manifest_digest {};
    bool has_predecessor = false;
    context_store_format_digest predecessor_manifest_digest {};
};

struct context_store_anchor_key_record {
    context_store_key_disposition disposition = context_store_key_disposition::unknown;
    context_store_registered_id key_id;
    uint64_t generation = 0;
    context_store_key_view master_key;
};

struct context_store_anchor_policy {
    context_store_anchor_key_record key;
    context_store_anchor_body expected;
};

enum class context_store_anchor_status : uint8_t {
    authenticated_unadmitted,
    structural_rejection,
    invalid_policy,
    unknown_key,
    revoked_key,
    read_disabled_key,
    key_generation_mismatch,
    authentication_failed,
    authority_mismatch,
    rollback_detected,
    replay_mismatch,
    output_too_small,
};

struct context_store_anchor_result {
    context_store_anchor_status status = context_store_anchor_status::structural_rejection;
    context_store_format_digest envelope_digest {};
    size_t encoded_size = 0;
    bool has_authenticated_anchor() const noexcept {
        return authenticated_ && status == context_store_anchor_status::authenticated_unadmitted;
    }
    const context_store_anchor_body * authenticated_anchor() const noexcept {
        return has_authenticated_anchor() ? &anchor_ : nullptr;
    }

private:
    bool authenticated_ = false;
    context_store_anchor_body anchor_;
    friend context_store_anchor_result context_store_encode_anchor_v1(
        const context_store_anchor_body &, const context_store_anchor_key_record &, uint8_t *, size_t) noexcept;
    friend context_store_anchor_result context_store_verify_anchor_v1(
        const uint8_t *, size_t, const context_store_anchor_policy &) noexcept;
};

// Memory-only deterministic encoder and verifier. Neither result is cache-hit
// eligible and neither API performs I/O or retains borrowed key material.
context_store_anchor_result context_store_encode_anchor_v1(
    const context_store_anchor_body & anchor,
    const context_store_anchor_key_record & key,
    uint8_t * output,
    size_t output_capacity) noexcept;

context_store_anchor_result context_store_verify_anchor_v1(
    const uint8_t * data,
    size_t size,
    const context_store_anchor_policy & policy) noexcept;

const char * context_store_anchor_status_name(context_store_anchor_status status) noexcept;

} // namespace halofpx
