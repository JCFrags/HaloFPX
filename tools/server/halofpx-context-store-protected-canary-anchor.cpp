#include "halofpx-context-store-protected-canary-anchor.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace halofpx {
namespace {

constexpr char anchor_key_domain[] = "halofpx.anchor-key.v1";
constexpr char anchor_auth_domain[] = "halofpx.anchor-auth.v1";
constexpr char anchor_digest_domain[] = "halofpx.anchor.v1";
constexpr char protected_anchor_key_id[] = "halofpx-protected-anchor-v1";

struct bounded_buffer {
    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> bytes {};
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

bool nonzero(const uint8_t * data, size_t size) noexcept {
    uint8_t combined = 0;
    for (size_t index = 0; index < size; ++index) combined |= data[index];
    return combined != 0;
}

bool valid_registered_id(const context_store_registered_id & id) noexcept {
    if (id.size == 0 || id.size > context_store_registered_id_max_bytes) return false;
    const auto * data = reinterpret_cast<const uint8_t *>(id.bytes.data());
    for (size_t index = 0; index < id.size; ++index) {
        if (data[index] == 0 || data[index] > 0x7f) return false;
    }
    return true;
}

bool valid_wire_policy(
        const context_store_protected_canary_anchor_body & body,
        const context_store_protected_canary_anchor_key & key) noexcept {
    return body.generation != 0 &&
        ((body.generation == 1 && !body.has_predecessor) ||
         (body.generation > 1 && body.has_predecessor)) &&
        valid_registered_id(key.key_id) && key.master_key.data != nullptr &&
        key.master_key.size != 0 &&
        key.master_key.size <= context_store_master_key_max_bytes;
}

bool exact_protected_key_id(const context_store_registered_id & id) noexcept {
    return id.size == sizeof(protected_anchor_key_id) - 1 &&
        std::equal(id.bytes.begin(), id.bytes.begin() + id.size,
                   protected_anchor_key_id);
}

bool valid_closed_policy(
        const context_store_protected_canary_anchor_body & body,
        const context_store_protected_canary_anchor_key & key) noexcept {
    return valid_wire_policy(body, key) &&
        body.policy_epoch == 1 && body.manifest_key_generation == 1 &&
        body.authority_epoch == 1 && body.generation == 1 &&
        !body.has_predecessor && key.generation == 1 &&
        exact_protected_key_id(key.key_id) &&
        key.master_key.size == context_store_protected_canary_anchor_master_key_bytes &&
        nonzero(body.store_uuid.data(), body.store_uuid.size()) &&
        nonzero(body.namespace_id.data(), body.namespace_id.size()) &&
        nonzero(body.checkpoint_lineage_id.data(), body.checkpoint_lineage_id.size()) &&
        nonzero(body.selected_manifest_digest.data(), body.selected_manifest_digest.size()) &&
        !nonzero(body.predecessor_manifest_digest.data(), body.predecessor_manifest_digest.size());
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

bool cbor_text(
        bounded_buffer & output,
        const context_store_registered_id & value) noexcept {
    return valid_registered_id(value) && cbor_head(output, 3, value.size) &&
        output.append(reinterpret_cast<const uint8_t *>(value.bytes.data()), value.size);
}

bool encode_body(
        bounded_buffer & output,
        const context_store_protected_canary_anchor_body & body) noexcept {
    return cbor_map(output, 11) &&
        cbor_uint(output, 0) && cbor_uint(output, 1) &&
        cbor_uint(output, 1) && cbor_uint(output, 0) &&
        cbor_uint(output, 2) && cbor_bytes(output, body.store_uuid) &&
        cbor_uint(output, 3) && cbor_bytes(output, body.namespace_id) &&
        cbor_uint(output, 4) && cbor_uint(output, body.policy_epoch) &&
        cbor_uint(output, 5) && cbor_bytes(output, body.checkpoint_lineage_id) &&
        cbor_uint(output, 6) && cbor_uint(output, body.manifest_key_generation) &&
        cbor_uint(output, 7) && cbor_uint(output, body.authority_epoch) &&
        cbor_uint(output, 8) && cbor_uint(output, body.generation) &&
        cbor_uint(output, 9) && cbor_bytes(output, body.selected_manifest_digest) &&
        cbor_uint(output, 10) &&
        (body.has_predecessor ? cbor_bytes(output, body.predecessor_manifest_digest) :
                                output.append(0xf6));
}

bool encode_authentication_input(
        bounded_buffer & output,
        const context_store_protected_canary_anchor_body & body,
        const context_store_protected_canary_anchor_key & key) noexcept {
    return cbor_map(output, 4) &&
        cbor_uint(output, 0) && encode_body(output, body) &&
        cbor_uint(output, 1) && cbor_text(output, key.key_id) &&
        cbor_uint(output, 2) && cbor_uint(output, 1) &&
        cbor_uint(output, 3) && cbor_uint(output, key.generation);
}

bool derive_anchor_key(
        const context_store_protected_canary_anchor_body & body,
        const context_store_protected_canary_anchor_key & key,
        context_store_format_digest & derived) noexcept {
    bounded_buffer input;
    const uint16_t key_id_size = key.key_id.size;
    const uint8_t key_id_length[2] = {
        static_cast<uint8_t>(key_id_size >> 8),
        static_cast<uint8_t>(key_id_size),
    };
    uint8_t generation[8] {};
    for (size_t index = 0; index < sizeof(generation); ++index) {
        generation[sizeof(generation) - 1 - index] =
            static_cast<uint8_t>(key.generation >> (index * 8));
    }
    return input.append(reinterpret_cast<const uint8_t *>(anchor_key_domain),
                        sizeof(anchor_key_domain)) &&
        input.append(key_id_length, sizeof(key_id_length)) &&
        input.append(reinterpret_cast<const uint8_t *>(key.key_id.bytes.data()),
                     key.key_id.size) &&
        input.append(body.store_uuid.data(), body.store_uuid.size()) &&
        input.append(body.namespace_id.data(), body.namespace_id.size()) &&
        input.append(generation, sizeof(generation)) &&
        context_store_hmac_sha256(key.master_key.data, key.master_key.size,
                                  input.bytes.data(), input.size, derived);
}

bool compute_envelope_digest(
        const uint8_t * data,
        size_t size,
        context_store_format_digest & digest) noexcept {
    bounded_buffer input;
    return input.append(reinterpret_cast<const uint8_t *>(anchor_digest_domain),
                        sizeof(anchor_digest_domain)) &&
        input.append(data, size) &&
        context_store_sha256(input.bytes.data(), input.size, digest);
}

class cursor {
public:
    cursor(const uint8_t * data, size_t size) noexcept : data_(data), size_(size) {}
    bool map(size_t count) noexcept { uint64_t value; return head(5, value) && value == count; }
    bool key(uint64_t expected) noexcept { uint64_t value; return number(value) && value == expected; }
    bool number(uint64_t & value) noexcept { return head(0, value); }
    template <size_t N> bool bytes(std::array<uint8_t, N> & output) noexcept {
        uint64_t count;
        if (!head(2, count) || count != N || !have(N)) return false;
        std::copy_n(data_ + position_, N, output.begin());
        position_ += N;
        return true;
    }
    bool text(context_store_registered_id & output) noexcept {
        uint64_t count;
        if (!head(3, count) || count == 0 || count > output.bytes.size() ||
            !have(static_cast<size_t>(count))) return false;
        output.size = static_cast<uint8_t>(count);
        std::copy_n(data_ + position_, output.size, output.bytes.begin());
        position_ += output.size;
        return valid_registered_id(output);
    }
    bool null_value() noexcept {
        if (!have(1) || data_[position_] != 0xf6) return false;
        ++position_;
        return true;
    }
    bool end() const noexcept { return position_ == size_; }

private:
    bool have(size_t count) const noexcept { return count <= size_ - position_; }
    bool head(uint8_t major, uint64_t & value) noexcept {
        if (!have(1)) return false;
        const uint8_t initial = data_[position_++];
        if ((initial >> 5) != major) return false;
        const uint8_t additional = initial & 31;
        if (additional < 24) { value = additional; return true; }
        if (additional > 27) return false;
        const size_t count = size_t(1) << (additional - 24);
        if (!have(count)) return false;
        value = 0;
        for (size_t i = 0; i < count; ++i) value = (value << 8) | data_[position_++];
        const uint64_t minimum = additional == 24 ? 24 : additional == 25 ? 0x100 :
            additional == 26 ? 0x10000 : 0x100000000ULL;
        return value >= minimum;
    }
    const uint8_t * data_;
    size_t size_;
    size_t position_ = 0;
};

bool parse_complete_envelope(
        const uint8_t * data, size_t size,
        context_store_protected_canary_anchor_body & body,
        context_store_registered_id & key_id, uint64_t & key_generation) noexcept {
    if (!data || size == 0 || size > context_store_protected_canary_anchor_max_bytes) return false;
    cursor input(data, size);
    uint64_t value = 0;
    context_store_format_digest tag {};
    return input.map(2) && input.key(0) && input.map(4) && input.key(0) &&
        input.map(11) && input.key(0) && input.number(value) && value == 1 &&
        input.key(1) && input.number(value) && value == 0 &&
        input.key(2) && input.bytes(body.store_uuid) &&
        input.key(3) && input.bytes(body.namespace_id) &&
        input.key(4) && input.number(body.policy_epoch) &&
        input.key(5) && input.bytes(body.checkpoint_lineage_id) &&
        input.key(6) && input.number(body.manifest_key_generation) &&
        input.key(7) && input.number(body.authority_epoch) &&
        input.key(8) && input.number(body.generation) &&
        input.key(9) && input.bytes(body.selected_manifest_digest) &&
        input.key(10) && input.null_value() &&
        input.key(1) && input.text(key_id) &&
        input.key(2) && input.number(value) && value == 1 &&
        input.key(3) && input.number(key_generation) &&
        input.key(1) && input.bytes(tag) && input.end();
}

bool same_fixed(const context_store_protected_canary_anchor_body & actual,
                const context_store_protected_canary_anchor_body & expected) noexcept {
    return actual.store_uuid == expected.store_uuid &&
        actual.namespace_id == expected.namespace_id &&
        actual.policy_epoch == expected.policy_epoch &&
        actual.checkpoint_lineage_id == expected.checkpoint_lineage_id &&
        actual.manifest_key_generation == expected.manifest_key_generation &&
        actual.authority_epoch == expected.authority_epoch &&
        actual.generation == expected.generation && !actual.has_predecessor &&
        !nonzero(actual.predecessor_manifest_digest.data(), actual.predecessor_manifest_digest.size());
}

} // namespace

bool protected_canary_anchor_test_only::canonical_wire_v1(
        const context_store_protected_canary_anchor_body & body,
        const context_store_protected_canary_anchor_key & key,
        uint8_t * output,
        size_t output_capacity,
        size_t & output_size,
        context_store_format_digest & envelope_digest) noexcept {
    output_size = 0;
    envelope_digest.fill(0);
    if (!valid_wire_policy(body, key)) return false;

    bounded_buffer authentication_input;
    bounded_buffer authentication_message;
    bounded_buffer envelope;
    context_store_format_digest derived_key {};
    context_store_format_digest tag {};
    const bool encoded = encode_authentication_input(authentication_input, body, key) &&
        derive_anchor_key(body, key, derived_key) &&
        authentication_message.append(
            reinterpret_cast<const uint8_t *>(anchor_auth_domain),
            sizeof(anchor_auth_domain)) &&
        authentication_message.append(authentication_input.bytes.data(),
                                      authentication_input.size) &&
        context_store_hmac_sha256(
            derived_key.data(), derived_key.size(), authentication_message.bytes.data(),
            authentication_message.size, tag) &&
        cbor_map(envelope, 2) && cbor_uint(envelope, 0) &&
        envelope.append(authentication_input.bytes.data(), authentication_input.size) &&
        cbor_uint(envelope, 1) && cbor_bytes(envelope, tag) &&
        compute_envelope_digest(envelope.bytes.data(), envelope.size, envelope_digest);
    wipe(derived_key.data(), derived_key.size());
    wipe(tag.data(), tag.size());
    if (!encoded) {
        envelope_digest.fill(0);
        return false;
    }

    output_size = envelope.size;
    if (output == nullptr || output_capacity < envelope.size) return false;
    std::copy_n(envelope.bytes.data(), envelope.size, output);
    return true;
}

context_store_protected_canary_anchor_result
context_store_protected_canary_anchor_encode_v1(
        const context_store_protected_canary_anchor_body & body,
        const context_store_protected_canary_anchor_key & key,
        uint8_t * output,
        size_t output_capacity) noexcept {
    context_store_protected_canary_anchor_result result;
    if (!valid_closed_policy(body, key)) {
        result.status = context_store_protected_canary_anchor_status::invalid_policy;
        return result;
    }

    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> envelope {};
    if (!protected_canary_anchor_test_only::canonical_wire_v1(
            body, key, envelope.data(), envelope.size(), result.encoded_size,
            result.envelope_digest)) {
        result.status = context_store_protected_canary_anchor_status::invalid_policy;
        return result;
    }
    if (output == nullptr || output_capacity < result.encoded_size) {
        result.status = context_store_protected_canary_anchor_status::output_too_small;
        return result;
    }
    std::copy_n(envelope.data(), result.encoded_size, output);
    result.status = context_store_protected_canary_anchor_status::authenticated_exact;
    return result;
}

context_store_protected_canary_anchor_result
context_store_protected_canary_anchor_verify_v1(
        const uint8_t * data,
        size_t size,
        const context_store_protected_canary_anchor_body & expected,
        const context_store_protected_canary_anchor_key & key) noexcept {
    context_store_protected_canary_anchor_result result;
    if (!valid_closed_policy(expected, key)) {
        result.status = context_store_protected_canary_anchor_status::invalid_policy;
        return result;
    }
    if (data == nullptr || size == 0 ||
        size > context_store_protected_canary_anchor_max_bytes) {
        result.status = context_store_protected_canary_anchor_status::input_rejected;
        return result;
    }

    std::array<uint8_t, context_store_protected_canary_anchor_max_bytes> canonical {};
    size_t canonical_size = 0;
    context_store_format_digest canonical_digest {};
    if (!protected_canary_anchor_test_only::canonical_wire_v1(
            expected, key, canonical.data(), canonical.size(), canonical_size,
            canonical_digest)) {
        result.status = context_store_protected_canary_anchor_status::invalid_policy;
        return result;
    }
    result.encoded_size = size;
    if (!context_store_protected_canary_anchor_exact_envelope_equal(
            data, size, canonical.data(), canonical_size)) {
        result.status = context_store_protected_canary_anchor_status::authentication_failed;
        return result;
    }
    result.envelope_digest = canonical_digest;
    result.status = context_store_protected_canary_anchor_status::authenticated_exact;
    return result;
}

context_store_protected_canary_anchor_decode_result
context_store_protected_canary_anchor_decode_v1(
        const uint8_t * data,
        size_t size,
        const context_store_protected_canary_anchor_body & expected_fixed,
        const context_store_protected_canary_anchor_key & key) noexcept {
    context_store_protected_canary_anchor_decode_result result;
    if (!valid_registered_id(key.key_id) || !exact_protected_key_id(key.key_id) ||
        key.generation != 1 || !key.master_key.data ||
        key.master_key.size != context_store_protected_canary_anchor_master_key_bytes ||
        expected_fixed.policy_epoch != 1 || expected_fixed.manifest_key_generation != 1 ||
        expected_fixed.authority_epoch != 1 || expected_fixed.generation != 1 ||
        expected_fixed.has_predecessor ||
        !nonzero(expected_fixed.store_uuid.data(), expected_fixed.store_uuid.size()) ||
        !nonzero(expected_fixed.namespace_id.data(), expected_fixed.namespace_id.size()) ||
        !nonzero(expected_fixed.checkpoint_lineage_id.data(),
                 expected_fixed.checkpoint_lineage_id.size()) ||
        nonzero(expected_fixed.selected_manifest_digest.data(),
                expected_fixed.selected_manifest_digest.size()) ||
        nonzero(expected_fixed.predecessor_manifest_digest.data(),
                expected_fixed.predecessor_manifest_digest.size())) {
        result.status = context_store_protected_canary_anchor_status::invalid_policy;
        return result;
    }
    context_store_protected_canary_anchor_body parsed;
    context_store_registered_id parsed_key_id;
    uint64_t parsed_key_generation = 0;
    if (!parse_complete_envelope(data, size, parsed, parsed_key_id, parsed_key_generation)) {
        result.status = context_store_protected_canary_anchor_status::input_rejected;
        return result;
    }
    if (!same_fixed(parsed, expected_fixed) || !exact_protected_key_id(parsed_key_id) ||
        parsed_key_generation != key.generation || !valid_closed_policy(parsed, key)) {
        result.status = context_store_protected_canary_anchor_status::authentication_failed;
        return result;
    }
    const auto verified = context_store_protected_canary_anchor_verify_v1(
        data, size, parsed, key);
    if (!verified.authenticated()) {
        result.status = verified.status;
        return result;
    }
    result.carrier.body_ = parsed;
    result.carrier.authenticated_ = true;
    result.status = context_store_protected_canary_anchor_status::authenticated_exact;
    return result;
}

bool context_store_protected_canary_anchor_exact_envelope_equal(
        const uint8_t * left,
        size_t left_size,
        const uint8_t * right,
        size_t right_size) noexcept {
    if (left == nullptr || right == nullptr || left_size == 0 ||
        left_size > context_store_protected_canary_anchor_max_bytes ||
        right_size == 0 || right_size > context_store_protected_canary_anchor_max_bytes ||
        left_size != right_size) {
        return false;
    }
    uint8_t difference = 0;
    for (size_t index = 0; index < left_size; ++index) {
        difference |= static_cast<uint8_t>(left[index] ^ right[index]);
    }
    return difference == 0;
}

const char * context_store_protected_canary_anchor_status_name(
        context_store_protected_canary_anchor_status status) noexcept {
    switch (status) {
        case context_store_protected_canary_anchor_status::authenticated_exact:
            return "authenticated-exact";
        case context_store_protected_canary_anchor_status::invalid_policy:
            return "invalid-policy";
        case context_store_protected_canary_anchor_status::input_rejected:
            return "input-rejected";
        case context_store_protected_canary_anchor_status::authentication_failed:
            return "authentication-failed";
        case context_store_protected_canary_anchor_status::output_too_small:
            return "output-too-small";
    }
    return "unknown";
}

} // namespace halofpx
