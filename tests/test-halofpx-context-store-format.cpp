#include "halofpx-context-store-format.h"

#include <atomic>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace {

using bytes = std::vector<uint8_t>;

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
        for (int shift = 24; shift >= 0; shift -= 8) {
            output.push_back(static_cast<uint8_t>(value >> shift));
        }
    } else {
        output.push_back(static_cast<uint8_t>((major << 5) | 27));
        for (int shift = 56; shift >= 0; shift -= 8) {
            output.push_back(static_cast<uint8_t>(value >> shift));
        }
    }
}

void append_uint(bytes & output, uint64_t value) {
    append_head(output, 0, value);
}

void append_map(bytes & output, uint64_t pairs) {
    append_head(output, 5, pairs);
}

void append_array(bytes & output, uint64_t items) {
    append_head(output, 4, items);
}

void append_bytes(bytes & output, size_t length, uint8_t value) {
    append_head(output, 2, length);
    output.insert(output.end(), length, value);
}

void append_text(bytes & output, const std::string & value) {
    append_head(output, 3, value.size());
    output.insert(output.end(), value.begin(), value.end());
}

struct fixture_options {
    uint64_t major = 1;
    uint64_t minor = 0;
    uint64_t generation = 1;
    bool predecessor = false;
    bool noncanonical_generation = false;
    uint64_t world_size = 1;
    uint64_t rank_count = 1;
    bool duplicate_rank = false;
    bool reverse_rank_order = false;
    uint64_t object_count = 1;
    bool coordinator_owned = false;
    bool object_rank_out_of_range = false;
    bool object_ownership_mismatch = false;
    bool object_compatibility_mismatch = false;
    bool duplicate_object_id = false;
    bool required = true;
    std::string profile = "profile.synthetic.v1";
    uint64_t durability = 0;
    uint64_t algorithm = 1;
    uint64_t key_generation = 1;
    size_t tag_length = 32;
    bool reverse_outer_keys = false;
};

void append_digest(bytes & output, uint8_t value) {
    append_bytes(output, 32, value);
}

void append_body(bytes & output, const fixture_options & options) {
    append_map(output, 15);
    append_uint(output, 0); append_uint(output, options.major);
    append_uint(output, 1); append_uint(output, options.minor);
    append_uint(output, 2); append_bytes(output, 16, 0x02);
    append_uint(output, 3); append_digest(output, 0x03);
    append_uint(output, 4);
    if (options.noncanonical_generation && options.generation < 24) {
        output.push_back(0x18);
        output.push_back(static_cast<uint8_t>(options.generation));
    } else {
        append_uint(output, options.generation);
    }
    append_uint(output, 5);
    if (options.predecessor) append_digest(output, 0x05); else output.push_back(0xf6);

    append_uint(output, 6);
    append_map(output, 16);
    for (uint64_t field = 0; field < 16; ++field) {
        append_uint(output, field);
        append_digest(output, static_cast<uint8_t>(0x20 + field));
    }
    append_uint(output, 7); append_digest(output, 0x70);
    append_uint(output, 8); append_digest(output, 0x80);
    append_uint(output, 9); append_uint(output, 7);

    append_uint(output, 10);
    append_map(output, 6);
    append_uint(output, 0); append_text(output, "plan.synthetic.v1");
    append_uint(output, 1); append_text(output, "single-or-tensor");
    append_uint(output, 2); append_uint(output, options.world_size);
    append_uint(output, 3); append_digest(output, 0xa3);
    append_uint(output, 4); append_uint(output, 9);
    append_uint(output, 5); append_array(output, options.rank_count);
    for (uint64_t index = 0; index < options.rank_count; ++index) {
        uint64_t rank = index;
        if (options.duplicate_rank && index == 1) rank = 0;
        if (options.reverse_rank_order && options.rank_count == 2) rank = 1 - index;
        append_map(output, 3);
        append_uint(output, 0); append_uint(output, rank);
        append_uint(output, 1); append_digest(output, static_cast<uint8_t>(0xb0 + rank));
        append_uint(output, 2); append_digest(output, static_cast<uint8_t>(0xc0 + rank));
    }

    append_uint(output, 11); append_text(output, options.profile);
    append_uint(output, 12); append_array(output, options.object_count);
    for (uint64_t index = 0; index < options.object_count; ++index) {
        const uint8_t object_id = options.duplicate_object_id ? 0xd0 : static_cast<uint8_t>(0xd0 + index);
        append_map(output, 13);
        append_uint(output, 0); append_digest(output, object_id);
        append_uint(output, 1); append_text(output, "tokens");
        append_uint(output, 2); append_text(output, "codec.synthetic.v1");
        append_uint(output, 3); append_uint(output, 1);
        append_uint(output, 4); append_uint(output, 0);
        append_uint(output, 5); output.push_back(options.required ? 0xf5 : 0xf4);
        append_uint(output, 6); append_uint(output, 64);
        append_uint(output, 7); append_digest(output, 0xd7);
        append_uint(output, 8); append_uint(output, 16);
        append_uint(output, 9); append_uint(output, 8);
        append_uint(output, 10);
        if (options.coordinator_owned) {
            output.push_back(0xf6);
        } else {
            append_uint(output, options.object_rank_out_of_range ? options.world_size : index % options.world_size);
        }
        append_uint(output, 11);
        const uint64_t owner_rank = options.world_size == 0 ? 0 : index % options.world_size;
        append_digest(output, options.object_ownership_mismatch ? 0xee : static_cast<uint8_t>(0xb0 + owner_rank));
        append_uint(output, 12); append_digest(output, options.object_compatibility_mismatch ? 0xef : 0x70);
    }

    append_uint(output, 13); append_digest(output, 0xe3);
    append_uint(output, 14); append_uint(output, options.durability);
}

bytes make_fixture(const fixture_options & options = {}) {
    bytes auth_input;
    append_map(auth_input, 4);
    append_uint(auth_input, 0); append_body(auth_input, options);
    append_uint(auth_input, 1); append_text(auth_input, "test-key-v1");
    append_uint(auth_input, 2); append_uint(auth_input, options.algorithm);
    append_uint(auth_input, 3); append_uint(auth_input, options.key_generation);

    bytes output;
    append_map(output, 2);
    if (options.reverse_outer_keys) {
        append_uint(output, 1); append_bytes(output, options.tag_length, 0xf1);
        append_uint(output, 0); output.insert(output.end(), auth_input.begin(), auth_input.end());
    } else {
        append_uint(output, 0); output.insert(output.end(), auth_input.begin(), auth_input.end());
        append_uint(output, 1); append_bytes(output, options.tag_length, 0xf1);
    }
    return output;
}

void require_rejected(const bytes & fixture) {
    const auto result = halofpx::context_store_parse_manifest_v1(fixture.data(), fixture.size());
    assert(result.status != halofpx::context_store_manifest_parse_status::structural_only);
}

} // namespace

int main() {
    const bytes valid = make_fixture();
    const bytes valid_before = valid;
    const auto parsed = halofpx::context_store_parse_manifest_v1(valid.data(), valid.size());
    assert(parsed.status == halofpx::context_store_manifest_parse_status::structural_only);
    assert(valid == valid_before);
    assert(parsed.manifest.generation == 1);
    assert(!parsed.manifest.has_predecessor);
    assert(parsed.manifest.world_size == 1);
    assert(parsed.manifest.rank_count == 1);
    assert(parsed.manifest.object_count == 1);
    assert(parsed.manifest.authentication_key_generation == 1);
    assert(parsed.manifest.authentication_input_size > 0);
    assert(parsed.manifest.authentication_input_offset < valid.size());
    assert(parsed.manifest.state_profile_id.size == std::string("profile.synthetic.v1").size());
    assert(std::string(parsed.manifest.topology_plan_schema_id.bytes.data(),
                       parsed.manifest.topology_plan_schema_id.size) == "plan.synthetic.v1");
    assert(std::string(parsed.manifest.topology_execution_mode.bytes.data(),
                       parsed.manifest.topology_execution_mode.size) == "single-or-tensor");
    assert(parsed.manifest.global_plan_digest[0] == 0xa3);
    assert(parsed.manifest.topology_epoch == 9);
    assert(parsed.manifest.rank_ownership[0][0] == 0xb0);
    assert(parsed.manifest.rank_placements[0][0] == 0xc0);
    assert(parsed.manifest.producer_identity[0] == 0xe3);
    assert(parsed.manifest.durability_mode == 0);

    const auto & reference = parsed.manifest.object_references[0];
    assert(reference.object_id[0] == 0xd0);
    assert(std::string(reference.stream_type.bytes.data(), reference.stream_type.size) == "tokens");
    assert(std::string(reference.codec_id.bytes.data(), reference.codec_id.size) == "codec.synthetic.v1");
    assert(reference.codec_schema_major == 1 && reference.codec_schema_minor == 0);
    assert(reference.required);
    assert(reference.frame_bytes == 64);
    assert(reference.token_sequence_digest[0] == 0xd7);
    assert(reference.logical_position == 16);
    assert(reference.output_boundary == 8);
    assert(reference.has_logical_rank && reference.logical_rank == 0);
    assert(reference.rank_ownership_digest[0] == 0xb0);
    assert(reference.compatibility_root[0] == 0x70);

    assert(halofpx::context_store_parse_manifest_v1(nullptr, 0).status ==
           halofpx::context_store_manifest_parse_status::input_empty);

    for (size_t length = 0; length < valid.size(); ++length) {
        const auto truncated = halofpx::context_store_parse_manifest_v1(valid.data(), length);
        assert(truncated.status != halofpx::context_store_manifest_parse_status::structural_only);
    }

    bytes trailing = valid;
    trailing.push_back(0x00);
    assert(halofpx::context_store_parse_manifest_v1(trailing.data(), trailing.size()).status ==
           halofpx::context_store_manifest_parse_status::trailing_data);

    bytes oversized(halofpx::context_store_manifest_max_bytes + 1, 0);
    assert(halofpx::context_store_parse_manifest_v1(oversized.data(), oversized.size()).status ==
           halofpx::context_store_manifest_parse_status::manifest_too_large);

    require_rejected(bytes { 0xbf });
    require_rejected(bytes { 0x82 });

    fixture_options options;
    options.noncanonical_generation = true;
    require_rejected(make_fixture(options));

    options = {}; options.major = 2; require_rejected(make_fixture(options));
    options = {}; options.minor = 1; require_rejected(make_fixture(options));
    options = {}; options.generation = 0; require_rejected(make_fixture(options));
    options = {}; options.generation = 1; options.predecessor = true; require_rejected(make_fixture(options));
    options = {}; options.generation = 2; options.predecessor = false; require_rejected(make_fixture(options));
    options = {}; options.world_size = 2; options.rank_count = 1; require_rejected(make_fixture(options));
    options = {}; options.world_size = 2; options.rank_count = 2; options.duplicate_rank = true; require_rejected(make_fixture(options));
    options = {}; options.world_size = 2; options.rank_count = 2; options.reverse_rank_order = true; require_rejected(make_fixture(options));
    options = {}; options.object_count = 0; require_rejected(make_fixture(options));
    options = {}; options.object_count = 2; options.duplicate_object_id = true; require_rejected(make_fixture(options));
    options = {}; options.required = false; require_rejected(make_fixture(options));
    options = {}; options.object_rank_out_of_range = true; require_rejected(make_fixture(options));
    options = {}; options.object_ownership_mismatch = true; require_rejected(make_fixture(options));
    options = {}; options.object_compatibility_mismatch = true; require_rejected(make_fixture(options));
    options = {}; options.profile = ""; require_rejected(make_fixture(options));
    options = {}; options.profile = std::string(129, 'a'); require_rejected(make_fixture(options));
    options = {}; options.profile = std::string("bad\0id", 6); require_rejected(make_fixture(options));
    options = {}; options.profile = "profile.\xc3\xb8.\xe5\x90\x88";
    {
        const auto utf8 = make_fixture(options);
        assert(halofpx::context_store_parse_manifest_v1(utf8.data(), utf8.size()).status ==
               halofpx::context_store_manifest_parse_status::structural_only);
    }
    options = {}; options.profile = std::string("\xc0\x80", 2); require_rejected(make_fixture(options));
    options = {}; options.profile = std::string("\xed\xa0\x80", 3); require_rejected(make_fixture(options));
    options = {}; options.profile = std::string("\xf4\x90\x80\x80", 4); require_rejected(make_fixture(options));
    options = {}; options.profile = std::string("\xe2\x82", 2); require_rejected(make_fixture(options));
    options = {}; options.durability = 3; require_rejected(make_fixture(options));
    options = {}; options.algorithm = 2; require_rejected(make_fixture(options));
    options = {}; options.tag_length = 31; require_rejected(make_fixture(options));
    options = {}; options.reverse_outer_keys = true; require_rejected(make_fixture(options));

    for (uint64_t generation : { 23ULL, 24ULL, 255ULL, 256ULL }) {
        options = {};
        options.generation = generation;
        options.predecessor = true;
        const auto boundary = make_fixture(options);
        const auto result = halofpx::context_store_parse_manifest_v1(boundary.data(), boundary.size());
        assert(result.status == halofpx::context_store_manifest_parse_status::structural_only);
        assert(result.manifest.generation == generation);
    }

    options = {};
    options.world_size = 128;
    options.rank_count = 128;
    options.object_count = 128;
    const auto maximum = make_fixture(options);
    const auto maximum_result = halofpx::context_store_parse_manifest_v1(maximum.data(), maximum.size());
    assert(maximum_result.status == halofpx::context_store_manifest_parse_status::structural_only);
    assert(maximum_result.manifest.rank_count == 128);
    assert(maximum_result.manifest.object_count == 128);

    options = {};
    options.coordinator_owned = true;
    const auto coordinator = make_fixture(options);
    const auto coordinator_result =
        halofpx::context_store_parse_manifest_v1(coordinator.data(), coordinator.size());
    assert(coordinator_result.status == halofpx::context_store_manifest_parse_status::structural_only);
    assert(!coordinator_result.manifest.object_references[0].has_logical_rank);

    std::atomic<bool> concurrent_ok { true };
    std::vector<std::thread> threads;
    for (int thread_id = 0; thread_id < 8; ++thread_id) {
        threads.emplace_back([&valid, &concurrent_ok]() {
            for (int iteration = 0; iteration < 1000; ++iteration) {
                if (halofpx::context_store_parse_manifest_v1(valid.data(), valid.size()).status !=
                    halofpx::context_store_manifest_parse_status::structural_only) {
                    concurrent_ok.store(false);
                    return;
                }
            }
        });
    }
    for (auto & thread : threads) thread.join();
    assert(concurrent_ok.load());

    assert(std::string(halofpx::context_store_manifest_parse_status_name(
               halofpx::context_store_manifest_parse_status::structural_only)) == "structural-only");
    return 0;
}
