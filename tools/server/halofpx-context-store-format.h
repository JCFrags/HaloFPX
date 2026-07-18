#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

constexpr size_t context_store_manifest_max_bytes = 1024 * 1024;
constexpr size_t context_store_manifest_max_objects = 128;
constexpr size_t context_store_manifest_max_ranks = 128;
constexpr size_t context_store_registered_id_max_bytes = 128;
constexpr size_t context_store_compatibility_component_count = 16;

// Format-local by design: the offline structural parser must not depend on
// the L03 provider seam or expose its candidate/provider types transitively.
using context_store_format_digest = std::array<uint8_t, 32>;

enum class context_store_manifest_parse_status : uint8_t {
    structural_only,
    input_empty,
    manifest_too_large,
    truncated,
    noncanonical,
    wrong_type,
    wrong_field,
    wrong_version,
    limit_exceeded,
    invalid_value,
    trailing_data,
    semantic_mismatch,
};

struct context_store_registered_id {
    std::array<char, context_store_registered_id_max_bytes + 1> bytes {};
    uint8_t size = 0;
};

struct context_store_object_reference {
    context_store_format_digest object_id {};
    context_store_registered_id stream_type;
    uint64_t frame_bytes = 0;
};

struct context_store_parsed_manifest {
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest checkpoint_lineage_id {};
    uint64_t generation = 0;
    bool has_predecessor = false;
    context_store_format_digest predecessor_manifest_digest {};
    std::array<context_store_format_digest, context_store_compatibility_component_count> compatibility_components {};
    context_store_format_digest compatibility_root {};
    size_t compatibility_manifest_offset = 0;
    size_t compatibility_manifest_size = 0;
    context_store_format_digest scope_namespace {};
    uint64_t policy_epoch = 0;
    uint64_t world_size = 0;
    uint64_t topology_epoch = 0;
    size_t rank_count = 0;
    std::array<context_store_format_digest, context_store_manifest_max_ranks> rank_ownership {};
    size_t object_count = 0;
    std::array<context_store_object_reference, context_store_manifest_max_objects> object_references {};
    context_store_registered_id state_profile_id;
    context_store_format_digest producer_identity {};
    uint8_t durability_mode = 0;
    context_store_registered_id authentication_key_id;
    uint64_t authentication_key_generation = 0;
    context_store_format_digest authentication_tag {};
    size_t authentication_input_offset = 0;
    size_t authentication_input_size = 0;
};

struct context_store_manifest_parse_result {
    context_store_manifest_parse_status status = context_store_manifest_parse_status::input_empty;
    // Bounded cursor position at which rejection was detected. This is a
    // diagnostic only and is not guaranteed to identify the first bad byte.
    size_t error_offset = 0;
    context_store_parsed_manifest manifest;
};

// Parses only the closed authenticated-manifest-v1 structure from memory.
// structural_only means syntax, canonical encoding, bounds, and local semantic
// checks passed. It remains untrusted and unadmitted: it does not authenticate,
// authorize, hash compatibility
// submanifests/objects, decode payloads, admit a codec, or create a cache hit.
context_store_manifest_parse_result context_store_parse_manifest_v1(
    const uint8_t * data,
    size_t size) noexcept;

const char * context_store_manifest_parse_status_name(
    context_store_manifest_parse_status status) noexcept;

} // namespace halofpx
