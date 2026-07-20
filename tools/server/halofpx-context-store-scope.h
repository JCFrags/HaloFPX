#pragma once

#include "halofpx-context-store-auth.h"

#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_scope_master_key_bytes = 32;
constexpr size_t context_store_scope_principal_max_bytes = 16384;
constexpr size_t context_store_scope_registered_id_max_bytes = 128;

struct context_store_scope_bytes {
    const uint8_t * data = nullptr;
    size_t size = 0;
};

enum class context_store_scope_resolve_status : uint8_t {
    resolved,
    invalid_master_key,
    invalid_policy_key_id,
    invalid_authentication_issuer,
    invalid_principal,
    invalid_security_domain,
    invalid_policy_epoch,
    invalid_compatibility_root,
};

struct context_store_scope_policy_v1 {
    context_store_scope_bytes policy_key;
    context_store_scope_bytes policy_key_id;
    context_store_scope_bytes authentication_issuer;
    context_store_scope_bytes authenticated_principal;
    context_store_scope_bytes security_domain;
    uint64_t policy_epoch = 0;
    context_store_format_digest compatibility_root {};
};

struct context_store_scope_resolve_result {
    context_store_scope_resolve_status status = context_store_scope_resolve_status::invalid_master_key;
    context_store_format_digest namespace_id {};

    bool resolved() const noexcept {
        return status == context_store_scope_resolve_status::resolved;
    }
};

// Resolves trusted authentication-layer bytes into an opaque private namespace.
// Rejection returns a zero namespace and performs no I/O or identity retention.
context_store_scope_resolve_result context_store_resolve_private_scope_v1(
    const context_store_scope_policy_v1 & policy) noexcept;

} // namespace halofpx
