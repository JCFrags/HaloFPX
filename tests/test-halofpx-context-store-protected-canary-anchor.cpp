#include "halofpx-context-store-protected-canary-anchor.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace {

halofpx::context_store_registered_id registered_id(const std::string & value) {
    halofpx::context_store_registered_id result;
    assert(!value.empty() && value.size() <= halofpx::context_store_registered_id_max_bytes);
    result.size = static_cast<uint8_t>(value.size());
    std::copy(value.begin(), value.end(), result.bytes.begin());
    return result;
}

std::vector<uint8_t> decode_hex(const char * text) {
    const auto nibble = [](char value) -> uint8_t {
        if (value >= '0' && value <= '9') return static_cast<uint8_t>(value - '0');
        assert(value >= 'a' && value <= 'f');
        return static_cast<uint8_t>(value - 'a' + 10);
    };
    const size_t size = std::char_traits<char>::length(text);
    assert(size % 2 == 0);
    std::vector<uint8_t> result(size / 2);
    for (size_t index = 0; index < result.size(); ++index) {
        result[index] = static_cast<uint8_t>((nibble(text[index * 2]) << 4) |
                                             nibble(text[index * 2 + 1]));
    }
    return result;
}

halofpx::context_store_protected_canary_anchor_body canary_body() {
    halofpx::context_store_protected_canary_anchor_body body;
    body.store_uuid.fill(0x11);
    body.namespace_id.fill(0x22);
    body.policy_epoch = 1;
    body.checkpoint_lineage_id.fill(0x44);
    body.manifest_key_generation = 1;
    body.authority_epoch = 1;
    body.generation = 1;
    body.selected_manifest_digest.fill(0x77);
    body.has_predecessor = false;
    return body;
}

struct canary_fixture {
    std::array<uint8_t, 32> master {};
    halofpx::context_store_protected_canary_anchor_body body = canary_body();
    halofpx::context_store_protected_canary_anchor_key key;
    std::array<uint8_t, halofpx::context_store_protected_canary_anchor_max_bytes> envelope {};
    size_t size = 0;
};

canary_fixture make_canary_fixture() {
    canary_fixture fixture;
    for (size_t index = 0; index < fixture.master.size(); ++index) {
        fixture.master[index] = static_cast<uint8_t>(index);
    }
    fixture.key.key_id = registered_id("halofpx-protected-anchor-v1");
    fixture.key.generation = 1;
    fixture.key.master_key = { fixture.master.data(), fixture.master.size() };
    const auto result = halofpx::context_store_protected_canary_anchor_encode_v1(
        fixture.body, fixture.key, fixture.envelope.data(), fixture.envelope.size());
    assert(result.authenticated());
    fixture.size = result.encoded_size;
    return fixture;
}

void refresh_key(canary_fixture & fixture) {
    fixture.key.master_key = { fixture.master.data(), fixture.master.size() };
}

void test_existing_adr0008_cross_golden() {
    halofpx::context_store_protected_canary_anchor_body body;
    body.store_uuid.fill(0x11);
    body.namespace_id.fill(0x22);
    body.policy_epoch = 3;
    body.checkpoint_lineage_id.fill(0x44);
    body.manifest_key_generation = 5;
    body.authority_epoch = 6;
    body.generation = 2;
    body.selected_manifest_digest.fill(0x77);
    body.has_predecessor = true;
    body.predecessor_manifest_digest.fill(0x88);

    std::array<uint8_t, 64> master {};
    for (size_t index = 0; index < master.size(); ++index) master[index] = static_cast<uint8_t>(index);
    halofpx::context_store_protected_canary_anchor_key key;
    key.key_id = registered_id("anchor-key-v1");
    key.generation = 9;
    key.master_key = { master.data(), master.size() };

    std::array<uint8_t, halofpx::context_store_protected_canary_anchor_max_bytes> actual {};
    size_t actual_size = 0;
    halofpx::context_store_format_digest digest {};
    assert(halofpx::protected_canary_anchor_test_only::canonical_wire_v1(
        body, key, actual.data(), actual.size(), actual_size, digest));

    const auto expected = decode_hex(
        "a200a400ab0001010002501111111111111111111111111111111103582022222222222222222222222222222222222222222222222222222222222222220403055820444444444444444444444444444444444444444444444444444444444444444406050706080209582077777777777777777777777777777777777777777777777777777777777777770a58208888888888888888888888888888888888888888888888888888888888888888016d616e63686f722d6b65792d76310201030901582041b4d7a3821784aa8776ac4dad38db57ffea381e892d597e0efca1b9717274a3");
    const auto expected_digest = decode_hex(
        "0e2ecaa98c3b05cedc60b5ca3b5947ddd49f62790367edde905be20967be55a3");
    const auto expected_tag = decode_hex(
        "41b4d7a3821784aa8776ac4dad38db57ffea381e892d597e0efca1b9717274a3");
    assert(actual_size == 229 && expected.size() == actual_size);
    assert(std::equal(expected.begin(), expected.end(), actual.begin()));
    assert(std::equal(expected_digest.begin(), expected_digest.end(), digest.begin()));
    assert(std::equal(expected_tag.begin(), expected_tag.end(), actual.begin() + actual_size - 32));

    // The untrusted vector primitive can encode the historical fixture, but
    // the product entry point cannot admit its generation-two policy.
    assert(halofpx::context_store_protected_canary_anchor_encode_v1(
        body, key, actual.data(), actual.size()).status ==
        halofpx::context_store_protected_canary_anchor_status::invalid_policy);
}

void test_closed_round_trip_and_mutations() {
    auto fixture = make_canary_fixture();
    refresh_key(fixture);
    const auto verified = halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size, fixture.body, fixture.key);
    assert(verified.authenticated() && verified.encoded_size == fixture.size);
    assert(halofpx::context_store_protected_canary_anchor_exact_envelope_equal(
        fixture.envelope.data(), fixture.size, fixture.envelope.data(), fixture.size));

    for (size_t offset = 0; offset < fixture.size; ++offset) {
        auto mutated = fixture.envelope;
        mutated[offset] ^= 1;
        assert(halofpx::context_store_protected_canary_anchor_verify_v1(
            mutated.data(), fixture.size, fixture.body, fixture.key).status ==
            halofpx::context_store_protected_canary_anchor_status::authentication_failed);
    }
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size - 1, fixture.body, fixture.key).status ==
        halofpx::context_store_protected_canary_anchor_status::authentication_failed);
    auto trailing = fixture.envelope;
    trailing[fixture.size] = 0;
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        trailing.data(), fixture.size + 1, fixture.body, fixture.key).status ==
        halofpx::context_store_protected_canary_anchor_status::authentication_failed);

    auto expected = fixture.body;
    expected.selected_manifest_digest[0] ^= 1;
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size, expected, fixture.key).status ==
        halofpx::context_store_protected_canary_anchor_status::authentication_failed);
    auto key = fixture.key;
    auto changed_master = fixture.master;
    changed_master[0] ^= 1;
    key.master_key = { changed_master.data(), changed_master.size() };
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size, fixture.body, key).status ==
        halofpx::context_store_protected_canary_anchor_status::authentication_failed);
}

void test_closed_policy_and_bounds() {
    auto fixture = make_canary_fixture();
    refresh_key(fixture);
    const auto invalid_body = [&](halofpx::context_store_protected_canary_anchor_body body) {
        std::array<uint8_t, halofpx::context_store_protected_canary_anchor_max_bytes> output {};
        assert(halofpx::context_store_protected_canary_anchor_encode_v1(
            body, fixture.key, output.data(), output.size()).status ==
            halofpx::context_store_protected_canary_anchor_status::invalid_policy);
    };
    auto body = fixture.body; body.policy_epoch = 2; invalid_body(body);
    body = fixture.body; body.manifest_key_generation = 2; invalid_body(body);
    body = fixture.body; body.authority_epoch = 2; invalid_body(body);
    body = fixture.body; body.generation = 2; body.has_predecessor = true; invalid_body(body);
    body = fixture.body; body.has_predecessor = true; invalid_body(body);
    body = fixture.body; body.predecessor_manifest_digest[0] = 1; invalid_body(body);
    body = fixture.body; body.store_uuid.fill(0); invalid_body(body);
    body = fixture.body; body.namespace_id.fill(0); invalid_body(body);
    body = fixture.body; body.checkpoint_lineage_id.fill(0); invalid_body(body);
    body = fixture.body; body.selected_manifest_digest.fill(0); invalid_body(body);

    auto key = fixture.key; key.generation = 2;
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size, fixture.body, key).status ==
        halofpx::context_store_protected_canary_anchor_status::invalid_policy);
    key = fixture.key; key.master_key.size = 31;
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size, fixture.body, key).status ==
        halofpx::context_store_protected_canary_anchor_status::invalid_policy);
    key = fixture.key; key.key_id = registered_id("different-protected-anchor-v1");
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size, fixture.body, key).status ==
        halofpx::context_store_protected_canary_anchor_status::invalid_policy);
    key = fixture.key; key.key_id.size = 0;
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        fixture.envelope.data(), fixture.size, fixture.body, key).status ==
        halofpx::context_store_protected_canary_anchor_status::invalid_policy);

    std::array<uint8_t, halofpx::context_store_protected_canary_anchor_max_bytes> output;
    output.fill(0xa5);
    const auto small = halofpx::context_store_protected_canary_anchor_encode_v1(
        fixture.body, fixture.key, output.data(), 8);
    assert(small.status == halofpx::context_store_protected_canary_anchor_status::output_too_small);
    assert(small.encoded_size == fixture.size);
    assert(std::all_of(output.begin(), output.end(), [](uint8_t byte) { return byte == 0xa5; }));

    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        nullptr, 0, fixture.body, fixture.key).status ==
        halofpx::context_store_protected_canary_anchor_status::input_rejected);
    std::vector<uint8_t> oversized(halofpx::context_store_protected_canary_anchor_max_bytes + 1);
    assert(halofpx::context_store_protected_canary_anchor_verify_v1(
        oversized.data(), oversized.size(), fixture.body, fixture.key).status ==
        halofpx::context_store_protected_canary_anchor_status::input_rejected);
}

} // namespace

int main() {
    test_existing_adr0008_cross_golden();
    test_closed_round_trip_and_mutations();
    test_closed_policy_and_bounds();
}
