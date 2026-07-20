#pragma once

#include "halofpx-context-store-auth.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_v1_attempt_wire_max_bytes = 2048;
constexpr size_t context_store_v1_attempt_master_key_bytes = 32;
constexpr size_t context_store_v1_attempt_id_bytes = 32;
constexpr size_t context_store_v1_attempt_anchor_max_bytes = 1024;

struct context_store_v1_attempt_byte_view {
    const uint8_t * data = nullptr;
    size_t size = 0;
};

// The authority supplies a fresh, internally generated attempt_id. The wire
// layer only rejects the all-zero sentinel; it never generates randomness.
// proposed_anchor_envelope is hashed over its exact bytes and committed along
// with its exact length. No parsed or normalized anchor representation exists.
struct context_store_v1_attempt_body {
    std::array<uint8_t, context_store_v1_attempt_id_bytes> attempt_id {};
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest namespace_id {};
    context_store_format_digest checkpoint_lineage_id {};
    context_store_format_digest manifest_digest {};
    context_store_format_digest ordered_object_set_commitment {};
    context_store_format_digest aggregate_source_commitment {};
    context_store_format_digest data_root_identity_commitment {};
    context_store_format_digest anchor_root_identity_commitment {};
    context_store_v1_attempt_byte_view proposed_anchor_envelope;
};

struct context_store_v1_attempt_key {
    context_store_key_view master_key;
};

enum class context_store_v1_attempt_terminal_status : uint8_t {
    success = 1,
    aborted = 2,
};

enum class context_store_v1_attempt_wire_status : uint8_t {
    authenticated_exact,
    invalid_policy,
    input_rejected,
    authentication_failed,
    output_too_small,
};

struct context_store_v1_attempt_wire_result {
    context_store_v1_attempt_wire_status status =
        context_store_v1_attempt_wire_status::input_rejected;
    context_store_format_digest envelope_digest {};
    context_store_format_digest proposed_anchor_envelope_digest {};
    size_t encoded_size = 0;

    bool authenticated() const noexcept {
        return status == context_store_v1_attempt_wire_status::authenticated_exact;
    }
};

// Canonical, bounded, generation-one/null-predecessor pending intent.
context_store_v1_attempt_wire_result context_store_v1_attempt_pending_encode(
    const context_store_v1_attempt_body & body,
    const context_store_v1_attempt_key & key,
    uint8_t * output,
    size_t output_capacity) noexcept;

context_store_v1_attempt_wire_result context_store_v1_attempt_pending_verify(
    const uint8_t * data,
    size_t size,
    const context_store_v1_attempt_body & expected,
    const context_store_v1_attempt_key & key) noexcept;

// Terminal success and terminal abort are distinct canonical authenticated
// envelopes. A pending envelope can never authenticate as either terminal.
context_store_v1_attempt_wire_result context_store_v1_attempt_terminal_encode(
    const context_store_v1_attempt_body & body,
    context_store_v1_attempt_terminal_status terminal_status,
    const context_store_v1_attempt_key & key,
    uint8_t * output,
    size_t output_capacity) noexcept;

context_store_v1_attempt_wire_result context_store_v1_attempt_terminal_verify(
    const uint8_t * data,
    size_t size,
    const context_store_v1_attempt_body & expected,
    context_store_v1_attempt_terminal_status expected_status,
    const context_store_v1_attempt_key & key) noexcept;

const char * context_store_v1_attempt_wire_status_name(
    context_store_v1_attempt_wire_status status) noexcept;

} // namespace halofpx
