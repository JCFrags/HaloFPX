#include "halofpx-context-store-auth.h"

#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(!std::is_aggregate_v<halofpx::context_store_manifest_verify_result>);
static_assert(std::is_same_v<
    decltype(std::declval<const halofpx::context_store_manifest_verify_result &>().authenticated_object_reference(0)),
    const halofpx::context_store_object_reference *>);

namespace {

using bytes = std::vector<uint8_t>;
using halofpx::context_store_format_digest;

void append_head(bytes & output, uint8_t major, uint64_t value) {
    if (value < 24) {
        output.push_back(static_cast<uint8_t>((major << 5) | value));
    } else if (value <= 0xff) {
        output.push_back(static_cast<uint8_t>((major << 5) | 24));
        output.push_back(static_cast<uint8_t>(value));
    } else if (value <= 0xffff) {
        output.push_back(static_cast<uint8_t>((major << 5) | 25));
        output.push_back(static_cast<uint8_t>(value >> 8));
        output.push_back(static_cast<uint8_t>(value));
    } else if (value <= 0xffffffffULL) {
        output.push_back(static_cast<uint8_t>((major << 5) | 26));
        for (int shift = 24; shift >= 0; shift -= 8) output.push_back(static_cast<uint8_t>(value >> shift));
    } else {
        output.push_back(static_cast<uint8_t>((major << 5) | 27));
        for (int shift = 56; shift >= 0; shift -= 8) output.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void append_uint(bytes & output, uint64_t value) { append_head(output, 0, value); }
void append_map(bytes & output, uint64_t pairs) { append_head(output, 5, pairs); }
void append_array(bytes & output, uint64_t items) { append_head(output, 4, items); }

void append_bytes(bytes & output, const uint8_t * data, size_t size) {
    append_head(output, 2, size);
    output.insert(output.end(), data, data + size);
}

void append_fill(bytes & output, size_t size, uint8_t value) {
    append_head(output, 2, size);
    output.insert(output.end(), size, value);
}

void append_digest(bytes & output, const context_store_format_digest & digest) {
    append_bytes(output, digest.data(), digest.size());
}

void append_text(bytes & output, const std::string & value) {
    append_head(output, 3, value.size());
    output.insert(output.end(), value.begin(), value.end());
}

bytes domain_message(const char * domain, const bytes & body) {
    bytes output(reinterpret_cast<const uint8_t *>(domain),
                 reinterpret_cast<const uint8_t *>(domain) + std::strlen(domain) + 1);
    output.insert(output.end(), body.begin(), body.end());
    return output;
}

context_store_format_digest sha256(const bytes & input) {
    context_store_format_digest output {};
    assert(halofpx::context_store_sha256(input.data(), input.size(), output));
    return output;
}

context_store_format_digest hmac(const bytes & key, const bytes & input) {
    context_store_format_digest output {};
    assert(halofpx::context_store_hmac_sha256(
        key.data(), key.size(), input.data(), input.size(), output));
    return output;
}

std::string hex(const context_store_format_digest & value) {
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string output(value.size() * 2, '0');
    for (size_t index = 0; index < value.size(); ++index) {
        output[index * 2] = alphabet[value[index] >> 4];
        output[index * 2 + 1] = alphabet[value[index] & 0x0f];
    }
    return output;
}

halofpx::context_store_registered_id registered_id(const std::string & value) {
    halofpx::context_store_registered_id output;
    assert(!value.empty() && value.size() <= halofpx::context_store_registered_id_max_bytes);
    output.size = static_cast<uint8_t>(value.size());
    std::copy(value.begin(), value.end(), output.bytes.begin());
    return output;
}

struct fixture_options {
    uint8_t component_delta = 0;
    bool corrupt_stored_root = false;
    std::string object_type = "tokens";
    std::string key_id = "test-key-v1";
    uint64_t key_generation = 1;
    uint64_t generation = 1;
};

struct signed_fixture {
    bytes manifest;
    bytes master_key;
    halofpx::context_store_manifest_verification_policy policy;
    context_store_format_digest derived_key {};
    context_store_format_digest tag {};

    void bind_key() {
        policy.key.master_key = { master_key.data(), master_key.size() };
    }
};

signed_fixture make_signed_fixture(const fixture_options & options = {}) {
    signed_fixture fixture;
    fixture.master_key.assign(32, 0x11);

    for (size_t index = 0; index < fixture.policy.compatibility.components.size(); ++index) {
        fixture.policy.compatibility.components[index].fill(static_cast<uint8_t>(0x20 + index));
    }
    fixture.policy.compatibility.components[0][0] ^= options.component_delta;

    bytes compatibility_map;
    append_map(compatibility_map, 16);
    for (size_t index = 0; index < fixture.policy.compatibility.components.size(); ++index) {
        append_uint(compatibility_map, index);
        append_digest(compatibility_map, fixture.policy.compatibility.components[index]);
    }
    fixture.policy.compatibility.root = sha256(domain_message("halofpx.compat.v1", compatibility_map));
    context_store_format_digest stored_root = fixture.policy.compatibility.root;
    if (options.corrupt_stored_root) stored_root[0] ^= 0x01;

    bytes body;
    append_map(body, 15);
    append_uint(body, 0); append_uint(body, 1);
    append_uint(body, 1); append_uint(body, 0);
    append_uint(body, 2); append_fill(body, 16, 0x02);
    append_uint(body, 3); append_fill(body, 32, 0x03);
    append_uint(body, 4); append_uint(body, options.generation);
    append_uint(body, 5);
    if (options.generation == 1) body.push_back(0xf6);
    else append_fill(body, 32, 0x05);
    append_uint(body, 6); body.insert(body.end(), compatibility_map.begin(), compatibility_map.end());
    append_uint(body, 7); append_digest(body, stored_root);
    append_uint(body, 8); append_fill(body, 32, 0x80);
    append_uint(body, 9); append_uint(body, 7);
    append_uint(body, 10);
    append_map(body, 6);
    append_uint(body, 0); append_text(body, "plan.synthetic.v1");
    append_uint(body, 1); append_text(body, "single");
    append_uint(body, 2); append_uint(body, 1);
    append_uint(body, 3); append_fill(body, 32, 0xa3);
    append_uint(body, 4); append_uint(body, 9);
    append_uint(body, 5); append_array(body, 1);
    append_map(body, 3);
    append_uint(body, 0); append_uint(body, 0);
    append_uint(body, 1); append_fill(body, 32, 0xb0);
    append_uint(body, 2); append_fill(body, 32, 0xc0);
    append_uint(body, 11); append_text(body, "profile.synthetic.v1");
    append_uint(body, 12); append_array(body, 1);
    append_map(body, 13);
    append_uint(body, 0); append_fill(body, 32, 0xd0);
    append_uint(body, 1); append_text(body, options.object_type);
    append_uint(body, 2); append_text(body, "codec.synthetic.v1");
    append_uint(body, 3); append_uint(body, 1);
    append_uint(body, 4); append_uint(body, 0);
    append_uint(body, 5); body.push_back(0xf5);
    append_uint(body, 6); append_uint(body, 64);
    append_uint(body, 7); append_fill(body, 32, 0xd7);
    append_uint(body, 8); append_uint(body, 16);
    append_uint(body, 9); append_uint(body, 8);
    append_uint(body, 10); append_uint(body, 0);
    append_uint(body, 11); append_fill(body, 32, 0xb0);
    append_uint(body, 12); append_digest(body, stored_root);
    append_uint(body, 13); append_fill(body, 32, 0xe3);
    append_uint(body, 14); append_uint(body, 0);

    bytes auth_input;
    append_map(auth_input, 4);
    append_uint(auth_input, 0); auth_input.insert(auth_input.end(), body.begin(), body.end());
    append_uint(auth_input, 1); append_text(auth_input, options.key_id);
    append_uint(auth_input, 2); append_uint(auth_input, 1);
    append_uint(auth_input, 3); append_uint(auth_input, options.key_generation);

    constexpr char key_domain[] = "halofpx.manifest-key.v1";
    bytes kdf_input(reinterpret_cast<const uint8_t *>(key_domain),
                    reinterpret_cast<const uint8_t *>(key_domain) + std::strlen(key_domain));
    kdf_input.push_back(0);
    kdf_input.push_back(static_cast<uint8_t>(options.key_id.size() >> 8));
    kdf_input.push_back(static_cast<uint8_t>(options.key_id.size()));
    kdf_input.insert(kdf_input.end(), options.key_id.begin(), options.key_id.end());
    kdf_input.insert(kdf_input.end(), 16, 0x02);
    kdf_input.insert(kdf_input.end(), 32, 0x80);
    for (int shift = 56; shift >= 0; shift -= 8) {
        kdf_input.push_back(static_cast<uint8_t>(options.key_generation >> shift));
    }
    fixture.derived_key = hmac(fixture.master_key, kdf_input);
    fixture.tag = hmac(bytes(fixture.derived_key.begin(), fixture.derived_key.end()),
                       domain_message("halofpx.manifest-auth.v1", auth_input));

    append_map(fixture.manifest, 2);
    append_uint(fixture.manifest, 0);
    fixture.manifest.insert(fixture.manifest.end(), auth_input.begin(), auth_input.end());
    append_uint(fixture.manifest, 1);
    append_digest(fixture.manifest, fixture.tag);

    fixture.policy.key.disposition = halofpx::context_store_key_disposition::active;
    fixture.policy.key.key_id = registered_id(options.key_id);
    fixture.policy.key.generation = options.key_generation;
    fixture.policy.anchor.store_uuid.fill(0x02);
    fixture.policy.anchor.checkpoint_lineage_id.fill(0x03);
    fixture.policy.anchor.namespace_id.fill(0x80);
    fixture.policy.anchor.policy_epoch = 7;
    fixture.policy.anchor.key_generation = options.key_generation;
    fixture.policy.anchor.generation = options.generation;
    fixture.policy.anchor.has_predecessor = options.generation > 1;
    if (fixture.policy.anchor.has_predecessor) {
        fixture.policy.anchor.predecessor_manifest_digest.fill(0x05);
    }
    fixture.policy.anchor.selected_manifest_digest =
        sha256(domain_message("halofpx.manifest.v1", fixture.manifest));
    fixture.bind_key();
    return fixture;
}

halofpx::context_store_manifest_verify_status verify(signed_fixture & fixture) {
    fixture.bind_key();
    return halofpx::context_store_verify_manifest_v1(
        fixture.manifest.data(), fixture.manifest.size(), fixture.policy).status;
}

void test_standard_vectors() {
    context_store_format_digest digest {};
    assert(halofpx::context_store_sha256(nullptr, 0, digest));
    assert(hex(digest) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    const bytes abc = { 'a', 'b', 'c' };
    assert(hex(sha256(abc)) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    const bytes key1(20, 0x0b);
    const bytes message1 = { 'H','i',' ','T','h','e','r','e' };
    assert(hex(hmac(key1, message1)) == "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    const bytes long_key(131, 0xaa);
    const std::string long_message = "Test Using Larger Than Block-Size Key - Hash Key First";
    assert(hex(hmac(long_key, bytes(long_message.begin(), long_message.end()))) ==
        "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");

    const std::pair<size_t, const char *> boundary_vectors[] = {
        { 55, "26ee0116778740a66fe2ba10ea063748b27306acc99188ec812746d4e8d70083" },
        { 56, "4cf71e2b0aa0fcc0c271f68353026a77b8e50153632a8e4a73833cd64080e92e" },
        { 63, "a1942663a5b8b93dffc9c4ff5f62c71a1c021d1fcc1e470dd46172abace1bca5" },
        { 64, "bb626e5577021df95ea17eb6339e75904855b80087e40660931c4a89b302f74a" },
        { 65, "667f84020d981fcedce2816e4e9969a02d5c317a0aef56a6c588175820f82a81" },
    };
    for (const auto & vector : boundary_vectors) {
        assert(hex(sha256(bytes(vector.first, 0xa5))) == vector.second);
    }

    assert(!halofpx::context_store_sha256(nullptr, 1, digest));
    assert(!halofpx::context_store_hmac_sha256(nullptr, 1, abc.data(), abc.size(), digest));
}

void test_authenticated_terminal_miss() {
    auto fixture = make_signed_fixture();
    assert(hex(fixture.policy.compatibility.root) == "98e4f525cca02e38645b162e130c55cf0c91172389b24ea22414d6c9f42952f3");
    assert(hex(fixture.derived_key) == "32842cd046eecc78fe2bbdb9b6f8e5ce2561f4fdbc198a7f32d5eec8e1e251f9");
    assert(hex(fixture.tag) == "81f26ac59223fbd9cdcfe5fec47f0db0bf53c886523d7f1b2c9e1d3497a7b54c");
    assert(hex(fixture.policy.anchor.selected_manifest_digest) == "5139b769df3ff7b40ac5177b45aafb78991d9b0a4e9a48663b4a9a83e8be261f");
    fixture.bind_key();
    const auto result = halofpx::context_store_verify_manifest_v1(
        fixture.manifest.data(), fixture.manifest.size(), fixture.policy);
    assert(result.status == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    assert(result.authenticated_object_count() == 1);
    const auto * reference = result.authenticated_object_reference(0);
    assert(reference != nullptr && result.authenticated_object_reference(1) == nullptr);
    assert(reference->object_id[0] == 0xd0);
    assert(reference->stream_type.size == 6);
    assert(std::string(reference->stream_type.bytes.data(), 6) == "tokens");
    assert(reference->frame_bytes == 64);

    auto changed_descriptor = make_signed_fixture({ 0, false, "sampler" });
    assert(verify(changed_descriptor) == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    auto multibyte_key = make_signed_fixture({ 0, false, "tokens", "key.\xc3\xb8.v1", 3 });
    assert(verify(multibyte_key) == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    auto explicitly_registered_zero = make_signed_fixture({ 0, false, "tokens", "zero-key", 0 });
    assert(verify(explicitly_registered_zero) == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    fixture_options generation_two_options;
    generation_two_options.generation = 2;
    auto generation_two = make_signed_fixture(generation_two_options);
    assert(verify(generation_two) == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    generation_two.policy.anchor.predecessor_manifest_digest[0] ^= 0x01;
    assert(verify(generation_two) == halofpx::context_store_manifest_verify_status::replay_mismatch);
}

void test_authentication_and_key_rejection() {
    for (size_t tag_index : { size_t(0), size_t(16), size_t(31) }) {
        auto fixture = make_signed_fixture();
        fixture.manifest[fixture.manifest.size() - 32 + tag_index] ^= 0x01;
        fixture.bind_key();
        const auto result = halofpx::context_store_verify_manifest_v1(
            fixture.manifest.data(), fixture.manifest.size(), fixture.policy);
        assert(result.status == halofpx::context_store_manifest_verify_status::authentication_failed);
        assert(result.authenticated_object_count() == 0);
    }
    auto body_flip = make_signed_fixture();
    body_flip.manifest[20] ^= 0x01;
    assert(verify(body_flip) == halofpx::context_store_manifest_verify_status::authentication_failed);

    auto unknown = make_signed_fixture();
    unknown.policy.key.key_id = registered_id("unknown-key");
    unknown.policy.key.master_key = {};
    assert(halofpx::context_store_verify_manifest_v1(
        unknown.manifest.data(), unknown.manifest.size(), unknown.policy).status ==
        halofpx::context_store_manifest_verify_status::unknown_key);
    unknown.bind_key();
    assert(verify(unknown) == halofpx::context_store_manifest_verify_status::unknown_key);
    auto revoked = make_signed_fixture();
    revoked.policy.key.disposition = halofpx::context_store_key_disposition::revoked;
    revoked.policy.key.master_key = {};
    assert(halofpx::context_store_verify_manifest_v1(
        revoked.manifest.data(), revoked.manifest.size(), revoked.policy).status ==
        halofpx::context_store_manifest_verify_status::revoked_key);
    revoked.bind_key();
    assert(verify(revoked) == halofpx::context_store_manifest_verify_status::revoked_key);
    auto disabled = make_signed_fixture();
    disabled.policy.key.disposition = halofpx::context_store_key_disposition::read_disabled;
    disabled.policy.key.master_key = {};
    assert(halofpx::context_store_verify_manifest_v1(
        disabled.manifest.data(), disabled.manifest.size(), disabled.policy).status ==
        halofpx::context_store_manifest_verify_status::read_disabled_key);
    disabled.bind_key();
    assert(verify(disabled) == halofpx::context_store_manifest_verify_status::read_disabled_key);
    auto generation = make_signed_fixture();
    generation.policy.key.generation = 2;
    assert(verify(generation) == halofpx::context_store_manifest_verify_status::key_generation_mismatch);
    auto unavailable = make_signed_fixture();
    unavailable.policy.key.master_key = {};
    assert(halofpx::context_store_verify_manifest_v1(
        unavailable.manifest.data(), unavailable.manifest.size(), unavailable.policy).status ==
        halofpx::context_store_manifest_verify_status::invalid_policy);
}

void test_authority_replay_and_compatibility_rejection() {
    auto authority = make_signed_fixture();
    authority.policy.anchor.namespace_id[0] ^= 0x01;
    assert(verify(authority) == halofpx::context_store_manifest_verify_status::authority_mismatch);

    auto replay = make_signed_fixture();
    replay.policy.anchor.selected_manifest_digest[31] ^= 0x01;
    assert(verify(replay) == halofpx::context_store_manifest_verify_status::replay_mismatch);
    auto replay_generation = make_signed_fixture();
    replay_generation.policy.anchor.generation = 2;
    assert(verify(replay_generation) == halofpx::context_store_manifest_verify_status::replay_mismatch);
    auto replay_key_generation = make_signed_fixture();
    replay_key_generation.policy.anchor.key_generation = 2;
    assert(verify(replay_key_generation) == halofpx::context_store_manifest_verify_status::replay_mismatch);
    auto replay_lineage = make_signed_fixture();
    replay_lineage.policy.anchor.checkpoint_lineage_id[0] ^= 0x01;
    assert(verify(replay_lineage) == halofpx::context_store_manifest_verify_status::replay_mismatch);

    auto corrupt = make_signed_fixture({ 0, true });
    assert(verify(corrupt) == halofpx::context_store_manifest_verify_status::compatibility_corrupt);

    auto expected_mismatch = make_signed_fixture();
    expected_mismatch.policy.compatibility.components[3][4] ^= 0x01;
    assert(verify(expected_mismatch) == halofpx::context_store_manifest_verify_status::compatibility_mismatch);

    for (size_t component = 0; component < halofpx::context_store_compatibility_component_count; ++component) {
        auto mismatch = make_signed_fixture();
        mismatch.policy.compatibility.components[component][31] ^= 0x01;
        assert(verify(mismatch) == halofpx::context_store_manifest_verify_status::compatibility_mismatch);
    }
    auto resigned_changed_component = make_signed_fixture({ 1 });
    auto original = make_signed_fixture();
    resigned_changed_component.policy.compatibility = original.policy.compatibility;
    assert(verify(resigned_changed_component) == halofpx::context_store_manifest_verify_status::compatibility_mismatch);

    auto structurally_bad = make_signed_fixture();
    structurally_bad.manifest.pop_back();
    assert(verify(structurally_bad) == halofpx::context_store_manifest_verify_status::structural_rejection);
}

} // namespace

int main() {
    test_standard_vectors();
    test_authenticated_terminal_miss();
    test_authentication_and_key_rejection();
    test_authority_replay_and_compatibility_rejection();
    return 0;
}
