#pragma once

#include "halofpx-context-store-auth.h"

#include <cstddef>
#include <cstdint>

namespace halofpx {

enum class context_store_object_verify_status : uint8_t {
    object_verified_unadmitted,
    manifest_unadmitted,
    object_index_out_of_range,
    invalid_limits,
    input_empty,
    frame_limit_exceeded,
    truncated,
    wrong_magic,
    wrong_domain,
    invalid_stream_type,
    payload_length_invalid,
    payload_limit_exceeded,
    trailing_data,
    descriptor_mismatch,
    object_digest_mismatch,
};

struct context_store_object_limits {
    uint64_t max_frame_bytes = 0;
    uint64_t max_payload_bytes = 0;
};

struct context_store_object_verify_result {
    context_store_object_verify_status status = context_store_object_verify_status::manifest_unadmitted;
    context_store_format_digest computed_object_id {};
    size_t payload_offset = 0;
    size_t payload_size = 0;
};

// Verifies one caller-owned immutable frame against a reference carried only by
// an authenticated L04b result. Success remains unadmitted and exposes only a
// bounded offset/size; no codec, payload decode, candidate, or hit is created.
context_store_object_verify_result context_store_verify_object_frame_v1(
    const uint8_t * frame,
    size_t size,
    const context_store_object_limits & limits,
    const context_store_manifest_verify_result & manifest,
    size_t object_index) noexcept;

const char * context_store_object_verify_status_name(
    context_store_object_verify_status status) noexcept;

} // namespace halofpx
