#include "halofpx-context-store-scope.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {
namespace {

constexpr uint8_t namespace_domain[] = {
    'h','a','l','o','f','p','x','.',
    'n','a','m','e','s','p','a','c','e','.',
    'v','1',0
};

constexpr size_t cbor_head_max_bytes = 9;
constexpr size_t preimage_max_bytes = sizeof(namespace_domain) + 1 + 7 +
    3 * (cbor_head_max_bytes + context_store_scope_registered_id_max_bytes) +
    cbor_head_max_bytes + context_store_scope_principal_max_bytes +
    cbor_head_max_bytes + 1 + 2 + 32;

void wipe(void * memory, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(memory);
    while (size-- != 0) {
        *bytes++ = 0;
    }
}

bool valid_required_span(context_store_scope_bytes value, size_t maximum) noexcept {
    return value.data != nullptr && value.size != 0 && value.size <= maximum;
}

bool valid_registered_id(context_store_scope_bytes value) noexcept {
    if (!valid_required_span(value, context_store_scope_registered_id_max_bytes)) {
        return false;
    }
    for (size_t index = 0; index < value.size; ++index) {
        if (value.data[index] < 0x21 || value.data[index] > 0x7e) return false;
    }
    return true;
}

void append_bytes(
        std::array<uint8_t, preimage_max_bytes> & preimage,
        size_t & used,
        const uint8_t * value,
        size_t size) noexcept {
    for (size_t index = 0; index < size; ++index) {
        preimage[used + index] = value[index];
    }
    used += size;
}

void append_cbor_head(
        std::array<uint8_t, preimage_max_bytes> & preimage,
        size_t & used,
        uint8_t major,
        uint64_t value) noexcept {
    if (value < 24) {
        preimage[used++] = static_cast<uint8_t>((major << 5) | value);
    } else if (value <= UINT8_MAX) {
        preimage[used++] = static_cast<uint8_t>((major << 5) | 24);
        preimage[used++] = static_cast<uint8_t>(value);
    } else if (value <= UINT16_MAX) {
        preimage[used++] = static_cast<uint8_t>((major << 5) | 25);
        preimage[used++] = static_cast<uint8_t>(value >> 8);
        preimage[used++] = static_cast<uint8_t>(value);
    } else if (value <= UINT32_MAX) {
        preimage[used++] = static_cast<uint8_t>((major << 5) | 26);
        for (size_t index = 0; index < 4; ++index) {
            preimage[used++] = static_cast<uint8_t>(value >> ((3 - index) * 8));
        }
    } else {
        preimage[used++] = static_cast<uint8_t>((major << 5) | 27);
        for (size_t index = 0; index < 8; ++index) {
            preimage[used++] = static_cast<uint8_t>(value >> ((7 - index) * 8));
        }
    }
}

void append_registered_id(
        std::array<uint8_t, preimage_max_bytes> & preimage,
        size_t & used,
        context_store_scope_bytes value) noexcept {
    append_cbor_head(preimage, used, 3, value.size);
    append_bytes(preimage, used, value.data, value.size);
}

void append_byte_string(
        std::array<uint8_t, preimage_max_bytes> & preimage,
        size_t & used,
        const uint8_t * value,
        size_t size) noexcept {
    append_cbor_head(preimage, used, 2, size);
    append_bytes(preimage, used, value, size);
}

bool nonzero(const context_store_format_digest & value) noexcept {
    uint8_t combined = 0;
    for (const uint8_t byte : value) combined |= byte;
    return combined != 0;
}

} // namespace

context_store_scope_resolve_result context_store_resolve_private_scope_v1(
        const context_store_scope_policy_v1 & policy) noexcept {
    context_store_scope_resolve_result result;
    if (policy.policy_key.data == nullptr ||
        policy.policy_key.size != context_store_scope_master_key_bytes) {
        return result;
    }
    if (!valid_registered_id(policy.policy_key_id)) {
        result.status = context_store_scope_resolve_status::invalid_policy_key_id;
        return result;
    }
    if (!valid_registered_id(policy.authentication_issuer)) {
        result.status = context_store_scope_resolve_status::invalid_authentication_issuer;
        return result;
    }
    if (!valid_required_span(policy.authenticated_principal, context_store_scope_principal_max_bytes)) {
        result.status = context_store_scope_resolve_status::invalid_principal;
        return result;
    }
    if (!valid_registered_id(policy.security_domain)) {
        result.status = context_store_scope_resolve_status::invalid_security_domain;
        return result;
    }
    if (policy.policy_epoch == 0) {
        result.status = context_store_scope_resolve_status::invalid_policy_epoch;
        return result;
    }
    if (!nonzero(policy.compatibility_root)) {
        result.status = context_store_scope_resolve_status::invalid_compatibility_root;
        return result;
    }

    std::array<uint8_t, preimage_max_bytes> preimage {};
    size_t used = 0;
    append_bytes(preimage, used, namespace_domain, sizeof(namespace_domain));
    append_cbor_head(preimage, used, 5, 7);
    append_cbor_head(preimage, used, 0, 0);
    append_registered_id(preimage, used, policy.policy_key_id);
    append_cbor_head(preimage, used, 0, 1);
    append_registered_id(preimage, used, policy.authentication_issuer);
    append_cbor_head(preimage, used, 0, 2);
    append_byte_string(preimage, used,
        policy.authenticated_principal.data, policy.authenticated_principal.size);
    append_cbor_head(preimage, used, 0, 3);
    append_registered_id(preimage, used, policy.security_domain);
    append_cbor_head(preimage, used, 0, 4);
    append_cbor_head(preimage, used, 0, policy.policy_epoch);
    append_cbor_head(preimage, used, 0, 5);
    append_cbor_head(preimage, used, 0, 0);
    append_cbor_head(preimage, used, 0, 6);
    append_byte_string(preimage, used,
        policy.compatibility_root.data(), policy.compatibility_root.size());

    const bool derived = context_store_hmac_sha256(
        policy.policy_key.data,
        policy.policy_key.size,
        preimage.data(),
        used,
        result.namespace_id);
    wipe(preimage.data(), preimage.size());
    if (!derived) {
        result.namespace_id.fill(0);
        return result;
    }
    result.status = context_store_scope_resolve_status::resolved;
    return result;
}

} // namespace halofpx
