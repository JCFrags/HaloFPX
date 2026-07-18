#include "halofpx-context-store-object.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace halofpx {
namespace {

constexpr std::array<uint8_t, 8> object_magic = {
    0x48, 0x41, 0x4c, 0x4f, 0x4f, 0x42, 0x4a, 0x01
};
constexpr char object_domain[] = "halofpx.object.v1";
constexpr size_t minimum_frame_bytes =
    object_magic.size() + 2 + (sizeof(object_domain) - 1) + 2 + 1 + 8;

class frame_cursor {
public:
    frame_cursor(const uint8_t * data, size_t size) noexcept : data_(data), size_(size) {}

    bool take(size_t count, const uint8_t *& output) noexcept {
        if (position_ > size_ || count > size_ - position_) {
            return false;
        }
        output = data_ + position_;
        position_ += count;
        return true;
    }

    bool read_u16(uint16_t & output) noexcept {
        const uint8_t * bytes = nullptr;
        if (!take(2, bytes)) return false;
        output = static_cast<uint16_t>((static_cast<uint16_t>(bytes[0]) << 8) | bytes[1]);
        return true;
    }

    bool read_u64(uint64_t & output) noexcept {
        const uint8_t * bytes = nullptr;
        if (!take(8, bytes)) return false;
        output = 0;
        for (size_t index = 0; index < 8; ++index) {
            output = (output << 8) | bytes[index];
        }
        return true;
    }

    size_t position() const noexcept { return position_; }
    size_t remaining() const noexcept { return position_ <= size_ ? size_ - position_ : 0; }

private:
    const uint8_t * data_ = nullptr;
    size_t size_ = 0;
    size_t position_ = 0;
};

bool bytes_equal(const uint8_t * left, const uint8_t * right, size_t size) noexcept {
    uint8_t difference = 0;
    for (size_t index = 0; index < size; ++index) {
        difference = static_cast<uint8_t>(difference | (left[index] ^ right[index]));
    }
    return difference == 0;
}

bool registered_type_matches(
        const uint8_t * type,
        size_t type_size,
        const context_store_registered_id & expected) noexcept {
    if (type_size != expected.size) return false;
    for (size_t index = 0; index < type_size; ++index) {
        const uint8_t expected_byte = static_cast<uint8_t>(expected.bytes[index]);
        if (type[index] != expected_byte || type[index] == 0 || type[index] >= 0x80) {
            return false;
        }
    }
    return true;
}

bool u64_fits_size(uint64_t value) noexcept {
    if constexpr (sizeof(size_t) < sizeof(uint64_t)) {
        return value <= std::numeric_limits<size_t>::max();
    }
    return true;
}

} // namespace

context_store_object_verify_result context_store_verify_object_frame_v1(
        const uint8_t * frame,
        size_t size,
        const context_store_object_limits & limits,
        const context_store_manifest_verify_result & manifest,
        size_t object_index) noexcept {
    context_store_object_verify_result result;
    const size_t object_count = manifest.authenticated_object_count();
    if (object_count == 0 || object_count > context_store_manifest_max_objects) {
        return result;
    }
    if (object_index >= object_count) {
        result.status = context_store_object_verify_status::object_index_out_of_range;
        return result;
    }
    if (limits.max_frame_bytes < minimum_frame_bytes || limits.max_payload_bytes == 0 ||
        limits.max_frame_bytes > UINT64_MAX / 8) {
        result.status = context_store_object_verify_status::invalid_limits;
        return result;
    }

    const auto * reference_ptr = manifest.authenticated_object_reference(object_index);
    if (reference_ptr == nullptr) {
        result.status = context_store_object_verify_status::manifest_unadmitted;
        return result;
    }
    const auto & reference = *reference_ptr;
    if (reference.frame_bytes > limits.max_frame_bytes || size > limits.max_frame_bytes) {
        result.status = context_store_object_verify_status::frame_limit_exceeded;
        return result;
    }
    if (frame == nullptr || size == 0) {
        result.status = context_store_object_verify_status::input_empty;
        return result;
    }

    frame_cursor cursor(frame, size);
    const uint8_t * value = nullptr;
    if (!cursor.take(object_magic.size(), value)) {
        result.status = context_store_object_verify_status::truncated;
        return result;
    }
    if (!bytes_equal(value, object_magic.data(), object_magic.size())) {
        result.status = context_store_object_verify_status::wrong_magic;
        return result;
    }

    uint16_t domain_size = 0;
    if (!cursor.read_u16(domain_size)) {
        result.status = context_store_object_verify_status::truncated;
        return result;
    }
    if (domain_size != sizeof(object_domain) - 1) {
        result.status = context_store_object_verify_status::wrong_domain;
        return result;
    }
    if (!cursor.take(domain_size, value)) {
        result.status = context_store_object_verify_status::truncated;
        return result;
    }
    if (!bytes_equal(value, reinterpret_cast<const uint8_t *>(object_domain), domain_size)) {
        result.status = context_store_object_verify_status::wrong_domain;
        return result;
    }

    uint16_t type_size = 0;
    if (!cursor.read_u16(type_size)) {
        result.status = context_store_object_verify_status::truncated;
        return result;
    }
    if (type_size == 0 || type_size > context_store_registered_id_max_bytes) {
        result.status = context_store_object_verify_status::invalid_stream_type;
        return result;
    }
    const uint8_t * stream_type = nullptr;
    if (!cursor.take(type_size, stream_type)) {
        result.status = context_store_object_verify_status::truncated;
        return result;
    }
    for (size_t index = 0; index < type_size; ++index) {
        if (stream_type[index] == 0 || stream_type[index] >= 0x80) {
            result.status = context_store_object_verify_status::invalid_stream_type;
            return result;
        }
    }

    uint64_t payload_size = 0;
    if (!cursor.read_u64(payload_size)) {
        result.status = context_store_object_verify_status::truncated;
        return result;
    }
    if (!u64_fits_size(payload_size)) {
        result.status = context_store_object_verify_status::payload_length_invalid;
        return result;
    }
    if (payload_size > limits.max_payload_bytes) {
        result.status = context_store_object_verify_status::payload_limit_exceeded;
        return result;
    }
    if (payload_size > cursor.remaining()) {
        result.status = context_store_object_verify_status::truncated;
        return result;
    }
    if (payload_size < cursor.remaining()) {
        result.status = context_store_object_verify_status::trailing_data;
        return result;
    }

    if (!u64_fits_size(reference.frame_bytes) || reference.frame_bytes != size ||
        !registered_type_matches(stream_type, type_size, reference.stream_type)) {
        result.status = context_store_object_verify_status::descriptor_mismatch;
        return result;
    }
    if (!context_store_sha256_bounded(frame, size, limits.max_frame_bytes, result.computed_object_id) ||
        !bytes_equal(result.computed_object_id.data(), reference.object_id.data(), reference.object_id.size())) {
        result.status = context_store_object_verify_status::object_digest_mismatch;
        return result;
    }

    result.status = context_store_object_verify_status::object_verified_unadmitted;
    result.payload_offset = cursor.position();
    result.payload_size = static_cast<size_t>(payload_size);
    return result;
}

const char * context_store_object_verify_status_name(
        context_store_object_verify_status status) noexcept {
    switch (status) {
        case context_store_object_verify_status::object_verified_unadmitted: return "object-verified-unadmitted";
        case context_store_object_verify_status::manifest_unadmitted:        return "manifest-unadmitted";
        case context_store_object_verify_status::object_index_out_of_range:  return "object-index-out-of-range";
        case context_store_object_verify_status::invalid_limits:             return "invalid-limits";
        case context_store_object_verify_status::input_empty:                return "input-empty";
        case context_store_object_verify_status::frame_limit_exceeded:       return "frame-limit-exceeded";
        case context_store_object_verify_status::truncated:                  return "truncated";
        case context_store_object_verify_status::wrong_magic:                return "wrong-magic";
        case context_store_object_verify_status::wrong_domain:               return "wrong-domain";
        case context_store_object_verify_status::invalid_stream_type:        return "invalid-stream-type";
        case context_store_object_verify_status::payload_length_invalid:     return "payload-length-invalid";
        case context_store_object_verify_status::payload_limit_exceeded:     return "payload-limit-exceeded";
        case context_store_object_verify_status::trailing_data:              return "trailing-data";
        case context_store_object_verify_status::descriptor_mismatch:        return "descriptor-mismatch";
        case context_store_object_verify_status::object_digest_mismatch:     return "object-digest-mismatch";
    }
    return "unknown";
}

} // namespace halofpx
