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

struct context_store_anchor_result;

// Owned, copyable proof carrier created only by the bounded encoder/verifier.
// A public default instance is deliberately unauthenticated; callers cannot
// populate or mark it trusted. It retains no key material.
class context_store_authenticated_anchor {
public:
    context_store_authenticated_anchor() = default;

    bool authenticated() const noexcept { return authenticated_; }
    const context_store_anchor_body * body() const noexcept { return authenticated_ ? &body_ : nullptr; }
    const context_store_registered_id * authentication_key_id() const noexcept { return authenticated_ ? &key_id_ : nullptr; }
    uint64_t authentication_key_generation() const noexcept { return authenticated_ ? key_generation_ : 0; }
    const context_store_format_digest * authority_binding() const noexcept { return authenticated_ ? &authority_binding_ : nullptr; }
    const context_store_format_digest * envelope_digest() const noexcept { return authenticated_ ? &digest_ : nullptr; }
    const uint8_t * envelope_data() const noexcept { return authenticated_ ? envelope_.data() : nullptr; }
    size_t envelope_size() const noexcept { return authenticated_ ? envelope_size_ : 0; }

private:
    bool authenticated_ = false;
    context_store_anchor_body body_;
    context_store_registered_id key_id_;
    uint64_t key_generation_ = 0;
    context_store_format_digest authority_binding_ {};
    context_store_format_digest digest_ {};
    std::array<uint8_t, context_store_anchor_max_bytes> envelope_ {};
    size_t envelope_size_ = 0;

    friend context_store_anchor_result context_store_encode_anchor_v1(
        const context_store_anchor_body &, const context_store_anchor_key_record &, uint8_t *, size_t) noexcept;
    friend context_store_anchor_result context_store_verify_anchor_v1(
        const uint8_t *, size_t, const context_store_anchor_policy &) noexcept;
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
        return carrier_.authenticated() && status == context_store_anchor_status::authenticated_unadmitted;
    }
    const context_store_anchor_body * authenticated_anchor() const noexcept {
        return has_authenticated_anchor() ? carrier_.body() : nullptr;
    }
    const context_store_authenticated_anchor * authenticated_carrier() const noexcept {
        return has_authenticated_anchor() ? &carrier_ : nullptr;
    }

private:
    context_store_authenticated_anchor carrier_;
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
