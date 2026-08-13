#include "halofpx-context-store-live-authority-v1.h"

extern "C" {
#include "sha256/sha256.h"
}

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <numeric>
#include <vector>

namespace halofpx {
namespace {

using status = context_store_live_authority_status_v1;
using digest = context_store_format_digest;

constexpr char component_domain[] = "halofpx.compat-component.v1";
constexpr char source_tree_domain[] = "halofpx.source-tree.v1";
constexpr char global_plan_domain[] = "halofpx.global-plan.v1";
constexpr char rank_ownership_domain[] = "halofpx.rank-ownership.v1";
constexpr char rank_placement_domain[] = "halofpx.rank-placement.v1";
constexpr uint64_t component_preimage_max_bytes = 256ULL * 1024 * 1024;
constexpr size_t max_metadata_entries = 65535;
constexpr size_t max_tensor_entries = 65535;
constexpr size_t max_token_entries = 4ULL * 1024 * 1024;
constexpr size_t max_merge_entries = 4ULL * 1024 * 1024;
constexpr size_t max_parameter_entries = 65535;
constexpr size_t max_special_token_entries = 65535;

constexpr std::array<size_t, 6> derived_components {{ 0, 1, 2, 3, 6, 14 }};

bool valid_bytes(const context_store_live_bytes_v1 & bytes) noexcept {
    return bytes.size == 0 || bytes.data != nullptr;
}

bool nonempty_bytes(const context_store_live_bytes_v1 & bytes) noexcept {
    return bytes.data != nullptr && bytes.size != 0 &&
        bytes.size <= std::numeric_limits<uint64_t>::max() / 8;
}

bool valid_utf8(const char * data, size_t size) noexcept {
    if (data == nullptr && size != 0) {
        return false;
    }
    size_t offset = 0;
    while (offset < size) {
        const uint8_t first = static_cast<uint8_t>(data[offset++]);
        if (first == 0) {
            return false;
        }
        if (first < 0x80) {
            continue;
        }
        size_t continuation_count = 0;
        uint32_t value = 0;
        uint32_t minimum = 0;
        if ((first & 0xe0U) == 0xc0U) {
            continuation_count = 1;
            value = first & 0x1fU;
            minimum = 0x80;
        } else if ((first & 0xf0U) == 0xe0U) {
            continuation_count = 2;
            value = first & 0x0fU;
            minimum = 0x800;
        } else if ((first & 0xf8U) == 0xf0U) {
            continuation_count = 3;
            value = first & 0x07U;
            minimum = 0x10000;
        } else {
            return false;
        }
        if (continuation_count > size - offset) {
            return false;
        }
        for (size_t index = 0; index < continuation_count; ++index) {
            const uint8_t next = static_cast<uint8_t>(data[offset++]);
            if ((next & 0xc0U) != 0x80U) {
                return false;
            }
            value = (value << 6) | (next & 0x3fU);
        }
        if (value < minimum || value > 0x10ffffU ||
            (value >= 0xd800U && value <= 0xdfffU)) {
            return false;
        }
    }
    return true;
}

bool valid_text(const context_store_live_text_v1 & text, size_t maximum) noexcept {
    return text.size != 0 && text.size <= maximum &&
        text.data != nullptr && valid_utf8(text.data, text.size);
}

bool text_equal(
        const context_store_live_text_v1 & left,
        const context_store_live_text_v1 & right) noexcept {
    return left.size == right.size &&
        (left.size == 0 || std::memcmp(left.data, right.data, left.size) == 0);
}

bool text_equal_literal(
        const context_store_live_text_v1 & text,
        const char * literal) noexcept {
    const size_t size = std::strlen(literal);
    return text.size == size && text.data != nullptr &&
        std::memcmp(text.data, literal, size) == 0;
}

bool placeholder_text(const context_store_live_text_v1 & text) noexcept {
    constexpr std::array<const char *, 5> placeholders {{
        "unknown", "UNKNOWN", "unset", "UNSET", "0",
    }};
    return std::any_of(placeholders.begin(), placeholders.end(),
        [&text](const char * value) { return text_equal_literal(text, value); });
}

bool mutable_source_revision(const context_store_live_text_v1 & text) noexcept {
    constexpr std::array<const char *, 15> placeholders {{
        "unknown", "UNKNOWN", "unset", "UNSET", "0",
        "latest", "LATEST", "head", "HEAD", "tip", "TIP",
        "main", "MAIN", "master", "MASTER",
    }};
    return std::any_of(placeholders.begin(), placeholders.end(),
        [&text](const char * value) { return text_equal_literal(text, value); });
}

bool bytes_equal(
        const context_store_live_bytes_v1 & left,
        const context_store_live_bytes_v1 & right) noexcept {
    return left.size == right.size &&
        (left.size == 0 || std::memcmp(left.data, right.data, left.size) == 0);
}

bool digest_nonzero(const digest & value) noexcept {
    uint8_t accumulated = 0;
    for (const uint8_t byte : value) {
        accumulated = static_cast<uint8_t>(accumulated | byte);
    }
    return accumulated != 0;
}

class hash_accumulator {
public:
    explicit hash_accumulator(uint64_t maximum =
            std::numeric_limits<uint64_t>::max() / 8) noexcept : maximum_(maximum) {
        sha256_init(&context_);
    }

    bool update(const void * data, size_t size) noexcept {
        if (!ok_ || (data == nullptr && size != 0) ||
            count_ > maximum_ || size > maximum_ - count_) {
            ok_ = false;
            return false;
        }
        if (size != 0) {
            sha256_update(&context_, static_cast<const uint8_t *>(data), size);
        }
        count_ += size;
        return true;
    }

    bool byte(uint8_t value) noexcept {
        return update(&value, 1);
    }

    bool uint16_be(uint16_t value) noexcept {
        const std::array<uint8_t, 2> bytes {{
            static_cast<uint8_t>(value >> 8),
            static_cast<uint8_t>(value),
        }};
        return update(bytes.data(), bytes.size());
    }

    bool uint32_be(uint32_t value) noexcept {
        std::array<uint8_t, 4> bytes {};
        for (size_t index = 0; index < bytes.size(); ++index) {
            bytes[bytes.size() - index - 1] = static_cast<uint8_t>(value >> (index * 8));
        }
        return update(bytes.data(), bytes.size());
    }

    bool uint64_be(uint64_t value) noexcept {
        std::array<uint8_t, 8> bytes {};
        for (size_t index = 0; index < bytes.size(); ++index) {
            bytes[bytes.size() - index - 1] = static_cast<uint8_t>(value >> (index * 8));
        }
        return update(bytes.data(), bytes.size());
    }

    bool finish(digest & output) noexcept {
        if (!ok_) {
            output = {};
            return false;
        }
        sha256_final(&context_, output.data());
        ok_ = false;
        return digest_nonzero(output);
    }

private:
    sha256_t context_ {};
    uint64_t count_ = 0;
    uint64_t maximum_ = 0;
    bool ok_ = true;
};

bool hash_exact_bytes(
        const context_store_live_bytes_v1 & bytes,
        digest & output) noexcept {
    if (!valid_bytes(bytes) || bytes.size > std::numeric_limits<uint64_t>::max() / 8) {
        return false;
    }
    hash_accumulator hash;
    return hash.update(bytes.data, bytes.size) && hash.finish(output);
}

class component_cbor_writer {
public:
    explicit component_cbor_writer(size_t component_index) noexcept :
            label_(context_store_compatibility_component_label_v1(component_index)) {
        if (label_ == nullptr) {
            ok_ = false;
            return;
        }
        const size_t label_size = std::strlen(label_);
        if (label_size > std::numeric_limits<uint16_t>::max() ||
            !hash_.update(component_domain, sizeof(component_domain)) ||
            !hash_.uint16_be(static_cast<uint16_t>(label_size)) ||
            !hash_.update(label_, label_size)) {
            ok_ = false;
        }
    }

    bool unsigned_integer(uint64_t value) noexcept { return type_value(0, value); }
    bool negative_integer(int64_t value) noexcept {
        if (value >= 0) {
            ok_ = false;
            return false;
        }
        const uint64_t encoded = static_cast<uint64_t>(-(value + 1));
        return type_value(1, encoded);
    }
    bool byte_string_header(uint64_t size) noexcept { return type_value(2, size); }
    bool text_string_header(uint64_t size) noexcept { return type_value(3, size); }
    bool array(size_t size) noexcept { return type_value(4, size); }
    bool map(size_t size) noexcept { return type_value(5, size); }
    bool null_value() noexcept { return put(0xf6); }
    bool boolean(bool value) noexcept { return put(value ? 0xf5 : 0xf4); }

    bool bytes(const context_store_live_bytes_v1 & value) noexcept {
        return valid_bytes(value) && byte_string_header(value.size) &&
            raw(value.data, value.size);
    }

    bool bytes(const digest & value) noexcept {
        return byte_string_header(value.size()) && raw(value.data(), value.size());
    }

    bool bytes(const std::array<uint8_t, 8> & value) noexcept {
        return byte_string_header(value.size()) && raw(value.data(), value.size());
    }

    bool text(const context_store_live_text_v1 & value) noexcept {
        return text_string_header(value.size) && raw(value.data, value.size);
    }

    bool raw(const void * data, size_t size) noexcept {
        if (!ok_ || !hash_.update(data, size)) {
            ok_ = false;
        }
        return ok_;
    }

    bool finish(digest & output) noexcept {
        return ok_ && hash_.finish(output);
    }

private:
    bool put(uint8_t value) noexcept {
        if (!ok_ || !hash_.byte(value)) {
            ok_ = false;
        }
        return ok_;
    }

    bool type_value(uint8_t major, uint64_t value) noexcept {
        if (!ok_) {
            return false;
        }
        const uint8_t prefix = static_cast<uint8_t>(major << 5);
        if (value < 24) {
            return put(static_cast<uint8_t>(prefix | value));
        }
        if (value <= 0xff) {
            return put(static_cast<uint8_t>(prefix | 24)) &&
                put(static_cast<uint8_t>(value));
        }
        if (value <= 0xffff) {
            return put(static_cast<uint8_t>(prefix | 25)) &&
                hash_.uint16_be(static_cast<uint16_t>(value));
        }
        if (value <= 0xffffffffULL) {
            return put(static_cast<uint8_t>(prefix | 26)) &&
                hash_.uint32_be(static_cast<uint32_t>(value));
        }
        return put(static_cast<uint8_t>(prefix | 27)) && hash_.uint64_be(value);
    }

    const char * label_ = nullptr;
    hash_accumulator hash_ { component_preimage_max_bytes };
    bool ok_ = true;
};

std::array<uint8_t, 9> cbor_text_header(size_t size, size_t & header_size) noexcept {
    std::array<uint8_t, 9> result {};
    if (size < 24) {
        result[0] = static_cast<uint8_t>(0x60U | size);
        header_size = 1;
    } else if (size <= 0xff) {
        result[0] = 0x78;
        result[1] = static_cast<uint8_t>(size);
        header_size = 2;
    } else if (size <= 0xffff) {
        result[0] = 0x79;
        result[1] = static_cast<uint8_t>(size >> 8);
        result[2] = static_cast<uint8_t>(size);
        header_size = 3;
    } else if (size <= 0xffffffffULL) {
        result[0] = 0x7a;
        for (size_t index = 0; index < 4; ++index) {
            result[4 - index] = static_cast<uint8_t>(size >> (index * 8));
        }
        header_size = 5;
    } else {
        result[0] = 0x7b;
        for (size_t index = 0; index < 8; ++index) {
            result[8 - index] = static_cast<uint8_t>(size >> (index * 8));
        }
        header_size = 9;
    }
    return result;
}

bool canonical_text_less(
        const context_store_live_text_v1 & left,
        const context_store_live_text_v1 & right) noexcept {
    size_t left_header_size = 0;
    size_t right_header_size = 0;
    const auto left_header = cbor_text_header(left.size, left_header_size);
    const auto right_header = cbor_text_header(right.size, right_header_size);
    const size_t common_header = std::min(left_header_size, right_header_size);
    const int header_order = std::memcmp(
        left_header.data(), right_header.data(), common_header);
    if (header_order != 0) {
        return header_order < 0;
    }
    if (left_header_size != right_header_size) {
        return left_header_size < right_header_size;
    }
    const size_t common = std::min(left.size, right.size);
    const int text_order = common == 0 ? 0 :
        std::memcmp(left.data, right.data, common);
    return text_order < 0 || (text_order == 0 && left.size < right.size);
}

bool finite_float64(const std::array<uint8_t, 8> & bits) noexcept {
    const uint16_t exponent = static_cast<uint16_t>(
        ((bits[0] & 0x7fU) << 4) | (bits[1] >> 4));
    return exponent != 0x7ffU;
}

bool validate_typed_value(
        const context_store_live_typed_value_v1 & value,
        size_t depth,
        size_t & item_count) noexcept {
    if (depth > context_store_live_authority_max_typed_depth_v1 ||
        item_count >= max_parameter_entries) {
        return false;
    }
    ++item_count;
    switch (value.kind) {
        case context_store_live_value_kind_v1::unsigned_integer:
            return true;
        case context_store_live_value_kind_v1::negative_integer:
            return value.negative_value < 0;
        case context_store_live_value_kind_v1::byte_string:
            return valid_bytes(value.byte_string) &&
                value.byte_string.size <= context_store_live_authority_max_text_bytes_v1;
        case context_store_live_value_kind_v1::text_string:
            return value.text_string.size <= context_store_live_authority_max_text_bytes_v1 &&
                (value.text_string.size == 0 ||
                 (value.text_string.data != nullptr &&
                  valid_utf8(value.text_string.data, value.text_string.size)));
        case context_store_live_value_kind_v1::boolean:
            return true;
        case context_store_live_value_kind_v1::float64_bits:
            return finite_float64(value.float64_bits);
        case context_store_live_value_kind_v1::array:
            if ((value.array_values == nullptr && value.array_value_count != 0) ||
                value.array_value_count > max_parameter_entries - item_count) {
                return false;
            }
            for (size_t index = 0; index < value.array_value_count; ++index) {
                if (!validate_typed_value(value.array_values[index], depth + 1, item_count)) {
                    return false;
                }
            }
            return true;
    }
    return false;
}

bool encode_typed_value(
        component_cbor_writer & writer,
        const context_store_live_typed_value_v1 & value) noexcept {
    if (!writer.array(2)) {
        return false;
    }
    switch (value.kind) {
        case context_store_live_value_kind_v1::unsigned_integer:
            return writer.unsigned_integer(0) && writer.unsigned_integer(value.unsigned_value);
        case context_store_live_value_kind_v1::negative_integer:
            return writer.unsigned_integer(1) && writer.negative_integer(value.negative_value);
        case context_store_live_value_kind_v1::byte_string:
            return writer.unsigned_integer(2) && writer.bytes(value.byte_string);
        case context_store_live_value_kind_v1::text_string:
            return writer.unsigned_integer(3) && writer.text(value.text_string);
        case context_store_live_value_kind_v1::boolean:
            return writer.unsigned_integer(4) && writer.boolean(value.boolean_value);
        case context_store_live_value_kind_v1::float64_bits:
            return writer.unsigned_integer(5) && writer.bytes(value.float64_bits);
        case context_store_live_value_kind_v1::array:
            if (!writer.unsigned_integer(6) || !writer.array(value.array_value_count)) {
                return false;
            }
            for (size_t index = 0; index < value.array_value_count; ++index) {
                if (!encode_typed_value(writer, value.array_values[index])) {
                    return false;
                }
            }
            return true;
    }
    return false;
}

struct artifact_set {
    std::vector<size_t> order;
    std::vector<digest> digests;
};

status collect_artifacts(
        const context_store_live_artifact_v1 * artifacts,
        size_t count,
        size_t maximum,
        status missing_status,
        status ambiguous_status,
        artifact_set & output) {
    if (artifacts == nullptr || count == 0 || count > maximum) {
        return missing_status;
    }
    output.order.resize(count);
    output.digests.resize(count);
    std::iota(output.order.begin(), output.order.end(), 0);
    for (size_t index = 0; index < count; ++index) {
        if (!valid_text(artifacts[index].role,
                context_store_live_authority_max_registered_id_bytes_v1) ||
            placeholder_text(artifacts[index].role) ||
            !nonempty_bytes(artifacts[index].exact_bytes) ||
            !hash_exact_bytes(artifacts[index].exact_bytes, output.digests[index])) {
            return missing_status;
        }
        for (size_t prior = 0; prior < index; ++prior) {
            if (artifacts[prior].semantic_order == artifacts[index].semantic_order ||
                text_equal(artifacts[prior].role, artifacts[index].role)) {
                return ambiguous_status;
            }
        }
    }
    std::sort(output.order.begin(), output.order.end(),
        [artifacts](size_t left, size_t right) {
            return artifacts[left].semantic_order < artifacts[right].semantic_order;
        });
    for (size_t index = 0; index < count; ++index) {
        if (artifacts[output.order[index]].semantic_order != index) {
            return ambiguous_status;
        }
    }
    return status::built;
}

bool encode_artifacts(
        component_cbor_writer & writer,
        const context_store_live_artifact_v1 * artifacts,
        const artifact_set & set) noexcept {
    if (!writer.array(set.order.size())) {
        return false;
    }
    for (const size_t original_index : set.order) {
        const auto & artifact = artifacts[original_index];
        if (!writer.map(4) ||
            !writer.unsigned_integer(0) || !writer.text(artifact.role) ||
            !writer.unsigned_integer(1) || !writer.unsigned_integer(artifact.semantic_order) ||
            !writer.unsigned_integer(2) || !writer.unsigned_integer(artifact.exact_bytes.size) ||
            !writer.unsigned_integer(3) || !writer.bytes(set.digests[original_index])) {
            return false;
        }
    }
    return true;
}

status collect_parameters(
        const context_store_live_parameter_v1 * parameters,
        size_t count,
        bool require_nonempty,
        std::vector<size_t> & order) {
    if ((parameters == nullptr && count != 0) ||
        (require_nonempty && count == 0) || count > max_parameter_entries) {
        return status::invalid_input;
    }
    order.resize(count);
    std::iota(order.begin(), order.end(), 0);
    for (size_t index = 0; index < count; ++index) {
        size_t item_count = 0;
        if (!valid_text(parameters[index].id,
                context_store_live_authority_max_registered_id_bytes_v1) ||
            placeholder_text(parameters[index].id) ||
            !validate_typed_value(parameters[index].value, 0, item_count)) {
            return status::invalid_input;
        }
    }
    std::sort(order.begin(), order.end(), [parameters](size_t left, size_t right) {
        return canonical_text_less(parameters[left].id, parameters[right].id);
    });
    for (size_t index = 1; index < order.size(); ++index) {
        if (text_equal(parameters[order[index - 1]].id,
                parameters[order[index]].id)) {
            return status::duplicate_component_preimage;
        }
    }
    return status::built;
}

bool encode_parameters(
        component_cbor_writer & writer,
        const context_store_live_parameter_v1 * parameters,
        const std::vector<size_t> & order) noexcept {
    if (!writer.array(order.size())) {
        return false;
    }
    for (const size_t index : order) {
        if (!writer.map(2) ||
            !writer.unsigned_integer(0) || !writer.text(parameters[index].id) ||
            !writer.unsigned_integer(1) || !encode_typed_value(writer, parameters[index].value)) {
            return false;
        }
    }
    return true;
}

context_store_live_authority_result_v1 failure(
        status value,
        size_t component = context_store_compatibility_v1_component_count) noexcept {
    context_store_live_authority_result_v1 result;
    result.status = value;
    result.failure_component = component;
    result.expectation = {};
    return result;
}

status build_model_bytes_component(
        const context_store_live_model_v1 & model,
        digest & output) {
    if (model.declared_shard_count == 0 ||
        model.shard_count != model.declared_shard_count) {
        return status::missing_model_artifact;
    }
    artifact_set shards;
    const status collected = collect_artifacts(
        model.shards, model.shard_count, 1024,
        status::missing_model_artifact, status::ambiguous_shard_plan, shards);
    if (collected != status::built) {
        return collected;
    }
    component_cbor_writer writer(0);
    if (!writer.map(1) || !writer.unsigned_integer(0) ||
        !encode_artifacts(writer, model.shards, shards) || !writer.finish(output)) {
        return status::canonicalization_failed;
    }
    return status::built;
}

status build_model_metadata_component(
        const context_store_live_model_v1 & model,
        digest & output) {
    if (!valid_text(model.metadata_schema_id,
            context_store_live_authority_max_registered_id_bytes_v1) ||
        placeholder_text(model.metadata_schema_id) ||
        model.metadata == nullptr || model.metadata_count == 0 ||
        model.metadata_count > max_metadata_entries ||
        model.tensors == nullptr || model.tensor_count == 0 ||
        model.tensor_count > max_tensor_entries) {
        return status::incomplete_typed_model_semantics;
    }

    // Tensor locations are authority only when they describe ranges inside
    // the exact shard bytes already bound by component 0. Discovery order is
    // intentionally irrelevant; shard_order is the semantic artifact order.
    std::vector<uint64_t> shard_sizes(model.shard_count);
    std::vector<bool> shard_seen(model.shard_count);
    for (size_t index = 0; index < model.shard_count; ++index) {
        const auto & shard = model.shards[index];
        if (shard.semantic_order >= model.shard_count ||
            shard_seen[shard.semantic_order] ||
            !nonempty_bytes(shard.exact_bytes)) {
            return status::incomplete_typed_model_semantics;
        }
        shard_seen[shard.semantic_order] = true;
        shard_sizes[shard.semantic_order] =
            static_cast<uint64_t>(shard.exact_bytes.size);
    }

    std::vector<size_t> metadata_order(model.metadata_count);
    std::iota(metadata_order.begin(), metadata_order.end(), 0);
    for (size_t index = 0; index < model.metadata_count; ++index) {
        size_t item_count = 0;
        if (!valid_text(model.metadata[index].key,
                context_store_live_authority_max_text_bytes_v1) ||
            !validate_typed_value(model.metadata[index].value, 0, item_count)) {
            return status::incomplete_typed_model_semantics;
        }
    }
    std::sort(metadata_order.begin(), metadata_order.end(),
        [&model](size_t left, size_t right) {
            return canonical_text_less(model.metadata[left].key, model.metadata[right].key);
        });
    for (size_t index = 1; index < metadata_order.size(); ++index) {
        if (text_equal(model.metadata[metadata_order[index - 1]].key,
                model.metadata[metadata_order[index]].key)) {
            return status::ambiguous_model_semantics;
        }
    }

    std::vector<size_t> tensor_order(model.tensor_count);
    std::iota(tensor_order.begin(), tensor_order.end(), 0);
    for (size_t index = 0; index < model.tensor_count; ++index) {
        const auto & tensor = model.tensors[index];
        if (!valid_text(tensor.name, context_store_live_authority_max_text_bytes_v1) ||
            tensor.shard_order >= model.shard_count ||
            tensor.dimensions == nullptr || tensor.dimension_count == 0 ||
            tensor.dimension_count > 16 || tensor.exact_encoded_bytes == 0 ||
            tensor.exact_container_offset >
                std::numeric_limits<uint64_t>::max() - tensor.exact_encoded_bytes ||
            tensor.exact_container_offset + tensor.exact_encoded_bytes >
                shard_sizes[tensor.shard_order]) {
            return status::incomplete_typed_model_semantics;
        }
        for (size_t dimension = 0; dimension < tensor.dimension_count; ++dimension) {
            if (tensor.dimensions[dimension] == 0) {
                return status::incomplete_typed_model_semantics;
            }
        }
    }
    std::sort(tensor_order.begin(), tensor_order.end(),
        [&model](size_t left, size_t right) {
            const auto & lhs = model.tensors[left];
            const auto & rhs = model.tensors[right];
            if (lhs.shard_order != rhs.shard_order) {
                return lhs.shard_order < rhs.shard_order;
            }
            if (lhs.exact_container_offset != rhs.exact_container_offset) {
                return lhs.exact_container_offset < rhs.exact_container_offset;
            }
            return canonical_text_less(lhs.name, rhs.name);
        });
    for (size_t index = 1; index < tensor_order.size(); ++index) {
        const auto & prior = model.tensors[tensor_order[index - 1]];
        const auto & current_tensor = model.tensors[tensor_order[index]];
        if (prior.shard_order == current_tensor.shard_order &&
            prior.exact_container_offset == current_tensor.exact_container_offset) {
            return status::ambiguous_model_semantics;
        }
        if (prior.shard_order == current_tensor.shard_order) {
            if (prior.exact_container_offset >
                    std::numeric_limits<uint64_t>::max() - prior.exact_encoded_bytes ||
                prior.exact_container_offset + prior.exact_encoded_bytes >
                    current_tensor.exact_container_offset) {
                return status::ambiguous_model_semantics;
            }
        }
    }
    std::vector<size_t> tensor_name_order = tensor_order;
    std::sort(tensor_name_order.begin(), tensor_name_order.end(),
        [&model](size_t left, size_t right) {
            return canonical_text_less(model.tensors[left].name, model.tensors[right].name);
        });
    for (size_t index = 1; index < tensor_name_order.size(); ++index) {
        if (text_equal(model.tensors[tensor_name_order[index - 1]].name,
                model.tensors[tensor_name_order[index]].name)) {
            return status::ambiguous_model_semantics;
        }
    }

    component_cbor_writer writer(1);
    if (!writer.map(3) ||
        !writer.unsigned_integer(0) || !writer.text(model.metadata_schema_id) ||
        !writer.unsigned_integer(1) || !writer.array(metadata_order.size())) {
        return status::canonicalization_failed;
    }
    for (const size_t index : metadata_order) {
        if (!writer.map(2) ||
            !writer.unsigned_integer(0) || !writer.text(model.metadata[index].key) ||
            !writer.unsigned_integer(1) || !encode_typed_value(writer, model.metadata[index].value)) {
            return status::canonicalization_failed;
        }
    }
    if (!writer.unsigned_integer(2) || !writer.array(tensor_order.size())) {
        return status::canonicalization_failed;
    }
    for (const size_t index : tensor_order) {
        const auto & tensor = model.tensors[index];
        if (!writer.map(6) ||
            !writer.unsigned_integer(0) || !writer.text(tensor.name) ||
            !writer.unsigned_integer(1) || !writer.array(tensor.dimension_count)) {
            return status::canonicalization_failed;
        }
        for (size_t dimension = 0; dimension < tensor.dimension_count; ++dimension) {
            if (!writer.unsigned_integer(tensor.dimensions[dimension])) {
                return status::canonicalization_failed;
            }
        }
        if (!writer.unsigned_integer(2) || !writer.unsigned_integer(tensor.tensor_type) ||
            !writer.unsigned_integer(3) ||
                !writer.unsigned_integer(tensor.exact_container_offset) ||
            !writer.unsigned_integer(4) ||
                !writer.unsigned_integer(tensor.exact_encoded_bytes) ||
            !writer.unsigned_integer(5) || !writer.unsigned_integer(tensor.shard_order)) {
            return status::canonicalization_failed;
        }
    }
    return writer.finish(output) ? status::built : status::canonicalization_failed;
}

status build_tokenizer_component(
        const context_store_live_tokenizer_v1 & tokenizer,
        digest & output) {
    artifact_set artifacts;
    status collected = collect_artifacts(
        tokenizer.artifacts, tokenizer.artifact_count, 64,
        status::incomplete_tokenizer, status::ambiguous_tokenizer, artifacts);
    if (collected != status::built) {
        return collected;
    }
    if (tokenizer.tokens == nullptr || tokenizer.token_count == 0 ||
        tokenizer.token_count > max_token_entries ||
        (tokenizer.merges == nullptr && tokenizer.merge_count != 0) ||
        tokenizer.merge_count > max_merge_entries ||
        (tokenizer.special_tokens == nullptr && tokenizer.special_token_count != 0)) {
        return status::incomplete_tokenizer;
    }

    std::vector<size_t> token_order(tokenizer.token_count);
    std::iota(token_order.begin(), token_order.end(), 0);
    for (size_t index = 0; index < tokenizer.token_count; ++index) {
        const auto & token = tokenizer.tokens[index];
        if (!valid_bytes(token.exact_token_bytes) ||
            token.exact_token_bytes.size > context_store_live_authority_max_text_bytes_v1 ||
            !finite_float64(token.score_bits)) {
            return status::incomplete_tokenizer;
        }
    }
    std::sort(token_order.begin(), token_order.end(), [&tokenizer](size_t left, size_t right) {
        return tokenizer.tokens[left].token_id < tokenizer.tokens[right].token_id;
    });
    for (size_t index = 1; index < token_order.size(); ++index) {
        if (tokenizer.tokens[token_order[index - 1]].token_id ==
            tokenizer.tokens[token_order[index]].token_id) {
            return status::ambiguous_tokenizer;
        }
    }
    for (size_t index = 0; index < token_order.size(); ++index) {
        if (tokenizer.tokens[token_order[index]].token_id != index) {
            return status::incomplete_tokenizer;
        }
    }

    std::vector<size_t> merge_order(tokenizer.merge_count);
    std::iota(merge_order.begin(), merge_order.end(), 0);
    for (size_t index = 0; index < tokenizer.merge_count; ++index) {
        const auto & merge = tokenizer.merges[index];
        if (!valid_bytes(merge.left) || !valid_bytes(merge.right) ||
            merge.left.size > context_store_live_authority_max_text_bytes_v1 ||
            merge.right.size > context_store_live_authority_max_text_bytes_v1) {
            return status::incomplete_tokenizer;
        }
    }
    const auto bytes_less = [](const context_store_live_bytes_v1 & left,
                               const context_store_live_bytes_v1 & right) {
        const size_t common = std::min(left.size, right.size);
        const int order = common == 0 ? 0 : std::memcmp(left.data, right.data, common);
        return order < 0 || (order == 0 && left.size < right.size);
    };
    std::sort(merge_order.begin(), merge_order.end(),
        [&tokenizer, &bytes_less](size_t left, size_t right) {
            const auto & lhs = tokenizer.merges[left];
            const auto & rhs = tokenizer.merges[right];
            if (bytes_equal(lhs.left, rhs.left)) {
                return bytes_less(lhs.right, rhs.right);
            }
            return bytes_less(lhs.left, rhs.left);
        });
    for (size_t index = 1; index < merge_order.size(); ++index) {
        const auto & prior = tokenizer.merges[merge_order[index - 1]];
        const auto & current_merge = tokenizer.merges[merge_order[index]];
        if (bytes_equal(prior.left, current_merge.left) &&
            bytes_equal(prior.right, current_merge.right)) {
            return status::ambiguous_tokenizer;
        }
    }

    if (tokenizer.special_token_count > max_special_token_entries) {
        return status::incomplete_tokenizer;
    }
    std::vector<size_t> special_order(tokenizer.special_token_count);
    std::iota(special_order.begin(), special_order.end(), 0);
    for (size_t index = 0; index < tokenizer.special_token_count; ++index) {
        if (!valid_text(tokenizer.special_tokens[index].role,
                context_store_live_authority_max_registered_id_bytes_v1) ||
            placeholder_text(tokenizer.special_tokens[index].role)) {
            return status::incomplete_tokenizer;
        }
    }
    std::sort(special_order.begin(), special_order.end(),
        [&tokenizer](size_t left, size_t right) {
            return canonical_text_less(
                tokenizer.special_tokens[left].role,
                tokenizer.special_tokens[right].role);
        });
    for (size_t index = 1; index < special_order.size(); ++index) {
        if (text_equal(tokenizer.special_tokens[special_order[index - 1]].role,
                tokenizer.special_tokens[special_order[index]].role)) {
            return status::ambiguous_tokenizer;
        }
    }
    for (const size_t index : special_order) {
        if (tokenizer.special_tokens[index].token_id >= tokenizer.token_count) {
            return status::incomplete_tokenizer;
        }
    }

    std::vector<size_t> policy_order;
    collected = collect_parameters(
        tokenizer.policy, tokenizer.policy_count, true, policy_order);
    if (collected != status::built) {
        return collected == status::duplicate_component_preimage ?
            status::ambiguous_tokenizer : status::incomplete_tokenizer;
    }

    component_cbor_writer writer(2);
    if (!writer.map(5) ||
        !writer.unsigned_integer(0) || !encode_artifacts(writer, tokenizer.artifacts, artifacts) ||
        !writer.unsigned_integer(1) || !writer.array(token_order.size())) {
        return status::canonicalization_failed;
    }
    for (const size_t index : token_order) {
        const auto & token = tokenizer.tokens[index];
        if (!writer.map(4) ||
            !writer.unsigned_integer(0) || !writer.unsigned_integer(token.token_id) ||
            !writer.unsigned_integer(1) || !writer.bytes(token.exact_token_bytes) ||
            !writer.unsigned_integer(2) || !writer.bytes(token.score_bits) ||
            !writer.unsigned_integer(3) || !writer.unsigned_integer(token.token_type)) {
            return status::canonicalization_failed;
        }
    }
    if (!writer.unsigned_integer(2) || !writer.array(tokenizer.merge_count)) {
        return status::canonicalization_failed;
    }
    for (size_t index = 0; index < tokenizer.merge_count; ++index) {
        if (!writer.map(2) ||
            !writer.unsigned_integer(0) || !writer.bytes(tokenizer.merges[index].left) ||
            !writer.unsigned_integer(1) || !writer.bytes(tokenizer.merges[index].right)) {
            return status::canonicalization_failed;
        }
    }
    if (!writer.unsigned_integer(3) || !writer.array(special_order.size())) {
        return status::canonicalization_failed;
    }
    for (const size_t index : special_order) {
        if (!writer.map(2) ||
            !writer.unsigned_integer(0) || !writer.text(tokenizer.special_tokens[index].role) ||
            !writer.unsigned_integer(1) ||
                !writer.unsigned_integer(tokenizer.special_tokens[index].token_id)) {
            return status::canonicalization_failed;
        }
    }
    if (!writer.unsigned_integer(4) ||
        !encode_parameters(writer, tokenizer.policy, policy_order)) {
        return status::canonicalization_failed;
    }
    return writer.finish(output) ? status::built : status::canonicalization_failed;
}

status build_chat_template_component(
        const context_store_live_chat_template_v1 & chat,
        digest & output) noexcept {
    if (chat.effective_candidate_count != 1) {
        return status::ambiguous_template_selection;
    }
    if (!nonempty_bytes(chat.exact_template_bytes) ||
        !valid_text(chat.renderer_id,
            context_store_live_authority_max_registered_id_bytes_v1) ||
        placeholder_text(chat.renderer_id) ||
        !nonempty_bytes(chat.renderer_executable_bytes)) {
        return status::incomplete_template_identity;
    }
    digest template_digest {};
    digest renderer_digest {};
    if (!hash_exact_bytes(chat.exact_template_bytes, template_digest) ||
        !hash_exact_bytes(chat.renderer_executable_bytes, renderer_digest)) {
        return status::incomplete_template_identity;
    }
    component_cbor_writer writer(3);
    if (!writer.map(4) ||
        !writer.unsigned_integer(0) ||
            !writer.unsigned_integer(chat.exact_template_bytes.size) ||
        !writer.unsigned_integer(1) || !writer.bytes(template_digest) ||
        !writer.unsigned_integer(2) || !writer.text(chat.renderer_id) ||
        !writer.unsigned_integer(3) || !writer.bytes(renderer_digest) ||
        !writer.finish(output)) {
        return status::canonicalization_failed;
    }
    return status::built;
}

bool framed_source_identity(
        const context_store_live_runtime_v1 & runtime,
        digest & output) noexcept {
    if (runtime.source_revision.size > std::numeric_limits<uint16_t>::max()) {
        return false;
    }
    hash_accumulator hash;
    return hash.update(source_tree_domain, sizeof(source_tree_domain)) &&
        hash.uint16_be(static_cast<uint16_t>(runtime.source_revision.size)) &&
        hash.update(runtime.source_revision.data, runtime.source_revision.size) &&
        hash.uint64_be(runtime.exact_source_tree_manifest_bytes.size) &&
        hash.update(runtime.exact_source_tree_manifest_bytes.data,
            runtime.exact_source_tree_manifest_bytes.size) &&
        hash.byte(runtime.source_is_dirty ? 1 : 0) &&
        hash.uint64_be(runtime.exact_dirty_tree_manifest_bytes.size) &&
        hash.update(runtime.exact_dirty_tree_manifest_bytes.data,
            runtime.exact_dirty_tree_manifest_bytes.size) &&
        hash.finish(output);
}

status build_runtime_component(
        const context_store_live_runtime_v1 & runtime,
        digest & output) {
    if (!valid_text(runtime.source_revision,
            context_store_live_authority_max_registered_id_bytes_v1) ||
        mutable_source_revision(runtime.source_revision) ||
        !nonempty_bytes(runtime.exact_source_tree_manifest_bytes) ||
        !valid_text(runtime.toolchain_id,
            context_store_live_authority_max_registered_id_bytes_v1) ||
        placeholder_text(runtime.toolchain_id) ||
        !valid_text(runtime.state_abi_id,
            context_store_live_authority_max_registered_id_bytes_v1) ||
        placeholder_text(runtime.state_abi_id)) {
        return status::incomplete_build_abi;
    }
    if (!runtime.source_is_immutable ||
        (runtime.source_is_dirty && !nonempty_bytes(runtime.exact_dirty_tree_manifest_bytes)) ||
        (!runtime.source_is_dirty && runtime.exact_dirty_tree_manifest_bytes.size != 0) ||
        !valid_bytes(runtime.exact_dirty_tree_manifest_bytes)) {
        return status::mutable_build_identity;
    }

    artifact_set artifacts;
    status collected = collect_artifacts(
        runtime.artifacts, runtime.artifact_count, 1024,
        status::incomplete_build_abi, status::incomplete_build_abi, artifacts);
    if (collected != status::built || artifacts.order.empty() ||
        !text_equal_literal(runtime.artifacts[artifacts.order.front()].role, "executable")) {
        return status::incomplete_build_abi;
    }
    std::vector<size_t> option_order;
    collected = collect_parameters(
        runtime.build_options, runtime.build_option_count, true, option_order);
    if (collected != status::built) {
        return status::incomplete_build_abi;
    }
    digest source_identity {};
    if (!framed_source_identity(runtime, source_identity)) {
        return status::incomplete_build_abi;
    }

    component_cbor_writer writer(6);
    if (!writer.map(5) ||
        !writer.unsigned_integer(0) || !writer.bytes(source_identity) ||
        !writer.unsigned_integer(1) || !encode_artifacts(writer, runtime.artifacts, artifacts) ||
        !writer.unsigned_integer(2) || !writer.text(runtime.toolchain_id) ||
        !writer.unsigned_integer(3) ||
            !encode_parameters(writer, runtime.build_options, option_order) ||
        !writer.unsigned_integer(4) || !writer.text(runtime.state_abi_id) ||
        !writer.finish(output)) {
        return status::canonicalization_failed;
    }
    return status::built;
}

bool framed_global_plan(
        const context_store_live_bytes_v1 & plan,
        digest & output) noexcept {
    hash_accumulator hash;
    return hash.update(global_plan_domain, sizeof(global_plan_domain)) &&
        hash.uint64_be(plan.size) && hash.update(plan.data, plan.size) &&
        hash.finish(output);
}

bool framed_rank_ownership(
        const context_store_live_rank_v1 & rank,
        digest & output) noexcept {
    hash_accumulator hash;
    return hash.update(rank_ownership_domain, sizeof(rank_ownership_domain)) &&
        hash.uint32_be(rank.logical_rank) &&
        hash.uint64_be(rank.ownership_plan.size) &&
        hash.update(rank.ownership_plan.data, rank.ownership_plan.size) &&
        hash.finish(output);
}

bool framed_rank_placement(
        const context_store_live_rank_v1 & rank,
        digest & output) noexcept {
    if (rank.stable_endpoint_id.size > std::numeric_limits<uint16_t>::max() ||
        rank.stable_device_id.size > std::numeric_limits<uint16_t>::max()) {
        return false;
    }
    hash_accumulator hash;
    return hash.update(rank_placement_domain, sizeof(rank_placement_domain)) &&
        hash.uint32_be(rank.logical_rank) &&
        hash.uint16_be(static_cast<uint16_t>(rank.stable_endpoint_id.size)) &&
        hash.update(rank.stable_endpoint_id.data, rank.stable_endpoint_id.size) &&
        hash.uint16_be(static_cast<uint16_t>(rank.stable_device_id.size)) &&
        hash.update(rank.stable_device_id.data, rank.stable_device_id.size) &&
        hash.uint64_be(rank.placement_plan.size) &&
        hash.update(rank.placement_plan.data, rank.placement_plan.size) &&
        hash.finish(output);
}

status build_topology_component(
        const context_store_live_topology_v1 & topology,
        digest & output) noexcept {
    if (!valid_text(topology.plan_schema_id,
            context_store_live_authority_max_registered_id_bytes_v1) ||
        placeholder_text(topology.plan_schema_id) ||
        !valid_text(topology.execution_mode_id,
            context_store_live_authority_max_registered_id_bytes_v1) ||
        placeholder_text(topology.execution_mode_id) ||
        topology.world_size != context_store_live_authority_world_size_v1 ||
        topology.rank_count != context_store_live_authority_rank_count_v1 ||
        topology.ranks == nullptr || topology.stable_topology_epoch == 0 ||
        !nonempty_bytes(topology.global_plan)) {
        return status::incomplete_topology;
    }

    std::array<digest, context_store_live_authority_rank_count_v1> ownership {};
    std::array<digest, context_store_live_authority_rank_count_v1> placement {};
    std::array<digest, 1 + context_store_live_authority_rank_count_v1 * 2> raw_facts {};
    if (!hash_exact_bytes(topology.global_plan, raw_facts[0])) {
        return status::incomplete_topology;
    }
    for (size_t index = 0; index < topology.rank_count; ++index) {
        const auto & rank = topology.ranks[index];
        if (rank.logical_rank != index ||
            !valid_text(rank.stable_endpoint_id,
                context_store_live_authority_max_registered_id_bytes_v1) ||
            placeholder_text(rank.stable_endpoint_id) ||
            !valid_text(rank.stable_device_id,
                context_store_live_authority_max_registered_id_bytes_v1) ||
            placeholder_text(rank.stable_device_id) ||
            !nonempty_bytes(rank.ownership_plan) || !nonempty_bytes(rank.placement_plan) ||
            !hash_exact_bytes(rank.ownership_plan, raw_facts[1 + index * 2]) ||
            !hash_exact_bytes(rank.placement_plan, raw_facts[2 + index * 2])) {
            return status::ambiguous_rank_plan;
        }
        for (size_t prior = 0; prior < index; ++prior) {
            if (text_equal(topology.ranks[prior].stable_endpoint_id,
                    rank.stable_endpoint_id) ||
                text_equal(topology.ranks[prior].stable_device_id,
                    rank.stable_device_id)) {
                return status::ambiguous_rank_plan;
            }
        }
        if (!framed_rank_ownership(rank, ownership[index]) ||
            !framed_rank_placement(rank, placement[index])) {
            return status::ambiguous_rank_plan;
        }
    }
    for (size_t left = 0; left < raw_facts.size(); ++left) {
        for (size_t right = left + 1; right < raw_facts.size(); ++right) {
            if (raw_facts[left] == raw_facts[right]) {
                return status::placeholder_equal_facts;
            }
        }
    }
    digest global_plan {};
    if (!framed_global_plan(topology.global_plan, global_plan)) {
        return status::incomplete_topology;
    }

    component_cbor_writer writer(14);
    if (!writer.map(1) || !writer.unsigned_integer(0) ||
        !writer.map(6) ||
        !writer.unsigned_integer(0) || !writer.text(topology.plan_schema_id) ||
        !writer.unsigned_integer(1) || !writer.text(topology.execution_mode_id) ||
        !writer.unsigned_integer(2) || !writer.unsigned_integer(topology.world_size) ||
        !writer.unsigned_integer(3) || !writer.bytes(global_plan) ||
        !writer.unsigned_integer(4) ||
            !writer.unsigned_integer(topology.stable_topology_epoch) ||
        !writer.unsigned_integer(5) || !writer.array(topology.rank_count)) {
        return status::canonicalization_failed;
    }
    for (size_t index = 0; index < topology.rank_count; ++index) {
        if (!writer.map(3) ||
            !writer.unsigned_integer(0) ||
                !writer.unsigned_integer(topology.ranks[index].logical_rank) ||
            !writer.unsigned_integer(1) || !writer.bytes(ownership[index]) ||
            !writer.unsigned_integer(2) || !writer.bytes(placement[index])) {
            return status::canonicalization_failed;
        }
    }
    return writer.finish(output) ? status::built : status::canonicalization_failed;
}

bool is_derived_component(size_t index) noexcept {
    return std::find(derived_components.begin(), derived_components.end(), index) !=
        derived_components.end();
}

status collect_supplements(
        const context_store_live_authority_inputs_v1 & inputs,
        const context_store_live_supplement_validator_v1 & validator,
        std::array<const context_store_live_supplement_v1 *,
            context_store_compatibility_v1_component_count> & mapped) noexcept {
    if (inputs.supplement_count != context_store_live_authority_supplement_count_v1 ||
        inputs.supplements == nullptr) {
        return status::missing_component_preimage;
    }
    std::array<digest, context_store_live_authority_supplement_count_v1> raw_digests {};
    for (size_t input_index = 0; input_index < inputs.supplement_count; ++input_index) {
        const auto & supplement = inputs.supplements[input_index];
        if (supplement.component_index >= mapped.size() ||
            is_derived_component(supplement.component_index)) {
            return status::invalid_component_preimage;
        }
        if (mapped[supplement.component_index] != nullptr) {
            return status::duplicate_component_preimage;
        }
        if (!nonempty_bytes(supplement.exact_dcbor_preimage) ||
            supplement.exact_dcbor_preimage.size > component_preimage_max_bytes ||
            (supplement.exact_dcbor_preimage.data[0] & 0xe0U) != 0xa0U ||
            supplement.exact_dcbor_preimage.data[0] == 0xbfU ||
            !hash_exact_bytes(supplement.exact_dcbor_preimage, raw_digests[input_index])) {
            return status::invalid_component_preimage;
        }
        if (!validator.validate(
                supplement.component_index,
                supplement.exact_dcbor_preimage)) {
            return status::unvalidated_component_preimage;
        }
        for (size_t prior = 0; prior < input_index; ++prior) {
            if (raw_digests[prior] == raw_digests[input_index]) {
                return status::placeholder_equal_facts;
            }
        }
        mapped[supplement.component_index] = &supplement;
    }
    for (size_t index = 0; index < mapped.size(); ++index) {
        if (!is_derived_component(index) && mapped[index] == nullptr) {
            return status::missing_component_preimage;
        }
    }
    return status::built;
}

status build_supplement_component(
        const context_store_live_supplement_v1 & supplement,
        digest & output) noexcept {
    component_cbor_writer writer(supplement.component_index);
    return writer.raw(supplement.exact_dcbor_preimage.data,
               supplement.exact_dcbor_preimage.size) && writer.finish(output) ?
        status::built : status::canonicalization_failed;
}

context_store_live_authority_result_v1 build_impl(
        const context_store_live_authority_inputs_v1 & inputs,
        const context_store_live_supplement_validator_v1 & validator) {
    std::array<digest, context_store_compatibility_v1_component_count> components {};
    status current = build_model_bytes_component(inputs.model, components[0]);
    if (current != status::built) return failure(current, 0);
    current = build_model_metadata_component(inputs.model, components[1]);
    if (current != status::built) return failure(current, 1);
    current = build_tokenizer_component(inputs.tokenizer, components[2]);
    if (current != status::built) return failure(current, 2);
    current = build_chat_template_component(inputs.chat_template, components[3]);
    if (current != status::built) return failure(current, 3);
    current = build_runtime_component(inputs.runtime, components[6]);
    if (current != status::built) return failure(current, 6);
    current = build_topology_component(inputs.topology, components[14]);
    if (current != status::built) return failure(current, 14);

    std::array<const context_store_live_supplement_v1 *,
        context_store_compatibility_v1_component_count> supplements {};
    current = collect_supplements(inputs, validator, supplements);
    if (current != status::built) {
        return failure(current);
    }

    for (size_t index = 0; index < components.size(); ++index) {
        if (is_derived_component(index)) {
            continue;
        }
        current = build_supplement_component(*supplements[index], components[index]);
        if (current != status::built) {
            return failure(current, index);
        }
    }
    for (size_t left = 0; left < components.size(); ++left) {
        if (!digest_nonzero(components[left])) {
            return failure(status::canonicalization_failed, left);
        }
        for (size_t right = left + 1; right < components.size(); ++right) {
            if (components[left] == components[right]) {
                return failure(status::placeholder_equal_facts, right);
            }
        }
    }

    std::array<context_store_compatibility_component_digest_v1,
        context_store_compatibility_v1_component_count> closed {};
    for (size_t index = 0; index < closed.size(); ++index) {
        closed[index].label = context_store_compatibility_component_label_v1(index);
        closed[index].label_size = std::strlen(closed[index].label);
        closed[index].digest = components[index];
    }
    const auto built = context_store_build_compatibility_expectation_v1(
        closed.data(), closed.size());
    if (built.status != context_store_compatibility_build_status_v1::built) {
        return failure(status::compatibility_build_failed);
    }
    context_store_live_authority_result_v1 result;
    result.status = status::built;
    result.failure_component = context_store_compatibility_v1_component_count;
    result.expectation = built.expectation;
    return result;
}

} // namespace

context_store_live_authority_result_v1 context_store_build_live_authority_v1(
        const context_store_live_authority_inputs_v1 & inputs,
        const context_store_live_supplement_validator_v1 & validator) noexcept {
    try {
        return build_impl(inputs, validator);
    } catch (...) {
        return failure(context_store_live_authority_status_v1::canonicalization_failed);
    }
}

const char * context_store_live_authority_status_name_v1(
        context_store_live_authority_status_v1 value) noexcept {
    switch (value) {
        case status::built: return "built";
        case status::invalid_input: return "invalid-input";
        case status::missing_model_artifact: return "missing-model-artifact";
        case status::ambiguous_shard_plan: return "ambiguous-shard-plan";
        case status::incomplete_typed_model_semantics: return "incomplete-typed-model-semantics";
        case status::ambiguous_model_semantics: return "ambiguous-model-semantics";
        case status::incomplete_tokenizer: return "incomplete-tokenizer";
        case status::ambiguous_tokenizer: return "ambiguous-tokenizer";
        case status::ambiguous_template_selection: return "ambiguous-template-selection";
        case status::incomplete_template_identity: return "incomplete-template-identity";
        case status::incomplete_build_abi: return "incomplete-build-abi";
        case status::mutable_build_identity: return "mutable-build-identity";
        case status::incomplete_topology: return "incomplete-topology";
        case status::ambiguous_rank_plan: return "ambiguous-rank-plan";
        case status::missing_component_preimage: return "missing-component-preimage";
        case status::duplicate_component_preimage: return "duplicate-component-preimage";
        case status::invalid_component_preimage: return "invalid-component-preimage";
        case status::unvalidated_component_preimage: return "unvalidated-component-preimage";
        case status::placeholder_equal_facts: return "placeholder-equal-facts";
        case status::canonicalization_failed: return "canonicalization-failed";
        case status::compatibility_build_failed: return "compatibility-build-failed";
    }
    return "unknown";
}

} // namespace halofpx
