#include "halofpx-context-store-live-authority-v1.h"

#include <algorithm>
#include <array>
#undef NDEBUG
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace {

using halofpx::context_store_live_artifact_v1;
using halofpx::context_store_live_authority_inputs_v1;
using halofpx::context_store_live_authority_result_v1;
using halofpx::context_store_live_authority_status_v1;
using halofpx::context_store_live_bytes_v1;
using halofpx::context_store_live_metadata_entry_v1;
using halofpx::context_store_live_parameter_v1;
using halofpx::context_store_live_supplement_v1;
using halofpx::context_store_live_text_v1;
using halofpx::context_store_live_typed_value_v1;
using halofpx::context_store_live_value_kind_v1;

context_store_live_text_v1 text(const char * value) {
    return { value, std::strlen(value) };
}

context_store_live_bytes_v1 bytes(const std::vector<uint8_t> & value) {
    return { value.data(), value.size() };
}

context_store_live_bytes_v1 bytes(const char * value) {
    return { reinterpret_cast<const uint8_t *>(value), std::strlen(value) };
}

context_store_live_typed_value_v1 uint_value(uint64_t value) {
    context_store_live_typed_value_v1 result;
    result.kind = context_store_live_value_kind_v1::unsigned_integer;
    result.unsigned_value = value;
    return result;
}

context_store_live_typed_value_v1 text_value(const char * value) {
    context_store_live_typed_value_v1 result;
    result.kind = context_store_live_value_kind_v1::text_string;
    result.text_string = text(value);
    return result;
}

context_store_live_typed_value_v1 bool_value(bool value) {
    context_store_live_typed_value_v1 result;
    result.kind = context_store_live_value_kind_v1::boolean;
    result.boolean_value = value;
    return result;
}

void cbor_type_value(std::vector<uint8_t> & output, uint8_t major, uint64_t value) {
    const uint8_t prefix = static_cast<uint8_t>(major << 5);
    if (value < 24) {
        output.push_back(static_cast<uint8_t>(prefix | value));
    } else if (value <= 0xff) {
        output.push_back(static_cast<uint8_t>(prefix | 24));
        output.push_back(static_cast<uint8_t>(value));
    } else if (value <= 0xffff) {
        output.push_back(static_cast<uint8_t>(prefix | 25));
        output.push_back(static_cast<uint8_t>(value >> 8));
        output.push_back(static_cast<uint8_t>(value));
    } else {
        assert(false && "fixture integer unexpectedly large");
    }
}

void cbor_uint(std::vector<uint8_t> & output, uint64_t value) {
    cbor_type_value(output, 0, value);
}

void cbor_map(std::vector<uint8_t> & output, size_t size) {
    cbor_type_value(output, 5, size);
}

void cbor_array(std::vector<uint8_t> & output, size_t size) {
    cbor_type_value(output, 4, size);
}

void cbor_text(std::vector<uint8_t> & output, const char * value) {
    const size_t size = std::strlen(value);
    cbor_type_value(output, 3, size);
    output.insert(output.end(), value, value + size);
}

void cbor_digest(std::vector<uint8_t> & output, uint8_t fill) {
    cbor_type_value(output, 2, 32);
    output.insert(output.end(), 32, fill);
}

void cbor_typed_bool(std::vector<uint8_t> & output, bool value) {
    cbor_array(output, 2);
    cbor_uint(output, 4);
    output.push_back(value ? 0xf5 : 0xf4);
}

void cbor_parameter_bool(
        std::vector<uint8_t> & output,
        const char * id,
        bool value) {
    cbor_map(output, 2);
    cbor_uint(output, 0);
    cbor_text(output, id);
    cbor_uint(output, 1);
    cbor_typed_bool(output, value);
}

std::vector<uint8_t> supplemental_preimage(size_t component) {
    std::vector<uint8_t> output;
    switch (component) {
        case 4: // rendered system/tool context
            cbor_map(output, 5);
            cbor_uint(output, 0); cbor_uint(output, 0);
            cbor_uint(output, 1); cbor_digest(output, 0x44);
            cbor_uint(output, 2); cbor_text(output, "no-extra-context-v1");
            cbor_uint(output, 3); cbor_uint(output, 0);
            cbor_uint(output, 4); cbor_digest(output, 0x45);
            break;
        case 5: // no adapters/projectors
            cbor_map(output, 2);
            cbor_uint(output, 0); cbor_array(output, 0);
            cbor_uint(output, 1); cbor_array(output, 0);
            break;
        case 7: // backend/device ABI
            cbor_map(output, 5);
            cbor_uint(output, 0); cbor_text(output, "rocmfpx-v1");
            cbor_uint(output, 1); cbor_text(output, "gfx1151");
            cbor_uint(output, 2); cbor_array(output, 1);
            cbor_map(output, 4);
            cbor_uint(output, 0); cbor_text(output, "rocm-runtime");
            cbor_uint(output, 1); cbor_uint(output, 0);
            cbor_uint(output, 2); cbor_uint(output, 17);
            cbor_uint(output, 3); cbor_digest(output, 0x77);
            cbor_uint(output, 3); cbor_text(output, "halofpx-memory-v1");
            cbor_uint(output, 4); cbor_array(output, 0);
            break;
        case 8: // quantization/KV
            cbor_map(output, 4);
            cbor_uint(output, 0); cbor_array(output, 1);
            cbor_map(output, 4);
            cbor_uint(output, 0); cbor_text(output, "blk.0.attn_q");
            cbor_uint(output, 1); cbor_text(output, "iq3_xxs");
            cbor_uint(output, 2); cbor_array(output, 2); cbor_uint(output, 8); cbor_uint(output, 8);
            cbor_uint(output, 3); cbor_text(output, "rocmfpx-v1");
            cbor_uint(output, 1); cbor_text(output, "q6_k-v1");
            cbor_uint(output, 2); cbor_text(output, "q4_0-v1");
            cbor_uint(output, 3); cbor_array(output, 0);
            break;
        case 9: // resolved context/RoPE/window
            cbor_map(output, 1);
            cbor_uint(output, 0); cbor_array(output, 1);
            cbor_parameter_bool(output, "resolved-flash-attention", true);
            break;
        case 10: // sampler/logits
            cbor_map(output, 2);
            cbor_uint(output, 0); cbor_text(output, "greedy-v1");
            cbor_uint(output, 1); cbor_array(output, 0);
            break;
        case 11: // grammar/parser/tool
            cbor_map(output, 4);
            cbor_uint(output, 0); cbor_text(output, "none-v1");
            cbor_uint(output, 1); cbor_uint(output, 0);
            cbor_uint(output, 2); cbor_digest(output, 0xbb);
            cbor_uint(output, 3); cbor_array(output, 0);
            break;
        case 12: // RNG
            cbor_map(output, 4);
            cbor_uint(output, 0); cbor_text(output, "greedy-no-rng-v1");
            cbor_uint(output, 1); cbor_text(output, "no-state-v1");
            cbor_uint(output, 2); cbor_text(output, "exact-token-prefix-v1");
            cbor_uint(output, 3); cbor_array(output, 0);
            break;
        case 13: // target-only/no speculation
            cbor_map(output, 4);
            cbor_uint(output, 0); cbor_digest(output, 0xdd);
            cbor_uint(output, 1); output.push_back(0xf6);
            cbor_uint(output, 2); cbor_text(output, "target-only-v1");
            cbor_uint(output, 3); cbor_array(output, 0);
            break;
        case 15: // authenticated private scope
            cbor_map(output, 4);
            cbor_uint(output, 0); cbor_text(output, "halofpx-private-v1");
            cbor_uint(output, 1); cbor_uint(output, 7);
            cbor_uint(output, 2); cbor_uint(output, 0);
            cbor_uint(output, 3); cbor_text(output, "halofpx-keyring-v1");
            break;
        default:
            assert(false && "unexpected supplemental component");
    }
    return output;
}

struct fixture {
    std::vector<uint8_t> shard0 = std::vector<uint8_t>(64, 0);
    std::vector<uint8_t> shard1 = std::vector<uint8_t>(32, 0);
    std::array<context_store_live_artifact_v1, 2> shards {};

    std::array<context_store_live_typed_value_v1, 2> metadata_array_values {};
    std::array<context_store_live_metadata_entry_v1, 3> metadata {};
    std::array<uint64_t, 2> tensor0_dims {{ 8, 8 }};
    std::array<uint64_t, 2> tensor1_dims {{ 8, 4 }};
    std::array<halofpx::context_store_live_tensor_entry_v1, 2> tensors {};

    std::vector<uint8_t> tokenizer_bytes { 0x54, 0x4f, 0x4b, 0x01 };
    std::array<context_store_live_artifact_v1, 1> tokenizer_artifacts {};
    std::array<halofpx::context_store_live_token_entry_v1, 3> tokens {};
    std::vector<uint8_t> token0 { 0x61 };
    std::vector<uint8_t> token1 { 0x62 };
    std::vector<uint8_t> token2 { 0x61, 0x62 };
    std::vector<uint8_t> merge_left { 0x61 };
    std::vector<uint8_t> merge_right { 0x62 };
    std::array<halofpx::context_store_live_merge_entry_v1, 1> merges {};
    std::array<halofpx::context_store_live_special_token_v1, 2> specials {};
    std::array<context_store_live_parameter_v1, 1> tokenizer_policy {};

    std::vector<uint8_t> template_bytes {
        '{', '{', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e', 's', ' ', '}', '}',
    };
    std::vector<uint8_t> renderer_bytes { 0x52, 0x45, 0x4e, 0x44, 0x01 };

    std::vector<uint8_t> source_receipt { 0x74, 0x72, 0x65, 0x65, 0x01 };
    std::vector<uint8_t> executable_bytes { 0x45, 0x4c, 0x46, 0x01 };
    std::vector<uint8_t> library_bytes { 0x52, 0x4f, 0x43, 0x4d, 0x01 };
    std::array<context_store_live_artifact_v1, 2> runtime_artifacts {};
    std::array<context_store_live_parameter_v1, 1> build_options {};

    std::vector<uint8_t> global_plan { 0x50, 0x4c, 0x41, 0x4e, 0x01 };
    std::vector<uint8_t> ownership0 { 0x4f, 0x57, 0x4e, 0x00 };
    std::vector<uint8_t> ownership1 { 0x4f, 0x57, 0x4e, 0x01 };
    std::vector<uint8_t> placement0 { 0x50, 0x4c, 0x41, 0x43, 0x00 };
    std::vector<uint8_t> placement1 { 0x50, 0x4c, 0x41, 0x43, 0x01 };
    std::array<halofpx::context_store_live_rank_v1, 2> ranks {};

    const std::array<size_t, 10> supplemental_indices {{ 4, 5, 7, 8, 9, 10, 11, 12, 13, 15 }};
    std::array<std::vector<uint8_t>, 10> supplemental_bytes {};
    std::array<context_store_live_supplement_v1, 10> supplements {};
    context_store_live_authority_inputs_v1 inputs {};

    fixture() {
        shard0[0] = 0x47;
        shard0[1] = 0x47;
        shard0[2] = 0x55;
        shard0[3] = 0x46;
        shard0[4] = 0x01;
        shard1[0] = 0x47;
        shard1[1] = 0x47;
        shard1[2] = 0x55;
        shard1[3] = 0x46;
        shard1[4] = 0x02;
        for (size_t index = 0; index < supplemental_indices.size(); ++index) {
            supplemental_bytes[index] = supplemental_preimage(supplemental_indices[index]);
        }
        bind();
    }

    void bind() {
        shards = {{
            { text("model-shard-00000-of-00002"), 0, bytes(shard0) },
            { text("model-shard-00001-of-00002"), 1, bytes(shard1) },
        }};

        metadata_array_values[0] = uint_value(32);
        metadata_array_values[1] = text_value("silu");
        auto array_value = context_store_live_typed_value_v1 {};
        array_value.kind = context_store_live_value_kind_v1::array;
        array_value.array_values = metadata_array_values.data();
        array_value.array_value_count = metadata_array_values.size();
        metadata = {{
            { text("general.architecture"), text_value("llama") },
            { text("llama.block_count"), uint_value(2) },
            { text("halofpx.typed.array"), array_value },
        }};
        tensors = {{
            { text("blk.0.attn_q.weight"), 0, tensor0_dims.data(), tensor0_dims.size(), 7, 0, 64 },
            { text("blk.1.attn_q.weight"), 1, tensor1_dims.data(), tensor1_dims.size(), 7, 0, 32 },
        }};

        tokenizer_artifacts = {{
            { text("tokenizer-gguf-fields"), 0, bytes(tokenizer_bytes) },
        }};
        tokens = {{
            { 0, bytes(token0), {}, 1 },
            { 1, bytes(token1), {}, 1 },
            { 2, bytes(token2), {}, 1 },
        }};
        merges = {{ { bytes(merge_left), bytes(merge_right) } }};
        specials = {{
            { text("bos"), 0 },
            { text("eos"), 1 },
        }};
        tokenizer_policy = {{
            { text("byte-fallback"), bool_value(true) },
        }};

        runtime_artifacts = {{
            { text("executable"), 0, bytes(executable_bytes) },
            { text("lib-rocmfpx"), 1, bytes(library_bytes) },
        }};
        build_options = {{
            { text("halofpx-rpc-local-state"), bool_value(true) },
        }};

        ranks = {{
            { 0, text("strix-0-service-v1"), text("strix-0-gfx1151-v1"),
                bytes(ownership0), bytes(placement0) },
            { 1, text("strix-1-service-v1"), text("strix-1-gfx1151-v1"),
                bytes(ownership1), bytes(placement1) },
        }};
        for (size_t index = 0; index < supplements.size(); ++index) {
            supplements[index] = {
                supplemental_indices[index], bytes(supplemental_bytes[index]),
            };
        }

        inputs.model = {
            shards.data(), shards.size(), shards.size(), text("gguf-v3-typed-v1"),
            metadata.data(), metadata.size(), tensors.data(), tensors.size(),
        };
        inputs.tokenizer = {
            tokenizer_artifacts.data(), tokenizer_artifacts.size(),
            tokens.data(), tokens.size(), merges.data(), merges.size(),
            specials.data(), specials.size(), tokenizer_policy.data(), tokenizer_policy.size(),
        };
        inputs.chat_template = {
            1, bytes(template_bytes), text("minja-pinned-v1"), bytes(renderer_bytes),
        };
        inputs.runtime = {
            text("b77f2bce-clean-fixture"), bytes(source_receipt), true, false, {},
            runtime_artifacts.data(), runtime_artifacts.size(),
            text("clang-rocm-7.0.2-v1"), build_options.data(), build_options.size(),
            text("llama-state-seq-internal-v1"),
        };
        inputs.topology = {
            text("halofpx-dual-strix-plan-v1"), text("fixed-two-rank-rpc-v1"), 2,
            bytes(global_plan), 7, ranks.data(), ranks.size(),
        };
        inputs.supplements = supplements.data();
        inputs.supplement_count = supplements.size();
    }

    void reverse_nonsemantic_discovery_order() {
        std::reverse(shards.begin(), shards.end());
        std::reverse(metadata.begin(), metadata.end());
        std::reverse(tensors.begin(), tensors.end());
        std::reverse(tokens.begin(), tokens.end());
        std::reverse(specials.begin(), specials.end());
        std::reverse(runtime_artifacts.begin(), runtime_artifacts.end());
        std::reverse(supplements.begin(), supplements.end());
    }
};

bool zero_expectation(const context_store_live_authority_result_v1 & result) {
    const auto zero_digest = [](const auto & value) {
        return std::all_of(value.begin(), value.end(),
            [](uint8_t byte) { return byte == 0; });
    };
    return zero_digest(result.expectation.root) &&
        std::all_of(result.expectation.components.begin(),
            result.expectation.components.end(), zero_digest);
}

void expect_refused(
        const context_store_live_authority_result_v1 & result,
        context_store_live_authority_status_v1 expected,
        size_t component = halofpx::context_store_compatibility_v1_component_count) {
    assert(result.status == expected);
    assert(result.failure_component == component);
    assert(zero_expectation(result));
}

class fixture_validator final :
        public halofpx::context_store_live_supplement_validator_v1 {
public:
    bool validate(
            size_t component_index,
            context_store_live_bytes_v1 exact_dcbor_preimage) const noexcept override {
        ++calls;
        try {
            const auto expected = supplemental_preimage(component_index);
            return exact_dcbor_preimage.size == expected.size() &&
                (expected.empty() || std::memcmp(
                    exact_dcbor_preimage.data, expected.data(), expected.size()) == 0);
        } catch (...) {
            return false;
        }
    }

    mutable size_t calls = 0;
};

context_store_live_authority_result_v1 build(
        fixture & value,
        const halofpx::context_store_live_supplement_validator_v1 & validator) {
    return halofpx::context_store_build_live_authority_v1(value.inputs, validator);
}

context_store_live_authority_result_v1 build(fixture & value) {
    const fixture_validator validator;
    return build(value, validator);
}

void assert_only_component_changed(
        const context_store_live_authority_result_v1 & baseline,
        const context_store_live_authority_result_v1 & changed,
        size_t expected_component) {
    assert(baseline.status == context_store_live_authority_status_v1::built);
    assert(changed.status == context_store_live_authority_status_v1::built);
    assert(baseline.expectation.root != changed.expectation.root);
    for (size_t index = 0; index < baseline.expectation.components.size(); ++index) {
        assert((baseline.expectation.components[index] != changed.expectation.components[index]) ==
            (index == expected_component));
    }
}

void test_builds_deterministically_and_owns_authority() {
    fixture first;
    fixture second;
    second.reverse_nonsemantic_discovery_order();

    const auto first_result = build(first);
    const auto second_result = build(second);
    assert(first_result.status == context_store_live_authority_status_v1::built);
    assert(second_result.status == context_store_live_authority_status_v1::built);
    assert(first_result.expectation.root == second_result.expectation.root);
    assert(first_result.expectation.components == second_result.expectation.components);

    const halofpx::context_store_format_digest golden_root {{
        0x51, 0x52, 0x79, 0x64, 0x36, 0xf4, 0x9d, 0x2c,
        0x35, 0x1b, 0xd0, 0x45, 0x9f, 0x4f, 0xbf, 0x5c,
        0x5f, 0xba, 0x89, 0x94, 0x67, 0x2d, 0xfa, 0x74,
        0x04, 0x2a, 0xb2, 0xf4, 0x62, 0x26, 0xd4, 0xc8,
    }};
    assert(first_result.expectation.root == golden_root);

    const auto owned_copy = first_result.expectation;
    first.shard0[0] ^= 0xff;
    first.metadata[1].value.unsigned_value += 1;
    assert(first_result.expectation.root == owned_copy.root);
    assert(first_result.expectation.components == owned_copy.components);
}

void test_binds_live_inputs() {
    fixture baseline_fixture;
    const auto baseline = build(baseline_fixture);
    assert(baseline.status == context_store_live_authority_status_v1::built);

    fixture model_bytes;
    model_bytes.shard0[0] ^= 0x80;
    assert_only_component_changed(baseline, build(model_bytes), 0);

    fixture metadata;
    metadata.metadata[1].value.unsigned_value += 1;
    assert_only_component_changed(baseline, build(metadata), 1);

    fixture metadata_array;
    metadata_array.metadata_array_values[0].unsigned_value += 1;
    assert_only_component_changed(baseline, build(metadata_array), 1);

    fixture tokenizer;
    tokenizer.merge_left[0] ^= 0x20;
    assert_only_component_changed(baseline, build(tokenizer), 2);

    fixture chat;
    chat.template_bytes[0] ^= 0x01;
    assert_only_component_changed(baseline, build(chat), 3);

    fixture renderer;
    renderer.renderer_bytes[0] ^= 0x01;
    assert_only_component_changed(baseline, build(renderer), 3);

    fixture runtime;
    runtime.executable_bytes[0] ^= 0x01;
    assert_only_component_changed(baseline, build(runtime), 6);

    fixture state_abi;
    state_abi.inputs.runtime.state_abi_id = text("llama-state-seq-internal-v2");
    assert_only_component_changed(baseline, build(state_abi), 6);

    fixture topology;
    topology.ownership1[0] ^= 0x01;
    assert_only_component_changed(baseline, build(topology), 14);

    fixture placement;
    placement.placement1[0] ^= 0x01;
    assert_only_component_changed(baseline, build(placement), 14);
}

void test_refuses_missing_or_ambiguous_model_artifacts() {
    fixture missing;
    missing.shards[0].exact_bytes = {};
    fixture_validator validator;
    expect_refused(build(missing, validator),
        context_store_live_authority_status_v1::missing_model_artifact, 0);
    assert(validator.calls == 0);

    fixture count_mismatch;
    count_mismatch.inputs.model.declared_shard_count = 3;
    expect_refused(build(count_mismatch),
        context_store_live_authority_status_v1::missing_model_artifact, 0);

    fixture duplicate_order;
    duplicate_order.shards[1].semantic_order = 0;
    expect_refused(build(duplicate_order),
        context_store_live_authority_status_v1::ambiguous_shard_plan, 0);

    fixture duplicate_role;
    duplicate_role.shards[1].role = duplicate_role.shards[0].role;
    expect_refused(build(duplicate_role),
        context_store_live_authority_status_v1::ambiguous_shard_plan, 0);
}

void test_refuses_missing_or_ambiguous_model_semantics() {
    fixture missing_metadata;
    missing_metadata.inputs.model.metadata_count = 0;
    expect_refused(build(missing_metadata),
        context_store_live_authority_status_v1::incomplete_typed_model_semantics, 1);

    fixture duplicate_metadata;
    duplicate_metadata.metadata[1].key = duplicate_metadata.metadata[0].key;
    expect_refused(build(duplicate_metadata),
        context_store_live_authority_status_v1::ambiguous_model_semantics, 1);

    fixture duplicate_tensor;
    duplicate_tensor.tensors[1].name = duplicate_tensor.tensors[0].name;
    expect_refused(build(duplicate_tensor),
        context_store_live_authority_status_v1::ambiguous_model_semantics, 1);

    fixture tensor_past_shard;
    tensor_past_shard.tensors[0].exact_container_offset = 1;
    expect_refused(build(tensor_past_shard),
        context_store_live_authority_status_v1::incomplete_typed_model_semantics, 1);

    fixture original;
    original.shard1.resize(96);
    original.bind();

    fixture changed_tensor_shard;
    changed_tensor_shard.shard1.resize(96);
    changed_tensor_shard.bind();
    changed_tensor_shard.tensors[0].shard_order = 1;
    changed_tensor_shard.tensors[0].exact_container_offset = 32;
    const auto changed_shard_result = build(changed_tensor_shard);
    assert(changed_shard_result.status == context_store_live_authority_status_v1::built);
    assert_only_component_changed(build(original), changed_shard_result, 1);

    fixture missing_tokenizer;
    missing_tokenizer.inputs.tokenizer.policy_count = 0;
    expect_refused(build(missing_tokenizer),
        context_store_live_authority_status_v1::incomplete_tokenizer, 2);

    fixture duplicate_token;
    duplicate_token.tokens[1].token_id = duplicate_token.tokens[0].token_id;
    expect_refused(build(duplicate_token),
        context_store_live_authority_status_v1::ambiguous_tokenizer, 2);

    fixture duplicate_special;
    duplicate_special.specials[1].role = duplicate_special.specials[0].role;
    expect_refused(build(duplicate_special),
        context_store_live_authority_status_v1::ambiguous_tokenizer, 2);

    fixture no_template;
    no_template.inputs.chat_template.effective_candidate_count = 0;
    expect_refused(build(no_template),
        context_store_live_authority_status_v1::ambiguous_template_selection, 3);

    fixture two_templates;
    two_templates.inputs.chat_template.effective_candidate_count = 2;
    expect_refused(build(two_templates),
        context_store_live_authority_status_v1::ambiguous_template_selection, 3);

    fixture missing_renderer;
    missing_renderer.inputs.chat_template.renderer_id = {};
    expect_refused(build(missing_renderer),
        context_store_live_authority_status_v1::incomplete_template_identity, 3);
}

void test_refuses_incomplete_or_mutable_build_identity() {
    fixture missing_state_abi;
    missing_state_abi.inputs.runtime.state_abi_id = {};
    expect_refused(build(missing_state_abi),
        context_store_live_authority_status_v1::incomplete_build_abi, 6);

    fixture unknown_source;
    unknown_source.inputs.runtime.source_revision = text("unknown");
    expect_refused(build(unknown_source),
        context_store_live_authority_status_v1::incomplete_build_abi, 6);

    fixture mutable_source_name;
    mutable_source_name.inputs.runtime.source_revision = text("latest");
    expect_refused(build(mutable_source_name),
        context_store_live_authority_status_v1::incomplete_build_abi, 6);

    fixture mutable_source;
    mutable_source.inputs.runtime.source_is_immutable = false;
    expect_refused(build(mutable_source),
        context_store_live_authority_status_v1::mutable_build_identity, 6);

    fixture uncaptured_dirty_source;
    uncaptured_dirty_source.inputs.runtime.source_is_dirty = true;
    expect_refused(build(uncaptured_dirty_source),
        context_store_live_authority_status_v1::mutable_build_identity, 6);

    fixture no_executable;
    no_executable.runtime_artifacts[0].role = text("not-an-executable");
    expect_refused(build(no_executable),
        context_store_live_authority_status_v1::incomplete_build_abi, 6);
}

void test_refuses_incomplete_or_ambiguous_topology_and_supplements() {
    fixture world_one;
    world_one.inputs.topology.world_size = 1;
    expect_refused(build(world_one),
        context_store_live_authority_status_v1::incomplete_topology, 14);

    fixture missing_rank;
    missing_rank.inputs.topology.rank_count = 1;
    expect_refused(build(missing_rank),
        context_store_live_authority_status_v1::incomplete_topology, 14);

    fixture reordered_rank;
    reordered_rank.ranks[0].logical_rank = 1;
    reordered_rank.ranks[1].logical_rank = 0;
    expect_refused(build(reordered_rank),
        context_store_live_authority_status_v1::ambiguous_rank_plan, 14);

    fixture duplicate_endpoint;
    duplicate_endpoint.ranks[1].stable_endpoint_id =
        duplicate_endpoint.ranks[0].stable_endpoint_id;
    expect_refused(build(duplicate_endpoint),
        context_store_live_authority_status_v1::ambiguous_rank_plan, 14);

    fixture placeholder_plan;
    placeholder_plan.ranks[1].placement_plan = placeholder_plan.ranks[0].ownership_plan;
    expect_refused(build(placeholder_plan),
        context_store_live_authority_status_v1::placeholder_equal_facts, 14);

    fixture missing_supplement;
    missing_supplement.inputs.supplement_count -= 1;
    expect_refused(build(missing_supplement),
        context_store_live_authority_status_v1::missing_component_preimage);

    fixture duplicate_supplement;
    duplicate_supplement.supplements[1].component_index =
        duplicate_supplement.supplements[0].component_index;
    expect_refused(build(duplicate_supplement),
        context_store_live_authority_status_v1::duplicate_component_preimage);

    fixture placeholder_supplement;
    placeholder_supplement.supplements[1].exact_dcbor_preimage =
        placeholder_supplement.supplements[0].exact_dcbor_preimage;
    expect_refused(build(placeholder_supplement),
        context_store_live_authority_status_v1::unvalidated_component_preimage);

    fixture truncated_supplement;
    const uint8_t truncated_map = 0xa1;
    truncated_supplement.supplements[0].exact_dcbor_preimage = {
        &truncated_map, 1,
    };
    expect_refused(build(truncated_supplement),
        context_store_live_authority_status_v1::unvalidated_component_preimage);
}

} // namespace

int main() {
    test_builds_deterministically_and_owns_authority();
    test_binds_live_inputs();
    test_refuses_missing_or_ambiguous_model_artifacts();
    test_refuses_missing_or_ambiguous_model_semantics();
    test_refuses_incomplete_or_mutable_build_identity();
    test_refuses_incomplete_or_ambiguous_topology_and_supplements();
    assert(std::strcmp(halofpx::context_store_live_authority_status_name_v1(
        context_store_live_authority_status_v1::built), "built") == 0);
    return 0;
}
