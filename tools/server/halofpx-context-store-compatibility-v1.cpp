#include "halofpx-context-store-compatibility-v1.h"

#include <array>
#include <cstring>

namespace halofpx {
namespace {

constexpr char compatibility_domain[] = "halofpx.compat.v1";

constexpr std::array<const char *, context_store_compatibility_v1_component_count> labels {{
    "model_bytes_and_shards",
    "model_metadata",
    "tokenizer_bytes_and_policy",
    "chat_template_bytes_renderer_and_rendered_output",
    "system_and_tool_context",
    "adapter_projector_set_and_order",
    "runtime_abi_and_build",
    "backend_and_device_abi",
    "quantization_and_kv_layout",
    "context_rope_window_and_position",
    "sampler_and_logits_processors",
    "grammar_parser_and_tool_state",
    "rng_state_and_counter",
    "target_draft_mtp_speculative_state",
    "topology_plan_rank_world_placement_epoch",
    "security_domain_and_scope_policy",
}};

constexpr size_t compatibility_wire_size =
    sizeof(compatibility_domain) + 1 +
    context_store_compatibility_v1_component_count * (1 + 2 + 32);

bool digest_nonzero(const context_store_format_digest & digest) noexcept {
    volatile uint8_t accumulated = 0;
    for (const uint8_t value : digest) {
        accumulated = static_cast<uint8_t>(accumulated | value);
    }
    return accumulated != 0;
}

int label_index(const char * label, size_t label_size) noexcept {
    if (label == nullptr || label_size == 0) {
        return -1;
    }
    for (size_t index = 0; index < labels.size(); ++index) {
        const size_t expected_size = std::strlen(labels[index]);
        if (label_size == expected_size &&
            std::memcmp(label, labels[index], expected_size) == 0) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

} // namespace

const char * context_store_compatibility_component_label_v1(size_t index) noexcept {
    return index < labels.size() ? labels[index] : nullptr;
}

context_store_compatibility_build_result_v1 context_store_build_compatibility_expectation_v1(
        const context_store_compatibility_component_digest_v1 * components,
        size_t component_count) noexcept {
    context_store_compatibility_build_result_v1 result;
    if (component_count != context_store_compatibility_v1_component_count) {
        result.status = context_store_compatibility_build_status_v1::wrong_component_count;
        return result;
    }
    if (components == nullptr) {
        result.status = context_store_compatibility_build_status_v1::invalid_input;
        return result;
    }

    std::array<bool, context_store_compatibility_v1_component_count> seen {};
    for (size_t input_index = 0; input_index < component_count; ++input_index) {
        const auto & component = components[input_index];
        const int mapped_index = label_index(component.label, component.label_size);
        if (mapped_index < 0) {
            result.status = context_store_compatibility_build_status_v1::unknown_component;
            result.expectation = {};
            return result;
        }
        const size_t index = static_cast<size_t>(mapped_index);
        if (seen[index]) {
            result.status = context_store_compatibility_build_status_v1::duplicate_component;
            result.expectation = {};
            return result;
        }
        if (index != input_index) {
            result.status = context_store_compatibility_build_status_v1::misordered_component;
            result.expectation = {};
            return result;
        }
        if (!digest_nonzero(component.digest)) {
            result.status = context_store_compatibility_build_status_v1::zero_component_digest;
            result.expectation = {};
            return result;
        }
        seen[index] = true;
        result.expectation.components[index] = component.digest;
    }

    std::array<uint8_t, compatibility_wire_size> wire {};
    size_t offset = 0;
    std::memcpy(wire.data() + offset, compatibility_domain, sizeof(compatibility_domain));
    offset += sizeof(compatibility_domain);
    wire[offset++] = 0xb0; // Deterministic-CBOR map(16).
    for (size_t index = 0; index < result.expectation.components.size(); ++index) {
        wire[offset++] = static_cast<uint8_t>(index);
        wire[offset++] = 0x58;
        wire[offset++] = 0x20;
        std::memcpy(wire.data() + offset, result.expectation.components[index].data(),
            result.expectation.components[index].size());
        offset += result.expectation.components[index].size();
    }
    if (offset != wire.size() ||
        !context_store_sha256_bounded(wire.data(), wire.size(), wire.size(),
            result.expectation.root) ||
        !digest_nonzero(result.expectation.root)) {
        result.expectation = {};
        result.status = context_store_compatibility_build_status_v1::invalid_input;
        return result;
    }
    result.status = context_store_compatibility_build_status_v1::built;
    return result;
}

const char * context_store_compatibility_build_status_name_v1(
        context_store_compatibility_build_status_v1 status) noexcept {
    switch (status) {
        case context_store_compatibility_build_status_v1::built: return "built";
        case context_store_compatibility_build_status_v1::invalid_input: return "invalid-input";
        case context_store_compatibility_build_status_v1::wrong_component_count: return "wrong-component-count";
        case context_store_compatibility_build_status_v1::unknown_component: return "unknown-component";
        case context_store_compatibility_build_status_v1::duplicate_component: return "duplicate-component";
        case context_store_compatibility_build_status_v1::misordered_component: return "misordered-component";
        case context_store_compatibility_build_status_v1::zero_component_digest: return "zero-component-digest";
    }
    return "unknown";
}

} // namespace halofpx
