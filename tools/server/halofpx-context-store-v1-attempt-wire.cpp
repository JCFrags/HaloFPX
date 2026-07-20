#include "halofpx-context-store-v1-attempt-wire.h"

#include <algorithm>
#include <array>

namespace halofpx {
namespace {

constexpr char pending_key_domain[] = "halofpx.attempt-pending-key.v1";
constexpr char pending_auth_domain[] = "halofpx.attempt-pending-auth.v1";
constexpr char pending_digest_domain[] = "halofpx.attempt-pending-envelope.v1";
constexpr char terminal_key_domain[] = "halofpx.attempt-terminal-key.v1";
constexpr char terminal_auth_domain[] = "halofpx.attempt-terminal-auth.v1";
constexpr char terminal_digest_domain[] = "halofpx.attempt-terminal-envelope.v1";

enum class wire_kind : uint8_t {
    pending = 0,
    terminal = 1,
};

struct bounded_buffer {
    std::array<uint8_t, context_store_v1_attempt_wire_max_bytes> bytes {};
    size_t size = 0;

    bool append(uint8_t value) noexcept {
        if (size == bytes.size()) return false;
        bytes[size++] = value;
        return true;
    }

    bool append(const uint8_t * data, size_t count) noexcept {
        if ((data == nullptr && count != 0) || count > bytes.size() - size) return false;
        std::copy_n(data, count, bytes.begin() + size);
        size += count;
        return true;
    }
};

void wipe(void * memory, size_t size) noexcept {
    volatile uint8_t * bytes = static_cast<volatile uint8_t *>(memory);
    while (size-- != 0) *bytes++ = 0;
}

template <size_t Size>
bool nonzero(const std::array<uint8_t, Size> & value) noexcept {
    uint8_t combined = 0;
    for (const uint8_t byte : value) combined |= byte;
    return combined != 0;
}

bool nonzero(const uint8_t * data, size_t size) noexcept {
    if (data == nullptr && size != 0) return false;
    uint8_t combined = 0;
    for (size_t index = 0; index < size; ++index) combined |= data[index];
    return combined != 0;
}

bool valid_terminal_status(context_store_v1_attempt_terminal_status status) noexcept {
    return status == context_store_v1_attempt_terminal_status::success ||
        status == context_store_v1_attempt_terminal_status::aborted;
}

bool valid_policy(
        const context_store_v1_attempt_body & body,
        const context_store_v1_attempt_key & key) noexcept {
    return key.master_key.data != nullptr &&
        key.master_key.size == context_store_v1_attempt_master_key_bytes &&
        nonzero(key.master_key.data, key.master_key.size) &&
        nonzero(body.attempt_id) && nonzero(body.store_uuid) &&
        nonzero(body.namespace_id) && nonzero(body.checkpoint_lineage_id) &&
        nonzero(body.manifest_digest) &&
        nonzero(body.ordered_object_set_commitment) &&
        nonzero(body.aggregate_source_commitment) &&
        nonzero(body.data_root_identity_commitment) &&
        nonzero(body.anchor_root_identity_commitment) &&
        body.proposed_anchor_envelope.data != nullptr &&
        body.proposed_anchor_envelope.size != 0 &&
        body.proposed_anchor_envelope.size <= context_store_v1_attempt_anchor_max_bytes;
}

bool cbor_head(bounded_buffer & output, uint8_t major, uint64_t value) noexcept {
    if (value < 24) return output.append(static_cast<uint8_t>((major << 5) | value));
    const size_t count = value <= UINT8_MAX ? 1 : value <= UINT16_MAX ? 2 :
        value <= UINT32_MAX ? 4 : 8;
    const uint8_t additional = count == 1 ? 24 : count == 2 ? 25 : count == 4 ? 26 : 27;
    if (!output.append(static_cast<uint8_t>((major << 5) | additional))) return false;
    for (size_t remaining = count; remaining != 0; --remaining) {
        if (!output.append(static_cast<uint8_t>(value >> ((remaining - 1) * 8)))) return false;
    }
    return true;
}

bool cbor_uint(bounded_buffer & output, uint64_t value) noexcept {
    return cbor_head(output, 0, value);
}

bool cbor_map(bounded_buffer & output, uint64_t count) noexcept {
    return cbor_head(output, 5, count);
}

template <size_t Size>
bool cbor_bytes(bounded_buffer & output, const std::array<uint8_t, Size> & value) noexcept {
    return cbor_head(output, 2, Size) && output.append(value.data(), value.size());
}

bool proposed_anchor_digest(
        const context_store_v1_attempt_body & body,
        context_store_format_digest & digest) noexcept {
    return context_store_sha256_bounded(
        body.proposed_anchor_envelope.data,
        body.proposed_anchor_envelope.size,
        context_store_v1_attempt_anchor_max_bytes,
        digest);
}

bool encode_body(
        bounded_buffer & output,
        const context_store_v1_attempt_body & body,
        wire_kind kind,
        uint8_t terminal_status,
        const context_store_format_digest & anchor_digest) noexcept {
    // Keys are monotonically ordered. Publication generation is exactly one
    // and key 2 is the exact CBOR null predecessor.
    return cbor_map(output, 16) &&
        cbor_uint(output, 0) && cbor_uint(output, 1) &&
        cbor_uint(output, 1) && cbor_uint(output, 1) &&
        cbor_uint(output, 2) && output.append(0xf6) &&
        cbor_uint(output, 3) && cbor_uint(output, static_cast<uint8_t>(kind)) &&
        cbor_uint(output, 4) && cbor_uint(output, terminal_status) &&
        cbor_uint(output, 5) && cbor_bytes(output, body.attempt_id) &&
        cbor_uint(output, 6) && cbor_bytes(output, body.store_uuid) &&
        cbor_uint(output, 7) && cbor_bytes(output, body.namespace_id) &&
        cbor_uint(output, 8) && cbor_bytes(output, body.checkpoint_lineage_id) &&
        cbor_uint(output, 9) && cbor_bytes(output, body.manifest_digest) &&
        cbor_uint(output, 10) && cbor_bytes(output, body.ordered_object_set_commitment) &&
        cbor_uint(output, 11) && cbor_bytes(output, body.aggregate_source_commitment) &&
        cbor_uint(output, 12) && cbor_bytes(output, body.data_root_identity_commitment) &&
        cbor_uint(output, 13) && cbor_bytes(output, body.anchor_root_identity_commitment) &&
        cbor_uint(output, 14) && cbor_bytes(output, anchor_digest) &&
        cbor_uint(output, 15) && cbor_uint(output, body.proposed_anchor_envelope.size);
}

bool derive_key(
        const context_store_v1_attempt_body & body,
        const context_store_v1_attempt_key & key,
        wire_kind kind,
        context_store_format_digest & derived) noexcept {
    const char * domain = kind == wire_kind::pending ? pending_key_domain : terminal_key_domain;
    const size_t domain_size = kind == wire_kind::pending ? sizeof(pending_key_domain) :
        sizeof(terminal_key_domain);
    bounded_buffer input;
    const uint8_t generation[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
    return input.append(reinterpret_cast<const uint8_t *>(domain), domain_size) &&
        input.append(generation, sizeof(generation)) &&
        input.append(body.attempt_id.data(), body.attempt_id.size()) &&
        input.append(body.store_uuid.data(), body.store_uuid.size()) &&
        input.append(body.namespace_id.data(), body.namespace_id.size()) &&
        input.append(body.checkpoint_lineage_id.data(), body.checkpoint_lineage_id.size()) &&
        context_store_hmac_sha256(key.master_key.data, key.master_key.size,
            input.bytes.data(), input.size, derived);
}

bool compute_envelope_digest(
        const uint8_t * data,
        size_t size,
        wire_kind kind,
        context_store_format_digest & digest) noexcept {
    const char * domain = kind == wire_kind::pending ? pending_digest_domain : terminal_digest_domain;
    const size_t domain_size = kind == wire_kind::pending ? sizeof(pending_digest_domain) :
        sizeof(terminal_digest_domain);
    bounded_buffer input;
    return input.append(reinterpret_cast<const uint8_t *>(domain), domain_size) &&
        input.append(data, size) &&
        context_store_sha256_bounded(input.bytes.data(), input.size,
            context_store_v1_attempt_wire_max_bytes, digest);
}

bool exact_equal(
        const uint8_t * left,
        size_t left_size,
        const uint8_t * right,
        size_t right_size) noexcept {
    if (left == nullptr || right == nullptr || left_size == 0 || left_size != right_size ||
        left_size > context_store_v1_attempt_wire_max_bytes) {
        return false;
    }
    uint8_t difference = 0;
    for (size_t index = 0; index < left_size; ++index) {
        difference |= static_cast<uint8_t>(left[index] ^ right[index]);
    }
    return difference == 0;
}

bool canonical_wire(
        const context_store_v1_attempt_body & body,
        const context_store_v1_attempt_key & key,
        wire_kind kind,
        uint8_t terminal_status,
        bounded_buffer & envelope,
        context_store_format_digest & envelope_digest,
        context_store_format_digest & anchor_digest) noexcept {
    bounded_buffer authentication_input;
    bounded_buffer authentication_message;
    context_store_format_digest derived_key {};
    context_store_format_digest tag {};
    const char * auth_domain = kind == wire_kind::pending ? pending_auth_domain : terminal_auth_domain;
    const size_t auth_domain_size = kind == wire_kind::pending ? sizeof(pending_auth_domain) :
        sizeof(terminal_auth_domain);

    // Construct every potentially failing bounded message before the derived
    // key exists. Derived key and tag are wiped on every subsequent path.
    const bool prepared = proposed_anchor_digest(body, anchor_digest) &&
        encode_body(authentication_input, body, kind, terminal_status, anchor_digest) &&
        authentication_message.append(reinterpret_cast<const uint8_t *>(auth_domain), auth_domain_size) &&
        authentication_message.append(authentication_input.bytes.data(), authentication_input.size);
    if (!prepared) return false;

    const bool authenticated = derive_key(body, key, kind, derived_key) &&
        context_store_hmac_sha256(derived_key.data(), derived_key.size(),
            authentication_message.bytes.data(), authentication_message.size, tag);
    wipe(derived_key.data(), derived_key.size());
    if (!authenticated) {
        wipe(tag.data(), tag.size());
        return false;
    }

    const bool encoded = cbor_map(envelope, 2) && cbor_uint(envelope, 0) &&
        envelope.append(authentication_input.bytes.data(), authentication_input.size) &&
        cbor_uint(envelope, 1) && cbor_bytes(envelope, tag) &&
        compute_envelope_digest(envelope.bytes.data(), envelope.size, kind, envelope_digest);
    wipe(tag.data(), tag.size());
    return encoded;
}

context_store_v1_attempt_wire_result encode(
        const context_store_v1_attempt_body & body,
        const context_store_v1_attempt_key & key,
        wire_kind kind,
        uint8_t terminal_status,
        uint8_t * output,
        size_t output_capacity) noexcept {
    context_store_v1_attempt_wire_result result;
    if (!valid_policy(body, key) ||
        (kind == wire_kind::terminal &&
         !valid_terminal_status(static_cast<context_store_v1_attempt_terminal_status>(terminal_status)))) {
        result.status = context_store_v1_attempt_wire_status::invalid_policy;
        return result;
    }

    bounded_buffer envelope;
    if (!canonical_wire(body, key, kind, terminal_status, envelope,
            result.envelope_digest, result.proposed_anchor_envelope_digest)) {
        result.status = context_store_v1_attempt_wire_status::invalid_policy;
        return result;
    }
    result.encoded_size = envelope.size;
    if (output == nullptr || output_capacity < envelope.size) {
        result.status = context_store_v1_attempt_wire_status::output_too_small;
        return result;
    }
    std::copy_n(envelope.bytes.data(), envelope.size, output);
    result.status = context_store_v1_attempt_wire_status::authenticated_exact;
    return result;
}

context_store_v1_attempt_wire_result verify(
        const uint8_t * data,
        size_t size,
        const context_store_v1_attempt_body & expected,
        const context_store_v1_attempt_key & key,
        wire_kind kind,
        uint8_t terminal_status) noexcept {
    context_store_v1_attempt_wire_result result;
    if (!valid_policy(expected, key) ||
        (kind == wire_kind::terminal &&
         !valid_terminal_status(static_cast<context_store_v1_attempt_terminal_status>(terminal_status)))) {
        result.status = context_store_v1_attempt_wire_status::invalid_policy;
        return result;
    }
    if (data == nullptr || size == 0 || size > context_store_v1_attempt_wire_max_bytes) {
        result.status = context_store_v1_attempt_wire_status::input_rejected;
        return result;
    }

    bounded_buffer canonical;
    if (!canonical_wire(expected, key, kind, terminal_status, canonical,
            result.envelope_digest, result.proposed_anchor_envelope_digest)) {
        result.status = context_store_v1_attempt_wire_status::invalid_policy;
        return result;
    }
    result.encoded_size = size;
    if (!exact_equal(data, size, canonical.bytes.data(), canonical.size)) {
        result.envelope_digest.fill(0);
        result.proposed_anchor_envelope_digest.fill(0);
        result.status = context_store_v1_attempt_wire_status::authentication_failed;
        return result;
    }
    result.status = context_store_v1_attempt_wire_status::authenticated_exact;
    return result;
}

} // namespace

context_store_v1_attempt_wire_result context_store_v1_attempt_pending_encode(
        const context_store_v1_attempt_body & body,
        const context_store_v1_attempt_key & key,
        uint8_t * output,
        size_t output_capacity) noexcept {
    return encode(body, key, wire_kind::pending, 0, output, output_capacity);
}

context_store_v1_attempt_wire_result context_store_v1_attempt_pending_verify(
        const uint8_t * data,
        size_t size,
        const context_store_v1_attempt_body & expected,
        const context_store_v1_attempt_key & key) noexcept {
    return verify(data, size, expected, key, wire_kind::pending, 0);
}

context_store_v1_attempt_wire_result context_store_v1_attempt_terminal_encode(
        const context_store_v1_attempt_body & body,
        context_store_v1_attempt_terminal_status terminal_status,
        const context_store_v1_attempt_key & key,
        uint8_t * output,
        size_t output_capacity) noexcept {
    return encode(body, key, wire_kind::terminal, static_cast<uint8_t>(terminal_status),
        output, output_capacity);
}

context_store_v1_attempt_wire_result context_store_v1_attempt_terminal_verify(
        const uint8_t * data,
        size_t size,
        const context_store_v1_attempt_body & expected,
        context_store_v1_attempt_terminal_status expected_status,
        const context_store_v1_attempt_key & key) noexcept {
    return verify(data, size, expected, key, wire_kind::terminal,
        static_cast<uint8_t>(expected_status));
}

const char * context_store_v1_attempt_wire_status_name(
        context_store_v1_attempt_wire_status status) noexcept {
    switch (status) {
        case context_store_v1_attempt_wire_status::authenticated_exact: return "authenticated-exact";
        case context_store_v1_attempt_wire_status::invalid_policy: return "invalid-policy";
        case context_store_v1_attempt_wire_status::input_rejected: return "input-rejected";
        case context_store_v1_attempt_wire_status::authentication_failed: return "authentication-failed";
        case context_store_v1_attempt_wire_status::output_too_small: return "output-too-small";
    }
    return "unknown";
}

} // namespace halofpx
