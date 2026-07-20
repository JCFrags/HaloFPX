#include "halofpx-context-store-v1-attempt-wire.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

namespace {

struct fixture {
    std::array<uint8_t, halofpx::context_store_v1_attempt_master_key_bytes> master {};
    std::array<uint8_t, 5> anchor = { 0xa1, 0x01, 0x02, 0x03, 0x04 };
    halofpx::context_store_v1_attempt_body body;
    halofpx::context_store_v1_attempt_key key;
};

fixture make_fixture() {
    fixture value;
    for (size_t index = 0; index < value.master.size(); ++index) {
        value.master[index] = static_cast<uint8_t>(index);
    }
    value.body.attempt_id.fill(0x11);
    value.body.store_uuid.fill(0x22);
    value.body.namespace_id.fill(0x33);
    value.body.checkpoint_lineage_id.fill(0x44);
    value.body.manifest_digest.fill(0x55);
    value.body.ordered_object_set_commitment.fill(0x66);
    value.body.aggregate_source_commitment.fill(0x77);
    value.body.data_root_identity_commitment.fill(0x88);
    value.body.anchor_root_identity_commitment.fill(0x99);
    value.body.proposed_anchor_envelope = { value.anchor.data(), value.anchor.size() };
    value.key.master_key = { value.master.data(), value.master.size() };
    return value;
}

void refresh_views(fixture & value) {
    value.body.proposed_anchor_envelope = { value.anchor.data(), value.anchor.size() };
    value.key.master_key = { value.master.data(), value.master.size() };
}

halofpx::context_store_format_digest digest_from_hex(std::string_view text) {
    assert(text.size() == 64);
    const auto nibble = [](char value) -> uint8_t {
        if (value >= '0' && value <= '9') return static_cast<uint8_t>(value - '0');
        assert(value >= 'a' && value <= 'f');
        return static_cast<uint8_t>(value - 'a' + 10);
    };
    halofpx::context_store_format_digest digest {};
    for (size_t index = 0; index < digest.size(); ++index) {
        digest[index] = static_cast<uint8_t>((nibble(text[index * 2]) << 4) |
                                             nibble(text[index * 2 + 1]));
    }
    return digest;
}

struct encoded_wire {
    std::array<uint8_t, halofpx::context_store_v1_attempt_wire_max_bytes> bytes {};
    halofpx::context_store_v1_attempt_wire_result result;
};

encoded_wire encode_pending(fixture & value) {
    refresh_views(value);
    encoded_wire encoded;
    encoded.result = halofpx::context_store_v1_attempt_pending_encode(
        value.body, value.key, encoded.bytes.data(), encoded.bytes.size());
    assert(encoded.result.authenticated());
    return encoded;
}

encoded_wire encode_terminal(
        fixture & value,
        halofpx::context_store_v1_attempt_terminal_status status) {
    refresh_views(value);
    encoded_wire encoded;
    encoded.result = halofpx::context_store_v1_attempt_terminal_encode(
        value.body, status, value.key, encoded.bytes.data(), encoded.bytes.size());
    assert(encoded.result.authenticated());
    return encoded;
}

void test_golden_and_determinism() {
    auto value = make_fixture();
    refresh_views(value);
    const auto pending = encode_pending(value);
    const auto pending_again = encode_pending(value);
    const auto success = encode_terminal(
        value, halofpx::context_store_v1_attempt_terminal_status::success);
    const auto aborted = encode_terminal(
        value, halofpx::context_store_v1_attempt_terminal_status::aborted);

    assert(pending.result.encoded_size == pending_again.result.encoded_size);
    assert(std::equal(pending.bytes.begin(),
        pending.bytes.begin() + pending.result.encoded_size, pending_again.bytes.begin()));
    assert(pending.result.envelope_digest == pending_again.result.envelope_digest);
    assert(pending.result.proposed_anchor_envelope_digest ==
        pending_again.result.proposed_anchor_envelope_digest);

    // Each fixed digest locks the exact canonical envelope, including the
    // independent pending/terminal authentication and status domains.
    assert(pending.result.encoded_size == 383);
    assert(success.result.encoded_size == 383);
    assert(aborted.result.encoded_size == 383);
    assert(pending.result.envelope_digest == digest_from_hex(
        "f7624f3fdf920ed8b5b53b85198d3fcf06f45f4677986db876209435570a9625"));
    assert(success.result.envelope_digest == digest_from_hex(
        "64921cd67e285eafc90dee65797338b796436245937c3b9ba4d6d84448804bb3"));
    assert(aborted.result.envelope_digest == digest_from_hex(
        "944f6cb6da359e493806cd27d952b13323ad1744d056c0783431df8cb121971a"));
    assert(pending.result.proposed_anchor_envelope_digest == digest_from_hex(
        "6596067aab2b7f4e8aee5dc513d45e5fbe0df54b9c0305d05c7c03559c8adf80"));
    assert(pending.result.envelope_digest != success.result.envelope_digest);
    assert(success.result.envelope_digest != aborted.result.envelope_digest);
}

void test_exact_authentication_and_tamper() {
    auto value = make_fixture();
    refresh_views(value);
    const auto pending = encode_pending(value);
    const auto success = encode_terminal(
        value, halofpx::context_store_v1_attempt_terminal_status::success);

    assert(halofpx::context_store_v1_attempt_pending_verify(
        pending.bytes.data(), pending.result.encoded_size, value.body, value.key).authenticated());
    assert(halofpx::context_store_v1_attempt_terminal_verify(
        success.bytes.data(), success.result.encoded_size, value.body,
        halofpx::context_store_v1_attempt_terminal_status::success, value.key).authenticated());

    auto tampered = pending.bytes;
    tampered[pending.result.encoded_size / 2] ^= 1;
    assert(halofpx::context_store_v1_attempt_pending_verify(
        tampered.data(), pending.result.encoded_size, value.body, value.key).status ==
        halofpx::context_store_v1_attempt_wire_status::authentication_failed);

    auto wrong_master = value.master;
    wrong_master[0] ^= 1;
    auto wrong_key = value.key;
    wrong_key.master_key = { wrong_master.data(), wrong_master.size() };
    assert(halofpx::context_store_v1_attempt_pending_verify(
        pending.bytes.data(), pending.result.encoded_size, value.body, wrong_key).status ==
        halofpx::context_store_v1_attempt_wire_status::authentication_failed);

    assert(halofpx::context_store_v1_attempt_terminal_verify(
        success.bytes.data(), success.result.encoded_size, value.body,
        halofpx::context_store_v1_attempt_terminal_status::aborted, value.key).status ==
        halofpx::context_store_v1_attempt_wire_status::authentication_failed);
    assert(halofpx::context_store_v1_attempt_pending_verify(
        success.bytes.data(), success.result.encoded_size, value.body, value.key).status ==
        halofpx::context_store_v1_attempt_wire_status::authentication_failed);
}

void test_bound_fields() {
    auto value = make_fixture();
    refresh_views(value);
    const auto pending = encode_pending(value);
    const auto reject = [&](halofpx::context_store_v1_attempt_body changed) {
        assert(halofpx::context_store_v1_attempt_pending_verify(
            pending.bytes.data(), pending.result.encoded_size, changed, value.key).status ==
            halofpx::context_store_v1_attempt_wire_status::authentication_failed);
    };

    auto changed = value.body; changed.attempt_id[0] ^= 1; reject(changed);
    changed = value.body; changed.store_uuid[0] ^= 1; reject(changed);
    changed = value.body; changed.namespace_id[0] ^= 1; reject(changed);
    changed = value.body; changed.checkpoint_lineage_id[0] ^= 1; reject(changed);
    changed = value.body; changed.manifest_digest[0] ^= 1; reject(changed);
    changed = value.body; changed.ordered_object_set_commitment[0] ^= 1; reject(changed);
    changed = value.body; changed.aggregate_source_commitment[0] ^= 1; reject(changed);
    changed = value.body; changed.data_root_identity_commitment[0] ^= 1; reject(changed);
    changed = value.body; changed.anchor_root_identity_commitment[0] ^= 1; reject(changed);

    auto changed_anchor = value.anchor;
    changed_anchor[0] ^= 1;
    changed = value.body;
    changed.proposed_anchor_envelope = { changed_anchor.data(), changed_anchor.size() };
    reject(changed);
    changed = value.body;
    changed.proposed_anchor_envelope = { value.anchor.data(), value.anchor.size() - 1 };
    reject(changed);
}

void test_policy_and_bounds() {
    auto value = make_fixture();
    refresh_views(value);
    std::array<uint8_t, halofpx::context_store_v1_attempt_wire_max_bytes> output;
    output.fill(0xa5);

    auto invalid = value.body;
    invalid.attempt_id.fill(0);
    assert(halofpx::context_store_v1_attempt_pending_encode(
        invalid, value.key, output.data(), output.size()).status ==
        halofpx::context_store_v1_attempt_wire_status::invalid_policy);
    auto key = value.key;
    key.master_key.size = 31;
    assert(halofpx::context_store_v1_attempt_pending_encode(
        value.body, key, output.data(), output.size()).status ==
        halofpx::context_store_v1_attempt_wire_status::invalid_policy);
    assert(halofpx::context_store_v1_attempt_terminal_encode(
        value.body, static_cast<halofpx::context_store_v1_attempt_terminal_status>(3),
        value.key, output.data(), output.size()).status ==
        halofpx::context_store_v1_attempt_wire_status::invalid_policy);

    const auto pending = encode_pending(value);
    const auto small = halofpx::context_store_v1_attempt_pending_encode(
        value.body, value.key, output.data(), 8);
    assert(small.status == halofpx::context_store_v1_attempt_wire_status::output_too_small);
    assert(small.encoded_size == pending.result.encoded_size);
    assert(std::all_of(output.begin(), output.end(), [](uint8_t byte) { return byte == 0xa5; }));

    assert(halofpx::context_store_v1_attempt_pending_verify(
        nullptr, 0, value.body, value.key).status ==
        halofpx::context_store_v1_attempt_wire_status::input_rejected);
    std::vector<uint8_t> oversized(halofpx::context_store_v1_attempt_wire_max_bytes + 1);
    assert(halofpx::context_store_v1_attempt_pending_verify(
        oversized.data(), oversized.size(), value.body, value.key).status ==
        halofpx::context_store_v1_attempt_wire_status::input_rejected);
}

} // namespace

int main() {
    test_golden_and_determinism();
    test_exact_authentication_and_tamper();
    test_bound_fields();
    test_policy_and_bounds();
}
