#include "halofpx-context-store-v1-read-only.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

using bytes = std::vector<uint8_t>;
using halofpx::context_store_format_digest;

void head(bytes & out, uint8_t major, uint64_t value) {
    if (value < 24) {
        out.push_back(static_cast<uint8_t>((major << 5) | value));
    } else if (value <= 0xff) {
        out.push_back(static_cast<uint8_t>((major << 5) | 24)); out.push_back(static_cast<uint8_t>(value));
    } else if (value <= 0xffff) {
        out.push_back(static_cast<uint8_t>((major << 5) | 25));
        out.push_back(static_cast<uint8_t>(value >> 8)); out.push_back(static_cast<uint8_t>(value));
    } else if (value <= 0xffffffffULL) {
        out.push_back(static_cast<uint8_t>((major << 5) | 26));
        for (int shift = 24; shift >= 0; shift -= 8) out.push_back(static_cast<uint8_t>(value >> shift));
    } else {
        out.push_back(static_cast<uint8_t>((major << 5) | 27));
        for (int shift = 56; shift >= 0; shift -= 8) out.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void uint(bytes & out, uint64_t value) { head(out, 0, value); }
void map(bytes & out, uint64_t count) { head(out, 5, count); }
void array(bytes & out, uint64_t count) { head(out, 4, count); }
void bstr(bytes & out, const uint8_t * data, size_t size) {
    head(out, 2, size); out.insert(out.end(), data, data + size);
}
void fill(bytes & out, size_t size, uint8_t value) {
    head(out, 2, size); out.insert(out.end(), size, value);
}
void text(bytes & out, const std::string & value) {
    head(out, 3, value.size()); out.insert(out.end(), value.begin(), value.end());
}
void digest(bytes & out, const context_store_format_digest & value) {
    bstr(out, value.data(), value.size());
}

bytes domain(const char * name, const bytes & body) {
    bytes out(reinterpret_cast<const uint8_t *>(name),
        reinterpret_cast<const uint8_t *>(name) + std::strlen(name) + 1);
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

context_store_format_digest sha(const bytes & input) {
    context_store_format_digest value {};
    assert(halofpx::context_store_sha256(input.data(), input.size(), value));
    return value;
}

context_store_format_digest hmac(const bytes & key, const bytes & input) {
    context_store_format_digest value {};
    assert(halofpx::context_store_hmac_sha256(
        key.data(), key.size(), input.data(), input.size(), value));
    return value;
}

std::string hex(const context_store_format_digest & value) {
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string output(value.size() * 2, '0');
    for (size_t i = 0; i < value.size(); ++i) {
        output[i * 2] = alphabet[value[i] >> 4];
        output[i * 2 + 1] = alphabet[value[i] & 0x0f];
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

std::string registered_id_string(const halofpx::context_store_registered_id & value) {
    return std::string(value.bytes.begin(), value.bytes.begin() + value.size);
}

void append_u16(bytes & out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value >> 8)); out.push_back(static_cast<uint8_t>(value));
}

void append_u64(bytes & out, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) out.push_back(static_cast<uint8_t>(value >> shift));
}

bytes make_frame(const std::string & type, const bytes & payload) {
    bytes frame = { 0x48,0x41,0x4c,0x4f,0x4f,0x42,0x4a,0x01 };
    const std::string frame_domain = "halofpx.object.v1";
    append_u16(frame, static_cast<uint16_t>(frame_domain.size()));
    frame.insert(frame.end(), frame_domain.begin(), frame_domain.end());
    append_u16(frame, static_cast<uint16_t>(type.size()));
    frame.insert(frame.end(), type.begin(), type.end());
    append_u64(frame, payload.size());
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

struct fixture_options {
    std::string profile = "profile.synthetic.full-v1";
    std::array<std::string, 2> codecs = { "codec.tokens.v1", "codec.kv.v1" };
    uint64_t policy_epoch = 7;
};

struct signed_two_object_fixture {
    bytes manifest;
    bytes master_key;
    halofpx::context_store_manifest_verification_policy policy;
    std::array<bytes, 2> frames;
    std::array<halofpx::context_store_v1_frame_view, 2> frame_views {};
    halofpx::context_store_authenticated_manifest_metadata admission_metadata;
    std::array<halofpx::context_store_object_reference, 2> admission_objects {};

    void refresh() {
        policy.key.master_key = { master_key.data(), master_key.size() };
        for (size_t i = 0; i < frames.size(); ++i) {
            frame_views[i] = { frames[i].data(), frames[i].size() };
        }
    }

    halofpx::context_store_v1_read_only_source source() {
        refresh();
        halofpx::context_store_v1_read_only_source source;
        source.manifest_data = manifest.data();
        source.manifest_size = manifest.size();
        source.verification_policy = policy;
        source.admission.manifest = admission_metadata;
        source.admission.objects = admission_objects.data();
        source.admission.object_count = admission_objects.size();
        source.frames = frame_views.data();
        source.frame_count = frame_views.size();
        source.object_limits = { 4096, 1024 };
        source.max_total_frame_bytes = 8192;
        return source;
    }
};

signed_two_object_fixture make_fixture(const fixture_options & options = {}) {
    signed_two_object_fixture fixture;
    fixture.frames = {
        make_frame("tokens", { 0x01,0x02,0x03,0x04,0x05 }),
        make_frame("kv", { 0xa0,0xb1,0xc2,0xd3,0xe4,0xf5 })
    };
    const std::array<context_store_format_digest, 2> object_ids = {
        sha(fixture.frames[0]), sha(fixture.frames[1])
    };

    fixture.master_key.assign(32, 0x11);
    for (size_t i = 0; i < fixture.policy.compatibility.components.size(); ++i) {
        fixture.policy.compatibility.components[i].fill(static_cast<uint8_t>(0x20 + i));
    }
    bytes compatibility;
    map(compatibility, 16);
    for (size_t i = 0; i < fixture.policy.compatibility.components.size(); ++i) {
        uint(compatibility, i); digest(compatibility, fixture.policy.compatibility.components[i]);
    }
    fixture.policy.compatibility.root = sha(domain("halofpx.compat.v1", compatibility));

    bytes body;
    map(body, 15);
    uint(body, 0); uint(body, 1); uint(body, 1); uint(body, 0);
    uint(body, 2); fill(body, 16, 0x02); uint(body, 3); fill(body, 32, 0x03);
    uint(body, 4); uint(body, 1); uint(body, 5); body.push_back(0xf6);
    uint(body, 6); body.insert(body.end(), compatibility.begin(), compatibility.end());
    uint(body, 7); digest(body, fixture.policy.compatibility.root);
    uint(body, 8); fill(body, 32, 0x80); uint(body, 9); uint(body, options.policy_epoch);
    uint(body, 10); map(body, 6);
    uint(body, 0); text(body, "plan.synthetic.v1");
    uint(body, 1); text(body, "pipeline");
    uint(body, 2); uint(body, 2); uint(body, 3); fill(body, 32, 0xa3);
    uint(body, 4); uint(body, 9); uint(body, 5); array(body, 2);
    for (uint8_t rank = 0; rank < 2; ++rank) {
        map(body, 3); uint(body, 0); uint(body, rank);
        uint(body, 1); fill(body, 32, static_cast<uint8_t>(0xb0 + rank));
        uint(body, 2); fill(body, 32, static_cast<uint8_t>(0xc0 + rank));
    }
    uint(body, 11); text(body, options.profile); uint(body, 12); array(body, 2);
    const std::array<std::string, 2> streams = { "tokens", "kv" };
    for (uint8_t index = 0; index < 2; ++index) {
        map(body, 13);
        uint(body, 0); digest(body, object_ids[index]);
        uint(body, 1); text(body, streams[index]);
        uint(body, 2); text(body, options.codecs[index]);
        uint(body, 3); uint(body, 1); uint(body, 4); uint(body, index);
        uint(body, 5); body.push_back(0xf5); uint(body, 6); uint(body, fixture.frames[index].size());
        uint(body, 7); fill(body, 32, static_cast<uint8_t>(0xd7 + index));
        uint(body, 8); uint(body, 16 + index); uint(body, 9); uint(body, 8 + index);
        uint(body, 10); uint(body, index);
        uint(body, 11); fill(body, 32, static_cast<uint8_t>(0xb0 + index));
        uint(body, 12); digest(body, fixture.policy.compatibility.root);
    }
    uint(body, 13); fill(body, 32, 0xe3); uint(body, 14); uint(body, 0);

    const std::string key_id = "test-key-v1";
    bytes auth_input;
    map(auth_input, 4); uint(auth_input, 0); auth_input.insert(auth_input.end(), body.begin(), body.end());
    uint(auth_input, 1); text(auth_input, key_id); uint(auth_input, 2); uint(auth_input, 1);
    uint(auth_input, 3); uint(auth_input, 1);

    bytes kdf = domain("halofpx.manifest-key.v1", {});
    kdf.push_back(0); kdf.push_back(static_cast<uint8_t>(key_id.size()));
    kdf.insert(kdf.end(), key_id.begin(), key_id.end());
    kdf.insert(kdf.end(), 16, 0x02); kdf.insert(kdf.end(), 32, 0x80);
    kdf.insert(kdf.end(), 7, 0); kdf.push_back(1);
    const auto derived_key = hmac(fixture.master_key, kdf);
    const auto tag = hmac(bytes(derived_key.begin(), derived_key.end()),
        domain("halofpx.manifest-auth.v1", auth_input));

    map(fixture.manifest, 2); uint(fixture.manifest, 0);
    fixture.manifest.insert(fixture.manifest.end(), auth_input.begin(), auth_input.end());
    uint(fixture.manifest, 1); digest(fixture.manifest, tag);

    fixture.policy.key.disposition = halofpx::context_store_key_disposition::active;
    fixture.policy.key.key_id = registered_id(key_id); fixture.policy.key.generation = 1;
    fixture.policy.anchor.store_uuid.fill(0x02);
    fixture.policy.anchor.checkpoint_lineage_id.fill(0x03);
    fixture.policy.anchor.namespace_id.fill(0x80);
    fixture.policy.anchor.policy_epoch = options.policy_epoch;
    fixture.policy.anchor.key_generation = 1; fixture.policy.anchor.generation = 1;
    fixture.policy.anchor.selected_manifest_digest = sha(domain("halofpx.manifest.v1", fixture.manifest));
    fixture.refresh();

    const auto verified = halofpx::context_store_verify_manifest_v1(
        fixture.manifest.data(), fixture.manifest.size(), fixture.policy);
    assert(verified.status == halofpx::context_store_manifest_verify_status::authenticated_unadmitted);
    assert(verified.authenticated_object_count() == 2);
    assert(verified.authenticated_manifest_metadata() != nullptr);
    fixture.admission_metadata = *verified.authenticated_manifest_metadata();
    fixture.admission_objects[0] = *verified.authenticated_object_reference(0);
    fixture.admission_objects[1] = *verified.authenticated_object_reference(1);
    return fixture;
}

halofpx::context_store_lookup_request request_for(const signed_two_object_fixture & fixture) {
    halofpx::context_store_lookup_request request;
    request.identity.compatibility_root = fixture.admission_metadata.compatibility_root;
    request.identity.scope_namespace = fixture.admission_metadata.scope_namespace;
    request.identity.checkpoint_lineage_id = fixture.admission_metadata.checkpoint_lineage_id;
    request.identity.policy_epoch = fixture.admission_metadata.policy_epoch;
    return request;
}

std::unique_ptr<halofpx::context_store_provider> make_provider(signed_two_object_fixture & fixture) {
    return halofpx::make_context_store_v1_read_only_provider(fixture.source());
}

void test_golden_candidate_and_closed_capabilities() {
    auto fixture = make_fixture();
    auto provider = make_provider(fixture);
    assert(std::string(provider->name()) == "v1-read-only-synthetic");
    const auto capabilities = provider->capabilities();
    assert(!capabilities.persistent_reads && !capabilities.persistent_writes);
    assert(!capabilities.enumeration && !capabilities.anonymous_scope && !capabilities.shared_scope);
    assert(capabilities.admitted_state_profiles == 0 && capabilities.admitted_codecs == 0);
    assert(provider->publish({}) == halofpx::context_store_publish_status::disabled);

    auto result = provider->lookup(request_for(fixture));
    assert(result.is_hit() && result.status() == halofpx::context_store_lookup_status::hit);
    const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(result.candidate());
    assert(candidate != nullptr && candidate->object_count() == 2);
    assert(candidate->identity().compatibility_root == fixture.admission_metadata.compatibility_root);
    assert(candidate->manifest_digest() == fixture.policy.anchor.selected_manifest_digest);
    assert(candidate->generation() == 1 && candidate->world_size() == 2 && candidate->topology_epoch() == 9);
    assert(registered_id_string(candidate->state_profile_id()) == "profile.synthetic.full-v1");
    assert(candidate->producer_identity() == fixture.admission_metadata.producer_identity);
    assert(candidate->durability_mode() == 0);
    assert(candidate->descriptor(2) == nullptr && candidate->payload(2).data == nullptr);
    for (size_t i = 0; i < 2; ++i) {
        const auto * descriptor = candidate->descriptor(i);
        const auto payload = candidate->payload(i);
        assert(descriptor != nullptr);
        assert(descriptor->object_id == fixture.admission_objects[i].object_id);
        assert(descriptor->logical_rank == i && descriptor->has_logical_rank);
        assert(payload.size == (i == 0 ? 5 : 6));
        assert(std::equal(payload.data, payload.data + payload.size,
            fixture.frames[i].end() - static_cast<std::ptrdiff_t>(payload.size)));
    }

    const auto manifest_hash = hex(candidate->manifest_digest());
    const auto object0_hash = hex(candidate->descriptor(0)->object_id);
    const auto object1_hash = hex(candidate->descriptor(1)->object_id);
    std::cout << "manifest_sha256=" << manifest_hash << "\n"
              << "object0_sha256=" << object0_hash << "\n"
              << "object1_sha256=" << object1_hash << "\n";
    // Filled from this deterministic target-owned fixture after first execution.
    assert(manifest_hash == "f724dd615d4a5866a655422682d0781eebd0085a25edd587b169f6b8461e4e4b");
    assert(object0_hash == "c6a416f1d11eb0e35065f5ddf8912d9270f45e2cd96ba8fa416fd43f1565337c");
    assert(object1_hash == "e1aa2922082eb9433a3907c131fc4fabd26455da2d48189040c25e0da41fa214");
}

void test_identity_misses() {
    auto fixture = make_fixture();
    auto provider = make_provider(fixture);
    const auto base = request_for(fixture);
    for (size_t field = 0; field < 4; ++field) {
        auto request = base;
        if (field == 0) request.identity.scope_namespace[0] ^= 1;
        if (field == 1) request.identity.compatibility_root[0] ^= 1;
        if (field == 2) request.identity.checkpoint_lineage_id[0] ^= 1;
        if (field == 3) ++request.identity.policy_epoch;
        auto result = provider->lookup(request);
        assert(!result.is_hit() && result.candidate() == nullptr);
        const auto expected = field == 0 ? halofpx::context_store_lookup_status::miss_unauthorized :
            field == 1 ? halofpx::context_store_lookup_status::miss_incompatible :
                         halofpx::context_store_lookup_status::miss_replay;
        assert(result.status() == expected);
    }
}

void test_auth_key_replay_and_closed_admission() {
    auto golden = make_fixture();
    const auto request = request_for(golden);

    auto wrong_key = make_fixture();
    wrong_key.master_key[0] ^= 1;
    auto wrong_key_provider = make_provider(wrong_key);
    assert(wrong_key_provider->lookup(request).status() == halofpx::context_store_lookup_status::miss_corrupt);

    auto unknown_key = make_fixture();
    auto unknown_source = unknown_key.source();
    unknown_source.verification_policy.key.disposition = halofpx::context_store_key_disposition::unknown;
    auto unknown_provider = halofpx::make_context_store_v1_read_only_provider(unknown_source);
    assert(unknown_provider->lookup(request).status() == halofpx::context_store_lookup_status::miss_unauthorized);

    auto replay = make_fixture();
    auto replay_source = replay.source();
    replay_source.verification_policy.anchor.generation = 2;
    auto replay_provider = halofpx::make_context_store_v1_read_only_provider(replay_source);
    assert(replay_provider->lookup(request).status() == halofpx::context_store_lookup_status::miss_replay);

    fixture_options profile_options;
    profile_options.profile = "profile.synthetic.unadmitted";
    auto profile_variant = make_fixture(profile_options);
    auto profile_source = profile_variant.source();
    profile_source.admission.manifest = golden.admission_metadata;
    profile_source.admission.objects = golden.admission_objects.data();
    auto profile_provider = halofpx::make_context_store_v1_read_only_provider(profile_source);
    assert(profile_provider->lookup(request).status() == halofpx::context_store_lookup_status::miss_unsupported);

    fixture_options codec_options;
    codec_options.codecs[1] = "codec.kv.unadmitted";
    auto codec_variant = make_fixture(codec_options);
    auto codec_source = codec_variant.source();
    codec_source.admission.manifest = golden.admission_metadata;
    codec_source.admission.objects = golden.admission_objects.data();
    auto codec_provider = halofpx::make_context_store_v1_read_only_provider(codec_source);
    assert(codec_provider->lookup(request).status() == halofpx::context_store_lookup_status::miss_unsupported);
}

void test_second_object_corruption_is_atomic_miss() {
    auto fixture = make_fixture();
    auto provider_source = fixture.source();
    auto corrupt_frame = fixture.frames[1];
    corrupt_frame.back() ^= 1;
    auto corrupt_views = fixture.frame_views;
    corrupt_views[1] = { corrupt_frame.data(), corrupt_frame.size() };
    provider_source.frames = corrupt_views.data();
    auto provider = halofpx::make_context_store_v1_read_only_provider(provider_source);
    auto result = provider->lookup(request_for(fixture));
    assert(result.status() == halofpx::context_store_lookup_status::miss_corrupt);
    assert(!result.is_hit() && result.candidate() == nullptr);
}

void test_owned_source_lifetime_and_concurrency() {
    auto fixture = make_fixture();
    const auto request = request_for(fixture);
    auto provider = make_provider(fixture);
    std::fill(fixture.manifest.begin(), fixture.manifest.end(), 0);
    std::fill(fixture.master_key.begin(), fixture.master_key.end(), 0);
    for (auto & frame : fixture.frames) std::fill(frame.begin(), frame.end(), 0);
    fixture.admission_metadata = {};
    fixture.admission_objects = {};
    assert(provider->lookup(request).is_hit());

    std::array<std::thread, 4> workers;
    std::array<bool, 4> passed {};
    for (size_t worker = 0; worker < workers.size(); ++worker) {
        workers[worker] = std::thread([&, worker] {
            for (size_t iteration = 0; iteration < 32; ++iteration) {
                auto result = provider->lookup(request);
                if (!result.is_hit()) return;
                const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(result.candidate());
                if (candidate == nullptr || candidate->object_count() != 2 || candidate->payload(1).size != 6) return;
            }
            passed[worker] = true;
        });
    }
    for (auto & worker : workers) worker.join();
    assert(std::all_of(passed.begin(), passed.end(), [](bool value) { return value; }));
}

} // namespace

int main() {
    test_golden_candidate_and_closed_capabilities();
    test_identity_misses();
    test_auth_key_replay_and_closed_admission();
    test_second_object_corruption_is_atomic_miss();
    test_owned_source_lifetime_and_concurrency();
    std::cout << "halofpx full-v1 memory-only provider tests passed\n";
    return 0;
}
