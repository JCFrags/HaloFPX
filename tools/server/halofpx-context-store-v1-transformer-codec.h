#pragma once

#include "halofpx-context-store-state-transformer-v1.h"
#include "halofpx-context-store-v1-read-only.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace halofpx {

constexpr size_t context_store_v1_transformer_frame_count = 2;

struct context_store_v1_transformer_manifest_parameters {
    std::array<uint8_t, 16> store_uuid {};
    std::array<context_store_format_digest, context_store_compatibility_component_count>
        compatibility_components {};
    context_store_format_digest producer_identity {};
    context_store_format_digest global_plan_digest {};
    context_store_format_digest rank_ownership_digest {};
    context_store_format_digest rank_placement_digest {};
    uint64_t generation = 0;
    bool has_predecessor = false;
    context_store_format_digest predecessor_manifest_digest {};
    uint64_t topology_epoch = 0;
    uint64_t logical_position = 0;
    uint64_t output_boundary = 0;
    uint8_t durability_mode = 0;
    context_store_manifest_key_record signing_key;
};

struct context_store_v1_transformer_codec_limits {
    context_store_transformer_limits_v1 snapshot;
    uint64_t max_frame_bytes = 0;
    uint64_t max_manifest_bytes = 0;
};

enum class context_store_v1_transformer_codec_status : uint8_t {
    encoded,
    decoded,
    invalid_argument,
    unsupported_profile,
    incomplete_identity,
    incompatible_identity,
    incompatible_runtime,
    wrong_profile,
    wrong_object_roster,
    wrong_descriptor,
    wrong_schema,
    wrong_boundary,
    token_mismatch,
    token_limit_exceeded,
    state_limit_exceeded,
    corrupt_payload,
    authentication_failed,
    limit_exceeded,
    allocation_failed,
    internal_error,
};

struct context_store_v1_transformer_encoded_snapshot {
    std::vector<uint8_t> manifest;
    std::array<std::vector<uint8_t>, context_store_v1_transformer_frame_count> frames;
    context_store_format_digest manifest_digest {};
    context_store_authenticated_manifest_metadata admission_metadata {};
    std::array<context_store_object_reference, context_store_v1_transformer_frame_count>
        admission_objects {};
};

struct context_store_v1_transformer_encode_result {
    context_store_v1_transformer_codec_status status =
        context_store_v1_transformer_codec_status::invalid_argument;
    context_store_v1_transformer_encoded_snapshot encoded;
};

struct context_store_v1_transformer_decode_request {
    const context_store_v1_read_only_candidate * candidate = nullptr;
    const llama_token * expected_tokens = nullptr;
    size_t expected_token_count = 0;
    context_store_identity compatibility_identity {};
    context_store_transformer_profile_v1 profile {};
    context_store_format_digest producer_identity {};
    context_store_format_digest rank_ownership_digest {};
    uint64_t topology_epoch = 0;
    uint64_t logical_position = 0;
    uint64_t output_boundary = 0;
    context_store_transformer_limits_v1 limits {};
};

struct context_store_v1_transformer_decode_result {
    context_store_v1_transformer_codec_status status =
        context_store_v1_transformer_codec_status::invalid_argument;
    context_store_transformer_snapshot_v1 snapshot;
};

struct context_store_v1_transformer_manifest_inspection_request {
    const context_store_authenticated_manifest_metadata * metadata = nullptr;
    const context_store_object_reference * objects = nullptr;
    size_t object_count = 0;
    context_store_identity compatibility_identity {};
    context_store_transformer_profile_v1 profile {};
    std::array<uint8_t, 16> store_uuid {};
    context_store_format_digest producer_identity {};
    context_store_format_digest global_plan_digest {};
    context_store_format_digest rank_ownership_digest {};
    context_store_format_digest rank_placement_digest {};
    uint64_t topology_epoch = 0;
    context_store_v1_transformer_codec_limits limits {};
};

struct context_store_v1_transformer_manifest_inspection_result {
    context_store_v1_transformer_codec_status status =
        context_store_v1_transformer_codec_status::invalid_argument;
    size_t token_count = 0;
};

// Produces only target-owned canonical full-v1 bytes. The signing key is
// borrowed for this call and is never retained in the result.
context_store_v1_transformer_encode_result context_store_encode_transformer_snapshot_v1(
    const context_store_transformer_snapshot_v1 & snapshot,
    const context_store_v1_transformer_manifest_parameters & parameters,
    const context_store_v1_transformer_codec_limits & limits) noexcept;

// Decodes only a candidate already authenticated and frame-verified by the
// excluded full-v1 reader. Failure returns an empty snapshot and never mutates
// a llama_context.
context_store_v1_transformer_decode_result context_store_decode_transformer_snapshot_v1(
    const context_store_v1_transformer_decode_request & request) noexcept;

// Admits only the exact ordinary-transformer world1/rank0 generation-one
// manifest shape and derives its canonical token count without loading object
// payloads.  Inputs must already be authenticated by the manifest verifier.
context_store_v1_transformer_manifest_inspection_result
context_store_inspect_transformer_manifest_v1(
    const context_store_v1_transformer_manifest_inspection_request & request) noexcept;

const char * context_store_v1_transformer_codec_status_name(
    context_store_v1_transformer_codec_status status) noexcept;

} // namespace halofpx
