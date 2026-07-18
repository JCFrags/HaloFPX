#include "halofpx-context-store-format.h"

#include <algorithm>
#include <array>

namespace halofpx {
namespace {

bool valid_utf8_without_nul(const uint8_t * data, size_t size) noexcept {
    size_t index = 0;
    while (index < size) {
        const uint8_t first = data[index++];
        if (first == 0) {
            return false;
        }
        if (first <= 0x7f) {
            continue;
        }

        if (first >= 0xc2 && first <= 0xdf) {
            if (index >= size || data[index] < 0x80 || data[index] > 0xbf) {
                return false;
            }
            ++index;
            continue;
        }

        if (first >= 0xe0 && first <= 0xef) {
            if (index + 1 >= size) {
                return false;
            }
            const uint8_t second = data[index];
            const uint8_t third = data[index + 1];
            const bool second_ok = first == 0xe0 ? second >= 0xa0 && second <= 0xbf :
                                   first == 0xed ? second >= 0x80 && second <= 0x9f :
                                                  second >= 0x80 && second <= 0xbf;
            if (!second_ok || third < 0x80 || third > 0xbf) {
                return false;
            }
            index += 2;
            continue;
        }

        if (first >= 0xf0 && first <= 0xf4) {
            if (index + 2 >= size) {
                return false;
            }
            const uint8_t second = data[index];
            const uint8_t third = data[index + 1];
            const uint8_t fourth = data[index + 2];
            const bool second_ok = first == 0xf0 ? second >= 0x90 && second <= 0xbf :
                                   first == 0xf4 ? second >= 0x80 && second <= 0x8f :
                                                  second >= 0x80 && second <= 0xbf;
            if (!second_ok || third < 0x80 || third > 0xbf || fourth < 0x80 || fourth > 0xbf) {
                return false;
            }
            index += 3;
            continue;
        }

        return false;
    }
    return true;
}

class cbor_cursor {
public:
    cbor_cursor(
            const uint8_t * data,
            size_t size,
            context_store_manifest_parse_result & result) noexcept
        : data_(data), size_(size), result_(result) {
    }

    size_t position() const noexcept {
        return position_;
    }

    bool at_end() const noexcept {
        return position_ == size_;
    }

    bool read_map(uint64_t expected) noexcept {
        uint64_t actual = 0;
        return read_head(5, actual) && require_equal(actual, expected, context_store_manifest_parse_status::wrong_field);
    }

    bool read_array(uint64_t minimum, uint64_t maximum, uint64_t & actual) noexcept {
        return read_head(4, actual) &&
               require(actual >= minimum && actual <= maximum, context_store_manifest_parse_status::limit_exceeded);
    }

    bool read_key(uint64_t expected) noexcept {
        uint64_t actual = 0;
        return read_uint(actual) && require_equal(actual, expected, context_store_manifest_parse_status::wrong_field);
    }

    bool read_uint(uint64_t & value) noexcept {
        return read_head(0, value);
    }

    bool read_uint_equal(uint64_t expected, context_store_manifest_parse_status status) noexcept {
        uint64_t actual = 0;
        return read_uint(actual) && require_equal(actual, expected, status);
    }

    template <size_t N>
    bool read_bytes(std::array<uint8_t, N> & output) noexcept {
        uint64_t length = 0;
        if (!read_head(2, length)) {
            return false;
        }
        if (!require_equal(length, N, context_store_manifest_parse_status::invalid_value) || !have(N)) {
            return false;
        }
        std::copy_n(data_ + position_, N, output.begin());
        position_ += N;
        return true;
    }

    bool read_null_or_digest(bool & has_digest, context_store_format_digest & digest) noexcept {
        if (!have(1)) {
            return false;
        }
        if (data_[position_] == 0xf6) {
            ++position_;
            has_digest = false;
            digest.fill(0);
            return true;
        }
        has_digest = true;
        return read_bytes(digest);
    }

    bool read_null_or_uint(bool & is_null, uint64_t & value) noexcept {
        if (!have(1)) {
            return false;
        }
        if (data_[position_] == 0xf6) {
            ++position_;
            is_null = true;
            value = 0;
            return true;
        }
        is_null = false;
        return read_uint(value);
    }

    bool read_true() noexcept {
        if (!have(1)) {
            return false;
        }
        if (data_[position_] != 0xf5) {
            return fail(context_store_manifest_parse_status::wrong_type);
        }
        ++position_;
        return true;
    }

    bool read_registered_id(context_store_registered_id & output) noexcept {
        uint64_t length = 0;
        if (!read_head(3, length)) {
            return false;
        }
        if (!require(length >= 1 && length <= context_store_registered_id_max_bytes,
                     context_store_manifest_parse_status::limit_exceeded) ||
            !have(static_cast<size_t>(length))) {
            return false;
        }

        const size_t byte_length = static_cast<size_t>(length);
        if (!valid_utf8_without_nul(data_ + position_, byte_length)) {
            return fail(context_store_manifest_parse_status::invalid_value);
        }

        output.size = static_cast<uint8_t>(length);
        for (size_t index = 0; index < byte_length; ++index) {
            output.bytes[index] = static_cast<char>(data_[position_ + index]);
        }
        output.bytes[byte_length] = '\0';
        position_ += byte_length;
        return true;
    }

private:
    bool read_head(uint8_t expected_major, uint64_t & value) noexcept {
        if (!have(1)) {
            return false;
        }

        const uint8_t initial = data_[position_++];
        const uint8_t major = initial >> 5;
        const uint8_t additional = initial & 0x1f;
        if (major != expected_major) {
            return fail(context_store_manifest_parse_status::wrong_type);
        }

        if (additional < 24) {
            value = additional;
            return true;
        }
        if (additional > 27) {
            return fail(context_store_manifest_parse_status::noncanonical);
        }

        const size_t byte_count = size_t { 1 } << (additional - 24);
        if (!have(byte_count)) {
            return false;
        }

        value = 0;
        for (size_t index = 0; index < byte_count; ++index) {
            value = (value << 8) | data_[position_++];
        }

        const uint64_t minimum = additional == 24 ? 24ULL :
                                 additional == 25 ? 0x100ULL :
                                 additional == 26 ? 0x10000ULL : 0x100000000ULL;
        return require(value >= minimum, context_store_manifest_parse_status::noncanonical);
    }

    bool have(size_t count) noexcept {
        if (count > size_ - position_) {
            return fail(context_store_manifest_parse_status::truncated);
        }
        return true;
    }

    bool require(bool condition, context_store_manifest_parse_status status) noexcept {
        return condition || fail(status);
    }

    bool require_equal(
            uint64_t actual,
            uint64_t expected,
            context_store_manifest_parse_status status) noexcept {
        return require(actual == expected, status);
    }

    bool fail(context_store_manifest_parse_status status) noexcept {
        if (result_.status == context_store_manifest_parse_status::structural_only) {
            result_.status = status;
            result_.error_offset = position_;
        }
        return false;
    }

    const uint8_t * data_ = nullptr;
    size_t size_ = 0;
    size_t position_ = 0;
    context_store_manifest_parse_result & result_;
};

bool parse_compatibility_manifest(cbor_cursor & cursor) noexcept {
    if (!cursor.read_map(16)) {
        return false;
    }
    for (uint64_t field = 0; field < 16; ++field) {
        context_store_format_digest ignored {};
        if (!cursor.read_key(field) || !cursor.read_bytes(ignored)) {
            return false;
        }
    }
    return true;
}

bool parse_topology_manifest(
        cbor_cursor & cursor,
        context_store_parsed_manifest & manifest) noexcept {
    context_store_registered_id ignored_id;
    context_store_format_digest ignored_digest {};
    uint64_t rank_count = 0;

    if (!cursor.read_map(6) ||
        !cursor.read_key(0) || !cursor.read_registered_id(ignored_id) ||
        !cursor.read_key(1) || !cursor.read_registered_id(ignored_id) ||
        !cursor.read_key(2) || !cursor.read_uint(manifest.world_size) ||
        manifest.world_size < 1 || manifest.world_size > context_store_manifest_max_ranks ||
        !cursor.read_key(3) || !cursor.read_bytes(ignored_digest) ||
        !cursor.read_key(4) || !cursor.read_uint(manifest.topology_epoch) ||
        !cursor.read_key(5) ||
        !cursor.read_array(1, context_store_manifest_max_ranks, rank_count)) {
        return false;
    }
    if (rank_count != manifest.world_size) {
        return false;
    }

    std::array<bool, context_store_manifest_max_ranks> seen {};
    for (uint64_t index = 0; index < rank_count; ++index) {
        uint64_t logical_rank = 0;
        context_store_format_digest ownership_digest {};
        if (!cursor.read_map(3) ||
            !cursor.read_key(0) || !cursor.read_uint(logical_rank) ||
            logical_rank != index || logical_rank >= manifest.world_size || seen[static_cast<size_t>(logical_rank)] ||
            !cursor.read_key(1) || !cursor.read_bytes(ownership_digest) ||
            !cursor.read_key(2) || !cursor.read_bytes(ignored_digest)) {
            return false;
        }
        seen[static_cast<size_t>(logical_rank)] = true;
        manifest.rank_ownership[static_cast<size_t>(logical_rank)] = ownership_digest;
    }
    manifest.rank_count = static_cast<size_t>(rank_count);
    return true;
}

bool parse_object_descriptor(
        cbor_cursor & cursor,
        const context_store_parsed_manifest & manifest,
        context_store_format_digest & object_id) noexcept {
    context_store_registered_id ignored_id;
    context_store_format_digest ignored_digest {};
    context_store_format_digest object_compatibility {};
    uint64_t value = 0;

    if (!cursor.read_map(13) ||
        !cursor.read_key(0) || !cursor.read_bytes(object_id) ||
        !cursor.read_key(1) || !cursor.read_registered_id(ignored_id) ||
        !cursor.read_key(2) || !cursor.read_registered_id(ignored_id) ||
        !cursor.read_key(3) || !cursor.read_uint(value) ||
        !cursor.read_key(4) || !cursor.read_uint(value) ||
        !cursor.read_key(5) || !cursor.read_true() ||
        !cursor.read_key(6) || !cursor.read_uint(value) || value < 1 ||
        !cursor.read_key(7) || !cursor.read_bytes(ignored_digest) ||
        !cursor.read_key(8) || !cursor.read_uint(value) ||
        !cursor.read_key(9) || !cursor.read_uint(value) ||
        !cursor.read_key(10)) {
        return false;
    }

    bool coordinator_owned = false;
    uint64_t logical_rank = 0;
    if (!cursor.read_null_or_uint(coordinator_owned, logical_rank) ||
        (!coordinator_owned && logical_rank >= manifest.world_size)) {
        return false;
    }

    context_store_format_digest object_ownership {};
    return cursor.read_key(11) && cursor.read_bytes(object_ownership) &&
           (coordinator_owned || object_ownership == manifest.rank_ownership[static_cast<size_t>(logical_rank)]) &&
           cursor.read_key(12) && cursor.read_bytes(object_compatibility) &&
           object_compatibility == manifest.compatibility_root;
}

} // namespace

context_store_manifest_parse_result context_store_parse_manifest_v1(
        const uint8_t * data,
        size_t size) noexcept {
    context_store_manifest_parse_result result;
    if (data == nullptr || size == 0) {
        result.status = context_store_manifest_parse_status::input_empty;
        return result;
    }
    if (size > context_store_manifest_max_bytes) {
        result.status = context_store_manifest_parse_status::manifest_too_large;
        return result;
    }

    result.status = context_store_manifest_parse_status::structural_only;
    cbor_cursor cursor(data, size, result);
    auto & manifest = result.manifest;
    context_store_format_digest ignored_digest {};

    if (!cursor.read_map(2) || !cursor.read_key(0)) {
        return result;
    }
    manifest.authentication_input_offset = cursor.position();
    if (!cursor.read_map(4) ||
        !cursor.read_key(0) || !cursor.read_map(15) ||
        !cursor.read_key(0) || !cursor.read_uint_equal(1, context_store_manifest_parse_status::wrong_version) ||
        !cursor.read_key(1) || !cursor.read_uint_equal(0, context_store_manifest_parse_status::wrong_version) ||
        !cursor.read_key(2) || !cursor.read_bytes(manifest.store_uuid) ||
        !cursor.read_key(3) || !cursor.read_bytes(manifest.checkpoint_lineage_id) ||
        !cursor.read_key(4) || !cursor.read_uint(manifest.generation) || manifest.generation < 1 ||
        !cursor.read_key(5) || !cursor.read_null_or_digest(manifest.has_predecessor, manifest.predecessor_manifest_digest) ||
        (manifest.generation == 1 && manifest.has_predecessor) ||
        (manifest.generation > 1 && !manifest.has_predecessor) ||
        !cursor.read_key(6) || !parse_compatibility_manifest(cursor) ||
        !cursor.read_key(7) || !cursor.read_bytes(manifest.compatibility_root) ||
        !cursor.read_key(8) || !cursor.read_bytes(manifest.scope_namespace) ||
        !cursor.read_key(9) || !cursor.read_uint(manifest.policy_epoch) ||
        !cursor.read_key(10) || !parse_topology_manifest(cursor, manifest) ||
        !cursor.read_key(11) || !cursor.read_registered_id(manifest.state_profile_id) ||
        !cursor.read_key(12)) {
        if (result.status == context_store_manifest_parse_status::structural_only) {
            result.status = context_store_manifest_parse_status::semantic_mismatch;
            result.error_offset = cursor.position();
        }
        return result;
    }

    uint64_t object_count = 0;
    if (!cursor.read_array(1, context_store_manifest_max_objects, object_count)) {
        return result;
    }
    std::array<context_store_format_digest, context_store_manifest_max_objects> object_ids {};
    for (uint64_t index = 0; index < object_count; ++index) {
        if (!parse_object_descriptor(cursor, manifest, object_ids[static_cast<size_t>(index)])) {
            if (result.status == context_store_manifest_parse_status::structural_only) {
                result.status = context_store_manifest_parse_status::semantic_mismatch;
                result.error_offset = cursor.position();
            }
            return result;
        }
        for (uint64_t previous = 0; previous < index; ++previous) {
            if (object_ids[static_cast<size_t>(previous)] == object_ids[static_cast<size_t>(index)]) {
                result.status = context_store_manifest_parse_status::semantic_mismatch;
                result.error_offset = cursor.position();
                return result;
            }
        }
    }
    manifest.object_count = static_cast<size_t>(object_count);

    uint64_t durability = 0;
    if (!cursor.read_key(13) || !cursor.read_bytes(manifest.producer_identity) ||
        !cursor.read_key(14) || !cursor.read_uint(durability) || durability > 2 ||
        !cursor.read_key(1) || !cursor.read_registered_id(manifest.authentication_key_id) ||
        !cursor.read_key(2) || !cursor.read_uint_equal(1, context_store_manifest_parse_status::invalid_value) ||
        !cursor.read_key(3) || !cursor.read_uint(manifest.authentication_key_generation)) {
        if (result.status == context_store_manifest_parse_status::structural_only) {
            result.status = context_store_manifest_parse_status::semantic_mismatch;
            result.error_offset = cursor.position();
        }
        return result;
    }
    manifest.durability_mode = static_cast<uint8_t>(durability);
    manifest.authentication_input_size = cursor.position() - manifest.authentication_input_offset;

    if (!cursor.read_key(1) || !cursor.read_bytes(manifest.authentication_tag)) {
        return result;
    }
    if (!cursor.at_end()) {
        result.status = context_store_manifest_parse_status::trailing_data;
        result.error_offset = cursor.position();
    }
    return result;
}

const char * context_store_manifest_parse_status_name(
        context_store_manifest_parse_status status) noexcept {
    switch (status) {
        case context_store_manifest_parse_status::structural_only:     return "structural-only";
        case context_store_manifest_parse_status::input_empty:         return "input-empty";
        case context_store_manifest_parse_status::manifest_too_large:  return "manifest-too-large";
        case context_store_manifest_parse_status::truncated:           return "truncated";
        case context_store_manifest_parse_status::noncanonical:        return "noncanonical";
        case context_store_manifest_parse_status::wrong_type:          return "wrong-type";
        case context_store_manifest_parse_status::wrong_field:         return "wrong-field";
        case context_store_manifest_parse_status::wrong_version:       return "wrong-version";
        case context_store_manifest_parse_status::limit_exceeded:      return "limit-exceeded";
        case context_store_manifest_parse_status::invalid_value:       return "invalid-value";
        case context_store_manifest_parse_status::trailing_data:       return "trailing-data";
        case context_store_manifest_parse_status::semantic_mismatch:   return "semantic-mismatch";
    }
    return "invalid-status";
}

} // namespace halofpx
