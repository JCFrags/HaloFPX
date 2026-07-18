#include "halofpx-context-store-anchor.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

static_assert(!std::is_aggregate_v<halofpx::context_store_anchor_result>);

namespace {

halofpx::context_store_registered_id id(const std::string & value) {
    halofpx::context_store_registered_id result;
    assert(value.size() <= halofpx::context_store_registered_id_max_bytes);
    result.size = static_cast<uint8_t>(value.size());
    std::copy(value.begin(), value.end(), result.bytes.begin());
    return result;
}

halofpx::context_store_anchor_body anchor(uint64_t generation = 2) {
    halofpx::context_store_anchor_body result;
    result.store_uuid.fill(0x11);
    result.namespace_id.fill(0x22);
    result.policy_epoch = 3;
    result.checkpoint_lineage_id.fill(0x44);
    result.manifest_key_generation = 5;
    result.authority_epoch = 6;
    result.generation = generation;
    result.selected_manifest_digest.fill(0x77);
    result.has_predecessor = generation > 1;
    result.predecessor_manifest_digest.fill(0x88);
    return result;
}

struct fixture {
    std::array<uint8_t, 64> master {};
    halofpx::context_store_anchor_key_record key;
    halofpx::context_store_anchor_policy policy;
    std::array<uint8_t, halofpx::context_store_anchor_max_bytes> encoded {};
    size_t size = 0;
};

fixture make_fixture() {
    fixture f;
    for (size_t i = 0; i < f.master.size(); ++i) f.master[i] = static_cast<uint8_t>(i);
    f.key.disposition = halofpx::context_store_key_disposition::active;
    f.key.key_id = id("anchor-key-v1");
    f.key.generation = 9;
    f.key.master_key = { f.master.data(), f.master.size() };
    f.policy.key = f.key;
    f.policy.expected = anchor();
    const auto encoded = halofpx::context_store_encode_anchor_v1(f.policy.expected, f.key, f.encoded.data(), f.encoded.size());
    assert(encoded.status == halofpx::context_store_anchor_status::authenticated_unadmitted);
    f.size = encoded.encoded_size;
    return f;
}

void refresh_borrowed_key(fixture & f) {
    f.key.master_key = { f.master.data(), f.master.size() };
    f.policy.key.master_key = { f.master.data(), f.master.size() };
}

void test_round_trip_and_golden() {
    auto f = make_fixture();
    refresh_borrowed_key(f);
    const auto verified = halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, f.policy);
    assert(verified.status == halofpx::context_store_anchor_status::authenticated_unadmitted);
    assert(verified.has_authenticated_anchor());
    assert(verified.authenticated_anchor() != nullptr);
    assert(verified.encoded_size == f.size);
    // Fixed independent receipt values. These pin canonical bytes and domains.
    assert(f.size == 229);
    const std::array<uint8_t, 32> expected_digest = {
        0x0e,0x2e,0xca,0xa9,0x8c,0x3b,0x05,0xce,0xdc,0x60,0xb5,0xca,0x3b,0x59,0x47,0xdd,
        0xd4,0x9f,0x62,0x79,0x03,0x67,0xed,0xde,0x90,0x5b,0xe2,0x09,0x67,0xbe,0x55,0xa3 };
    assert(verified.envelope_digest == expected_digest);
    const std::array<uint8_t, 32> expected_tag = {
        0x41,0xb4,0xd7,0xa3,0x82,0x17,0x84,0xaa,0x87,0x76,0xac,0x4d,0xad,0x38,0xdb,0x57,
        0xff,0xea,0x38,0x1e,0x89,0x2d,0x59,0x7e,0x0e,0xfc,0xa1,0xb9,0x71,0x72,0x74,0xa3 };
    assert(std::equal(expected_tag.begin(), expected_tag.end(), f.encoded.begin() + f.size - expected_tag.size()));
}

void test_bounds_and_mutations() {
    auto f = make_fixture();
    refresh_borrowed_key(f);
    for (size_t n = 0; n < f.size; ++n) {
        const auto result = halofpx::context_store_verify_anchor_v1(f.encoded.data(), n, f.policy);
        assert(result.status == halofpx::context_store_anchor_status::structural_rejection);
        assert(!result.has_authenticated_anchor() && result.authenticated_anchor() == nullptr);
    }
    auto trailing = f.encoded; trailing[f.size] = 0;
    assert(halofpx::context_store_verify_anchor_v1(trailing.data(), f.size + 1, f.policy).status ==
        halofpx::context_store_anchor_status::structural_rejection);
    auto oversized = std::vector<uint8_t>(halofpx::context_store_anchor_max_bytes + 1, 0);
    assert(halofpx::context_store_verify_anchor_v1(oversized.data(), oversized.size(), f.policy).status ==
        halofpx::context_store_anchor_status::structural_rejection);
    for (size_t offset : { f.size - 32, f.size - 16, f.size - 1 }) {
        auto mutated = f.encoded; mutated[offset] ^= 1;
        assert(halofpx::context_store_verify_anchor_v1(mutated.data(), f.size, f.policy).status ==
            halofpx::context_store_anchor_status::authentication_failed);
    }
    auto body_mutation = f.encoded;
    const std::array<uint8_t, 8> selected_run = { 0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77 };
    const auto selected_at = std::search(body_mutation.begin(), body_mutation.begin() + f.size,
        selected_run.begin(), selected_run.end());
    assert(selected_at != body_mutation.begin() + f.size);
    *selected_at ^= 1;
    assert(halofpx::context_store_verify_anchor_v1(body_mutation.data(), f.size, f.policy).status ==
        halofpx::context_store_anchor_status::authentication_failed);
    auto noncanonical = f.encoded;
    noncanonical[0] = 0xb8; // non-shortest/invalid outer map header for this input
    assert(halofpx::context_store_verify_anchor_v1(noncanonical.data(), f.size, f.policy).status ==
        halofpx::context_store_anchor_status::structural_rejection);
    auto unchanged = f.encoded;
    (void) halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, f.policy);
    assert(f.encoded == unchanged);
}

void test_closed_canonical_parser_rejections() {
    auto f = make_fixture();
    refresh_borrowed_key(f);
    const auto reject = [&](std::vector<uint8_t> value) {
        const auto result = halofpx::context_store_verify_anchor_v1(value.data(), value.size(), f.policy);
        assert(result.status == halofpx::context_store_anchor_status::structural_rejection);
        assert(!result.has_authenticated_anchor());
    };
    const std::vector<uint8_t> original(f.encoded.begin(), f.encoded.begin() + f.size);
    auto mutate = [&](size_t offset, uint8_t value) { auto bytes = original; bytes[offset] = value; reject(std::move(bytes)); };

    mutate(0, 0xbf); // indefinite outer map
    mutate(0, 0xa3); // unexpected outer field count
    mutate(2, 0xa5); // wrong authentication-input map count
    mutate(4, 0xaa); // missing body field
    mutate(4, 0xbf); // indefinite body map
    mutate(5, 0x01); // reordered/duplicate first body key
    mutate(6, 0x02); // wrong major version
    mutate(8, 0x01); // wrong minor version
    mutate(10, 0x51); // malformed store UUID byte-string length

    auto nonshortest = original;
    nonshortest[6] = 0x18;
    nonshortest.insert(nonshortest.begin() + 7, 0x01);
    reject(std::move(nonshortest));

    const std::array<uint8_t, 7> algorithm_tail = { 0x02,0x01,0x03,0x09,0x01,0x58,0x20 };
    const auto algorithm_at = std::search(original.begin(), original.end(), algorithm_tail.begin(), algorithm_tail.end());
    assert(algorithm_at != original.end());
    auto wrong_algorithm = original; wrong_algorithm[static_cast<size_t>(algorithm_at - original.begin()) + 1] = 2; reject(std::move(wrong_algorithm));

    const std::string key_text = "anchor-key-v1";
    const auto key_at = std::search(original.begin(), original.end(), key_text.begin(), key_text.end());
    assert(key_at != original.end());
    auto nul_key = original; nul_key[static_cast<size_t>(key_at - original.begin()) + 3] = 0; reject(std::move(nul_key));
    auto non_ascii_key = original; non_ascii_key[static_cast<size_t>(key_at - original.begin())] = 0x80; reject(std::move(non_ascii_key));
    auto indefinite_text = original; indefinite_text[static_cast<size_t>(key_at - original.begin()) - 1] = 0x7f; reject(std::move(indefinite_text));

    auto bad_tag_length = original; bad_tag_length[f.size - 33] = 0x1f; reject(std::move(bad_tag_length));
}

void test_policy_and_replay() {
    auto f = make_fixture();
    refresh_borrowed_key(f);
    auto policy = f.policy;
    policy.key.disposition = halofpx::context_store_key_disposition::revoked;
    const auto revoked = halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, policy);
    assert(revoked.status == halofpx::context_store_anchor_status::revoked_key);
    assert(!revoked.has_authenticated_anchor());
    policy = f.policy; policy.key.disposition = halofpx::context_store_key_disposition::read_disabled;
    assert(halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, policy).status == halofpx::context_store_anchor_status::read_disabled_key);
    policy = f.policy; ++policy.key.generation;
    assert(halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, policy).status == halofpx::context_store_anchor_status::key_generation_mismatch);
    policy = f.policy; policy.key.key_id = id("different-key");
    assert(halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, policy).status == halofpx::context_store_anchor_status::unknown_key);
    policy = f.policy; ++policy.expected.authority_epoch;
    assert(halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, policy).status == halofpx::context_store_anchor_status::authority_mismatch);
    policy = f.policy; --policy.expected.generation; policy.expected.has_predecessor = false;
    assert(halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, policy).status == halofpx::context_store_anchor_status::replay_mismatch);
    policy = f.policy; ++policy.expected.generation;
    assert(halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, policy).status == halofpx::context_store_anchor_status::rollback_detected);

    std::array<uint8_t, halofpx::context_store_anchor_max_bytes> resigned {};
    auto changed = f.policy.expected; changed.store_uuid[0] ^= 1;
    auto encoded = halofpx::context_store_encode_anchor_v1(changed, f.key, resigned.data(), resigned.size());
    assert(encoded.status == halofpx::context_store_anchor_status::authenticated_unadmitted);
    assert(halofpx::context_store_verify_anchor_v1(resigned.data(), encoded.encoded_size, f.policy).status ==
        halofpx::context_store_anchor_status::authority_mismatch);
    changed = f.policy.expected; changed.generation = 1; changed.has_predecessor = false;
    encoded = halofpx::context_store_encode_anchor_v1(changed, f.key, resigned.data(), resigned.size());
    assert(halofpx::context_store_verify_anchor_v1(resigned.data(), encoded.encoded_size, f.policy).status ==
        halofpx::context_store_anchor_status::rollback_detected);
    changed = f.policy.expected; changed.generation = 3; changed.has_predecessor = true;
    encoded = halofpx::context_store_encode_anchor_v1(changed, f.key, resigned.data(), resigned.size());
    assert(halofpx::context_store_verify_anchor_v1(resigned.data(), encoded.encoded_size, f.policy).status ==
        halofpx::context_store_anchor_status::replay_mismatch);
    changed = f.policy.expected; changed.selected_manifest_digest[15] ^= 1;
    encoded = halofpx::context_store_encode_anchor_v1(changed, f.key, resigned.data(), resigned.size());
    assert(halofpx::context_store_verify_anchor_v1(resigned.data(), encoded.encoded_size, f.policy).status ==
        halofpx::context_store_anchor_status::replay_mismatch);

    std::array<uint8_t, halofpx::context_store_anchor_max_bytes> output {};
    auto invalid = anchor(1); invalid.has_predecessor = true;
    assert(halofpx::context_store_encode_anchor_v1(invalid, f.key, output.data(), output.size()).status == halofpx::context_store_anchor_status::invalid_policy);
    invalid = anchor(2); invalid.has_predecessor = false;
    assert(halofpx::context_store_encode_anchor_v1(invalid, f.key, output.data(), output.size()).status == halofpx::context_store_anchor_status::invalid_policy);
    output.fill(0xa5);
    assert(halofpx::context_store_encode_anchor_v1(anchor(), f.key, output.data(), 8).status == halofpx::context_store_anchor_status::output_too_small);
    assert(std::all_of(output.begin(), output.end(), [](uint8_t value) { return value == 0xa5; }));
    auto bad_key = f.key; bad_key.key_id.size = 0;
    assert(halofpx::context_store_encode_anchor_v1(anchor(), bad_key, output.data(), output.size()).status == halofpx::context_store_anchor_status::invalid_policy);
    auto multibyte_key = f.key; multibyte_key.key_id = id("anchor-\xc3\xb8-v1");
    assert(halofpx::context_store_encode_anchor_v1(anchor(), multibyte_key, output.data(), output.size()).status == halofpx::context_store_anchor_status::invalid_policy);
    auto nul_key = f.key; nul_key.key_id = id("anchor-key-v1"); nul_key.key_id.bytes[3] = '\0';
    assert(halofpx::context_store_encode_anchor_v1(anchor(), nul_key, output.data(), output.size()).status == halofpx::context_store_anchor_status::invalid_policy);
}

void test_deterministic_concurrency() {
    auto f = make_fixture();
    refresh_borrowed_key(f);
    std::array<halofpx::context_store_anchor_status, 16> statuses {};
    std::vector<std::thread> threads;
    for (size_t i = 0; i < statuses.size(); ++i) threads.emplace_back([&, i] {
        statuses[i] = halofpx::context_store_verify_anchor_v1(f.encoded.data(), f.size, f.policy).status;
    });
    for (auto & thread : threads) thread.join();
    for (auto status : statuses) assert(status == halofpx::context_store_anchor_status::authenticated_unadmitted);
}

} // namespace

int main() {
    test_round_trip_and_golden();
    test_bounds_and_mutations();
    test_closed_canonical_parser_rejections();
    test_policy_and_replay();
    test_deterministic_concurrency();
}
