#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

#include "halofpx-context-store-v1-transformer-codec.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using bytes = std::vector<uint8_t>;

void head(bytes & out, uint8_t major, uint64_t value) {
    if (value < 24) out.push_back(static_cast<uint8_t>((major << 5) | value));
    else if (value <= 0xff) {
        out.push_back(static_cast<uint8_t>((major << 5) | 24)); out.push_back(static_cast<uint8_t>(value));
    } else if (value <= 0xffff) {
        out.push_back(static_cast<uint8_t>((major << 5) | 25));
        out.push_back(static_cast<uint8_t>(value >> 8)); out.push_back(static_cast<uint8_t>(value));
    } else {
        out.push_back(static_cast<uint8_t>((major << 5) | 26));
        for (int shift = 24; shift >= 0; shift -= 8) out.push_back(static_cast<uint8_t>(value >> shift));
    }
}

halofpx::context_store_registered_id id(std::string_view text) {
    halofpx::context_store_registered_id out;
    out.size = static_cast<uint8_t>(text.size());
    std::copy(text.begin(), text.end(), out.bytes.begin());
    return out;
}

halofpx::context_store_transformer_profile_v1 profile() {
    halofpx::context_store_transformer_profile_v1 out;
    out.target_only = true;
    out.world_size = 1;
    out.rank = 0;
    out.architecture = halofpx::context_store_transformer_architecture_v1::transformer;
    out.greedy_memoryless_sampling = true;
    return out;
}

struct fixture {
    std::vector<uint8_t> key = std::vector<uint8_t>(32, 0x71);
    halofpx::context_store_transformer_snapshot_v1 snapshot;
    halofpx::context_store_v1_transformer_manifest_parameters parameters;
    halofpx::context_store_v1_transformer_codec_limits limits { { 64, 16 }, 4096, 65536 };

    fixture() {
        for (size_t i = 0; i < parameters.compatibility_components.size(); ++i) {
            parameters.compatibility_components[i].fill(static_cast<uint8_t>(0x20 + i));
        }
        bytes compatibility;
        head(compatibility, 5, 16);
        for (size_t i = 0; i < parameters.compatibility_components.size(); ++i) {
            head(compatibility, 0, i); head(compatibility, 2, 32);
            compatibility.insert(compatibility.end(),
                parameters.compatibility_components[i].begin(),
                parameters.compatibility_components[i].end());
        }
        const char domain[] = "halofpx.compat.v1";
        bytes preimage(reinterpret_cast<const uint8_t *>(domain),
            reinterpret_cast<const uint8_t *>(domain) + sizeof(domain));
        preimage.insert(preimage.end(), compatibility.begin(), compatibility.end());
        assert(halofpx::context_store_sha256(preimage.data(), preimage.size(),
            snapshot.compatibility_identity.compatibility_root));
        snapshot.compatibility_identity.scope_namespace.fill(0x81);
        snapshot.compatibility_identity.checkpoint_lineage_id.fill(0x82);
        snapshot.compatibility_identity.policy_epoch = 7;
        snapshot.profile = profile();
        snapshot.tokens = { -1, 0, 42, INT32_MAX };
        snapshot.state = { 0xa0, 0xb1, 0xc2, 0xd3, 0xe4 };

        parameters.store_uuid.fill(0x11);
        parameters.producer_identity.fill(0x31);
        parameters.global_plan_digest.fill(0x32);
        parameters.rank_ownership_digest.fill(0x33);
        parameters.rank_placement_digest.fill(0x34);
        parameters.generation = 1;
        parameters.topology_epoch = 9;
        parameters.logical_position = snapshot.tokens.size();
        parameters.output_boundary = 3;
        parameters.durability_mode = 0;
        parameters.signing_key.disposition = halofpx::context_store_key_disposition::active;
        parameters.signing_key.key_id = id("codec-test-key");
        parameters.signing_key.generation = 1;
        parameters.signing_key.master_key = { key.data(), key.size() };
    }
};

halofpx::context_store_manifest_verification_policy policy_for(
        fixture & value,
        const halofpx::context_store_v1_transformer_encoded_snapshot & encoded) {
    halofpx::context_store_manifest_verification_policy policy;
    policy.key = value.parameters.signing_key;
    policy.anchor.store_uuid = value.parameters.store_uuid;
    policy.anchor.checkpoint_lineage_id = value.snapshot.compatibility_identity.checkpoint_lineage_id;
    policy.anchor.namespace_id = value.snapshot.compatibility_identity.scope_namespace;
    policy.anchor.policy_epoch = value.snapshot.compatibility_identity.policy_epoch;
    policy.anchor.key_generation = value.parameters.signing_key.generation;
    policy.anchor.generation = value.parameters.generation;
    policy.anchor.selected_manifest_digest = encoded.manifest_digest;
    policy.compatibility.components = value.parameters.compatibility_components;
    policy.compatibility.root = value.snapshot.compatibility_identity.compatibility_root;
    return policy;
}

struct provider_inputs {
    std::array<halofpx::context_store_v1_frame_view, 2> views {};
    halofpx::context_store_v1_read_only_source source;
};

provider_inputs source_for(
        fixture & value,
        halofpx::context_store_v1_transformer_encoded_snapshot & encoded) {
    provider_inputs out;
    for (size_t i = 0; i < out.views.size(); ++i) {
        out.views[i] = { encoded.frames[i].data(), encoded.frames[i].size() };
    }
    out.source.manifest_data = encoded.manifest.data();
    out.source.manifest_size = encoded.manifest.size();
    out.source.verification_policy = policy_for(value, encoded);
    out.source.admission.manifest = encoded.admission_metadata;
    out.source.admission.objects = encoded.admission_objects.data();
    out.source.admission.object_count = encoded.admission_objects.size();
    out.source.frames = out.views.data();
    out.source.frame_count = out.views.size();
    out.source.object_limits = { value.limits.max_frame_bytes, value.limits.max_frame_bytes };
    out.source.max_total_frame_bytes = value.limits.max_frame_bytes * 2;
    return out;
}

halofpx::context_store_lookup_request lookup_request(const fixture & value) {
    halofpx::context_store_lookup_request request;
    request.identity = value.snapshot.compatibility_identity;
    return request;
}

halofpx::context_store_v1_transformer_decode_request decode_request(
        const fixture & value,
        const halofpx::context_store_v1_read_only_candidate * candidate) {
    halofpx::context_store_v1_transformer_decode_request request;
    request.candidate = candidate;
    request.expected_tokens = value.snapshot.tokens.data();
    request.expected_token_count = value.snapshot.tokens.size();
    request.compatibility_identity = value.snapshot.compatibility_identity;
    request.profile = value.snapshot.profile;
    request.producer_identity = value.parameters.producer_identity;
    request.rank_ownership_digest = value.parameters.rank_ownership_digest;
    request.topology_epoch = value.parameters.topology_epoch;
    request.logical_position = value.parameters.logical_position;
    request.output_boundary = value.parameters.output_boundary;
    request.limits = value.limits.snapshot;
    return request;
}

void test_round_trip() {
    fixture value;
    auto encoded = halofpx::context_store_encode_transformer_snapshot_v1(
        value.snapshot, value.parameters, value.limits);
    assert(encoded.status == halofpx::context_store_v1_transformer_codec_status::encoded);
    assert(encoded.encoded.frames.size() == 2 && !encoded.encoded.manifest.empty());

    auto inputs = source_for(value, encoded.encoded);
    auto provider = halofpx::make_context_store_v1_read_only_provider(inputs.source);
    auto lookup = provider->lookup(lookup_request(value));
    assert(lookup.is_hit());
    const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(
        lookup.candidate());
    assert(candidate != nullptr);
    auto decoded = halofpx::context_store_decode_transformer_snapshot_v1(
        decode_request(value, candidate));
    assert(decoded.status == halofpx::context_store_v1_transformer_codec_status::decoded);
    assert(decoded.snapshot.tokens == value.snapshot.tokens);
    assert(decoded.snapshot.state == value.snapshot.state);
    assert(decoded.snapshot.compatibility_identity.compatibility_root ==
        value.snapshot.compatibility_identity.compatibility_root);
}

void test_wrong_profile_and_codec_are_provider_misses() {
    fixture value;
    auto encoded = halofpx::context_store_encode_transformer_snapshot_v1(
        value.snapshot, value.parameters, value.limits);
    assert(encoded.status == halofpx::context_store_v1_transformer_codec_status::encoded);

    auto wrong_profile = encoded.encoded;
    wrong_profile.admission_metadata.state_profile_id = id("wrong.profile.v1");
    auto profile_inputs = source_for(value, wrong_profile);
    auto profile_provider = halofpx::make_context_store_v1_read_only_provider(profile_inputs.source);
    assert(profile_provider->lookup(lookup_request(value)).status() ==
        halofpx::context_store_lookup_status::miss_unsupported);

    auto wrong_codec = encoded.encoded;
    wrong_codec.admission_objects[1].codec_id = id("wrong.codec.v1");
    auto codec_inputs = source_for(value, wrong_codec);
    auto codec_provider = halofpx::make_context_store_v1_read_only_provider(codec_inputs.source);
    assert(codec_provider->lookup(lookup_request(value)).status() ==
        halofpx::context_store_lookup_status::miss_unsupported);
}

void test_corrupt_frame_is_atomic_provider_miss() {
    fixture value;
    auto encoded = halofpx::context_store_encode_transformer_snapshot_v1(
        value.snapshot, value.parameters, value.limits);
    assert(encoded.status == halofpx::context_store_v1_transformer_codec_status::encoded);
    encoded.encoded.frames[1].back() ^= 1;
    auto inputs = source_for(value, encoded.encoded);
    auto provider = halofpx::make_context_store_v1_read_only_provider(inputs.source);
    auto lookup = provider->lookup(lookup_request(value));
    assert(!lookup.is_hit() && lookup.candidate() == nullptr);
    assert(lookup.status() == halofpx::context_store_lookup_status::miss_corrupt);
}

void test_bounds_fail_closed() {
    fixture value;
    auto frame_limited = value.limits;
    frame_limited.max_frame_bytes = 32;
    auto frame_rejected = halofpx::context_store_encode_transformer_snapshot_v1(
        value.snapshot, value.parameters, frame_limited);
    assert(frame_rejected.status ==
        halofpx::context_store_v1_transformer_codec_status::limit_exceeded);
    assert(frame_rejected.encoded.manifest.empty() &&
        frame_rejected.encoded.frames[0].empty() &&
        frame_rejected.encoded.frames[1].empty());

    auto token_limited = value.limits;
    token_limited.snapshot.max_tokens = value.snapshot.tokens.size() - 1;
    auto rejected = halofpx::context_store_encode_transformer_snapshot_v1(
        value.snapshot, value.parameters, token_limited);
    assert(rejected.status ==
        halofpx::context_store_v1_transformer_codec_status::token_limit_exceeded);
    assert(rejected.encoded.manifest.empty() && rejected.encoded.frames[0].empty());

    auto encoded = halofpx::context_store_encode_transformer_snapshot_v1(
        value.snapshot, value.parameters, value.limits);
    auto inputs = source_for(value, encoded.encoded);
    auto provider = halofpx::make_context_store_v1_read_only_provider(inputs.source);
    auto lookup = provider->lookup(lookup_request(value));
    const auto * candidate = dynamic_cast<const halofpx::context_store_v1_read_only_candidate *>(
        lookup.candidate());
    auto request = decode_request(value, candidate);
    request.limits.max_state_bytes = value.snapshot.state.size() - 1;
    auto decoded = halofpx::context_store_decode_transformer_snapshot_v1(request);
    assert(decoded.status == halofpx::context_store_v1_transformer_codec_status::state_limit_exceeded);
    assert(decoded.snapshot.tokens.empty() && decoded.snapshot.state.empty());
}

} // namespace

int main() {
    test_round_trip();
    test_wrong_profile_and_codec_are_provider_misses();
    test_corrupt_frame_is_atomic_provider_miss();
    test_bounds_fail_closed();
    std::cout << "HaloFPX full-v1 transformer codec tests passed\n";
    return 0;
}
