#pragma once

#include "halofpx-context-store-auth.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_bootstrap_token_max_bytes = 2048;
using context_store_bootstrap_command_id = std::array<uint8_t, 32>;

struct context_store_bootstrap_admin_key_record {
    context_store_key_disposition disposition = context_store_key_disposition::unknown;
    context_store_registered_id key_id;
    uint64_t generation = 0;
    context_store_key_view master_key;
};

struct context_store_bootstrap_token_body {
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest namespace_id {};
    uint64_t policy_epoch = 0;
    context_store_format_digest checkpoint_lineage_id {};
    context_store_registered_id manifest_key_id;
    uint64_t manifest_key_generation = 0;
    context_store_format_digest compatibility_root {};
    uint64_t authority_epoch = 0;
    context_store_registered_id anchor_key_id;
    uint64_t anchor_key_generation = 0;
    context_store_format_digest selected_manifest_digest {};
    context_store_format_digest authority_scope_commitment {};
    context_store_registered_id protected_registry_id;
    uint64_t protected_registry_epoch = 0;
    context_store_format_digest protected_registry_snapshot_digest {};
    context_store_format_digest protected_registry_policy_digest {};
    uint64_t authorization_sequence = 0;
    context_store_bootstrap_command_id command_id {};
};

class context_store_authenticated_bootstrap_token;
struct context_store_bootstrap_token_result;

enum class context_store_bootstrap_token_status : uint8_t {
    authenticated_unconsumed,
    structural_rejection,
    output_too_small,
    invalid_policy,
    unknown_key,
    revoked_key,
    read_disabled_key,
    key_generation_mismatch,
    authentication_failed,
};

class context_store_authenticated_bootstrap_token {
public:
    context_store_authenticated_bootstrap_token() = default;
    const context_store_bootstrap_token_body * body() const noexcept { return valid_ ? &body_ : nullptr; }
    const context_store_format_digest * envelope_digest() const noexcept { return valid_ ? &envelope_digest_ : nullptr; }
    const context_store_registered_id * admin_key_id() const noexcept { return valid_ ? &admin_key_id_ : nullptr; }
    uint64_t admin_key_generation() const noexcept { return valid_ ? admin_key_generation_ : 0; }

private:
    bool valid_ = false;
    context_store_bootstrap_token_body body_;
    context_store_format_digest envelope_digest_ {};
    context_store_registered_id admin_key_id_;
    uint64_t admin_key_generation_ = 0;
    friend struct context_store_bootstrap_token_result;
    friend context_store_bootstrap_token_result context_store_encode_bootstrap_token_v1(
        const context_store_bootstrap_token_body &, const context_store_bootstrap_admin_key_record &,
        uint8_t *, size_t) noexcept;
    friend context_store_bootstrap_token_result context_store_verify_bootstrap_token_v1(
        const uint8_t *, size_t, const context_store_bootstrap_admin_key_record &) noexcept;
};

struct context_store_bootstrap_token_result {
    context_store_bootstrap_token_status status = context_store_bootstrap_token_status::structural_rejection;
    size_t encoded_size = 0;
    const context_store_authenticated_bootstrap_token * authenticated_carrier() const noexcept {
        return status == context_store_bootstrap_token_status::authenticated_unconsumed && carrier_.valid_ ? &carrier_ : nullptr;
    }
private:
    context_store_authenticated_bootstrap_token carrier_;
    void set_authenticated(const context_store_bootstrap_token_body & body,
            const context_store_bootstrap_admin_key_record & key,
            const context_store_format_digest & digest) noexcept {
        carrier_.body_ = body;
        carrier_.admin_key_id_ = key.key_id;
        carrier_.admin_key_generation_ = key.generation;
        carrier_.envelope_digest_ = digest;
        carrier_.valid_ = true;
        status = context_store_bootstrap_token_status::authenticated_unconsumed;
    }
    friend context_store_bootstrap_token_result context_store_encode_bootstrap_token_v1(
        const context_store_bootstrap_token_body &, const context_store_bootstrap_admin_key_record &,
        uint8_t *, size_t) noexcept;
    friend context_store_bootstrap_token_result context_store_verify_bootstrap_token_v1(
        const uint8_t *, size_t, const context_store_bootstrap_admin_key_record &) noexcept;
};

context_store_bootstrap_token_result context_store_encode_bootstrap_token_v1(
    const context_store_bootstrap_token_body & body,
    const context_store_bootstrap_admin_key_record & key,
    uint8_t * output,
    size_t capacity) noexcept;

context_store_bootstrap_token_result context_store_verify_bootstrap_token_v1(
    const uint8_t * data,
    size_t size,
    const context_store_bootstrap_admin_key_record & key) noexcept;

const char * context_store_bootstrap_token_status_name(context_store_bootstrap_token_status status) noexcept;

} // namespace halofpx
