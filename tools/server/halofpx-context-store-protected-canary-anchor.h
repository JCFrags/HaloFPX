#pragma once

#include "halofpx-context-store-auth.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_protected_canary_anchor_max_bytes = 1024;
constexpr size_t context_store_protected_canary_anchor_master_key_bytes = 32;

struct context_store_protected_canary_anchor_body {
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

struct context_store_protected_canary_anchor_key {
    context_store_registered_id key_id;
    uint64_t generation = 0;
    context_store_key_view master_key;
};

enum class context_store_protected_canary_anchor_status : uint8_t {
    authenticated_exact,
    invalid_policy,
    input_rejected,
    authentication_failed,
    output_too_small,
};

struct context_store_protected_canary_anchor_result {
    context_store_protected_canary_anchor_status status =
        context_store_protected_canary_anchor_status::input_rejected;
    context_store_format_digest envelope_digest {};
    size_t encoded_size = 0;

    bool authenticated() const noexcept {
        return status == context_store_protected_canary_anchor_status::authenticated_exact;
    }
};

struct context_store_protected_canary_anchor_decode_result;

class context_store_authenticated_protected_canary_anchor {
public:
    const context_store_protected_canary_anchor_body * body() const noexcept {
        return authenticated_ ? &body_ : nullptr;
    }
    bool authenticated() const noexcept { return authenticated_; }

private:
    bool authenticated_ = false;
    context_store_protected_canary_anchor_body body_;
    friend struct context_store_protected_canary_anchor_decode_result;
    friend context_store_protected_canary_anchor_decode_result
    context_store_protected_canary_anchor_decode_v1(
        const uint8_t *, size_t,
        const context_store_protected_canary_anchor_body &,
        const context_store_protected_canary_anchor_key &) noexcept;
};

struct context_store_protected_canary_anchor_decode_result {
    context_store_protected_canary_anchor_status status =
        context_store_protected_canary_anchor_status::input_rejected;
    context_store_authenticated_protected_canary_anchor carrier;

    const context_store_protected_canary_anchor_body * authenticated_body() const noexcept {
        return status == context_store_protected_canary_anchor_status::authenticated_exact
            ? carrier.body() : nullptr;
    }
};

namespace protected_canary_anchor_test_only {

// Test-only canonical ADR-0008 wire encoder. It deliberately performs no
// canary admission and returns no authenticated carrier. Product code must not
// use this namespace; it exists only for byte-exact legacy cross-golden tests.
bool canonical_wire_v1(
    const context_store_protected_canary_anchor_body & body,
    const context_store_protected_canary_anchor_key & key,
    uint8_t * output,
    size_t output_capacity,
    size_t & output_size,
    context_store_format_digest & envelope_digest) noexcept;

} // namespace protected_canary_anchor_test_only

// The only product policy: generation one, null predecessor, manifest and
// anchor key generations one, policy and authority epochs one, and exact 32-byte master
// key material. These functions retain no borrowed key material and do no I/O.
context_store_protected_canary_anchor_result
context_store_protected_canary_anchor_encode_v1(
    const context_store_protected_canary_anchor_body & body,
    const context_store_protected_canary_anchor_key & key,
    uint8_t * output,
    size_t output_capacity) noexcept;

// Authentication is equality with the one canonical envelope regenerated from
// the trusted closed policy. Size and every byte must match; parsed subsets and
// digest-only equality are never accepted.
context_store_protected_canary_anchor_result
context_store_protected_canary_anchor_verify_v1(
    const uint8_t * data,
    size_t size,
    const context_store_protected_canary_anchor_body & expected,
    const context_store_protected_canary_anchor_key & key) noexcept;

// Bounded generation-one discovery decoder. expected_fixed must contain the
// trusted fixed authority fields and a zero selected_manifest_digest. The body
// is exposed only after complete canonical parsing and exact MAC verification.
context_store_protected_canary_anchor_decode_result
context_store_protected_canary_anchor_decode_v1(
    const uint8_t * data,
    size_t size,
    const context_store_protected_canary_anchor_body & expected_fixed,
    const context_store_protected_canary_anchor_key & key) noexcept;

bool context_store_protected_canary_anchor_exact_envelope_equal(
    const uint8_t * left,
    size_t left_size,
    const uint8_t * right,
    size_t right_size) noexcept;

const char * context_store_protected_canary_anchor_status_name(
    context_store_protected_canary_anchor_status status) noexcept;

} // namespace halofpx
