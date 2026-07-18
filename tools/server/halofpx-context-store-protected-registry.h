#pragma once

#include "halofpx-context-store-auth.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_protected_registry_max_bytes = 1024;

struct context_store_protected_registry_key_record {
    context_store_key_disposition disposition = context_store_key_disposition::unknown;
    context_store_registered_id key_id;
    uint64_t generation = 0;
    context_store_key_view master_key;
};

struct context_store_protected_registry_body {
    context_store_registered_id registry_id;
    uint64_t registry_epoch = 0;
    context_store_format_digest authority_base_scope_commitment {};
    context_store_format_digest policy_commitment {};
    uint64_t last_consumed_sequence = 0;
};

enum class context_store_protected_registry_status : uint8_t {
    authenticated_unadmitted,
    structural_rejection,
    output_too_small,
    invalid_policy,
    unknown_key,
    revoked_key,
    read_disabled_key,
    key_generation_mismatch,
    authentication_failed,
};

class context_store_authenticated_protected_registry;
struct context_store_protected_registry_result;

class context_store_authenticated_protected_registry {
public:
    context_store_authenticated_protected_registry() = default;
    const context_store_protected_registry_body * body() const noexcept { return valid_ ? &body_ : nullptr; }
    const context_store_format_digest * envelope_digest() const noexcept { return valid_ ? &envelope_digest_ : nullptr; }
    const context_store_registered_id * key_id() const noexcept { return valid_ ? &key_id_ : nullptr; }
    uint64_t key_generation() const noexcept { return valid_ ? key_generation_ : 0; }
    const context_store_format_digest * authority_binding() const noexcept { return valid_ ? &authority_binding_ : nullptr; }
    const context_store_format_digest * key_continuity_commitment() const noexcept { return valid_ ? &key_continuity_commitment_ : nullptr; }
    const uint8_t * envelope_data() const noexcept { return valid_ ? envelope_.data() : nullptr; }
    size_t envelope_size() const noexcept { return valid_ ? envelope_size_ : 0; }

private:
    bool valid_ = false;
    context_store_protected_registry_body body_;
    context_store_format_digest envelope_digest_ {};
    context_store_registered_id key_id_;
    uint64_t key_generation_ = 0;
    context_store_format_digest authority_binding_ {};
    context_store_format_digest key_continuity_commitment_ {};
    std::array<uint8_t, context_store_protected_registry_max_bytes> envelope_ {};
    size_t envelope_size_ = 0;
    friend struct context_store_protected_registry_result;
};

struct context_store_protected_registry_result {
    context_store_protected_registry_status status = context_store_protected_registry_status::structural_rejection;
    size_t encoded_size = 0;
    const context_store_authenticated_protected_registry * authenticated_carrier() const noexcept {
        return status == context_store_protected_registry_status::authenticated_unadmitted && carrier_.valid_ ? &carrier_ : nullptr;
    }
private:
    context_store_authenticated_protected_registry carrier_;
    void set_authenticated(const context_store_protected_registry_body & body,
        const context_store_protected_registry_key_record & key,
        const context_store_format_digest & envelope_digest,
        const context_store_format_digest & authority_binding,
        const context_store_format_digest & key_continuity_commitment,
        const uint8_t * envelope, size_t envelope_size) noexcept;
    friend context_store_protected_registry_result context_store_encode_protected_registry_v1(
        const context_store_protected_registry_body &, const context_store_protected_registry_key_record &,
        uint8_t *, size_t) noexcept;
    friend context_store_protected_registry_result context_store_verify_protected_registry_v1(
        const uint8_t *, size_t, const context_store_protected_registry_key_record &) noexcept;
};

context_store_protected_registry_result context_store_encode_protected_registry_v1(
    const context_store_protected_registry_body & body,
    const context_store_protected_registry_key_record & key,
    uint8_t * output, size_t capacity) noexcept;

context_store_protected_registry_result context_store_verify_protected_registry_v1(
    const uint8_t * data, size_t size,
    const context_store_protected_registry_key_record & key) noexcept;

const char * context_store_protected_registry_status_name(context_store_protected_registry_status status) noexcept;

} // namespace halofpx
