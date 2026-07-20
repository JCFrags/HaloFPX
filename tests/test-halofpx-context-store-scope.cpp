#ifdef NDEBUG
#undef NDEBUG
#endif

#include "halofpx-context-store-scope.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace {

using halofpx::context_store_format_digest;
using halofpx::context_store_scope_bytes;
using halofpx::context_store_scope_policy_v1;
using halofpx::context_store_scope_resolve_status;

template <size_t Size>
context_store_scope_bytes view(const std::array<uint8_t, Size> & value) noexcept {
    return { value.data(), value.size() };
}

context_store_scope_bytes view(std::string_view value) noexcept {
    return { reinterpret_cast<const uint8_t *>(value.data()), value.size() };
}

bool zero(const context_store_format_digest & value) noexcept {
    return std::all_of(value.begin(), value.end(), [](uint8_t byte) { return byte == 0; });
}

context_store_scope_policy_v1 policy(
        const std::array<uint8_t, 32> & key,
        context_store_scope_bytes principal,
        const context_store_format_digest & compatibility) noexcept {
    return {
        view(key),
        view("canary-policy-key"),
        view("llama-server-api-key"),
        principal,
        view("halofpx-private"),
        1,
        compatibility,
    };
}

void deterministic_golden_and_separated() {
    const std::array<uint8_t, 32> key = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
        0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    };
    const std::array<uint8_t, 5> principal = { 'a','l','i','c','e' };
    context_store_format_digest compatibility {};
    for (size_t index = 0; index < compatibility.size(); ++index) {
        compatibility[index] = static_cast<uint8_t>(0x40 + index);
    }

    const auto first = halofpx::context_store_resolve_private_scope_v1(
        policy(key, view(principal), compatibility));
    const auto second = halofpx::context_store_resolve_private_scope_v1(
        policy(key, view(principal), compatibility));
    const context_store_format_digest expected = {
        0x96,0xd2,0xe5,0x65,0xef,0xac,0x1a,0x54,
        0x47,0xee,0xdb,0xd0,0x1e,0x6e,0xea,0x8e,
        0x0a,0xb6,0xad,0x8e,0x3a,0xf3,0xa3,0x72,
        0x87,0x9b,0x2b,0x2a,0x8c,0x6c,0x1b,0xf1,
    };
    assert(first.resolved() && second.resolved());
    assert(first.namespace_id == second.namespace_id);
    assert(first.namespace_id == expected);

    auto changed_principal = principal;
    changed_principal[0] ^= 1;
    assert(halofpx::context_store_resolve_private_scope_v1(
        policy(key, view(changed_principal), compatibility)).namespace_id != first.namespace_id);

    auto changed_key = key;
    changed_key[0] ^= 1;
    assert(halofpx::context_store_resolve_private_scope_v1(
        policy(changed_key, view(principal), compatibility)).namespace_id != first.namespace_id);

    auto changed_compatibility = compatibility;
    changed_compatibility[0] ^= 1;
    assert(halofpx::context_store_resolve_private_scope_v1(
        policy(key, view(principal), changed_compatibility)).namespace_id != first.namespace_id);

    auto changed = policy(key, view(principal), compatibility);
    changed.policy_key_id = view("other-policy-key");
    assert(halofpx::context_store_resolve_private_scope_v1(changed).namespace_id != first.namespace_id);
    changed = policy(key, view(principal), compatibility);
    changed.authentication_issuer = view("other-issuer");
    assert(halofpx::context_store_resolve_private_scope_v1(changed).namespace_id != first.namespace_id);
    changed = policy(key, view(principal), compatibility);
    changed.security_domain = view("other-domain");
    assert(halofpx::context_store_resolve_private_scope_v1(changed).namespace_id != first.namespace_id);
    changed = policy(key, view(principal), compatibility);
    changed.policy_epoch = 2;
    assert(halofpx::context_store_resolve_private_scope_v1(changed).namespace_id != first.namespace_id);
}

void bounds_and_invalid_inputs() {
    std::array<uint8_t, 32> key {};
    key.fill(0xa5);
    context_store_format_digest compatibility {};
    compatibility.fill(0x5a);
    std::array<uint8_t, halofpx::context_store_scope_principal_max_bytes> principal {};
    assert(halofpx::context_store_resolve_private_scope_v1(
        policy(key, view(principal), compatibility)).resolved());

    std::array<uint8_t, halofpx::context_store_scope_principal_max_bytes + 1> too_large {};
    auto value = policy(key, view(too_large), compatibility);
    auto result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_principal && zero(result.namespace_id));

    value = policy(key, { nullptr, 0 }, compatibility);
    result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_principal && zero(result.namespace_id));

    value = policy(key, view(principal), compatibility);
    value.policy_key = { key.data(), key.size() - 1 };
    result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_master_key && zero(result.namespace_id));

    value = policy(key, view(principal), compatibility);
    value.policy_key_id = view("invalid id");
    result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_policy_key_id && zero(result.namespace_id));

    value = policy(key, view(principal), compatibility);
    value.authentication_issuer = { nullptr, 0 };
    result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_authentication_issuer && zero(result.namespace_id));

    value = policy(key, view(principal), compatibility);
    value.security_domain = view("bad domain");
    result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_security_domain && zero(result.namespace_id));

    value = policy(key, view(principal), compatibility);
    value.policy_epoch = 0;
    result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_policy_epoch && zero(result.namespace_id));

    value = policy(key, view(principal), compatibility);
    value.compatibility_root.fill(0);
    result = halofpx::context_store_resolve_private_scope_v1(value);
    assert(result.status == context_store_scope_resolve_status::invalid_compatibility_root && zero(result.namespace_id));
}

} // namespace

int main() {
    deterministic_golden_and_separated();
    bounds_and_invalid_inputs();
    return 0;
}
