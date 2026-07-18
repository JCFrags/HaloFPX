#pragma once

#include "halofpx-context-store-protected-registry.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_protected_registry_successor_max_bytes = 1024;

struct context_store_bootstrap_consumption_receipt {
    uint64_t authorization_sequence = 0;
    context_store_format_digest command_id {};
    context_store_format_digest authorization_token_digest {};
    context_store_format_digest plan_commitment {};
    context_store_format_digest selected_manifest_digest {};
    context_store_format_digest proposed_anchor_envelope_digest {};
};

struct context_store_protected_registry_successor_body {
    context_store_registered_id registry_id;
    uint64_t registry_epoch = 0;
    context_store_format_digest authority_base_scope_commitment {};
    context_store_format_digest policy_commitment {};
    uint64_t consumed_authorization_high_water = 0;
    context_store_format_digest predecessor_snapshot_envelope_digest {};
    context_store_bootstrap_consumption_receipt receipt;
};

enum class context_store_protected_registry_successor_status : uint8_t {
    authenticated_unadmitted, structural_rejection, output_too_small, invalid_policy,
    unknown_key, revoked_key, read_disabled_key, key_generation_mismatch, authentication_failed,
};

class context_store_authenticated_protected_registry_successor {
public:
    context_store_authenticated_protected_registry_successor() = default;
    bool authenticated() const noexcept { return valid_; }
    const context_store_protected_registry_successor_body * body() const noexcept { return valid_ ? &body_ : nullptr; }
    const context_store_registered_id * key_id() const noexcept { return valid_ ? &key_id_ : nullptr; }
    uint64_t key_generation() const noexcept { return valid_ ? key_generation_ : 0; }
    const context_store_format_digest * envelope_digest() const noexcept { return valid_ ? &envelope_digest_ : nullptr; }
    const context_store_format_digest * authority_binding() const noexcept { return valid_ ? &authority_binding_ : nullptr; }
    const context_store_format_digest * key_continuity_commitment() const noexcept { return valid_ ? &key_continuity_commitment_ : nullptr; }
    const uint8_t * envelope_data() const noexcept { return valid_ ? envelope_.data() : nullptr; }
    size_t envelope_size() const noexcept { return valid_ ? envelope_size_ : 0; }
private:
    bool valid_ = false;
    context_store_protected_registry_successor_body body_;
    context_store_registered_id key_id_;
    uint64_t key_generation_ = 0;
    context_store_format_digest envelope_digest_ {}, authority_binding_ {}, key_continuity_commitment_ {};
    std::array<uint8_t, context_store_protected_registry_successor_max_bytes> envelope_ {};
    size_t envelope_size_ = 0;
    friend struct context_store_protected_registry_successor_result;
};

struct context_store_protected_registry_successor_result {
    context_store_protected_registry_successor_status status = context_store_protected_registry_successor_status::structural_rejection;
    size_t encoded_size = 0;
    const context_store_authenticated_protected_registry_successor * authenticated_carrier() const noexcept {
        return status == context_store_protected_registry_successor_status::authenticated_unadmitted && carrier_.valid_ ? &carrier_ : nullptr;
    }
private:
    context_store_authenticated_protected_registry_successor carrier_;
    void set_authenticated(const context_store_protected_registry_successor_body &, const context_store_protected_registry_key_record &,
        const context_store_format_digest &, const context_store_format_digest &, const context_store_format_digest &,
        const uint8_t *, size_t) noexcept;
    friend context_store_protected_registry_successor_result context_store_encode_protected_registry_successor_v1(
        const context_store_protected_registry_successor_body &, const context_store_protected_registry_key_record &, uint8_t *, size_t) noexcept;
    friend context_store_protected_registry_successor_result context_store_verify_protected_registry_successor_v1(
        const uint8_t *, size_t, const context_store_protected_registry_key_record &) noexcept;
};

context_store_protected_registry_successor_result context_store_encode_protected_registry_successor_v1(
    const context_store_protected_registry_successor_body &, const context_store_protected_registry_key_record &, uint8_t *, size_t) noexcept;
context_store_protected_registry_successor_result context_store_verify_protected_registry_successor_v1(
    const uint8_t *, size_t, const context_store_protected_registry_key_record &) noexcept;
const char * context_store_protected_registry_successor_status_name(context_store_protected_registry_successor_status) noexcept;

} // namespace halofpx
