#pragma once

#include "halofpx-context-store-compatibility-v1.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace halofpx {

// Standalone issue-#33 authority slice. All views are borrowed only for the
// synchronous build call. A successful result owns only immutable digests.
constexpr uint32_t context_store_live_authority_world_size_v1 = 2;
constexpr size_t context_store_live_authority_rank_count_v1 = 2;
constexpr size_t context_store_live_authority_max_registered_id_bytes_v1 = 128;
constexpr size_t context_store_live_authority_max_text_bytes_v1 = 16384;
constexpr size_t context_store_live_authority_max_typed_depth_v1 = 16;
constexpr size_t context_store_live_authority_supplement_count_v1 = 10;

struct context_store_live_bytes_v1 {
    const uint8_t * data = nullptr;
    size_t size = 0;
};

struct context_store_live_text_v1 {
    const char * data = nullptr;
    size_t size = 0;
};

struct context_store_live_artifact_v1 {
    context_store_live_text_v1 role;
    uint64_t semantic_order = 0;
    context_store_live_bytes_v1 exact_bytes;
};

enum class context_store_live_value_kind_v1 : uint8_t {
    unsigned_integer,
    negative_integer,
    byte_string,
    text_string,
    boolean,
    float64_bits,
    array,
};

struct context_store_live_typed_value_v1 {
    context_store_live_value_kind_v1 kind =
        context_store_live_value_kind_v1::unsigned_integer;
    uint64_t unsigned_value = 0;
    int64_t negative_value = -1;
    context_store_live_bytes_v1 byte_string;
    context_store_live_text_v1 text_string;
    bool boolean_value = false;
    std::array<uint8_t, 8> float64_bits {};
    const context_store_live_typed_value_v1 * array_values = nullptr;
    size_t array_value_count = 0;
};

struct context_store_live_parameter_v1 {
    context_store_live_text_v1 id;
    context_store_live_typed_value_v1 value;
};

struct context_store_live_metadata_entry_v1 {
    context_store_live_text_v1 key;
    context_store_live_typed_value_v1 value;
};

struct context_store_live_tensor_entry_v1 {
    context_store_live_text_v1 name;
    uint64_t shard_order = 0;
    const uint64_t * dimensions = nullptr;
    size_t dimension_count = 0;
    uint64_t tensor_type = 0;
    uint64_t exact_container_offset = 0;
    uint64_t exact_encoded_bytes = 0;
};

struct context_store_live_model_v1 {
    const context_store_live_artifact_v1 * shards = nullptr;
    size_t shard_count = 0;
    size_t declared_shard_count = 0;
    context_store_live_text_v1 metadata_schema_id;
    const context_store_live_metadata_entry_v1 * metadata = nullptr;
    size_t metadata_count = 0;
    const context_store_live_tensor_entry_v1 * tensors = nullptr;
    size_t tensor_count = 0;
};

struct context_store_live_token_entry_v1 {
    uint64_t token_id = 0;
    context_store_live_bytes_v1 exact_token_bytes;
    std::array<uint8_t, 8> score_bits {};
    uint64_t token_type = 0;
};

struct context_store_live_merge_entry_v1 {
    context_store_live_bytes_v1 left;
    context_store_live_bytes_v1 right;
};

struct context_store_live_special_token_v1 {
    context_store_live_text_v1 role;
    uint64_t token_id = 0;
};

struct context_store_live_tokenizer_v1 {
    const context_store_live_artifact_v1 * artifacts = nullptr;
    size_t artifact_count = 0;
    const context_store_live_token_entry_v1 * tokens = nullptr;
    size_t token_count = 0;
    const context_store_live_merge_entry_v1 * merges = nullptr;
    size_t merge_count = 0;
    const context_store_live_special_token_v1 * special_tokens = nullptr;
    size_t special_token_count = 0;
    const context_store_live_parameter_v1 * policy = nullptr;
    size_t policy_count = 0;
};

struct context_store_live_chat_template_v1 {
    // The adapter must resolve precedence before building authority. Exactly
    // one effective candidate is required, including synthesized templates.
    size_t effective_candidate_count = 0;
    context_store_live_bytes_v1 exact_template_bytes;
    context_store_live_text_v1 renderer_id;
    context_store_live_bytes_v1 renderer_executable_bytes;
};

struct context_store_live_runtime_v1 {
    context_store_live_text_v1 source_revision;
    // Exact canonical source-tree inventory/content bytes, not a caller digest.
    context_store_live_bytes_v1 exact_source_tree_manifest_bytes;
    bool source_is_immutable = false;
    bool source_is_dirty = false;
    // Required when dirty: exact canonical patch and untracked-content bytes.
    context_store_live_bytes_v1 exact_dirty_tree_manifest_bytes;
    const context_store_live_artifact_v1 * artifacts = nullptr;
    size_t artifact_count = 0;
    context_store_live_text_v1 toolchain_id;
    const context_store_live_parameter_v1 * build_options = nullptr;
    size_t build_option_count = 0;
    context_store_live_text_v1 state_abi_id;
};

struct context_store_live_rank_v1 {
    uint32_t logical_rank = 0;
    context_store_live_text_v1 stable_endpoint_id;
    context_store_live_text_v1 stable_device_id;
    context_store_live_bytes_v1 ownership_plan;
    context_store_live_bytes_v1 placement_plan;
};

struct context_store_live_topology_v1 {
    context_store_live_text_v1 plan_schema_id;
    context_store_live_text_v1 execution_mode_id;
    uint32_t world_size = 0;
    context_store_live_bytes_v1 global_plan;
    uint64_t stable_topology_epoch = 0;
    const context_store_live_rank_v1 * ranks = nullptr;
    size_t rank_count = 0;
};

// Exact deterministic-CBOR bytes for the ten component schemas outside this
// bounded slice. They are raw preimages, never component digests. A separate
// trusted closed-CDDL validator must accept the exact indexed bytes.
struct context_store_live_supplement_v1 {
    size_t component_index = 0;
    context_store_live_bytes_v1 exact_dcbor_preimage;
};

struct context_store_live_authority_inputs_v1 {
    context_store_live_model_v1 model;
    context_store_live_tokenizer_v1 tokenizer;
    context_store_live_chat_template_v1 chat_template;
    context_store_live_runtime_v1 runtime;
    context_store_live_topology_v1 topology;
    const context_store_live_supplement_v1 * supplements = nullptr;
    size_t supplement_count = 0;
};

class context_store_live_supplement_validator_v1 {
public:
    virtual ~context_store_live_supplement_validator_v1() = default;

    // Must parse the complete deterministic-CBOR item, enforce the exact
    // component-index CDDL and registered-ID registry, and reject trailing or
    // noncanonical bytes. It is called synchronously and must not mutate input.
    virtual bool validate(
        size_t component_index,
        context_store_live_bytes_v1 exact_dcbor_preimage) const noexcept = 0;
};

enum class context_store_live_authority_status_v1 : uint8_t {
    built,
    invalid_input,
    missing_model_artifact,
    ambiguous_shard_plan,
    incomplete_typed_model_semantics,
    ambiguous_model_semantics,
    incomplete_tokenizer,
    ambiguous_tokenizer,
    ambiguous_template_selection,
    incomplete_template_identity,
    incomplete_build_abi,
    mutable_build_identity,
    incomplete_topology,
    ambiguous_rank_plan,
    missing_component_preimage,
    duplicate_component_preimage,
    invalid_component_preimage,
    unvalidated_component_preimage,
    placeholder_equal_facts,
    canonicalization_failed,
    compatibility_build_failed,
};

struct context_store_live_authority_result_v1 {
    context_store_live_authority_status_v1 status =
        context_store_live_authority_status_v1::invalid_input;
    size_t failure_component = context_store_compatibility_v1_component_count;
    context_store_compatibility_expectation expectation {};
};

context_store_live_authority_result_v1 context_store_build_live_authority_v1(
    const context_store_live_authority_inputs_v1 & inputs,
    const context_store_live_supplement_validator_v1 & validator) noexcept;

const char * context_store_live_authority_status_name_v1(
    context_store_live_authority_status_v1 status) noexcept;

} // namespace halofpx
